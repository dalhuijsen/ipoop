/**********************************************************************

  Audacity: A Digital Audio Editor

  ODTask.cpp

  Created by Michael Chinen (mchinen)
  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

******************************************************************//**

\class ODTask
\brief ODTask is an abstract class that outlines the methods that will be used to
support On-Demand background loading of files.  These ODTasks are generally meant to be run
in a background thread.

*//*******************************************************************/


#include "ODTask.h"
#include "ODManager.h"
#include "../WaveTrack.h"
#include "../Project.h"
//temporarilly commented out till it is added to all projects
//#include "../Profiler.h"


DEFINE_EVENT_TYPE(EVT_ODTASK_COMPLETE)

/// Constructs an ODTask
ODTask::ODTask()
{

   static int sTaskNumber=0;
   mPercentComplete=0;
   mDoingTask=false;
   mTerminate = false;
   mNeedsODUpdate=false;
   mIsRunning = false;

   mTaskNumber=sTaskNumber++;

   mDemandSample=0;
}

//outside code must ensure this task is not scheduled again.
void ODTask::TerminateAndBlock()
{
   //one mutex pair for the value of mTerminate
   mTerminateMutex.Lock();
   mTerminate=true;
   //release all data the derived class may have allocated
   mTerminateMutex.Unlock();

   //and one mutex pair for the exit of the function
   mBlockUntilTerminateMutex.Lock();
//TODO lock mTerminate?
   mBlockUntilTerminateMutex.Unlock();

   //wait till we are out of doSome() to terminate.
   Terminate();
}

///Do a modular part of the task.  For example, if the task is to load the entire file, load one BlockFile.
///Relies on DoSomeInternal(), which is the subclasses must implement.
///@param amountWork the percent amount of the total job to do.  1.0 represents the entire job.  the default of 0.0
/// will do the smallest unit of work possible
void ODTask::DoSome(float amountWork)
{
   SetIsRunning(true);
   mBlockUntilTerminateMutex.Lock();

//   printf("%s %i subtask starting on new thread with priority\n", GetTaskName(),GetTaskNumber());

   mDoingTask=mTaskStarted=true;

   float workUntil = amountWork+PercentComplete();



   //check periodically to see if we should exit.
   mTerminateMutex.Lock();
   if(mTerminate)
   {
      mTerminateMutex.Unlock();
      SetIsRunning(false);
      mBlockUntilTerminateMutex.Unlock();
      return;
   }
   mTerminateMutex.Unlock();

   Update();


   if(UsesCustomWorkUntilPercentage())
      workUntil = ComputeNextWorkUntilPercentageComplete();

   if(workUntil<PercentComplete())
      workUntil = PercentComplete();

   //Do Some of the task.

   mTerminateMutex.Lock();
   while(PercentComplete() < workUntil && PercentComplete() < 1.0 && !mTerminate)
   {
      wxThread::This()->Yield();
      //release within the loop so we can cut the number of iterations short

      DoSomeInternal(); //keep the terminate mutex on so we don't remo
      mTerminateMutex.Unlock();
      //check to see if ondemand has been called
      if(GetNeedsODUpdate() && PercentComplete() < 1.0)
         ODUpdate();


      //But add the mutex lock back before we check the value again.
      mTerminateMutex.Lock();
   }
   mTerminateMutex.Unlock();
   mDoingTask=false;

   mTerminateMutex.Lock();
   //if it is not done, put it back onto the ODManager queue.
   if(PercentComplete() < 1.0&& !mTerminate)
   {
      ODManager::Instance()->AddTask(this);

      //we did a bit of progress - we should allow a resave.
      AudacityProject::AllProjectsDeleteLock();
      for(unsigned i=0; i<gAudacityProjects.GetCount(); i++)
      {
         if(IsTaskAssociatedWithProject(gAudacityProjects[i]))
         {
            //mark the changes so that the project can be resaved.
            gAudacityProjects[i]->GetUndoManager()->SetODChangesFlag();
            break;
         }
      }
      AudacityProject::AllProjectsDeleteUnlock();


//      printf("%s %i is %f done\n", GetTaskName(),GetTaskNumber(),PercentComplete());
   }
   else
   {
      //for profiling, uncomment and look in audacity.app/exe's folder for AudacityProfile.txt
      //static int tempLog =0;
      //if(++tempLog % 5==0)
         //END_TASK_PROFILING("On Demand Drag and Drop 5 80 mb files into audacity, 5 wavs per task");
      //END_TASK_PROFILING("On Demand open an 80 mb wav stereo file");

      wxCommandEvent event( EVT_ODTASK_COMPLETE );
      AudacityProject::AllProjectsDeleteLock();

      for(unsigned i=0; i<gAudacityProjects.GetCount(); i++)
      {
         if(IsTaskAssociatedWithProject(gAudacityProjects[i]))
         {
            //this assumes tasks are only associated with one project.
            gAudacityProjects[i]->GetEventHandler()->AddPendingEvent(event);
            //mark the changes so that the project can be resaved.
            gAudacityProjects[i]->GetUndoManager()->SetODChangesFlag();
            break;
         }
      }
      AudacityProject::AllProjectsDeleteUnlock();

//      printf("%s %i complete\n", GetTaskName(),GetTaskNumber());
   }
   mTerminateMutex.Unlock();
   SetIsRunning(false);
   mBlockUntilTerminateMutex.Unlock();
}

