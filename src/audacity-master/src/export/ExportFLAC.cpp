/**********************************************************************

Audacity: A Digital Audio Editor

ExportFLAC.cpp

Frederik M.J.V

This program is distributed under the GNU General Public License, version 2.
A copy of this license is included with this source.

Based on ExportOGG.cpp by:
Joshua Haberman

Portions from vorbis-tools, copyright 2000-2002 Michael Smith
<msmith@labyrinth.net.au>; Vorbize, Kenneth Arnold <kcarnold@yahoo.com>;
and libvorbis examples, Monty <monty@xiph.org>

**********************************************************************/

#include "../Audacity.h"

#ifdef USE_LIBFLAC

#include "Export.h"
#include "ExportFLAC.h"

#include <wx/progdlg.h>
#include <wx/ffile.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#include "FLAC++/encoder.h"

#include "../float_cast.h"
#include "../Project.h"
#include "../Mix.h"
#include "../Prefs.h"

#include "../Internat.h"
#include "../Tags.h"

//----------------------------------------------------------------------------
// ExportFLACOptions Class
//----------------------------------------------------------------------------

class ExportFLACOptions : public wxDialog
{
public:

   ExportFLACOptions(wxWindow *parent);
   void PopulateOrExchange(ShuttleGui & S);
   void OnOK(wxCommandEvent& event);

private:

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ExportFLACOptions, wxDialog)
   EVT_BUTTON(wxID_OK, ExportFLACOptions::OnOK)
END_EVENT_TABLE()

///
///
ExportFLACOptions::ExportFLACOptions(wxWindow *parent)
:  wxDialog(parent, wxID_ANY,
            wxString(_("Specify FLAC Options")))
{
   SetName(GetTitle());
   ShuttleGui S(this, eIsCreatingFromPrefs);

   PopulateOrExchange(S);
}

///
///
void ExportFLACOptions::PopulateOrExchange(ShuttleGui & S)
{
   wxArrayString flacLevelNames, flacLevelLabels;
   flacLevelLabels.Add(wxT("0")); flacLevelNames.Add(_("0 (fastest)"));
   flacLevelLabels.Add(wxT("1")); flacLevelNames.Add(_("1"));
   flacLevelLabels.Add(wxT("2")); flacLevelNames.Add(_("2"));
   flacLevelLabels.Add(wxT("3")); flacLevelNames.Add(_("3"));
   flacLevelLabels.Add(wxT("4")); flacLevelNames.Add(_("4"));
   flacLevelLabels.Add(wxT("5")); flacLevelNames.Add(_("5"));
   flacLevelLabels.Add(wxT("6")); flacLevelNames.Add(_("6"));
   flacLevelLabels.Add(wxT("7")); flacLevelNames.Add(_("7"));
   flacLevelLabels.Add(wxT("8")); flacLevelNames.Add(_("8 (best)"));

   wxArrayString flacBitDepthNames, flacBitDepthLabels;
   flacBitDepthLabels.Add(wxT("16")); flacBitDepthNames.Add(_("16 bit"));
   flacBitDepthLabels.Add(wxT("24")); flacBitDepthNames.Add(_("24 bit"));

   S.StartHorizontalLay(wxEXPAND, 0);
   {
      S.StartStatic(_("FLAC Export Setup"), 0);
      {
         S.StartTwoColumn();
         {
            S.TieChoice(_("Level:"), wxT("/FileFormats/FLACLevel"),
                        wxT("5"), flacLevelNames, flacLevelLabels);
            S.TieChoice(_("Bit depth:"), wxT("/FileFormats/FLACBitDepth"),
                        wxT("16"), flacBitDepthNames, flacBitDepthLabels);
         }
         S.EndTwoColumn();
      }
      S.EndStatic();
   }
   S.EndHorizontalLay();

   S.AddStandardButtons();

   Layout();
   Fit();
   SetMinSize(GetSize());
   Center();

   return;
}

///
///
void ExportFLACOptions::OnOK(wxCommandEvent& WXUNUSED(event))
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   EndModal(wxID_OK);

   return;
}

//----------------------------------------------------------------------------
// ExportFLAC Class
//----------------------------------------------------------------------------

#define SAMPLES_PER_RUN 8192

/* FLACPP_API_VERSION_CURRENT is 6 for libFLAC++ from flac-1.1.3 (see <FLAC++/export.h>) */
#if !defined FLACPP_API_VERSION_CURRENT || FLACPP_API_VERSION_CURRENT < 6
#define LEGACY_FLAC
#else
#undef LEGACY_FLAC
#endif

