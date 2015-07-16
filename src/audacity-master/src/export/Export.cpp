/**********************************************************************

  Audacity: A Digital Audio Editor

  Export.cpp

  Dominic Mazzoni

*******************************************************************//**

\class Export
\brief Main class to control the export function.

*//****************************************************************//**

\class ExportType
\brief Container for information about supported export types.

*//****************************************************************//**

\class ExportMixerDialog
\brief Dialog for advanced mixing.

*//****************************************************************//**

\class ExportMixerPanel
\brief Panel that displays mixing for advanced mixing option.

*//********************************************************************/

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/window.h>
#endif

#include <wx/dynarray.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/dcmemory.h>

#include "Export.h"
#include "ExportPCM.h"
#include "ExportMP3.h"
#include "ExportOGG.h"
#include "ExportFLAC.h"
#include "ExportCL.h"
#include "ExportMP2.h"
#include "ExportFFmpeg.h"

#include "sndfile.h"

#include "FileDialog.h"

#include "../Audacity.h"
#include "../DirManager.h"
#include "../FileFormats.h"
#include "../Internat.h"
#include "../LabelTrack.h"
#include "../Mix.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../Track.h"
#include "../WaveTrack.h"
#include "../widgets/Warning.h"
#include "../AColor.h"
#include "../TimeTrack.h"
#include "../Dependencies.h"

// Callback to display format options
static void ExportCallback(void *cbdata, int index)
{
   ((Exporter *) cbdata)->DisplayOptions(index);
}

//----------------------------------------------------------------------------
// ExportPlugin
//----------------------------------------------------------------------------
#include <wx/arrimpl.cpp>

WX_DEFINE_USER_EXPORTED_OBJARRAY(ExportPluginArray);
WX_DEFINE_USER_EXPORTED_OBJARRAY(FormatInfoArray);

ExportPlugin::ExportPlugin()
{
   mFormatInfos.Empty();
}

ExportPlugin::~ExportPlugin()
{
   mFormatInfos.Clear();
}

bool ExportPlugin::CheckFileName(wxFileName & WXUNUSED(filename), int WXUNUSED(format))
{
  return true;
}

/** \brief Add a new entry to the list of formats this plug-in can export
 *
 * To configure the format use SetFormat, SetCanMetaData etc with the index of
 * the format.
 * @return The number of formats currently set up. This is one more than the
 * index of the newly added format.
 */
int ExportPlugin::AddFormat()
{
   FormatInfo nf;
   mFormatInfos.Add(nf);
   return mFormatInfos.Count();
}

int ExportPlugin::GetFormatCount()
{
   return mFormatInfos.Count();
}

void ExportPlugin::Destroy()
{
   delete this;
}

/**
 * @param index The plugin to set the format for (range 0 to one less than the
 * count of formats)
 */
void ExportPlugin::SetFormat(const wxString & format, int index)
{
   mFormatInfos[index].mFormat = format;
}

void ExportPlugin::SetDescription(const wxString & description, int index)
{
   mFormatInfos[index].mDescription = description;
}

void ExportPlugin::AddExtension(const wxString &extension,int index)
{
   mFormatInfos[index].mExtensions.Add(extension);
}

void ExportPlugin::SetExtensions(const wxArrayString & extensions, int index)
{
   mFormatInfos[index].mExtensions = extensions;
}

void ExportPlugin::SetMask(const wxString & mask, int index)
{
   mFormatInfos[index].mMask = mask;
}

void ExportPlugin::SetMaxChannels(int maxchannels, int index)
{
   mFormatInfos[index].mMaxChannels = maxchannels;
}

void ExportPlugin::SetCanMetaData(bool canmetadata, int index)
{
   mFormatInfos[index].mCanMetaData = canmetadata;
}

wxString ExportPlugin::GetFormat(int index)
{
   return mFormatInfos[index].mFormat;
}

wxString ExportPlugin::GetDescription(int index)
{
   return mFormatInfos[index].mDescription;
}

wxString ExportPlugin::GetExtension(int index)
{
   return mFormatInfos[index].mExtensions[0];
}

wxArrayString ExportPlugin::GetExtensions(int index)
{
   return mFormatInfos[index].mExtensions;
}

wxString ExportPlugin::GetMask(int index)
{
   if (!mFormatInfos[index].mMask.IsEmpty()) {
      return mFormatInfos[index].mMask;
   }

   wxString mask = GetDescription(index) + wxT("|");

   // Build the mask
   wxString ext = GetExtension(index);
   wxArrayString exts = GetExtensions(index);
   for (size_t i = 0; i < exts.GetCount(); i++) {
      mask += wxT("*.") + exts[i] + wxT(";");
   }

   return mask;
}