bool ODTask::IsTaskAssociatedWithProject(AudacityProject* proj)
{
   TrackList *tracks = proj->GetTracks();
   TrackListIterator iter1(tracks);
   Track *tr = iter1.First();

   while (tr)
   {
      //go over all tracks in the project
      if (tr->GetKind() == Track::Wave)
      {
         //look inside our task's track list for one that matches this projects one.
         mWaveTrackMutex.Lock();
         for(int i=0;i<(int)mWaveTracks.size();i++)
         {
            if(mWaveTracks[i]==tr)
            {
               //if we find one, then the project is associated with us;return true
               mWaveTrackMutex.Unlock();
               return true;
            }
         }
         mWaveTrackMutex.Unlock();
      }
      tr = iter1.Next();
   }

   return false;

}

void ODTask::ODUpdate()
{
   Update();
   ResetNeedsODUpdate();
}

void ODTask::SetIsRunning(bool value)
{
   mIsRunningMutex.Lock();
   mIsRunning=value;
   mIsRunningMutex.Unlock();
}

bool ODTask::IsRunning()
{
   bool ret;
   mIsRunningMutex.Lock();
   ret= mIsRunning;
   mIsRunningMutex.Unlock();
   return ret;
}

sampleCount ODTask::GetDemandSample()
{
   sampleCount retval;
   mDemandSampleMutex.Lock();
   retval = mDemandSample;
   mDemandSampleMutex.Unlock();
   return retval;
}

void ODTask::SetDemandSample(sampleCount sample)
{

   mDemandSampleMutex.Lock();
   mDemandSample=sample;
   mDemandSampleMutex.Unlock();
}


///return the amount of the task that has been completed.  0.0 to 1.0
float ODTask::PercentComplete()
{
   mPercentCompleteMutex.Lock();
   float ret = mPercentComplete;
   mPercentCompleteMutex.Unlock();
   return ret;
}

///return
bool ODTask::IsComplete()
{
   return PercentComplete() >= 1.0 && !IsRunning();
}


WaveTrack* ODTask::GetWaveTrack(int i)
{
   WaveTrack* track = NULL;
   mWaveTrackMutex.Lock();
   if(i<(int)mWaveTracks.size())
      track = mWaveTracks[i];
   mWaveTrackMutex.Unlock();
   return track;
}

///Sets the wavetrack that will be analyzed for ODPCMAliasBlockFiles that will
///have their summaries computed and written to disk.
void ODTask::AddWaveTrack(WaveTrack* track)
{
   mWaveTracks.push_back(track);
}


int ODTask::GetNumWaveTracks()
{
   int num;
   mWaveTrackMutex.Lock();
   num = (int)mWaveTracks.size();
   mWaveTrackMutex.Unlock();
   return num;
}


void ODTask::SetNeedsODUpdate()
{
   mNeedsODUpdateMutex.Lock();
   mNeedsODUpdate=true;
   mNeedsODUpdateMutex.Unlock();
}
bool ODTask::GetNeedsODUpdate()
{
   bool ret;
   mNeedsODUpdateMutex.Lock();
   ret=mNeedsODUpdate;
   mNeedsODUpdateMutex.Unlock();
   return ret;
}

void ODTask::ResetNeedsODUpdate()
{
   mNeedsODUpdateMutex.Lock();
   mNeedsODUpdate=false;
   mNeedsODUpdateMutex.Unlock();
}

///does an od update and then recalculates the data.
void ODTask::RecalculatePercentComplete()
{
   if(GetNeedsODUpdate())
   {
      ODUpdate();
      CalculatePercentComplete();
   }
}

///changes the tasks associated with this Waveform to process the task from a different point in the track
///@param track the track to update
///@param seconds the point in the track from which the tasks associated with track should begin processing from.
void ODTask::DemandTrackUpdate(WaveTrack* track, double seconds)
{
   bool demandSampleChanged=false;
   mWaveTrackMutex.Lock();
   for(size_t i=0;i<mWaveTracks.size();i++)
   {
      if(track == mWaveTracks[i])
      {
         sampleCount newDemandSample = (sampleCount)(seconds * track->GetRate());
         demandSampleChanged = newDemandSample != GetDemandSample();
         SetDemandSample(newDemandSample);
         break;
      }
   }
   mWaveTrackMutex.Unlock();

   if(demandSampleChanged)
      SetNeedsODUpdate();

}


void ODTask::StopUsingWaveTrack(WaveTrack* track)
{
   mWaveTrackMutex.Lock();
   for(size_t i=0;i<mWaveTracks.size();i++)
   {
      if(mWaveTracks[i] == track)
         mWaveTracks[i]=NULL;
   }
   mWaveTrackMutex.Unlock();
}

///Replaces all instances to a wavetrack with a new one, effectively transferring the task.
void ODTask::ReplaceWaveTrack(WaveTrack* oldTrack,WaveTrack* newTrack)
{
   mWaveTrackMutex.Lock();
   for(size_t i=0;i<mWaveTracks.size();i++)
   {
      if(oldTrack == mWaveTracks[i])
      {
         mWaveTracks[i] = newTrack;
      }
   }
   mWaveTrackMutex.Unlock();
}
