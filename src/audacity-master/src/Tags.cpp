/**********************************************************************

  Audacity: A Digital Audio Editor

  Tags.cpp

  Dominic Mazzoni

*******************************************************************//**

\class Tags
\brief ID3 Tags (for MP3)

  This class started as an ID3 tag

  This class holds a few informational tags, such as Title, Author,
  etc. that can be associated with a project or other audio file.
  It is modeled after the ID3 format for MP3 files, and it can
  both import and export ID3 tags from/to MP2, MP3, and AIFF files.

  It can present the user with a dialog for editing this information.

  Use of this functionality requires that libid3tag be compiled in
  with Audacity.

*//****************************************************************//**

\class TagsEditor
\brief Derived from ExpandingToolBar, this dialog allows editing of Tags.

*//*******************************************************************/

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/window.h>
#endif

#include "Tags.h"

#include "Audacity.h"
#include "FileDialog.h"
#include "FileNames.h"
#include "Internat.h"
#include "Prefs.h"
#include "ShuttleGui.h"
#include "widgets/Grid.h"
#include "xml/XMLFileReader.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/textfile.h>
#include <wx/combobox.h>
#include <wx/display.h>

static const wxChar *DefaultGenres[] =
{
   wxT("Blues"),
   wxT("Classic Rock"),
   wxT("Country"),
   wxT("Dance"),
   wxT("Disco"),
   wxT("Funk"),
   wxT("Grunge"),
   wxT("Hip-Hop"),
   wxT("Jazz"),
   wxT("Metal"),
   wxT("New Age"),
   wxT("Oldies"),
   wxT("Other"),
   wxT("Pop"),
   wxT("R&B"),
   wxT("Rap"),
   wxT("Reggae"),
   wxT("Rock"),
   wxT("Techno"),
   wxT("Industrial"),
   wxT("Alternative"),
   wxT("Ska"),
   wxT("Death Metal"),
   wxT("Pranks"),
   wxT("Soundtrack"),
   wxT("Euro-Techno"),
   wxT("Ambient"),
   wxT("Trip-Hop"),
   wxT("Vocal"),
   wxT("Jazz+Funk"),
   wxT("Fusion"),
   wxT("Trance"),
   wxT("Classical"),
   wxT("Instrumental"),
   wxT("Acid"),
   wxT("House"),
   wxT("Game"),
   wxT("Sound Clip"),
   wxT("Gospel"),
   wxT("Noise"),
   wxT("Alt. Rock"),
   wxT("Bass"),
   wxT("Soul"),
   wxT("Punk"),
   wxT("Space"),
   wxT("Meditative"),
   wxT("Instrumental Pop"),
   wxT("Instrumental Rock"),
   wxT("Ethnic"),
   wxT("Gothic"),
   wxT("Darkwave"),
   wxT("Techno-Industrial"),
   wxT("Electronic"),
   wxT("Pop-Folk"),
   wxT("Eurodance"),
   wxT("Dream"),
   wxT("Southern Rock"),
   wxT("Comedy"),
   wxT("Cult"),
   wxT("Gangsta Rap"),
   wxT("Top 40"),
   wxT("Christian Rap"),
   wxT("Pop/Funk"),
   wxT("Jungle"),
   wxT("Native American"),
   wxT("Cabaret"),
   wxT("New Wave"),
   wxT("Psychedelic"),
   wxT("Rave"),
   wxT("Showtunes"),
   wxT("Trailer"),
   wxT("Lo-Fi"),
   wxT("Tribal"),
   wxT("Acid Punk"),
   wxT("Acid Jazz"),
   wxT("Polka"),
   wxT("Retro"),
   wxT("Musical"),
   wxT("Rock & Roll"),
   wxT("Hard Rock"),
   wxT("Folk"),
   wxT("Folk/Rock"),
   wxT("National Folk"),
   wxT("Swing"),
   wxT("Fast-Fusion"),
   wxT("Bebob"),
   wxT("Latin"),
   wxT("Revival"),
   wxT("Celtic"),
   wxT("Bluegrass"),
   wxT("Avantgarde"),
   wxT("Gothic Rock"),
   wxT("Progressive Rock"),
   wxT("Psychedelic Rock"),
   wxT("Symphonic Rock"),
   wxT("Slow Rock"),
   wxT("Big Band"),
   wxT("Chorus"),
   wxT("Easy Listening"),
   wxT("Acoustic"),
   wxT("Humour"),
   wxT("Speech"),
   wxT("Chanson"),
   wxT("Opera"),
   wxT("Chamber Music"),
   wxT("Sonata"),
   wxT("Symphony"),
   wxT("Booty Bass"),
   wxT("Primus"),
   wxT("Porn Groove"),
   wxT("Satire"),
   wxT("Slow Jam"),
   wxT("Club"),
   wxT("Tango"),
   wxT("Samba"),
   wxT("Folklore"),
   wxT("Ballad"),
   wxT("Power Ballad"),
   wxT("Rhythmic Soul"),
   wxT("Freestyle"),
   wxT("Duet"),
   wxT("Punk Rock"),
   wxT("Drum Solo"),
   wxT("A Cappella"),
   wxT("Euro-House"),
   wxT("Dance Hall"),
   wxT("Goa"),
   wxT("Drum & Bass"),
   wxT("Club-House"),
   wxT("Hardcore"),
   wxT("Terror"),
   wxT("Indie"),
   wxT("BritPop"),

   // Standard name is offensive (see "http://www.audacityteam.org/forum/viewtopic.php?f=11&t=3924").
   wxT("Offensive"), // wxT("Negerpunk"),

   wxT("Polsk Punk"),
   wxT("Beat"),
   wxT("Christian Gangsta Rap"),
   wxT("Heavy Metal"),
   wxT("Black Metal"),
   wxT("Crossover"),
   wxT("Contemporary Christian"),
   wxT("Christian Rock"),
   wxT("Merengue"),
   wxT("Salsa"),
   wxT("Thrash Metal"),
   wxT("Anime"),
   wxT("JPop"),
   wxT("Synthpop")
};