int ExportPlugin::GetMaxChannels(int index)
{
   return mFormatInfos[index].mMaxChannels;
}

bool ExportPlugin::GetCanMetaData(int index)
{
   return mFormatInfos[index].mCanMetaData;
}

bool ExportPlugin::IsExtension(wxString & ext, int index)
{
   bool isext = false;
   for (int i = index; i < GetFormatCount(); i = GetFormatCount())
   {
      wxString defext = GetExtension(i);
      wxArrayString defexts = GetExtensions(i);
      int indofext = defexts.Index(ext, false);
      if (defext == wxT("") || (indofext != wxNOT_FOUND))
         isext = true;
   }
   return isext;
}

bool ExportPlugin::DisplayOptions(wxWindow * WXUNUSED(parent), int WXUNUSED(format))
{
   return false;
}

int ExportPlugin::Export(AudacityProject *project,
                          int channels,
                          wxString fName,
                          bool selectedOnly,
                          double t0,
                          double t1,
                          MixerSpec *mixerSpec,
                          Tags * WXUNUSED(metadata),
                          int subformat)
{
   if (project == NULL) {
      project = GetActiveProject();
   }

  return DoExport(project, channels, fName, selectedOnly, t0, t1, mixerSpec, subformat);
}

int ExportPlugin::DoExport(AudacityProject * WXUNUSED(project),
                            int WXUNUSED(channels),
                            wxString WXUNUSED(fName),
                            bool WXUNUSED(selectedOnly),
                            double WXUNUSED(t0),
                            double WXUNUSED(t1),
                            MixerSpec * WXUNUSED(mixerSpec),
                            int WXUNUSED(subformat))
{
   return false;
}

//Create a mixer by computing the time warp factor
Mixer* ExportPlugin::CreateMixer(int numInputTracks, WaveTrack **inputTracks,
         TimeTrack *timeTrack,
         double startTime, double stopTime,
         int numOutChannels, int outBufferSize, bool outInterleaved,
         double outRate, sampleFormat outFormat,
         bool highQuality, MixerSpec *mixerSpec)
{
   // MB: the stop time should not be warped, this was a bug.
   return new Mixer(numInputTracks, inputTracks,
                  Mixer::WarpOptions(timeTrack),
                  startTime, stopTime,
                  numOutChannels, outBufferSize, outInterleaved,
                  outRate, outFormat,
                  highQuality, mixerSpec);
}
//----------------------------------------------------------------------------
// Export
//----------------------------------------------------------------------------

Exporter::Exporter()
{
   mMixerSpec = NULL;
   SetFileDialogTitle( _("Export Audio") );

   RegisterPlugin(New_ExportPCM());
   RegisterPlugin(New_ExportMP3());

#ifdef USE_LIBVORBIS
   RegisterPlugin(New_ExportOGG());
#endif

#ifdef USE_LIBFLAC
   RegisterPlugin(New_ExportFLAC());
#endif

#if USE_LIBTWOLAME
   RegisterPlugin(New_ExportMP2());
#endif

   // Command line export not available on Windows and Mac platforms
   RegisterPlugin(New_ExportCL());

#if defined(USE_FFMPEG)
   RegisterPlugin(New_ExportFFmpeg());
#endif
}

Exporter::~Exporter()
{
   for (size_t i = 0; i < mPlugins.GetCount(); i++) {
      mPlugins[i]->Destroy();
   }
   mPlugins.Clear();

   if (mMixerSpec) {
      delete mMixerSpec;
   }
}

void Exporter::SetFileDialogTitle( const wxString & DialogTitle )
{
   // The default title is "Export File"
   mFileDialogTitle = DialogTitle;
}

int Exporter::FindFormatIndex(int exportindex)
{
   int c = 0;
   for (size_t i = 0; i < mPlugins.GetCount(); i++)
   {
      for (int j = 0; j < mPlugins[i]->GetFormatCount(); j++)
      {
         if (exportindex == c) return j;
         c++;
      }
   }
   return 0;
}

void Exporter::RegisterPlugin(ExportPlugin *ExportPlugin)
{
   mPlugins.Add(ExportPlugin);
}

const ExportPluginArray Exporter::GetPlugins()
{
   return mPlugins;
}

bool Exporter::Process(AudacityProject *project, bool selectedOnly, double t0, double t1)
{
   // Save parms
   mProject = project;
   mSelectedOnly = selectedOnly;
   mT0 = t0;
   mT1 = t1;

   // Gather track information
   if (!ExamineTracks()) {
      return false;
   }

   // Ask user for file name
   if (!GetFilename()) {
      return false;
   }

   // Check for down mixing
   if (!CheckMix()) {
      return false;
   }

   // Let user edit MetaData
   if (mPlugins[mFormat]->GetCanMetaData(mSubFormat)) {
      if (!(project->GetTags()->ShowEditDialog(project, _("Edit Metadata"), mProject->GetShowId3Dialog()))) {
         return false;
      }
   }

   // Ensure filename doesn't interfere with project files.
   if (!CheckFilename()) {
      return false;
   }

   // Export the tracks
   bool success = ExportTracks();

   // Get rid of mixerspec
   if (mMixerSpec) {
      delete mMixerSpec;
      mMixerSpec = NULL;
   }

   return success;
}