static struct
{
   bool        do_exhaustive_model_search;
   bool        do_escape_coding;
   bool        do_mid_side_stereo;
   bool        loose_mid_side_stereo;
   unsigned    qlp_coeff_precision;
   unsigned    min_residual_partition_order;
   unsigned    max_residual_partition_order;
   unsigned    rice_parameter_search_dist;
   unsigned    max_lpc_order;
} flacLevels[] = {
   {  false,   false,   false,   false,   0, 2, 2, 0, 0  },
   {  false,   false,   true,    true,    0, 2, 2, 0, 0  },
   {  false,   false,   true,    false,   0, 0, 3, 0, 0  },
   {  false,   false,   false,   false,   0, 3, 3, 0, 6  },
   {  false,   false,   true,    true,    0, 3, 3, 0, 8  },
   {  false,   false,   true,    false,   0, 3, 3, 0, 8  },
   {  false,   false,   true,    false,   0, 0, 4, 0, 8  },
   {  true,    false,   true,    false,   0, 0, 6, 0, 8  },
   {  true,    false,   true,    false,   0, 0, 6, 0, 12 },
};

//----------------------------------------------------------------------------

class ExportFLAC : public ExportPlugin
{
public:

   ExportFLAC();
   void Destroy();

   // Required

   bool DisplayOptions(wxWindow *parent, int format = 0);
   int Export(AudacityProject *project,
               int channels,
               wxString fName,
               bool selectedOnly,
               double t0,
               double t1,
               MixerSpec *mixerSpec = NULL,
               Tags *metadata = NULL,
               int subformat = 0);

private:

   bool GetMetadata(AudacityProject *project, Tags *tags);

   FLAC__StreamMetadata *mMetadata;
};

//----------------------------------------------------------------------------

ExportFLAC::ExportFLAC()
:  ExportPlugin()
{
   AddFormat();
   SetFormat(wxT("FLAC"),0);
   AddExtension(wxT("flac"),0);
   SetMaxChannels(FLAC__MAX_CHANNELS,0);
   SetCanMetaData(true,0);
   SetDescription(_("FLAC Files"),0);
}

void ExportFLAC::Destroy()
{
   delete this;
}