Tags::Tags()
{
   mEditTitle = true;
   mEditTrackNumber = true;

   LoadDefaults();
   LoadGenres();
}

Tags::~Tags()
{
}

Tags & Tags::operator=(const Tags & src)
{
   mEditTitle = src.mEditTitle;
   mEditTrackNumber = src.mEditTrackNumber;

   mXref.clear();
   mXref = src.mXref;
   mMap.clear();
   mMap = src.mMap;

   mGenres.clear();
   mGenres = src.mGenres;

   return *this;
}

void Tags::LoadDefaults()
{
   wxString path;
   wxString name;
   wxString value;
   long ndx;
   bool cont;

   // Set the parent group
   path = gPrefs->GetPath();
   gPrefs->SetPath(wxT("/Tags"));

   // Process all entries in the group
   cont = gPrefs->GetFirstEntry(name, ndx);
   while (cont) {
      gPrefs->Read(name, &value, wxT(""));

      if (name == wxT("ID3V2")) {
         // LLL:  This is obsolute, but it must be handled and ignored.
      }
      else {
         SetTag(name, value);
      }

      cont = gPrefs->GetNextEntry(name, ndx);
   }

   // Restore original group
   gPrefs->SetPath(path);
}

bool Tags::IsEmpty()
{
   // At least one of these should be filled in, otherwise
   // it's assumed that the tags have not been set...
   if (HasTag(TAG_TITLE) || HasTag(TAG_ARTIST) || HasTag(TAG_ALBUM)) {
      return false;
   }

   return true;
}

void Tags::Clear()
{
   mXref.clear();
   mMap.clear();
}

void Tags::AllowEditTitle(bool editTitle)
{
   mEditTitle = editTitle;
}

void Tags::AllowEditTrackNumber(bool editTrackNumber)
{
   mEditTrackNumber = editTrackNumber;
}

int Tags::GetNumUserGenres()
{
   return mGenres.GetCount();
}

void Tags::LoadDefaultGenres()
{
   mGenres.Clear();
   for (size_t i = 0; i < WXSIZEOF(DefaultGenres); i++) {
      mGenres.Add(DefaultGenres[i]);
   }
}

void Tags::LoadGenres()
{
   wxFileName fn(FileNames::DataDir(), wxT("genres.txt"));
   wxTextFile tf(fn.GetFullPath());

   if (!tf.Exists() || !tf.Open()) {
      LoadDefaultGenres();
      return;
   }

   mGenres.Clear();

   int cnt = tf.GetLineCount();
   for (int i = 0; i < cnt; i++) {
      mGenres.Add(tf.GetLine(i));
   }
}

wxString Tags::GetUserGenre(int i)
{
   if (i >= 0 && i < GetNumUserGenres()) {
      return mGenres[i];
   }

   return wxT("");
}

wxString Tags::GetGenre(int i)
{
   int cnt = WXSIZEOF(DefaultGenres);

   if (i >= 0 && i < cnt) {
      return DefaultGenres[i];
   }

   return wxT("");
}

int Tags::GetGenre(const wxString & name)
{
   int cnt = WXSIZEOF(DefaultGenres);

   for (int i = 0; i < cnt; i++) {
      if (name.CmpNoCase(DefaultGenres[i])) {
         return i;
      }
   }

   return 255;
}