bool Exporter::Process(AudacityProject *project, int numChannels,
                       const wxChar *type, const wxString & filename,
                       bool selectedOnly, double t0, double t1)
{
   // Save parms
   mProject = project;
   mChannels = numChannels;
   mFilename = filename;
   mSelectedOnly = selectedOnly;
   mT0 = t0;
   mT1 = t1;
   mActualName = mFilename;

   for (size_t i = 0; i < mPlugins.GetCount(); i++) {
      for (int j = 0; j < mPlugins[i]->GetFormatCount(); j++)
      {
         if (mPlugins[i]->GetFormat(j).IsSameAs(type, false))
         {
            mFormat = i;
            mSubFormat = j;
            return ExportTracks();
         }
      }
   }

   return false;
}

bool Exporter::ExamineTracks()
{
   // Init
   mNumSelected = 0;
   mNumLeft = 0;
   mNumRight = 0;
   mNumMono = 0;

   // First analyze the selected audio, perform sanity checks, and provide
   // information as appropriate.

   // Tally how many are right, left, mono, and make sure at
   // least one track is selected (if selectedOnly==true)

   double earliestBegin = mT1;
   double latestEnd = mT0;

   TrackList *tracks = mProject->GetTracks();
   TrackListIterator iter1(tracks);
   Track *tr = iter1.First();

   while (tr) {
      if (tr->GetKind() == Track::Wave) {
         if ( (tr->GetSelected() || !mSelectedOnly) && !tr->GetMute() ) {  // don't count muted tracks
            mNumSelected++;

            if (tr->GetChannel() == Track::LeftChannel) {
               mNumLeft++;
            }
            else if (tr->GetChannel() == Track::RightChannel) {
               mNumRight++;
            }
            else if (tr->GetChannel() == Track::MonoChannel) {
               // It's a mono channel, but it may be panned
               float pan = ((WaveTrack*)tr)->GetPan();

               if (pan == -1.0)
                  mNumLeft++;
               else if (pan == 1.0)
                  mNumRight++;
               else if (pan == 0)
                  mNumMono++;
               else {
                  // Panned partially off-center. Mix as stereo.
                  mNumLeft++;
                  mNumRight++;
               }
            }

            if (tr->GetOffset() < earliestBegin) {
               earliestBegin = tr->GetOffset();
            }

            if (tr->GetEndTime() > latestEnd) {
               latestEnd = tr->GetEndTime();
            }

         }
      }

      tr = iter1.Next();
   }

   if (mNumSelected == 0) {
      wxString message;
      if(mSelectedOnly)
         message = _("All selected audio is muted.");
      else
         message = _("All audio is muted.");
      wxMessageBox(message,
                    _("Unable to export"),
                    wxOK | wxICON_INFORMATION);
      return false;
   }

   if (mT0 < earliestBegin)
      mT0 = earliestBegin;

   if (mT1 > latestEnd)
      mT1 = latestEnd;

   return true;
}

