/**********************************************************************

  Audacity: A Digital Audio Editor

  Printing.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_PRINTING__
#define __AUDACITY_PRINTING__

#include <wx/defs.h>
#include <wx/string.h>

class wxWindow;
class TrackList;

void HandlePageSetup(wxWindow *parent);
void HandlePrint(wxWindow *parent, wxString name, TrackList *tracks);

#endif // __AUDACITY_PRINTING__

