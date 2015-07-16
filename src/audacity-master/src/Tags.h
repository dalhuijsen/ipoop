/**********************************************************************

  Audacity: A Digital Audio Editor

  Tags.h

  Dominic Mazzoni

  This class holds a few informational tags, such as Title, Author,
  etc. that can be associated with a project or other audio file.
  It is modeled after the ID3 format for MP3 files, and it can
  both import ID3 tags from MP3 files, and export them as well.

  It can present the user with a dialog for editing this information.

  It only keeps track of the fields that are standard in ID3v1
  (title, author, artist, track num, year, genre, and comments),
  but it can export both ID3v1 or the newer ID3v2 format.  The primary
  reason you would want to export ID3v2 tags instead of ID3v1,
  since we're not supporting any v2 fields, is that ID3v2 tags are
  inserted at the BEGINNING of an mp3 file, which is far more
  useful for streaming.

  Use of this functionality requires that libid3tag be compiled in
  with Audacity.

**********************************************************************/

#ifndef __AUDACITY_TAGS__
#define __AUDACITY_TAGS__

#include "Audacity.h"
#include "widgets/Grid.h"
#include "xml/XMLTagHandler.h"

#include <wx/dialog.h>
#include <wx/hashmap.h>
#include <wx/notebook.h>
#include <wx/string.h>

class wxButton;
class wxChoice;
class wxComboBox;
class wxGridCellChoiceEditor;
class wxRadioButton;
class wxTextCtrl;

class Grid;
class ShuttleGui;
class TagsEditor;
class ComboEditor;

// We want a macro call like this:
// WX_DECLARE_USER_EXPORTED_STRING_HASH_MAP(wxString, TagMap, AUDACITY_DLL_API);
// Which wxWidgets does not supply!
// So use this undocumented variant:
WX_DECLARE_STRING_HASH_MAP_WITH_DECL( wxString, TagMap,class AUDACITY_DLL_API );
// Doing this means we can export class Tags without any worry,
// as every class it uses, including TagMap, is then exported.
// It's better than using #pragma warning(disable: 4251)
// and relying on the relevant parts of class Tags being private.

#define TAG_TITLE     wxT("TITLE")
#define TAG_ARTIST   wxT("ARTIST")
#define TAG_ALBUM    wxT("ALBUM")
#define TAG_TRACK    wxT("TRACKNUMBER")
#define TAG_YEAR     wxT("YEAR")
#define TAG_GENRE    wxT("GENRE")
#define TAG_COMMENTS wxT("COMMENTS")
#define TAG_SOFTWARE wxT("Software")
#define TAG_COPYRIGHT wxT("Copyright")

class AUDACITY_DLL_API Tags: public XMLTagHandler {

 public:
   Tags();  // constructor
   virtual ~Tags();

   Tags & operator= (const Tags & src );

   bool ShowEditDialog(wxWindow *parent, wxString title, bool force = false);

   virtual bool HandleXMLTag(const wxChar *tag, const wxChar **attrs);
   virtual XMLTagHandler *HandleXMLChild(const wxChar *tag);
   virtual void WriteXML(XMLWriter &xmlFile);

   void AllowEditTitle(bool editTitle);
   void AllowEditTrackNumber(bool editTrackNumber);

   void LoadDefaultGenres();
   void LoadGenres();

   int GetNumUserGenres();
   wxString GetUserGenre(int value);

   wxString GetGenre(int value);
   int GetGenre(const wxString & name);

   bool HasTag(const wxString & name);
   wxString GetTag(const wxString & name);

   bool GetFirst(wxString & name, wxString & value);
   bool GetNext(wxString & name, wxString & value);

   void SetTag(const wxString & name, const wxString & value);
   void SetTag(const wxString & name, const int & value);

   bool IsEmpty();
   void Clear();

 private:
   void LoadDefaults();

   TagMap::iterator mIter;
   TagMap mXref;
   TagMap mMap;

   wxArrayString mGenres;

   bool mEditTitle;
   bool mEditTrackNumber;
};

class TagsEditor: public wxDialog
{
 public:
   // constructors and destructors
   TagsEditor(wxWindow * parent,
              wxString title,
              Tags * tags,
              bool editTitle,
              bool editTrackNumber);

   virtual ~TagsEditor();

   void PopulateOrExchange(ShuttleGui & S);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

 private:
   void PopulateGenres();
   void SetEditors();

   void OnChange(wxGridEvent & event);

   void OnEdit(wxCommandEvent & event);
   void OnReset(wxCommandEvent & event);

   void OnClear(wxCommandEvent & event);

   void OnLoad(wxCommandEvent & event);
   void OnSave(wxCommandEvent & event);
   void OnSaveDefaults(wxCommandEvent & event);

   void OnAdd(wxCommandEvent & event);
   void OnRemove(wxCommandEvent & event);

   void OnOk(wxCommandEvent & event);
   void OnCancel(wxCommandEvent & event);

   bool IsWindowRectValid(const wxRect *windowRect) const;

 private:
   Tags *mTags;
   bool mEditTitle;
   bool mEditTrack;

   Tags mLocal;

   Grid *mGrid;
   ComboEditor *mComboEditor;
   wxGridCellStringRenderer *mStringRenderer;

   DECLARE_EVENT_TABLE()
};

#endif