bool Exporter::GetFilename()
{
   mFormat = -1;

   wxString maskString;
   wxString defaultFormat = gPrefs->Read(wxT("/Export/Format"),
                                         wxT("WAV"));

   mFilterIndex = 0;

   for (size_t i = 0; i < mPlugins.GetCount(); i++) {
      for (int j = 0; j < mPlugins[i]->GetFormatCount(); j++)
      {
         maskString += mPlugins[i]->GetMask(j) + wxT("|");
         if (mPlugins[i]->GetFormat(j) == defaultFormat) {
            mFormat = i;
            mSubFormat = j;
         }
         if (mFormat == -1) mFilterIndex++;
      }
   }
   if (mFormat == -1)
   {
      mFormat = 0;
      mFilterIndex = 0;
   }
   maskString.RemoveLast();

   mFilename.SetPath(gPrefs->Read(wxT("/Export/Path"), ::wxGetCwd()));
   mFilename.SetName(mProject->GetName());
   while (true) {

      FileDialog fd(mProject,
                    mFileDialogTitle,
                    mFilename.GetPath(),
                    mFilename.GetFullName(),
                    maskString,
                    wxFD_SAVE | wxRESIZE_BORDER | FD_NO_ADD_EXTENSION);
      mDialog = &fd;

      fd.SetFilterIndex(mFilterIndex);

      fd.EnableButton(_("&Options..."), ExportCallback, this);

      if (fd.ShowModal() == wxID_CANCEL) {
         return false;
      }

      mFilename = fd.GetPath();
      mFormat = fd.GetFilterIndex();
      mFilterIndex = fd.GetFilterIndex();

      int c = 0;
      for (size_t i = 0; i < mPlugins.GetCount(); i++)
      {
         for (int j = 0; j < mPlugins[i]->GetFormatCount(); j++)
         {
            if (mFilterIndex == c)
            {
               mFormat = i;
               mSubFormat = j;
            }
            c++;
         }
      }

      if (mFilename == wxT("")) {
         return false;
      }

      wxString ext = mFilename.GetExt();
      wxString defext = mPlugins[mFormat]->GetExtension(mSubFormat).Lower();

      //
      // Check the extension - add the default if it's not there,
      // and warn user if it's abnormal.
      //
      if (ext.IsEmpty()) {
         //
         // Make sure the user doesn't accidentally save the file
         // as an extension with no name, like just plain ".wav".
         //
         if (mFilename.GetName().Left(1) == wxT(".")) {
            wxString prompt = _("Are you sure you want to export the file as \"") +
                              mFilename.GetFullName() +
                              wxT("\"?\n");

            int action = wxMessageBox(prompt,
                                      _("Warning"),
                                      wxYES_NO | wxICON_EXCLAMATION);
            if (action != wxYES) {
               continue;
            }
         }

         mFilename.SetExt(defext);
      }
      else if (!mPlugins[mFormat]->CheckFileName(mFilename, mSubFormat))
      {
         continue;
      }
      else if (!ext.IsEmpty() && !mPlugins[mFormat]->IsExtension(ext,mSubFormat) && ext.CmpNoCase(defext)) {
         wxString prompt;
         prompt.Printf(_("You are about to export a %s file with the name \"%s\".\n\nNormally these files end in \".%s\", and some programs will not open files with nonstandard extensions.\n\nAre you sure you want to export the file under this name?"),
                       mPlugins[mFormat]->GetFormat(mSubFormat).c_str(),
                       mFilename.GetFullName().c_str(),
                       defext.c_str());

         int action = wxMessageBox(prompt,
                                   _("Warning"),
                                   wxYES_NO | wxICON_EXCLAMATION);
         if (action != wxYES) {
            continue;
         }
      }

      if (mFilename.GetFullPath().Length() >= 256) {
         wxMessageBox(_("Sorry, pathnames longer than 256 characters not supported."));
         continue;
      }

      // Check to see if we are writing to a path that a missing aliased file existed at.
      // This causes problems for the exporter, so we don't allow it.
      // Overwritting non-missing aliased files is okay.
      // Also, this can only happen for uncompressed audio.
      bool overwritingMissingAlias;
      overwritingMissingAlias = false;
      for (size_t i = 0; i < gAudacityProjects.GetCount(); i++) {
         AliasedFileArray aliasedFiles;
         FindDependencies(gAudacityProjects[i], &aliasedFiles);
         size_t j;
         for (j = 0; j< aliasedFiles.GetCount(); j++) {
            if (mFilename.GetFullPath() == aliasedFiles[j].mFileName.GetFullPath() &&
                !mFilename.FileExists()) {
               // Warn and return to the dialog
               wxMessageBox(_("You are attempting to overwrite an aliased file that is missing.\n\
The file cannot be written because the path is needed to restore the original audio to the project.\n\
Choose File > Check Dependencies to view the locations of all missing files.\n\
If you still wish to export, please choose a different filename or folder."));
               overwritingMissingAlias = true;
            }
         }
      }
      if (overwritingMissingAlias)
         continue;

      if (mFilename.FileExists()) {
         wxString prompt;

         prompt.Printf(_("A file named \"%s\" already exists.  Replace?"),
                       mFilename.GetFullPath().c_str());

         int action = wxMessageBox(prompt,
                                   _("Warning"),
                                   wxYES_NO | wxICON_EXCLAMATION);
         if (action != wxYES) {
            continue;
         }
      }

      break;
   }

   return true;
}

//
// For safety, if the file already exists it stores the filename
// the user wants in actualName, and returns a temporary file name.
// The calling function should rename the file when it's successfully
// exported.
//
bool Exporter::CheckFilename()
{
   //
   // Ensure that exporting a file by this name doesn't overwrite
   // one of the existing files in the project.  (If it would
   // overwrite an existing file, DirManager tries to rename the
   // existing file.)
   //

   if (!mProject->GetDirManager()->EnsureSafeFilename(mFilename))
      return false;

   gPrefs->Write(wxT("/Export/Format"), mPlugins[mFormat]->GetFormat(mSubFormat));
   gPrefs->Write(wxT("/Export/Path"), mFilename.GetPath());
   gPrefs->Flush();

   //
   // To be even safer, return a temporary file name based
   // on this one...
   //

   mActualName = mFilename;

   int suffix = 0;
   while (mFilename.FileExists()) {
      mFilename.SetName(mActualName.GetName() +
                        wxString::Format(wxT("%d"), suffix));
      suffix++;
   }

   return true;
}