int ExportFLAC::Export(AudacityProject *project,
                        int numChannels,
                        wxString fName,
                        bool selectionOnly,
                        double t0,
                        double t1,
                        MixerSpec *mixerSpec,
                        Tags *metadata,
                        int WXUNUSED(subformat))
{
   double    rate    = project->GetRate();
   TrackList *tracks = project->GetTracks();

   wxLogNull logNo;            // temporarily disable wxWidgets error messages
   int updateResult = eProgressSuccess;

   int levelPref;
   gPrefs->Read(wxT("/FileFormats/FLACLevel"), &levelPref, 5);

   wxString bitDepthPref =
      gPrefs->Read(wxT("/FileFormats/FLACBitDepth"), wxT("16"));

   FLAC::Encoder::File encoder;

#ifdef LEGACY_FLAC
   encoder.set_filename(OSOUTPUT(fName));
#endif
   encoder.set_channels(numChannels);
   encoder.set_sample_rate(lrint(rate));

   // See note in GetMetadata() about a bug in libflac++ 1.1.2
   if (!GetMetadata(project, metadata)) {
      return false;
   }

   if (mMetadata) {
      encoder.set_metadata(&mMetadata, 1);
   }

   sampleFormat format;
   if (bitDepthPref == wxT("24")) {
      format = int24Sample;
      encoder.set_bits_per_sample(24);
   } else { //convert float to 16 bits
      format = int16Sample;
      encoder.set_bits_per_sample(16);
   }

   // Duplicate the flac command line compression levels
   if (levelPref < 0 || levelPref > 8) {
      levelPref = 5;
   }
   encoder.set_do_exhaustive_model_search(flacLevels[levelPref].do_exhaustive_model_search);
   encoder.set_do_escape_coding(flacLevels[levelPref].do_escape_coding);
   if (numChannels != 2) {
      encoder.set_do_mid_side_stereo(false);
      encoder.set_loose_mid_side_stereo(false);
   }
   else {
      encoder.set_do_mid_side_stereo(flacLevels[levelPref].do_mid_side_stereo);
      encoder.set_loose_mid_side_stereo(flacLevels[levelPref].loose_mid_side_stereo);
   }
   encoder.set_qlp_coeff_precision(flacLevels[levelPref].qlp_coeff_precision);
   encoder.set_min_residual_partition_order(flacLevels[levelPref].min_residual_partition_order);
   encoder.set_max_residual_partition_order(flacLevels[levelPref].max_residual_partition_order);
   encoder.set_rice_parameter_search_dist(flacLevels[levelPref].rice_parameter_search_dist);
   encoder.set_max_lpc_order(flacLevels[levelPref].max_lpc_order);

#ifdef LEGACY_FLAC
   encoder.init();
#else
   wxFFile f;     // will be closed when it goes out of scope
   if (!f.Open(fName, wxT("w+b"))) {
      wxMessageBox(wxString::Format(_("FLAC export couldn't open %s"), fName.c_str()));
      return false;
   }

   // Even though there is an init() method that takes a filename, use the one that
   // takes a file handle because wxWidgets can open a file with a Unicode name and
   // libflac can't (under Windows).
   int status = encoder.init(f.fp());
   if (status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
      wxMessageBox(wxString::Format(_("FLAC encoder failed to initialize\nStatus: %d"), status));
      return false;
   }
#endif

   if (mMetadata) {
      ::FLAC__metadata_object_delete(mMetadata);
   }

   int numWaveTracks;
   WaveTrack **waveTracks;
   tracks->GetWaveTracks(selectionOnly, &numWaveTracks, &waveTracks);
   Mixer *mixer = CreateMixer(numWaveTracks, waveTracks,
                            tracks->GetTimeTrack(),
                            t0, t1,
                            numChannels, SAMPLES_PER_RUN, false,
                            rate, format, true, mixerSpec);
   delete [] waveTracks;

   int i, j;
   FLAC__int32 **tmpsmplbuf = new FLAC__int32*[numChannels];
   for (i = 0; i < numChannels; i++) {
      tmpsmplbuf[i] = (FLAC__int32 *) calloc(SAMPLES_PER_RUN, sizeof(FLAC__int32));
   }

   ProgressDialog *progress = new ProgressDialog(wxFileName(fName).GetName(),
         selectionOnly ?
         _("Exporting the selected audio as FLAC") :
         _("Exporting the entire project as FLAC"));

   while (updateResult == eProgressSuccess) {
      sampleCount samplesThisRun = mixer->Process(SAMPLES_PER_RUN);
      if (samplesThisRun == 0) { //stop encoding
         break;
      }
      else {
         for (i = 0; i < numChannels; i++) {
            samplePtr mixed = mixer->GetBuffer(i);
            if (format == int24Sample) {
               for (j = 0; j < samplesThisRun; j++) {
                  tmpsmplbuf[i][j] = ((int *) mixed)[j];
               }
            }
            else {
               for (j = 0; j < samplesThisRun; j++) {
                  tmpsmplbuf[i][j] = ((short *) mixed)[j];
               }
            }
         }
         encoder.process(tmpsmplbuf, samplesThisRun);
      }
      updateResult = progress->Update(mixer->MixGetCurrentTime()-t0, t1-t0);
   }
   f.Detach(); // libflac closes the file
   encoder.finish();

   delete progress;

   for (i = 0; i < numChannels; i++) {
      free(tmpsmplbuf[i]);
   }
   delete mixer;

   delete[] tmpsmplbuf;

   return updateResult;
}

bool ExportFLAC::DisplayOptions(wxWindow *parent, int WXUNUSED(format))
{
   ExportFLACOptions od(parent);

   od.ShowModal();

   return true;
}

// LL:  There's a bug in libflac++ 1.1.2 that prevents us from using
//      FLAC::Metadata::VorbisComment directly.  The set_metadata()
//      function allocates an array on the stack, but the base library
//      expects that array to be valid until the stream is initialized.
//
//      This has been fixed in 1.1.4.
bool ExportFLAC::GetMetadata(AudacityProject *project, Tags *tags)
{
   // Retrieve tags if needed
   if (tags == NULL)
      tags = project->GetTags();

   mMetadata = ::FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);

   wxString n, v;
   for (bool cont = tags->GetFirst(n, v); cont; cont = tags->GetNext(n, v)) {
      if (n == TAG_YEAR) {
         n = wxT("DATE");
      }
      FLAC::Metadata::VorbisComment::Entry entry(n.mb_str(wxConvUTF8),
                                                 v.mb_str(wxConvUTF8));
      ::FLAC__metadata_object_vorbiscomment_append_comment(mMetadata,
                                                           entry.get_entry(),
                                                           true);
   }

   return true;
}

ExportPlugin *New_ExportFLAC()
{
   return new ExportFLAC();
}

#endif // USE_LIBFLAC