bool Tags::HasTag(const wxString & name)
{
   wxString key = name;
   key.UpperCase();

   TagMap::iterator iter = mXref.find(key);
   return (iter != mXref.end());
}

wxString Tags::GetTag(const wxString & name)
{
   wxString key = name;
   key.UpperCase();

   TagMap::iterator iter = mXref.find(key);

   if (iter == mXref.end()) {
      return wxEmptyString;
   }

   return mMap[iter->second];
}

bool Tags::GetFirst(wxString & name, wxString & value)
{
   mIter = mMap.begin();
   if (mIter == mMap.end()) {
      return false;
   }

   name = mIter->first;
   value = mIter->second;

   return true;
}

bool Tags::GetNext(wxString & name, wxString & value)
{
   ++mIter;
   if (mIter == mMap.end()) {
      return false;
   }

   name = mIter->first;
   value = mIter->second;

   return true;
}

void Tags::SetTag(const wxString & name, const wxString & value)
{
   // We don't like empty names
   if (name.IsEmpty()) {
      return;
   }

   // All keys are uppercase
   wxString key = name;
   key.UpperCase();

   // Look it up
   TagMap::iterator iter = mXref.find(key);

   // Didn't find the tag
   if (iter == mXref.end()) {
      // Intention was to delete so no need to add it
      if (value.IsEmpty()) {
         return;
      }

      // Add a new tag
      mXref[key] = name;
      mMap[name] = value;
      return;
   }

   // Intention was to delete
   if (value.IsEmpty()) {
      mMap.erase(iter->second);
      mXref.erase(iter);
      return;
   }

   // Update the value
   mMap[iter->second] = value;
}

void Tags::SetTag(const wxString & name, const int & value)
{
   SetTag(name, wxString::Format(wxT("%d"), value));
}

bool Tags::HandleXMLTag(const wxChar *tag, const wxChar **attrs)
{
   if (wxStrcmp(tag, wxT("tags")) == 0) {
      return true;
   }

   if (wxStrcmp(tag, wxT("tag")) == 0) {
      wxString n, v;

      while (*attrs) {
         wxString attr = *attrs++;
         if (attr.IsEmpty())
            break;
         wxString value = *attrs++;

         if (!XMLValueChecker::IsGoodString(attr) ||
             !XMLValueChecker::IsGoodString(value)) {
            break;
         }

         if (attr == wxT("name")) {
            n = value;
         }
         else if (attr == wxT("value")) {
            v = value;
         }
      }

      if (n == wxT("id3v2")) {
         // LLL:  This is obsolute, but it must be handled and ignored.
      }
      else {
         SetTag(n, v);
      }

      return true;
   }

   return false;
}

XMLTagHandler *Tags::HandleXMLChild(const wxChar *tag)
{
   if (wxStrcmp(tag, wxT("tags")) == 0) {
      return this;
   }

   if (wxStrcmp(tag, wxT("tag")) == 0) {
      return this;
   }

   return NULL;
}

void Tags::WriteXML(XMLWriter &xmlFile)
{
   xmlFile.StartTag(wxT("tags"));

   wxString n, v;
   for (bool cont = GetFirst(n, v); cont; cont = GetNext(n, v)) {
      xmlFile.StartTag(wxT("tag"));
      xmlFile.WriteAttr(wxT("name"), n);
      xmlFile.WriteAttr(wxT("value"), v);
      xmlFile.EndTag(wxT("tag"));
   }

   xmlFile.EndTag(wxT("tags"));
}

bool Tags::ShowEditDialog(wxWindow *parent, wxString title, bool force)
{
   if (force) {
      TagsEditor dlg(parent, title, this, mEditTitle, mEditTrackNumber);

      return dlg.ShowModal() == wxID_OK;
   }

   return true;
}
//
// ComboEditor - Wrapper to prevent unwanted background erasure
//

class ComboEditor:public wxGridCellChoiceEditor
{
public:
   ComboEditor(const wxArrayString& choices, bool allowOthers = false)
   :  wxGridCellChoiceEditor(choices, allowOthers)
   {
   }

   virtual void PaintBackground(const wxRect& WXUNUSED(rectCell), wxGridCellAttr * WXUNUSED(attr))
   {
      // Ignore it (a must on the Mac as the erasure causes problems.)
   }

   void SetParameters(const wxString& params)
   {
      wxGridCellChoiceEditor::SetParameters(params);

      // Refresh the wxComboBox with new values
      if (Combo()) {
         Combo()->Clear();
         Combo()->Append(m_choices);
      }
   }