void Exporter::DisplayOptions(int index)
{
   int c = 0;
   int mf = -1, msf = -1;
   for (size_t i = 0; i < mPlugins.GetCount(); i++)
   {
      for (int j = 0; j < mPlugins[i]->GetFormatCount(); j++)
      {
         if (index == c)
         {
            mf = i;
            msf = j;
         }
         c++;
      }
   }
   // This shouldn't happen...
   if (index >= c) {
      return;
   }

#if defined(__WXMSW__)
   mPlugins[mf]->DisplayOptions(mProject, msf);
#else
   mPlugins[mf]->DisplayOptions(mDialog, msf);
#endif
}

bool Exporter::CheckMix()
{
   // Clean up ... should never happen
   if (mMixerSpec) {
      delete mMixerSpec;
      mMixerSpec = NULL;
   }

   // Detemine if exported file will be stereo or mono or multichannel,
   // and if mixing will occur.

   int downMix = gPrefs->Read(wxT("/FileFormats/ExportDownMix"), true);

   if (downMix) {
      if (mNumRight > 0 || mNumLeft > 0) {
         mChannels = 2;
      }
      else {
         mChannels = 1;
      }
      if (mChannels > mPlugins[mFormat]->GetMaxChannels(mSubFormat))
         mChannels = mPlugins[mFormat]->GetMaxChannels(mSubFormat);

      int numLeft =  mNumLeft + mNumMono;
      int numRight = mNumRight + mNumMono;

      if (numLeft > 1 || numRight > 1 || mNumLeft + mNumRight + mNumMono > mChannels) {
         if (mChannels == 2) {
            if (ShowWarningDialog(mProject,
                                  wxT("MixStereo"),
                                  _("Your tracks will be mixed down to two stereo channels in the exported file."),
                                  true) == wxID_CANCEL)
               return false;
         }
         else {
            if (ShowWarningDialog(mProject,
                                  wxT("MixMono"),
                                  _("Your tracks will be mixed down to a single mono channel in the exported file."),
                                  true) == wxID_CANCEL)
               return false;
         }
      }
   }
   else
   {
      ExportMixerDialog md(mProject->GetTracks(),
                           mSelectedOnly,
                           mPlugins[mFormat]->GetMaxChannels(mSubFormat),
                           NULL,
                           1,
                           _("Advanced Mixing Options"));

      if (md.ShowModal() != wxID_OK) {
         return false;
      }

      mMixerSpec = new MixerSpec(*(md.GetMixerSpec()));
      mChannels = mMixerSpec->GetNumChannels();
   }

   return true;
}

bool Exporter::ExportTracks()
{
   int success;

   // Keep original in case of failure
   if (mActualName != mFilename) {
      ::wxRenameFile(mActualName.GetFullPath(), mFilename.GetFullPath());
   }

   success = mPlugins[mFormat]->Export(mProject,
                                       mChannels,
                                       mActualName.GetFullPath(),
                                       mSelectedOnly,
                                       mT0,
                                       mT1,
                                       mMixerSpec,
                                       NULL,
                                       mSubFormat);

   if (mActualName != mFilename) {
      // Remove backup
      if (success == eProgressSuccess || success == eProgressStopped) {
         ::wxRemoveFile(mFilename.GetFullPath());
      }
      else {
         // Restore original, if needed
         ::wxRemoveFile(mActualName.GetFullPath());
         ::wxRenameFile(mFilename.GetFullPath(), mActualName.GetFullPath());
      }
   }

   return (success == eProgressSuccess || success == eProgressStopped);
}

//----------------------------------------------------------------------------
// ExportMixerPanel
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ExportMixerPanel, wxPanel)
    EVT_PAINT(ExportMixerPanel::OnPaint)
    EVT_MOUSE_EVENTS(ExportMixerPanel::OnMouseEvent)
END_EVENT_TABLE()

ExportMixerPanel::ExportMixerPanel( MixerSpec *mixerSpec,
      wxArrayString trackNames,wxWindow *parent, wxWindowID id,
      const wxPoint& pos, const wxSize& size):
   wxPanel(parent, id, pos, size)
{
   mBitmap = NULL;
   mWidth = 0;
   mHeight = 0;
   mMixerSpec = mixerSpec;
   mSelectedTrack = mSelectedChannel = -1;

   mTrackRects = new wxRect[ mMixerSpec->GetNumTracks() ];
   mChannelRects = new wxRect[ mMixerSpec->GetMaxNumChannels() ];

   mTrackNames = trackNames;
}

