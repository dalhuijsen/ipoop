/**********************************************************************

  Audacity: A Digital Audio Editor

  VoiceKey.cpp

  ?? Dominic Mazzoni
  ?? Shane Muller

*******************************************************************//*!

\class VoiceKey

\brief
This implements a voice key, detecting either the next "ON"
or "OFF" point

*//*******************************************************************/


#include "Audacity.h"

#include "VoiceKey.h"
#include <wx/string.h>
#include <wx/msgdlg.h>
#include <math.h>
#include <stdio.h>

#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/textfile.h>
#include <wx/intl.h>
#include <iostream>

using std::cout;
using std::endl;



VoiceKey::VoiceKey(){

   mWindowSize = 0.01;                    //size of analysis window in seconds

   mEnergyMean = .0006;                   // reasonable initial levels assuming sampling rate of
   mEnergySD = .0002;                      //  44100 hertz
   mSignChangesMean = .08;
   mSignChangesSD= .02;
   mDirectionChangesMean = .25;
   mDirectionChangesSD = .2;

   AdjustThreshold(2);

   mSilentWindowSize = .05;              //Amount of time (in seconds) below threshold to call it silence
   mSignalWindowSize = .05;              //Amount of time (in seconds) above threshold to call it signal


   mUseEnergy = true;
   mUseSignChangesLow = false;
   mUseSignChangesHigh = false;
   mUseDirectionChangesLow = false;
   mUseDirectionChangesHigh = false;


};


VoiceKey::~VoiceKey(){
};



//---------------------------------------------------------------------------
//                VoiceKey::On/Off Forward/Backward
//  This operates in two phases:
// First, you take chunks of samples that are WindowSize big.
// If you have a run of them where something passes the threshold for SignalWindowSize seconds,
// you return to the last empty block and scan forward one sample at a time until you find the
// starting point of the speech.




