/**********************************************************************

  Audacity: A Digital Audio Editor

  FileFormatPrefs.h

  Joshua Haberman
  Dominic Mazzoni
  James Crook

**********************************************************************/

#ifndef __AUDACITY_FILE_FORMAT_PREFS__
#define __AUDACITY_FILE_FORMAT_PREFS__

#include <wx/defs.h>

#include <wx/stattext.h>
#include <wx/window.h>

#include "../ShuttleGui.h"

#include "PrefsPanel.h"

class LibraryPrefs:public PrefsPanel
{
 public:
   LibraryPrefs(wxWindow * parent);
   ~LibraryPrefs();
   virtual bool Apply();

 private:
   void Populate();
   void PopulateOrExchange(ShuttleGui & S);
   void SetMP3VersionText(bool prompt = false);
   void SetFFmpegVersionText();

   void OnMP3FindButton(wxCommandEvent & e);
   void OnMP3DownButton(wxCommandEvent & e);
   void OnFFmpegFindButton(wxCommandEvent & e);
   void OnFFmpegDownButton(wxCommandEvent & e);

   wxStaticText *mMP3Version;
   wxStaticText *mFFmpegVersion;

   DECLARE_EVENT_TABLE();
};

#endif