ExportMixerPanel::~ExportMixerPanel()
{
   delete[] mTrackRects;
   delete[] mChannelRects;

   if (mBitmap) {
      delete mBitmap;
   }
}

//set the font on memDC such that text can fit in specified width and height
void ExportMixerPanel::SetFont( wxMemoryDC &memDC, wxString text, int width,
      int height )
{
   int l = 0, u = 13, m, w, h;
   wxFont font = memDC.GetFont();
   while( l < u - 1 )
   {
      m = ( l + u ) / 2;
      font.SetPointSize( m );
      memDC.SetFont( font );
      memDC.GetTextExtent( text, &w, &h );

      if( w < width && h < height )
         l = m;
      else
         u = m;
   }
   font.SetPointSize( l );
   memDC.SetFont( font );
}

void ExportMixerPanel::OnPaint(wxPaintEvent & WXUNUSED(event))
{
   wxPaintDC dc( this );

   int width, height;
   GetSize( &width, &height );

   if( !mBitmap || mWidth != width || mHeight != height )
   {
      if( mBitmap )
         delete mBitmap;

      mWidth = width;
      mHeight = height;
      mBitmap = new wxBitmap( mWidth, mHeight );
   }

   wxColour bkgnd = GetBackgroundColour();
   wxBrush bkgndBrush( bkgnd, wxSOLID );

   wxMemoryDC memDC;
   memDC.SelectObject( *mBitmap );

   //draw background
   wxRect bkgndRect;
   bkgndRect.x = 0;
   bkgndRect.y = 0;
   bkgndRect.width = mWidth;
   bkgndRect.height = mHeight;

   memDC.SetBrush( *wxWHITE_BRUSH );
   memDC.SetPen( *wxBLACK_PEN );
   memDC.DrawRectangle( bkgndRect );

   //box dimensions
   mBoxWidth = mWidth / 6;

   mTrackHeight = ( mHeight * 3 ) / ( mMixerSpec->GetNumTracks() * 4 );
   if( mTrackHeight > 30 )
      mTrackHeight = 30;

   mChannelHeight = ( mHeight * 3 ) / ( mMixerSpec->GetNumChannels() * 4 );
   if( mChannelHeight > 30 )
      mChannelHeight = 30;

   static double PI = 2 * acos( 0.0 );
   double angle = atan( ( 3.0 * mHeight ) / mWidth );
   double radius = mHeight / ( 2.0 * sin( PI - 2.0 * angle ) );
   double totAngle = ( asin( mHeight / ( 2.0 * radius ) ) * 2.0 );

   //draw tracks
   memDC.SetBrush( AColor::envelopeBrush );
   angle = totAngle / ( mMixerSpec->GetNumTracks() + 1 );

   int max = 0, w, h;
   for( int i = 1; i < mMixerSpec->GetNumTracks(); i++ )
      if( mTrackNames[ i ].length() > mTrackNames[ max ].length() )
         max = i;

   SetFont( memDC, mTrackNames[ max ], mBoxWidth, mTrackHeight );

   for( int i = 0; i < mMixerSpec->GetNumTracks(); i++ )
   {
      mTrackRects[ i ].x = ( int )( mBoxWidth * 2 + radius - radius *
         cos( totAngle / 2.0 - angle * ( i + 1 ) ) - mBoxWidth + 0.5 );
      mTrackRects[ i ].y = ( int )( mHeight * 0.5 - radius *
            sin( totAngle * 0.5 - angle * ( i + 1.0 ) ) -
            0.5 * mTrackHeight + 0.5 );

      mTrackRects[ i ].width = mBoxWidth;
      mTrackRects[ i ].height = mTrackHeight;

      memDC.SetPen( mSelectedTrack == i ? *wxRED_PEN : *wxBLACK_PEN );
      memDC.DrawRectangle( mTrackRects[ i ] );

      memDC.GetTextExtent( mTrackNames[ i ], &w, &h );
      memDC.DrawText( mTrackNames[ i ],
            mTrackRects[ i ].x + ( mBoxWidth - w ) / 2,
            mTrackRects[ i ].y + ( mTrackHeight - h ) / 2 );
   }

   //draw channels
   memDC.SetBrush( AColor::playRegionBrush[ 0 ] );
   angle = ( asin( mHeight / ( 2.0 * radius ) ) * 2.0 ) /
      ( mMixerSpec->GetNumChannels() + 1 );

   SetFont( memDC, wxT( "Channel: XX" ), mBoxWidth, mChannelHeight );
   memDC.GetTextExtent( wxT( "Channel: XX" ), &w, &h );

   for( int i = 0; i < mMixerSpec->GetNumChannels(); i++ )
   {
      mChannelRects[ i ].x = ( int )( mBoxWidth * 4 - radius  + radius *
         cos( totAngle * 0.5 - angle * ( i + 1 ) ) + 0.5 );
      mChannelRects[ i ].y = ( int )( mHeight * 0.5 - radius *
            sin( totAngle * 0.5 - angle * ( i + 1 ) ) -
            0.5 * mChannelHeight + 0.5 );

      mChannelRects[ i ].width = mBoxWidth;
      mChannelRects[ i ].height = mChannelHeight;

      memDC.SetPen( mSelectedChannel == i ? *wxRED_PEN : *wxBLACK_PEN );
      memDC.DrawRectangle( mChannelRects[ i ] );

      memDC.DrawText( wxString::Format( _( "Channel: %2d" ), i + 1 ),
            mChannelRects[ i ].x + ( mBoxWidth - w ) / 2,
            mChannelRects[ i ].y + ( mChannelHeight - h ) / 2 );
   }

   //draw links
   memDC.SetPen( wxPen( *wxBLACK, mHeight / 200 ) );
   for( int i = 0; i < mMixerSpec->GetNumTracks(); i++ )
      for( int j = 0; j < mMixerSpec->GetNumChannels(); j++ )
         if( mMixerSpec->mMap[ i ][ j ] )
            AColor::Line(memDC, mTrackRects[ i ].x + mBoxWidth,
                  mTrackRects[ i ].y + mTrackHeight / 2, mChannelRects[ j ].x,
                  mChannelRects[ j ].y + mChannelHeight / 2 );

   dc.Blit( 0, 0, mWidth, mHeight, &memDC, 0, 0, wxCOPY, FALSE );
}