   void SetSize(const wxRect& rectOrig)
   {
      wxRect rect(rectOrig);
      wxRect r = Combo()->GetRect();

      // Center the combo box in or over the cell
      rect.y -= (r.GetHeight() - rect.GetHeight()) / 2;
      rect.height = r.GetHeight();

      wxGridCellChoiceEditor::SetSize(rect);
   }
};

//
// Editor
//

#define LABEL_ARTIST    _("Artist Name")
#define LABEL_TITLE     _("Track Title")
#define LABEL_ALBUM     _("Album Title")
#define LABEL_TRACK     _("Track Number")
#define LABEL_YEAR      _("Year")
#define LABEL_GENRE     _("Genre")
#define LABEL_COMMENTS  _("Comments")

static wxString names[] =
{
   LABEL_ARTIST,
   LABEL_TITLE,
   LABEL_ALBUM,
   LABEL_TRACK,
   LABEL_YEAR,
   LABEL_GENRE,
   LABEL_COMMENTS
};

static struct
{
   wxString label;
   wxString name;
}
labelmap[] =
{
   {  LABEL_ARTIST,     TAG_ARTIST     },
   {  LABEL_TITLE,      TAG_TITLE      },
   {  LABEL_ALBUM,      TAG_ALBUM      },
   {  LABEL_TRACK,      TAG_TRACK      },
   {  LABEL_YEAR,       TAG_YEAR       },
   {  LABEL_GENRE,      TAG_GENRE      },
   {  LABEL_COMMENTS,   TAG_COMMENTS   }
};

#define STATICCNT WXSIZEOF(labelmap)

enum {
   ClearID = 10000,
   EditID,
   ResetID,
   LoadID,
   SaveID,
   SaveDefaultsID,
   AddID,
   RemoveID
};

BEGIN_EVENT_TABLE(TagsEditor, wxDialog)
#if wxCHECK_VERSION(3,0,0)
   EVT_GRID_CELL_CHANGED(TagsEditor::OnChange)
#else
   EVT_GRID_CELL_CHANGE(TagsEditor::OnChange)
#endif
   EVT_BUTTON(EditID, TagsEditor::OnEdit)
   EVT_BUTTON(ResetID, TagsEditor::OnReset)
   EVT_BUTTON(ClearID, TagsEditor::OnClear)
   EVT_BUTTON(LoadID, TagsEditor::OnLoad)
   EVT_BUTTON(SaveID, TagsEditor::OnSave)
   EVT_BUTTON(SaveDefaultsID, TagsEditor::OnSaveDefaults)
   EVT_BUTTON(AddID, TagsEditor::OnAdd)
   EVT_BUTTON(RemoveID, TagsEditor::OnRemove)
   EVT_BUTTON(wxID_CANCEL, TagsEditor::OnCancel)
   EVT_BUTTON(wxID_OK, TagsEditor::OnOk)
END_EVENT_TABLE()

TagsEditor::TagsEditor(wxWindow * parent,
                       wxString title,
                       Tags * tags,
                       bool editTitle,
                       bool editTrack)