//Move forward to find an ON region.
sampleCount VoiceKey::OnForward (WaveTrack & t, sampleCount start, sampleCount len) {

   if((mWindowSize) >= len+10){

      /* i18n-hint: Voice key is an experimental/incomplete feature that
         is used to navigate in vocal recordings, to move forwards and
         backwards by words.  So 'key' is being used in the sense of an index.
         This error message means that you've selected too short
         a region of audio to be able to use this feature.*/
      wxMessageBox(_("Selection is too small to use voice key."));
      return start;
   }
   else{

      sampleCount lastsubthresholdsample;     // keeps track of the sample number of the last sample to not exceed the threshold

      //Change the millisecond-based parameters into sample-based parameters
      double rate = t.GetRate();                                                     //Translates seconds to samples
      unsigned int WindowSizeInt = (unsigned int)(rate  * mWindowSize);               //Size of window to examine
      unsigned int SignalWindowSizeInt = (unsigned int)(rate  * mSignalWindowSize);   //This much signal is necessary to trip key

      int samplesleft = len - WindowSizeInt;   //Indexes the number of samples remaining in the selection
      lastsubthresholdsample = start;          //start this off at the selection start
      unsigned int i;                                   //iterates through waveblock
      int blockruns=0;                         //keeps track of the number of consecutive above-threshold blocks
      int blocksize;                           //The final block may be smaller than WindowSizeInt, so use this


      //This loop goes through the selection a block at a time.  If a long enough run
      //of above-threshold blocks occur, we return to the last sub-threshold block and
      //go through one sample at a time.
      //If there are fewer than 10 samples leftover, don't bother.

      for(i = start; samplesleft >=10; i+=(WindowSizeInt-1) , samplesleft -= (WindowSizeInt -1)){

         //Set blocksize so that it is the right size
         if((unsigned int)samplesleft < WindowSizeInt){
            blocksize = samplesleft;
         }
         else{
            blocksize = WindowSizeInt;
         }

         //Test whether we are above threshold (the number of stats)
         if(AboveThreshold(t,i,blocksize))
            {
               blockruns++;                   //Hit
            } else{
               blockruns=0;                   //Miss--start over
               lastsubthresholdsample = i;
            }

         //If the blockrun is long enough, break out of the loop early:
         if(blockruns > mSignalWindowSize/mWindowSize)
            break;

      }

      //Now, if we broke out early (samplesleft > 10), go back to the lastsubthresholdsample and look more carefully
      if(samplesleft > 10){


         //Calculate how many to scan through--we only have to go through (at most)
         //the first window + 1 samples--but we need another window samples to draw from.
         samplesleft = 2*WindowSizeInt+1;

         //To speed things up, create a local buffer to store things in, to avoid the costly t.Get();
         //Only go through the first SignalWindowSizeInt samples, and choose the first that trips the key.
         sampleFormat *buffer = new sampleFormat[samplesleft];
         t.Get((samplePtr)buffer, floatSample,lastsubthresholdsample,samplesleft);



         //Initialize these trend markers atrend and ztrend.  They keep track of the
         //up/down trends at the start and end of the evaluation window.
         int atrend = sgn(buffer[1]-buffer[0]);
         int ztrend = sgn(buffer[WindowSizeInt+1]-buffer[WindowSizeInt]);


         double erg=0;
         double  sc=0;
         double  dc=0;

         //Get initial test statistic values.
         if(mUseEnergy)
            erg = TestEnergy(t, lastsubthresholdsample, WindowSizeInt);

         if(mUseSignChangesLow || mUseSignChangesHigh)
            sc  = TestSignChanges(t,lastsubthresholdsample, WindowSizeInt);

         if(mUseDirectionChangesLow || mUseDirectionChangesHigh)
            dc  = TestDirectionChanges(t,lastsubthresholdsample,WindowSizeInt);


         //Now, go through the sound again, sample by sample.
         for(i=0; i<SignalWindowSizeInt-WindowSizeInt;i++){

            int tests = 0;
            int testThreshold = 0;
            //Update the test statistics
            if(mUseEnergy)
               {
                  TestEnergyUpdate(erg, WindowSizeInt,buffer[i],buffer[i+WindowSizeInt+1]);
                  tests += (int)(erg>mThresholdEnergy);
                  testThreshold++;
               }
            if(mUseSignChangesLow)
               {
                  TestSignChangesUpdate(sc,WindowSizeInt,buffer[i],buffer[i+1],buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(sc < mThresholdSignChangesLower);
                  testThreshold++;
               }

            if(mUseSignChangesHigh)
               {
                  TestSignChangesUpdate(sc,WindowSizeInt,buffer[i],buffer[i+1],buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(sc > mThresholdSignChangesUpper);
                  testThreshold++;
               }

            if(mUseDirectionChangesLow)
               {
                  TestDirectionChangesUpdate(dc,WindowSizeInt,atrend,buffer[i],buffer[i+1],ztrend,buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(dc < mThresholdDirectionChangesLower);
                  testThreshold++;
               }

            if(mUseDirectionChangesHigh)
               {
                  TestDirectionChangesUpdate(dc,WindowSizeInt,atrend,buffer[i],buffer[i+1],ztrend,buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(dc > mThresholdDirectionChangesUpper);
                  testThreshold++;
               }



            if(tests >= testThreshold)
               {	//Finish off on the first hit
                  break;
               }
         }

         //When we get here, i+lastsubthresholdsample is the best guess for where the word starts
         delete [] buffer;
         return i+lastsubthresholdsample;
      }
      else{
         //If we failed to find anything, return the start position
         return start ;
      }
   }
}

//Move backward from end to find an ON region.
sampleCount VoiceKey::OnBackward (WaveTrack & t, sampleCount end, sampleCount len) {


   if((mWindowSize) >= len+10){

      wxMessageBox(_("Selection is too small to use voice key."));
      return end;
   }
   else{

      sampleCount lastsubthresholdsample;     // keeps track of the sample number of the last sample to not exceed the threshold

      //Change the millisecond-based parameters into sample-based parameters
      double rate = t.GetRate();                                                     //Translates seconds to samples
      unsigned int WindowSizeInt = (unsigned int)(rate  * mWindowSize);               //Size of window to examine
      //unsigned int SilentWindowSizeInt = (unsigned int)(rate  * mSilentWindowSize);   //This much signal is necessary to trip key

      int samplesleft = len - WindowSizeInt;                 //Indexes the number of samples remaining in the selection
      lastsubthresholdsample = end;            //start this off at the end
      unsigned int i;                                   //iterates through waveblock
      int blockruns=0;                         //keeps track of the number of consecutive above-threshold blocks
      int blocksize;                           //The final block may be smaller than WindowSizeInt, so use this


      //This loop goes through the selection a block at a time in reverse order.  If a long enough run
      //of above-threshold blocks occur, we return to the last sub-threshold block and
      //go through one sample at a time.
      //If there are fewer than 10 samples leftover, don't bother.
      for(i = end - WindowSizeInt; samplesleft >=10; i-=(WindowSizeInt-1) , samplesleft -= (WindowSizeInt -1)){

         //Set blocksize so that it is the right size
         if(samplesleft < (int)WindowSizeInt){
            blocksize = samplesleft;
         }
         else{
            blocksize = WindowSizeInt;
         }


         //Test whether we are above threshold
         if(AboveThreshold(t,i,blocksize))
            {
               blockruns++;                   //Hit
            }
         else
            {
               blockruns=0;                   //Miss--start over
               lastsubthresholdsample = i+WindowSizeInt;
            }

         //If the blockrun is long enough, break out of the loop early:
         if(blockruns > mSilentWindowSize/mWindowSize)
            break;

      }

      //Now, if we broke out early (samplesleft > 10), go back to the lastsubthresholdsample and look more carefully
      if(samplesleft > 10){

         //Calculate how many to scan through--we only have to go through (at most)
         //the first window + 1 samples--but we need another window samples to draw from.
         samplesleft = 2*WindowSizeInt+1;

         //To speed things up, create a local buffer to store things in, to avoid the costly t.Get();
         //Only go through the first mSilentWindowSizeInt samples, and choose the first that trips the key.
         sampleFormat *buffer = new sampleFormat[samplesleft];
         t.Get((samplePtr)buffer, floatSample, lastsubthresholdsample-samplesleft,samplesleft);

         //Initialize these trend markers atrend and ztrend.  They keep track of the
         //up/down trends at the start and end of the evaluation window.
         int atrend = sgn(buffer[samplesleft - 2]-buffer[samplesleft - 1]);
         int ztrend = sgn(buffer[samplesleft - WindowSizeInt-2]-buffer[samplesleft - WindowSizeInt-2]);

         double erg=0;
         double sc = 0;
         double dc = 0;

         //Get initial test statistic values.
         if(mUseEnergy)
            erg = TestEnergy(t, lastsubthresholdsample, WindowSizeInt);
         if(mUseSignChangesLow || mUseSignChangesHigh)
            sc  = TestSignChanges(t,lastsubthresholdsample, WindowSizeInt);
         if(mUseDirectionChangesLow || mUseDirectionChangesHigh)
            dc  = TestDirectionChanges(t,lastsubthresholdsample,WindowSizeInt);

         //Now, go through the sound again, sample by sample.
         for(i=samplesleft-1;  i>WindowSizeInt; i--){
            int tests = 0;
            int testThreshold = 0;
            //Update the test statistics
            if(mUseEnergy)
               {
                  TestEnergyUpdate(erg, WindowSizeInt,buffer[i],buffer[i+WindowSizeInt+1]);
                  tests += (int)(erg>mThresholdEnergy);
                  testThreshold++;
               }
            if(mUseSignChangesLow)
               {
                  TestSignChangesUpdate(sc,WindowSizeInt,buffer[i],buffer[i+1],buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(sc < mThresholdSignChangesLower);
                  testThreshold++;
               }
            if(mUseSignChangesHigh)
               {
                  TestSignChangesUpdate(sc,WindowSizeInt,buffer[i],buffer[i+1],buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(sc > mThresholdSignChangesUpper);
                  testThreshold++;
               }
            if(mUseDirectionChangesLow)
               {
                  TestDirectionChangesUpdate(dc,WindowSizeInt,atrend,buffer[i],buffer[i+1],ztrend,buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(dc < mThresholdDirectionChangesLower);
                  testThreshold++;
               }
            if(mUseDirectionChangesHigh)
               {
                  TestDirectionChangesUpdate(dc,WindowSizeInt,atrend,buffer[i],buffer[i+1],ztrend,buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(dc > mThresholdDirectionChangesUpper);
                  testThreshold++;
               }

            if(tests >= testThreshold)
               {	//Finish off on the first hit
                  break;
               }
         }

         //When we get here, i+lastsubthresholdsample is the best guess for where the word starts
         delete [] buffer;
         return lastsubthresholdsample - samplesleft + i;
      }
      else{
         //If we failed to find anything, return the start position
         return end ;
      }
   }
}


//Move froward from the start to find an OFF region.
sampleCount VoiceKey::OffForward (WaveTrack & t, sampleCount start, sampleCount len) {

   if((mWindowSize) >= len+10){
      wxMessageBox(_("Selection is too small to use voice key."));

      return start;
   }
   else{

       sampleCount lastsubthresholdsample;     // keeps track of the sample number of the last sample to not exceed the threshold


      //Change the millisecond-based parameters into sample-based parameters
      double rate = t.GetRate();                                                     //Translates seconds to samples
      unsigned int WindowSizeInt = (unsigned int)(rate  * mWindowSize);               //Size of window to examine
      unsigned int SilentWindowSizeInt = (unsigned int)(rate  * mSilentWindowSize);   //This much signal is necessary to trip key

      int samplesleft = len - WindowSizeInt;   //Indexes the number of samples remaining in the selection
      lastsubthresholdsample = start;          //start this off at the selection start
      unsigned int i;                                   //iterates through waveblock
      int blockruns=0;                         //keeps track of the number of consecutive above-threshold blocks
      int blocksize;                           //The final block may be smaller than WindowSizeInt, so use this

      //This loop goes through the selection a block at a time.  If a long enough run
      //of above-threshold blocks occur, we return to the last sub-threshold block and
      //go through one sample at a time.
      //If there are fewer than 10 samples leftover, don't bother.
      for(i = start; samplesleft >=10; i+=(WindowSizeInt-1) , samplesleft -= (WindowSizeInt -1)){

         //Set blocksize so that it is the right size
         if(samplesleft < (int)WindowSizeInt){
            blocksize = samplesleft;
         }
         else{
            blocksize = WindowSizeInt;
         }

         if(!AboveThreshold(t,i,blocksize))
            {
               blockruns++;                   //Hit
            }
         else
            {
               blockruns=0;                   //Above threshold--start over
               lastsubthresholdsample = i;
            }

         //If the blockrun is long enough, break out of the loop early:
         if(blockruns > mSilentWindowSize/mWindowSize)
            break;

      }

      //Now, if we broke out early (samplesleft > 10), go back to the lastsubthresholdsample and look more carefully
      if(samplesleft > 10){


         //Calculate how many to scan through--we only have to go through (at most)
         //the first window + 1 samples--but we need another window samples to draw from.
         samplesleft = 2*WindowSizeInt+1;

         //To speed things up, create a local buffer to store things in, to avoid the costly t.Get();
         //Only go through the first SilentWindowSizeInt samples, and choose the first that trips the key.
         sampleFormat *buffer = new sampleFormat[samplesleft];
         t.Get((samplePtr)buffer, floatSample, lastsubthresholdsample,samplesleft);

         //Initialize these trend markers atrend and ztrend.  They keep track of the
         //up/down trends at the start and end of the evaluation window.
         int atrend = sgn(buffer[1]-buffer[0]);
         int ztrend = sgn(buffer[WindowSizeInt+1]-buffer[WindowSizeInt]);


         double erg=0;
         double sc=0;
         double dc=0;

         //Get initial test statistic values.
         if(mUseEnergy)
            erg = TestEnergy(t, lastsubthresholdsample, WindowSizeInt);
         if(mUseSignChangesLow || mUseSignChangesHigh)
            sc  = TestSignChanges(t,lastsubthresholdsample, WindowSizeInt);
         if(mUseDirectionChangesLow || mUseDirectionChangesHigh)
            dc  = TestDirectionChanges(t,lastsubthresholdsample,WindowSizeInt);

         //Now, go through the sound again, sample by sample.
         for(i=0; i<SilentWindowSizeInt-WindowSizeInt;i++){
            int tests = 0;
            int testThreshold = 0;
            //Update the test statistics
            if(mUseEnergy)
               {
                  TestEnergyUpdate(erg, WindowSizeInt,buffer[i],buffer[i+WindowSizeInt+1]);
                  tests += (int)(erg>mThresholdEnergy);
                  testThreshold++;
               }
            if(mUseSignChangesLow)
               {
                  TestSignChangesUpdate(sc,WindowSizeInt,buffer[i],buffer[i+1],buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(sc < mThresholdSignChangesLower);
                  testThreshold++;
               }
            if(mUseSignChangesHigh)
               {
                  TestSignChangesUpdate(sc,WindowSizeInt,buffer[i],buffer[i+1],buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(sc > mThresholdSignChangesUpper);
                  testThreshold++;
               }
            if(mUseDirectionChangesLow)
               {
                  TestDirectionChangesUpdate(dc,WindowSizeInt,atrend,buffer[i],buffer[i+1],ztrend,buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(dc < mThresholdDirectionChangesLower);
                  testThreshold++;
               }
            if(mUseDirectionChangesHigh)
               {
                  TestDirectionChangesUpdate(dc,WindowSizeInt,atrend,buffer[i],buffer[i+1],ztrend,buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(dc > mThresholdDirectionChangesUpper);
                  testThreshold++;
               }

            if(tests < testThreshold)
               {	//Finish off on the first below-threshold block
                  break;
               }
         }

         //When we get here, i+lastsubthresholdsample is the best guess for where the word starts
         delete [] buffer;
         return i+lastsubthresholdsample;
      }
      else{
         //If we failed to find anything, return the start position
         return start ;
      }
   }
}


//Move backward from the end to find an OFF region
sampleCount VoiceKey::OffBackward (WaveTrack & t, sampleCount end, sampleCount len) {


   if((mWindowSize) >= len+10){

      wxMessageBox(_("Selection is too small to use voice key."));
      return end;
   }
   else{

      sampleCount lastsubthresholdsample;     // keeps track of the sample number of the last sample to not exceed the threshold

      //Change the millisecond-based parameters into sample-based parameters
      double rate = t.GetRate();                                                     //Translates seconds to samples
      unsigned int WindowSizeInt = (unsigned int)(rate  * mWindowSize);               //Size of window to examine
      //unsigned int SilentWindowSizeInt = (unsigned int)(rate  * mSilentWindowSize);   //This much signal is necessary to trip key

      int samplesleft = len - WindowSizeInt;                 //Indexes the number of samples remaining in the selection
      lastsubthresholdsample = end;            //start this off at the end
      unsigned int i;                                   //iterates through waveblock
      int blockruns=0;                         //keeps track of the number of consecutive above-threshold blocks
      int blocksize;                           //The final block may be smaller than WindowSizeInt, so use this

      //This loop goes through the selection a block at a time in reverse order.  If a long enough run
      //of above-threshold blocks occur, we return to the last sub-threshold block and
      //go through one sample at a time.
      //If there are fewer than 10 samples leftover, don't bother.
      for(i = end - WindowSizeInt; samplesleft >=10; i-=(WindowSizeInt-1) , samplesleft -= (WindowSizeInt -1)){

         //Set blocksize so that it is the right size
         if(samplesleft < (int)WindowSizeInt){
            blocksize = samplesleft;
         }
         else{
            blocksize = WindowSizeInt;
         }

         if(!AboveThreshold(t,i,blocksize))
            {

               blockruns++;                   //Hit
            }
         else
            {
               blockruns=0;                   //Miss--start over
               lastsubthresholdsample = i+WindowSizeInt;

            }

         //If the blockrun is long enough, break out of the loop early:
         if(blockruns > mSilentWindowSize/mWindowSize)
            break;

      }

      //Now, if we broke out early (samplesleft > 10), go back to the lastsubthresholdsample and look more carefully
      if(samplesleft > 10){

         //Calculate how many to scan through--we only have to go through (at most)
         //the first window + 1 samples--but we need another window samples to draw from.
         samplesleft = 2*WindowSizeInt+1;

         //To speed things up, create a local buffer to store things in, to avoid the costly t.Get();
         //Only go through the first SilentWindowSizeInt samples, and choose the first that trips the key.
         sampleFormat *buffer = new sampleFormat[samplesleft];
         t.Get((samplePtr)buffer, floatSample, lastsubthresholdsample-samplesleft,samplesleft);

         //Initialize these trend markers atrend and ztrend.  They keep track of the
         //up/down trends at the start and end of the evaluation window.
         int atrend = sgn(buffer[samplesleft - 2]-buffer[samplesleft - 1]);
         int ztrend = sgn(buffer[samplesleft - WindowSizeInt-2]-buffer[samplesleft - WindowSizeInt-2]);

         double erg=0;
         double  sc=0;
         double  dc=0;
         //Get initial test statistic values.
         if(mUseEnergy)
            erg = TestEnergy(t, lastsubthresholdsample, WindowSizeInt);
         if(mUseSignChangesLow || mUseSignChangesHigh)
            sc  = TestSignChanges(t,lastsubthresholdsample, WindowSizeInt);
         if(mUseDirectionChangesLow || mUseDirectionChangesHigh)
            dc  = TestDirectionChanges(t,lastsubthresholdsample,WindowSizeInt);

         //Now, go through the sound again, sample by sample.
         for(i=samplesleft-1;  i>WindowSizeInt; i--){

            int tests = 0;
            int testThreshold = 0;
            //Update the test statistics
            if(mUseEnergy)
               {
                  TestEnergyUpdate(erg, WindowSizeInt,buffer[i],buffer[i+WindowSizeInt+1]);
                  tests += (int)(erg>mThresholdEnergy);
                  testThreshold++;
               }
            if(mUseSignChangesLow)
               {
                  TestSignChangesUpdate(sc,WindowSizeInt,buffer[i],buffer[i+1],buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(sc < mThresholdSignChangesLower);
                  testThreshold++;
               }
            if(mUseSignChangesHigh)
               {
                  TestSignChangesUpdate(sc,WindowSizeInt,buffer[i],buffer[i+1],buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(sc > mThresholdSignChangesUpper);
                  testThreshold++;
               }
            if(mUseDirectionChangesLow)
               {
                  TestDirectionChangesUpdate(dc,WindowSizeInt,atrend,buffer[i],buffer[i+1],ztrend,buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(dc < mThresholdDirectionChangesLower);
                  testThreshold++;
               }
            if(mUseDirectionChangesHigh)
               {
                  TestDirectionChangesUpdate(dc,WindowSizeInt,atrend,buffer[i],buffer[i+1],ztrend,buffer[i+WindowSizeInt],buffer[i+WindowSizeInt+1]);
                  tests += (int)(dc > mThresholdDirectionChangesUpper);
                  testThreshold++;
               }



            if(tests < testThreshold)
               {	//Finish off on the first hit
                  break;
               }
         }

         //When we get here, i+lastsubthresholdsample is the best guess for where the word starts
         delete [] buffer;
         return lastsubthresholdsample - samplesleft + i;
      }
      else{
         //If we failed to find anything, return the start position
         return end ;
      }
   }
}

//This tests whether a specified block region is above or below threshold.
bool VoiceKey::AboveThreshold(WaveTrack & t, sampleCount start, sampleCount len)
{

   double erg=0;
   double  sc=0;
   double  dc=0;   //These store three statistics: energy, signchanges, and directionchanges
   int tests =0;   //Keeps track of how many statistics surpass the threshold.
   int testThreshold=0;  //Keeps track of the threshold.

   //Calculate the test statistics
   if(mUseEnergy)
      {
         testThreshold++;
         erg = TestEnergy(t, start,len);
         tests +=(int)(erg > mThresholdEnergy);
#if 0
         std::cout << "Energy: " << erg << " " <<mThresholdEnergy << std::endl;
#endif
      }

   if(mUseSignChangesLow)
      {
         testThreshold++;
         sc  = TestSignChanges(t,start,len);
         tests += (int)(sc < mThresholdSignChangesLower);
#if 0
         std::cout << "SignChanges: " << sc << " " <<mThresholdSignChangesLower<< " < " << mThresholdSignChangesUpper << std::endl;
#endif

      }
   if(mUseSignChangesHigh)
      {
         testThreshold++;
         sc  = TestSignChanges(t,start,len);
         tests += (int)(sc > mThresholdSignChangesUpper);
#if 0
         std::cout << "SignChanges: " << sc << " " <<mThresholdSignChangesLower<< " < " << mThresholdSignChangesUpper << std::endl;
#endif

      }


   if(mUseDirectionChangesLow)
      {
         testThreshold++;
         dc  = TestDirectionChanges(t,start,len);
         tests += (int)(dc < mThresholdDirectionChangesLower);
#if 0
         std::cout << "DirectionChanges: " << dc << " " <<mThresholdDirectionChangesLower<< " < " << mThresholdDirectionChangesUpper << std::endl;
#endif
      }
   if(mUseDirectionChangesHigh)
      {
         testThreshold++;
         dc  = TestDirectionChanges(t,start,len);
         tests += (int)(dc > mThresholdDirectionChangesUpper);
#if 0
         std::cout << "DirectionChanges: " << dc << " " <<mThresholdDirectionChangesLower<< " < " << mThresholdDirectionChangesUpper << std::endl;
#endif
      }

   //Test whether we are above threshold (the number of stats)
   return (tests >= testThreshold);

}

//This adjusts the threshold.  Larger values of t expand the noise region,
//making more things be classified as noise (and requiring a stronger signal).
void VoiceKey::AdjustThreshold(double t){

   mThresholdAdjustment = t;
   mThresholdEnergy = mEnergyMean + mEnergySD * t;
   mThresholdSignChangesUpper = mSignChangesMean + mSignChangesSD * t;
   mThresholdSignChangesLower = mSignChangesMean - mSignChangesSD * t;
   mThresholdDirectionChangesUpper = mDirectionChangesMean + mDirectionChangesSD * t;
   mThresholdDirectionChangesLower = mDirectionChangesMean - mDirectionChangesSD * t;
};


//This 'calibrates' the voicekey to noise
void VoiceKey::CalibrateNoise(WaveTrack & t, sampleCount start, sampleCount len){
   //To calibrate the noise, we need to scan the sample block just like in the voicekey and
   //calculate the mean and standard deviation of the test statistics.
   //Then, we set the BaselineThreshold to be one

   wxBusyCursor busy;

   //initialize some sample statistics: sums of X and X^2

   double sumerg, sumerg2;
   double sumsc, sumsc2;
   double sumdc, sumdc2;
   double erg, sc, dc;
   //Now, change the millisecond-based parameters into sample-based parameters
   //(This depends on WaveTrack t)
   double rate = t.GetRate();
   unsigned int WindowSizeInt = (unsigned int)(rate  * mWindowSize);
   //   unsigned int SignalWindowSizeInt = (unsigned int)(rate  * mSignalWindowSize);


   //Get the first test statistics

   //Calibrate all of the statistic, because they might be
   //changed later.

   //   if(mUseEnergy)
   erg = TestEnergy(t, start, WindowSizeInt);

   //   if(mUseSignChanges)
   sc = TestSignChanges(t,start, WindowSizeInt);

   //   if(mUseDirectionChanges)
   dc = TestDirectionChanges(t,start,WindowSizeInt);

   sumerg =0.0;
   sumerg2 = 0.0;
   sumsc =0.0;
   sumsc2 = 0.0;
   sumdc =0.0;
   sumdc2 =0.0;


   //	int n = len - WindowSizeInt;        //This is how many samples we have
   int samplesleft = len - WindowSizeInt;
   int i;
   int blocksize;
   int samples=0;

   for(i = start; samplesleft >=10 ; i += (WindowSizeInt -1), samplesleft -= (WindowSizeInt -1) )
      {
         //Take samples chunk-by-chunk.
         //Normally, this should be in WindowSizeInt chunks, but at the end (if there are more than 10
         //samples left) take a chunk that eats the rest of the samples.

         samples++;          //Increment the number of samples we have
         if(samplesleft < (int)WindowSizeInt)
            {
               blocksize = samplesleft;
            }
         else
            {
               blocksize = WindowSizeInt;
            }

         erg = TestEnergy(t, i, blocksize);
         sumerg +=(double)erg;
         sumerg2 += pow((double)erg,2);

         sc = TestSignChanges(t,i, blocksize);
         sumsc += (double)sc;
         sumsc2 += pow((double)sc,2);


         dc = TestDirectionChanges(t,i,blocksize);
         sumdc += (double)dc;
         sumdc2 += pow((double)dc,2);
      }

   mEnergyMean = sumerg / samples;
   mEnergySD =  sqrt(sumerg2/samples - mEnergyMean*mEnergyMean);

   mSignChangesMean = sumsc / samples;
   mSignChangesSD = sqrt(sumsc2 / samples - mSignChangesMean * mSignChangesMean);

   mDirectionChangesMean = sumdc / samples;
   mDirectionChangesSD =sqrt(sumdc2 / samples - mDirectionChangesMean * mDirectionChangesMean) ;

   wxString text =   wxString::Format(_("Calibration Results\n"));
   /* i18n-hint: %1.4f is replaced by a number.  sd stands for 'Standard Deviations'*/
   text +=           wxString::Format(_("Energy                  -- mean: %1.4f  sd: (%1.4f)\n"),mEnergyMean,mEnergySD);
   text+=            wxString::Format(_("Sign Changes        -- mean: %1.4f  sd: (%1.4f)\n"),mSignChangesMean,mSignChangesSD);
   text+=            wxString::Format(_("Direction Changes  -- mean: %1.4f  sd: (%1.4f)\n"),mDirectionChangesMean,mDirectionChangesSD);
   wxMessageDialog  * stats =  new wxMessageDialog(NULL, text,
                                                  wxT("Calibration Complete"),
                                                  wxOK | wxICON_INFORMATION,
                                                  wxPoint(-1,-1));
   stats->ShowModal();
   delete stats;

   AdjustThreshold(mThresholdAdjustment);
}


void VoiceKey::SetKeyType(bool erg, bool scLow , bool scHigh,
                          bool dcLow, bool dcHigh)
{
   mUseEnergy = erg;
   mUseSignChangesLow = scLow;
   mUseSignChangesHigh = scHigh;
   mUseDirectionChangesLow = dcLow;
   mUseDirectionChangesHigh = dcHigh;
}


//This might continue over a number of blocks.
double VoiceKey::TestEnergy (WaveTrack & t, sampleCount start, sampleCount len)
{

   double sum = 1;
   sampleCount s = start;                                //Keep track of start
   sampleCount originalLen = len;                        //Keep track of the length of block to process (its not the length of t)
   sampleCount blockSize = t.GetMaxBlockSize();         //Determine size of sampling buffer


   if( blockSize > len)
      blockSize = len;
   float *buffer = new float[blockSize];       //Get a sampling buffer

   while(len > 0)
      {
         sampleCount block = t.GetBestBlockSize(s);          //Figure out how much to grab
         if(block > len)	block = len;                 //Don't grab too much!

         t.Get((samplePtr)buffer,floatSample, s,block);                      //grab the block;

         //Now, go through the block and calculate energy
         for(int i = 0; i< block; i++)
            {
               sum += buffer[i]*buffer[i];
            }

         len -= block;
         s += block;
      }

   delete [] buffer;
   return sum / originalLen;
}


//This will update RMSE by adding one element and subtracting another
void VoiceKey::TestEnergyUpdate (double & prevErg, int len, const sampleFormat & drop, const sampleFormat & add)
{
   //This is an updating formula for RMSE. It will only recalculate what's changed.
   prevErg =  prevErg + (double)(abs(add) - abs(drop))/len;

}


double VoiceKey::TestSignChanges(WaveTrack & t, sampleCount start, sampleCount len)
{


   sampleCount s = start;                                //Keep track of start
   sampleCount originalLen = len;                        //Keep track of the length of block to process (its not the length of t)
   sampleCount blockSize = t.GetMaxBlockSize();         //Determine size of sampling buffer
   unsigned long signchanges = 1;
   int currentsign=0;


   if( blockSize > len)
      blockSize = len;
   sampleFormat *buffer = new sampleFormat[blockSize];       //Get a sampling buffer

   while(len > 0) {

      sampleCount block = t.GetBestBlockSize(s);  //Figure out how much to grab
      if(block > len)	block = len;                 //Don't grab too much!

      t.Get((samplePtr)buffer,floatSample, s,block);                      //grab the block;

      if  (len == originalLen)
         {
            //The first time through, set stuff up special.
            currentsign = sgn(buffer[0]);
         }

      //Now, go through the block and calculate zero crossings

      for(int i = 0; i< block; i++)
         {
            if( sgn(buffer[i]) != currentsign)
               {
                  currentsign = sgn(buffer[i]);
                  signchanges++;
               }

         }
      len -= block;
      s += block;
   }
   delete [] buffer;
   return (double)signchanges/originalLen;
}

void VoiceKey::TestSignChangesUpdate(double & currentsignchanges, int len,
                                     const sampleFormat & a1,
                                     const sampleFormat & a2,
                                     const sampleFormat & z1,
                                     const sampleFormat & z2)
{

   if(sgn(a1)!=sgn(a2)) currentsignchanges -= 1.0/len;
   if(sgn(z1)!=sgn(z2)) currentsignchanges += 1.0/len;

}


double VoiceKey::TestDirectionChanges(WaveTrack & t, sampleCount start, sampleCount len)
{


   sampleCount s = start;                                //Keep track of start
   sampleCount originalLen = len;                        //Keep track of the length of block to process (its not the length of t)
   sampleCount blockSize = t.GetMaxBlockSize();         //Determine size of sampling buffer
   unsigned long directionchanges = 1;
   sampleFormat lastval=sampleFormat(0);
   int lastdirection=1;

   if( blockSize > len)
      blockSize = len;
   sampleFormat *buffer = new sampleFormat[blockSize];       //Get a sampling buffer

   while(len > 0) {

      sampleCount block = t.GetBestBlockSize(s);  //Figure out how much to grab
      if(block > len)	block = len;                 //Don't grab too much!

      t.Get((samplePtr)buffer,floatSample, s,block);                      //grab the block;

      if  (len == originalLen) {
         //The first time through, set stuff up special.
         lastval = buffer[0];
      }

      //Now, go through the block and calculate zero crossings


      for(int i = 0; i< block; i++){

         if( sgn(buffer[i]-lastval) != lastdirection) {
            directionchanges++;
            lastdirection = sgn(buffer[i] - lastval);
         }
         lastval = buffer[i];

      }
      len -= block;
      s += block;
   }
   delete [] buffer;
   return (double)directionchanges/originalLen;
}




// This method does an updating by looking at the trends
// This will change currentdirections and atrend/trend, so be warned.
void VoiceKey::TestDirectionChangesUpdate(double & currentdirectionchanges, int len,
                                          int & atrend, const sampleFormat & a1, const sampleFormat & a2,
                                          int & ztrend, const sampleFormat & z1, const sampleFormat & z2)
{

   if(sgn(a2 - a1)!= atrend ) {
      //Here, the direction shifted for the item we're dropping.
      currentdirectionchanges -= 1.0/len;
      atrend = sgn(a2-a1);
   }
   if(sgn(z2 - z1)!= ztrend){
      //Here, the direction shifts when we add an item
      currentdirectionchanges += 1.0/len;
      ztrend = sgn(z2-z1);
   }

}