double ExportMixerPanel::Distance( wxPoint &a, wxPoint &b )
{
   return sqrt( pow( a.x - b.x, 2.0 ) + pow( a.y - b.y, 2.0 ) );
}

//checks if p is on the line connecting la, lb with tolerence
bool ExportMixerPanel::IsOnLine( wxPoint p, wxPoint la, wxPoint lb )
{
   return Distance( p, la ) + Distance( p, lb ) - Distance( la, lb ) < 0.1;
}

void ExportMixerPanel::OnMouseEvent(wxMouseEvent & event)
{
   if( event.ButtonDown() )
   {
      bool reset = true;
      //check tracks
      for( int i = 0; i < mMixerSpec->GetNumTracks(); i++ )
         if( mTrackRects[ i ].Contains( event.m_x, event.m_y ) )
         {
            reset = false;
            if( mSelectedTrack == i )
               mSelectedTrack = -1;
            else
            {
               mSelectedTrack = i;
               if( mSelectedChannel != -1 )
                  mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ] =
                     !mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ];
            }
            goto found;
         }

      //check channels
      for( int i = 0; i < mMixerSpec->GetNumChannels(); i++ )
         if( mChannelRects[ i ].Contains( event.m_x, event.m_y ) )
         {
            reset = false;
            if( mSelectedChannel == i )
               mSelectedChannel = -1;
            else
            {
               mSelectedChannel = i;
               if( mSelectedTrack != -1 )
                  mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ] =
                     !mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ];
            }
            goto found;
         }

      //check links
      for( int i = 0; i < mMixerSpec->GetNumTracks(); i++ )
         for( int j = 0; j < mMixerSpec->GetNumChannels(); j++ )
            if( mMixerSpec->mMap[ i ][ j ]  && IsOnLine( wxPoint( event.m_x,
                        event.m_y ), wxPoint( mTrackRects[ i ].x + mBoxWidth,
                           mTrackRects[ i ].y + mTrackHeight / 2 ),
                     wxPoint( mChannelRects[ j ].x, mChannelRects[ j ].y +
                     mChannelHeight / 2 ) ) )
               mMixerSpec->mMap[ i ][ j ] = false;

found:
      if( reset )
         mSelectedTrack = mSelectedChannel = -1;
      Refresh( false );
   }
}

//----------------------------------------------------------------------------
// ExportMixerDialog
//----------------------------------------------------------------------------

enum
{
   ID_MIXERPANEL = 10001,
   ID_SLIDER_CHANNEL
};

BEGIN_EVENT_TABLE( ExportMixerDialog,wxDialog )
   EVT_BUTTON( wxID_OK, ExportMixerDialog::OnOk )
   EVT_BUTTON( wxID_CANCEL, ExportMixerDialog::OnCancel )
   EVT_SIZE( ExportMixerDialog::OnSize )
   EVT_SLIDER( ID_SLIDER_CHANNEL, ExportMixerDialog::OnSlider )