:  wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
            wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
   mTags(tags),
   mEditTitle(editTitle),
   mEditTrack(editTrack)
{
   SetName(GetTitle());

   names[0] = LABEL_ARTIST;
   names[1] = LABEL_TITLE;
   names[2] = LABEL_ALBUM;
   names[3] = LABEL_TRACK;
   names[4] = LABEL_YEAR;
   names[5] = LABEL_GENRE;
   names[6] = LABEL_COMMENTS;

   labelmap[0].label = LABEL_ARTIST;
   labelmap[1].label = LABEL_TITLE;
   labelmap[2].label = LABEL_ALBUM;
   labelmap[3].label = LABEL_TRACK;
   labelmap[4].label = LABEL_YEAR;
   labelmap[5].label = LABEL_GENRE;
   labelmap[6].label = LABEL_COMMENTS;

   labelmap[0].name = TAG_ARTIST;
   labelmap[1].name = TAG_TITLE;
   labelmap[2].name = TAG_ALBUM;
   labelmap[3].name = TAG_TRACK;
   labelmap[4].name = TAG_YEAR;
   labelmap[5].name = TAG_GENRE;
   labelmap[6].name = TAG_COMMENTS;

   mGrid = NULL;

   // Make a local copy of the passed in tags
   mLocal = *mTags;

   // Build, size, and position the dialog
   ShuttleGui S(this, eIsCreating);
   PopulateOrExchange(S);

   TransferDataToWindow();

   Layout();
   Fit();
   Center();
   SetSizeHints(GetSize());

   // Restore the original tags because TransferDataToWindow() will be called again
   mLocal.Clear();
   mLocal = *mTags;

   // Override size and position with last saved
   wxRect r = GetRect();
   gPrefs->Read(wxT("/TagsEditor/x"), &r.x, r.x);
   gPrefs->Read(wxT("/TagsEditor/y"), &r.y, r.y);
   gPrefs->Read(wxT("/TagsEditor/width"), &r.width, r.width);
   gPrefs->Read(wxT("/TagsEditor/height"), &r.height, r.height);
   //On multi-monitor systems, there's a chance the last saved window position is
   //on a monitor that has been removed or is unavailable.
   if (IsWindowRectValid(&r))
      Move(r.GetPosition());

   SetSize(r.GetSize());
   Layout();

   // Resize value column width based on width of columns and the vertical scrollbar
   wxScrollBar sb(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
   r = mGrid->GetClientRect();
   r.width -= mGrid->GetColSize(0);
   r.width -= sb.GetSize().GetWidth();
   r.width -= 10;
   r.width -= r.x;
   mGrid->SetColSize(1, r.width);

   // Load the genres
   PopulateGenres();
}

TagsEditor::~TagsEditor()
{
   delete mGrid;
// TODO:  Need to figure out if these should be deleted.  Looks like the wxGrid
//        code takes ownership and uses reference counting, but there's been
//        cases where they show up as memory leaks.
//   delete mStringRenderer;
//   delete mComboEditor;
}

void TagsEditor::PopulateOrExchange(ShuttleGui & S)
{
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxALIGN_LEFT, false);
      {
         S.AddUnits(_("Use arrow keys (or ENTER key after editing) to navigate fields."));
      }
      S.EndHorizontalLay();

      if (mGrid == NULL) {
         mGrid = new Grid(S.GetParent(),
                          wxID_ANY,
                          wxDefaultPosition,
                          wxDefaultSize,
                          wxSUNKEN_BORDER);

         mStringRenderer = new wxGridCellStringRenderer;
         mComboEditor = new ComboEditor(wxArrayString(), true);

         mGrid->RegisterDataType(wxT("Combo"), mStringRenderer, mComboEditor);

         mGrid->SetColLabelSize(mGrid->GetDefaultRowSize());

         wxArrayString cs(WXSIZEOF(names), names);

         // Build the initial (empty) grid
         mGrid->CreateGrid(0, 2);
         mGrid->SetRowLabelSize(0);
         mGrid->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
         mGrid->SetColLabelValue(0, _("Tag"));
         mGrid->SetColLabelValue(1, _("Value"));

         // Resize the name column and set default row height.
         wxComboBox tc(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, cs);
         mGrid->SetColSize(0, tc.GetSize().x);
         mGrid->SetColMinimalWidth(0, tc.GetSize().x);
      }
      S.Prop(true);
      S.AddWindow(mGrid, wxEXPAND | wxALL);

      S.StartMultiColumn(4, wxALIGN_CENTER);
      {
         S.Id(AddID).AddButton(_("&Add"));
         S.Id(RemoveID).AddButton(_("&Remove"));
         S.AddTitle(wxT(" "));
         S.Id(ClearID).AddButton(_("Cl&ear"));
      }
      S.EndMultiColumn();

      S.StartHorizontalLay(wxALIGN_CENTRE, false);
      {
         S.StartStatic(_("Genres"));
         {
            S.StartMultiColumn(4, wxALIGN_CENTER);
            {
               S.Id(EditID).AddButton(_("E&dit..."));
               S.Id(ResetID).AddButton(_("Rese&t..."));
            }
            S.EndMultiColumn();
         }
         S.EndStatic();
         S.StartStatic(_("Template"));
         {
            S.StartMultiColumn(4, wxALIGN_CENTER);
            {
               S.Id(LoadID).AddButton(_("&Load..."));
               S.Id(SaveID).AddButton(_("&Save..."));
               S.AddTitle(wxT(" "));
               S.Id(SaveDefaultsID).AddButton(_("Set De&fault"));
            }
            S.EndMultiColumn();
         }
         S.EndStatic();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();

   S.AddStandardButtons(eOkButton | eCancelButton);
}

bool TagsEditor::TransferDataFromWindow()
{
   int i, cnt = mGrid->GetNumberRows();

   if (mGrid->IsCellEditControlShown()) {
      mGrid->SaveEditControlValue();
      mGrid->HideCellEditControl();
   }

   mLocal.Clear();
   for (i = 0; i < cnt; i++) {
      wxString n = mGrid->GetCellValue(i, 0);
      wxString v = mGrid->GetCellValue(i, 1);

      if (n.IsEmpty()) {
         continue;
      }

      if (n.CmpNoCase(LABEL_ARTIST) == 0) {
         n = TAG_ARTIST;
      }
      else if (n.CmpNoCase(LABEL_TITLE) == 0) {
         n = TAG_TITLE;
      }
      else if (n.CmpNoCase(LABEL_ALBUM) == 0) {
         n = TAG_ALBUM;
      }
      else if (n.CmpNoCase(LABEL_TRACK) == 0) {
         n = TAG_TRACK;
      }
      else if (n.CmpNoCase(LABEL_YEAR) == 0) {
         n = TAG_YEAR;
      }
      else if (n.CmpNoCase(LABEL_GENRE) == 0) {
         n = TAG_GENRE;
      }
      else if (n.CmpNoCase(LABEL_COMMENTS) == 0) {
         n = TAG_COMMENTS;
      }

      mLocal.SetTag(n, v);
   }

   return true;
}

bool TagsEditor::TransferDataToWindow()
{
   size_t i;
   wxString n;
   wxString v;

   // Disable redrawing until we're done
   mGrid->BeginBatch();

   // Delete all rows
   if (mGrid->GetNumberRows()) {
      mGrid->DeleteRows(0, mGrid->GetNumberRows());
   }

   // Populate the static rows
   for (i = 0; i < STATICCNT; i++) {
      mGrid->AppendRows();

      mGrid->SetReadOnly(i, 0);
      mGrid->SetCellValue(i, 0, labelmap[i].label);
      mGrid->SetCellValue(i, 1, mLocal.GetTag(labelmap[i].name));

      if (!mEditTitle && mGrid->GetCellValue(i, 0).CmpNoCase(LABEL_TITLE) == 0) {
         mGrid->SetReadOnly(i, 1);
      }

      if (!mEditTrack && mGrid->GetCellValue(i, 0).CmpNoCase(LABEL_TRACK) == 0) {
         mGrid->SetReadOnly(i, 1);
      }

      mLocal.SetTag(labelmap[i].name, wxEmptyString);
   }

   // Populate the rest
   for (bool cont = mLocal.GetFirst(n, v); cont; cont = mLocal.GetNext(n, v)) {
      mGrid->AppendRows();

      mGrid->SetCellValue(i, 0, n);
      mGrid->SetCellValue(i, 1, v);
      i++;
   }

   // Add an extra one to help with initial sizing and to show it can be done
   mGrid->AppendRows(1);

   // We're done, so allow the grid to redraw
   mGrid->EndBatch();

   // Set the editors
   SetEditors();

   return true;
}

void TagsEditor::OnChange(wxGridEvent & event)
{
   static bool ischanging = false;

   // Prevent recursion
   if (ischanging) {
      return;
   }

   event.Skip();

   if (event.GetCol() != 0) {
      return;
   }

   wxString n = mGrid->GetCellValue(event.GetRow(), 0);
   for (size_t i = 0; i < STATICCNT; i++) {
      if (n.CmpNoCase(labelmap[i].label) == 0) {
         ischanging = true;
         wxBell();
         mGrid->SetGridCursor(i, 0);
         event.Veto();
         ischanging = false;
         break;
      }
   }

   return;
}

void TagsEditor::OnEdit(wxCommandEvent & WXUNUSED(event))
{
   if (mGrid->IsCellEditControlShown()) {
      mGrid->SaveEditControlValue();
      mGrid->HideCellEditControl();
   }

   wxDialog dlg(this, wxID_ANY, _("Edit Genres"),
                wxDefaultPosition, wxDefaultSize,
                wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
   dlg.SetName(dlg.GetTitle());
   wxTextCtrl *tc;

   ShuttleGui S(&dlg, eIsCreating);

   S.StartVerticalLay(true);
   {
      tc = S.AddTextWindow(wxT(""));
   }
   S.EndVerticalLay();

   S.AddStandardButtons();

   wxSortedArrayString g;
   int cnt = mLocal.GetNumUserGenres();
   for (int i = 0; i < cnt; i++) {
      g.Add(mLocal.GetUserGenre(i));
   }

   for (int i = 0; i < cnt; i++) {
      tc->AppendText(g[i] + wxT("\n"));
   }

   dlg.Center();
   if (dlg.ShowModal() == wxID_CANCEL) {
      return;
   }

   wxFileName fn(FileNames::DataDir(), wxT("genres.txt"));
   wxFile f(fn.GetFullPath(), wxFile::write);
   if (!f.IsOpened() || !f.Write(tc->GetValue())) {
      wxMessageBox(_("Unable to save genre file."), _("Reset Genres"));
      return;
   }

   mLocal.LoadGenres();

   PopulateGenres();
}

void TagsEditor::OnReset(wxCommandEvent & WXUNUSED(event))
{
   int id = wxMessageBox(_("Are you sure you want to reset the genre list to defaults?"),
                         _("Reset Genres"),
                         wxYES_NO);

   if (id == wxNO) {
      return;
   }
   mLocal.LoadDefaultGenres();

   wxFileName fn(FileNames::DataDir(), wxT("genres.txt"));
   wxTextFile tf(fn.GetFullPath());

   bool open = (tf.Exists() && tf.Open()) ||
               (!tf.Exists() && tf.Create());

   if (!open) {
      wxMessageBox(_("Unable to open genre file."), _("Reset Genres"));
      mLocal.LoadGenres();
      return;
   }

   tf.Clear();
   int cnt = mLocal.GetNumUserGenres();
   for (int i = 0; i < cnt; i++) {
      tf.AddLine(mLocal.GetUserGenre(i));
   }

   if (!tf.Write()) {
      wxMessageBox(_("Unable to save genre file."), _("Reset Genres"));
      mLocal.LoadGenres();
      return;
   }

   mLocal.LoadGenres();

   PopulateGenres();
}

void TagsEditor::OnClear(wxCommandEvent & WXUNUSED(event))
{
   mLocal.Clear();

   TransferDataToWindow();
}

void TagsEditor::OnLoad(wxCommandEvent & WXUNUSED(event))
{
   wxString fn;

   // Ask the user for the real name
   fn = FileSelector(_("Load Metadata As:"),
                     FileNames::DataDir(),
                     wxT("Tags.xml"),
                     wxT("xml"),
                     wxT("*.xml"),
                     wxFD_OPEN | wxRESIZE_BORDER,
                     this);

   // User canceled...
   if (fn.IsEmpty()) {
      return;
   }

   // Remember title and track in case they're read only
   wxString title = mLocal.GetTag(TAG_TITLE);
   wxString track = mLocal.GetTag(TAG_TRACK);

   // Clear current contents
   mLocal.Clear();

   // Load the metadata
   XMLFileReader reader;
   if (!reader.Parse(&mLocal, fn)) {
      // Inform user of load failure
      wxMessageBox(reader.GetErrorStr(),
                   _("Error Loading Metadata"),
                   wxOK | wxCENTRE,
                   this);
   }

   // Restore title
   if (!mEditTitle) {
      mLocal.SetTag(TAG_TITLE, title);
   }

   // Restore track
   if (!mEditTrack) {
      mLocal.SetTag(TAG_TRACK, track);
   }

   // Go fill up the window
   TransferDataToWindow();

   return;
}

void TagsEditor::OnSave(wxCommandEvent & WXUNUSED(event))
{
   wxString fn;

   // Refresh tags
   TransferDataFromWindow();

   // Ask the user for the real name
   fn = FileSelector(_("Save Metadata As:"),
                     FileNames::DataDir(),
                     wxT("Tags.xml"),
                     wxT("xml"),
                     wxT("*.xml"),
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER,
                     this);

   // User canceled...
   if (fn.IsEmpty()) {
      return;
   }

   // Create/Open the file
   XMLFileWriter writer;

   try
   {
      writer.Open(fn, wxT("wb"));

      // Remember title and track in case they're read only
      wxString title = mLocal.GetTag(TAG_TITLE);
      wxString track = mLocal.GetTag(TAG_TRACK);

      // Clear title
      if (!mEditTitle) {
         mLocal.SetTag(TAG_TITLE, wxEmptyString);
      }

      // Clear track
      if (!mEditTrack) {
         mLocal.SetTag(TAG_TRACK, wxEmptyString);
      }

      // Write the metadata
      mLocal.WriteXML(writer);

      // Restore title
      if (!mEditTitle) {
         mLocal.SetTag(TAG_TITLE, title);
      }

      // Restore track
      if (!mEditTrack) {
         mLocal.SetTag(TAG_TRACK, track);
      }

      // Close the file
      writer.Close();
   }
   catch (XMLFileWriterException* pException)
   {
      wxMessageBox(wxString::Format(
         _("Couldn't write to file \"%s\": %s"),
         fn.c_str(), pException->GetMessage().c_str()),
         _("Error Saving Tags File"), wxICON_ERROR, this);

      delete pException;
   }
}

void TagsEditor::OnSaveDefaults(wxCommandEvent & WXUNUSED(event))
{
   // Refresh tags
   TransferDataFromWindow();

   // Remember title and track in case they're read only
   wxString title = mLocal.GetTag(TAG_TITLE);
   wxString track = mLocal.GetTag(TAG_TRACK);

   // Clear title
   if (!mEditTitle) {
      mLocal.SetTag(TAG_TITLE, wxEmptyString);
   }

   // Clear track
   if (!mEditTrack) {
      mLocal.SetTag(TAG_TRACK, wxEmptyString);
   }

   // Remove any previous defaults
   gPrefs->DeleteGroup(wxT("/Tags"));

   // Write out each tag
   wxString n, v;
   for (bool cont = mLocal.GetFirst(n, v); cont; cont = mLocal.GetNext(n, v)) {
      gPrefs->Write(wxT("/Tags/") + n, v);
   }
   gPrefs->Flush();

   // Restore title
   if (!mEditTitle) {
      mLocal.SetTag(TAG_TITLE, title);
   }

   // Restore track
   if (!mEditTrack) {
      mLocal.SetTag(TAG_TRACK, track);
   }
}

void TagsEditor::OnAdd(wxCommandEvent & WXUNUSED(event))
{
   mGrid->AppendRows();
}

void TagsEditor::OnRemove(wxCommandEvent & WXUNUSED(event))
{
   size_t row = mGrid->GetGridCursorRow();

   if (!mEditTitle && mGrid->GetCellValue(row, 0).CmpNoCase(LABEL_TITLE) == 0) {
      return;
   }
   else if (!mEditTrack && mGrid->GetCellValue(row, 0).CmpNoCase(LABEL_TRACK) == 0) {
      return;
   }
   else if (row < STATICCNT) {
      mGrid->SetCellValue(row, 1, wxEmptyString);
   }
   else if (row >= STATICCNT) {
      mGrid->DeleteRows(row, 1);
   }
}

void TagsEditor::OnOk(wxCommandEvent & WXUNUSED(event))
{
   if (mGrid->IsCellEditControlShown()) {
      mGrid->SaveEditControlValue();
      mGrid->HideCellEditControl();
      return;
   }

   if (!Validate() || !TransferDataFromWindow()) {
      return;
   }

   *mTags = mLocal;

   wxRect r = GetRect();
   gPrefs->Write(wxT("/TagsEditor/x"), r.x);
   gPrefs->Write(wxT("/TagsEditor/y"), r.y);
   gPrefs->Write(wxT("/TagsEditor/width"), r.width);
   gPrefs->Write(wxT("/TagsEditor/height"), r.height);
   gPrefs->Flush();

   EndModal(wxID_OK);
}

void TagsEditor::OnCancel(wxCommandEvent & WXUNUSED(event))
{
   if (mGrid->IsCellEditControlShown()) {
      mGrid->GetCellEditor(mGrid->GetGridCursorRow(),
                           mGrid->GetGridCursorCol())
                           ->Reset();
      mGrid->HideCellEditControl();
      return;
   }

   EndModal(wxID_CANCEL);
}

void TagsEditor::SetEditors()
{
   int cnt = mGrid->GetNumberRows();

   for (int i = 0; i < cnt; i++) {
      wxString label = mGrid->GetCellValue(i, 0);
      if (label.CmpNoCase(LABEL_GENRE) == 0) {
         mGrid->SetCellEditor(i, 1, mGrid->GetDefaultEditorForType(wxT("Combo")));
      }
      else {
         mGrid->SetCellEditor(i, 1, NULL); //mGrid->GetDefaultEditor());
      }
   }
}

void TagsEditor::PopulateGenres()
{
   int cnt = mLocal.GetNumUserGenres();
   int i;
   wxString parm;
   wxSortedArrayString g;

   for (i = 0; i < cnt; i++) {
      g.Add(mLocal.GetUserGenre(i));
   }

   for (i = 0; i < cnt; i++) {
      parm = parm + (i == 0 ? wxT("") : wxT(",")) + g[i];
   }

   mGrid->GetDefaultEditorForType(wxT("Combo"))->SetParameters(parm);
}

bool TagsEditor::IsWindowRectValid(const wxRect *windowRect) const
{
   wxDisplay display;
   wxPoint topLeft(windowRect->GetTopLeft().x, windowRect->GetTopLeft().y);
   wxPoint topRight(windowRect->GetTopRight().x, windowRect->GetTopRight().y);
   wxPoint bottomLeft(windowRect->GetBottomLeft().x, windowRect->GetBottomLeft().y);
   wxPoint bottomRight(windowRect->GetBottomRight().x, windowRect->GetBottomRight().y);
   display.GetFromPoint(topLeft);
   if (display.GetFromPoint(topLeft) == -1 &&
       display.GetFromPoint(topRight) == -1 &&
       display.GetFromPoint(bottomLeft) == -1 &&
       display.GetFromPoint(bottomRight) == -1) {
      return false;
   }

   return true;
}
