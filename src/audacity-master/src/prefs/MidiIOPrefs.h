/**********************************************************************

  Audacity: A Digital Audio Editor

  AudioIOPrefs.h

  Joshua Haberman
  James Crook

**********************************************************************/

#include "../Experimental.h"
#ifdef EXPERIMENTAL_MIDI_OUT

#ifndef __AUDACITY_MIDI_IO_PREFS__
#define __AUDACITY_MIDI_IO_PREFS__

#include <wx/defs.h>

#include <wx/choice.h>
#include <wx/string.h>
#include <wx/window.h>

#include "../ShuttleGui.h"

#include "PrefsPanel.h"

class MidiIOPrefs:public PrefsPanel
{
 public:
   MidiIOPrefs(wxWindow * parent);
   virtual ~MidiIOPrefs();
   virtual bool Apply();
   virtual bool Validate();

 private:
   void Populate();
   void PopulateOrExchange(ShuttleGui & S);
   void GetNamesAndLabels();

   void OnHost(wxCommandEvent & e);
//   void OnDevice(wxCommandEvent & e);

   wxArrayString mHostNames;
   wxArrayString mHostLabels;

   wxString mPlayDevice;
#ifdef EXPERIMENTAL_MIDI_IN
   wxString mRecordDevice;
#endif
//   long mRecordChannels;

   wxChoice *mHost;
   wxChoice *mPlay;
   wxTextCtrl *mLatency;
#ifdef EXPERIMENTAL_MIDI_IN
   wxChoice *mRecord;
#endif
//   wxChoice *mChannels;

   DECLARE_EVENT_TABLE();
};

#endif

#endif