END_EVENT_TABLE()

ExportMixerDialog::ExportMixerDialog( TrackList *tracks, bool selectedOnly,
      int maxNumChannels, wxWindow *parent, wxWindowID id, const wxString &title,
      const wxPoint &position, const wxSize& size, long style ) :
   wxDialog( parent, id, title, position, size, style | wxRESIZE_BORDER )
{
   SetName(GetTitle());

   int numTracks = 0;
   TrackListIterator iter( tracks );

   for( Track *t = iter.First(); t; t = iter.Next() )
   {
      if( t->GetKind() == Track::Wave && ( t->GetSelected() || !selectedOnly ) && !t->GetMute() )
      {
         numTracks++;
         const wxString sTrackName = (t->GetName()).Left(20);
         if( t->GetChannel() == Track::LeftChannel )
            mTrackNames.Add(sTrackName + _( " - L" ));
         else if( t->GetChannel() == Track::RightChannel )
            mTrackNames.Add(sTrackName + _( " - R" ));
         else
            mTrackNames.Add(sTrackName);
      }
   }

   // JKC: This is an attempt to fix a 'watching brief' issue, where the slider is
   // sometimes not slidable.  My suspicion is that a mixer may incorrectly
   // state the number of channels - so we assume there are always at least two.
   // The downside is that if someone is exporting to a mono device, the dialog
   // will allow them to output to two channels. Hmm.  We may need to revisit this.
   if (maxNumChannels < 2 )
      maxNumChannels = 2;
   if (maxNumChannels > 32)
      maxNumChannels = 32;

   mMixerSpec = new MixerSpec( numTracks, maxNumChannels );

   wxBoxSizer *vertSizer = new wxBoxSizer( wxVERTICAL );

   wxWindow *mixerPanel = new ExportMixerPanel( mMixerSpec, mTrackNames, this,
         ID_MIXERPANEL, wxDefaultPosition, wxSize( 400, -1 ) );
   mixerPanel->SetName(_("Mixer Panel"));
   vertSizer->Add( mixerPanel, 1, wxEXPAND | wxALIGN_CENTRE | wxALL, 5 );

   wxBoxSizer *horSizer = new wxBoxSizer( wxHORIZONTAL );

   wxString label;
   label.Printf( _( "Output Channels: %2d" ), mMixerSpec->GetNumChannels() );
   mChannelsText = new wxStaticText( this, -1, label);
   horSizer->Add( mChannelsText, 0, wxALIGN_LEFT | wxALL, 5 );

   wxSlider *channels = new wxSlider( this, ID_SLIDER_CHANNEL,
         mMixerSpec->GetNumChannels(), 1, mMixerSpec->GetMaxNumChannels(),
         wxDefaultPosition, wxSize( 300, -1 ) );
   channels->SetName(label);
   horSizer->Add( channels, 0, wxEXPAND | wxALL, 5 );

   vertSizer->Add( horSizer, 0, wxALIGN_CENTRE | wxALL, 5 );

   vertSizer->Add( CreateStdButtonSizer(this, eCancelButton|eOkButton), 0, wxEXPAND );

   SetAutoLayout( true );
   SetSizer( vertSizer );
   vertSizer->Fit( this );
   vertSizer->SetSizeHints( this );

   SetSizeHints( 640, 480, 20000, 20000 );

   SetSize( 640, 480 );
}

ExportMixerDialog::~ExportMixerDialog()
{
   if( mMixerSpec )
   {
      delete mMixerSpec;
      mMixerSpec = NULL;
   }
}

void ExportMixerDialog::OnSize(wxSizeEvent &event)
{
   ExportMixerPanel *pnl = ( ( ExportMixerPanel* ) FindWindow( ID_MIXERPANEL ) );
   pnl->Refresh( false );
   event.Skip();
}

void ExportMixerDialog::OnSlider( wxCommandEvent & WXUNUSED(event))
{
   wxSlider *channels = ( wxSlider* )FindWindow( ID_SLIDER_CHANNEL );
   ExportMixerPanel *pnl = ( ( ExportMixerPanel* ) FindWindow( ID_MIXERPANEL ) );
   mMixerSpec->SetNumChannels( channels->GetValue() );
   pnl->Refresh( false );
   wxString label;
   label.Printf( _( "Output Channels: %2d" ), mMixerSpec->GetNumChannels() );
   mChannelsText->SetLabel( label );
   channels->SetName( label );
}

void ExportMixerDialog::OnOk(wxCommandEvent & WXUNUSED(event))
{
   EndModal( wxID_OK );
}

void ExportMixerDialog::OnCancel(wxCommandEvent & WXUNUSED(event))
{
   EndModal( wxID_CANCEL );
}

