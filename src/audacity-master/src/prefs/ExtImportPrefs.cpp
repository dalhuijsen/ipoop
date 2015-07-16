/**********************************************************************

  Audacity: A Digital Audio Editor

  ExtImportPrefs.cpp

  LRN

*******************************************************************//**

\class ExtImportPrefs
\brief A PrefsPanel used to select extended import filter options.

*//*******************************************************************/


#include <wx/defs.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/dnd.h>

#include "ExtImportPrefs.h"
#include "../Audacity.h"
#include "../AudacityApp.h"
#include "../Prefs.h"
#include "../ShuttleGui.h"

#define EXTIMPORT_MIME_SUPPORT 0

enum ExtImportPrefsControls
{
   EIPPluginList = 20000,
   EIPRuleTable,
   EIPAddRule,
   EIPDelRule,
   EIPMoveRuleUp,
   EIPMoveRuleDown,
   EIPMoveFilterUp,
   EIPMoveFilterDown
};

BEGIN_EVENT_TABLE(ExtImportPrefs, PrefsPanel)
   EVT_LIST_KEY_DOWN(EIPPluginList,ExtImportPrefs::OnPluginKeyDown)
   EVT_LIST_BEGIN_DRAG(EIPPluginList,ExtImportPrefs::OnPluginBeginDrag)
   EVT_KEY_DOWN (ExtImportPrefs::OnRuleTableKeyDown)
   EVT_GRID_CELL_LEFT_CLICK (ExtImportPrefs::OnRuleTableCellClick)
   EVT_GRID_EDITOR_HIDDEN (ExtImportPrefs::OnRuleTableEdit)
   EVT_GRID_SELECT_CELL (ExtImportPrefs::OnRuleTableSelect)
   EVT_GRID_RANGE_SELECT (ExtImportPrefs::OnRuleTableSelectRange)
   EVT_BUTTON(EIPAddRule,ExtImportPrefs::OnAddRule)
   EVT_BUTTON(EIPDelRule,ExtImportPrefs::OnDelRule)
   EVT_BUTTON(EIPMoveRuleUp,ExtImportPrefs::OnRuleMoveUp)
   EVT_BUTTON(EIPMoveRuleDown,ExtImportPrefs::OnRuleMoveDown)
   EVT_BUTTON(EIPMoveFilterUp,ExtImportPrefs::OnFilterMoveUp)
   EVT_BUTTON(EIPMoveFilterDown,ExtImportPrefs::OnFilterMoveDown)
END_EVENT_TABLE()

ExtImportPrefs::ExtImportPrefs(wxWindow * parent)
:   PrefsPanel(parent, _("Extended Import")), RuleTable(NULL),
    PluginList(NULL), mCreateTable (false), mDragFocus (NULL),
    mFakeKeyEvent (false), mStopRecursiveSelection (false), last_selected (-1)
{
   dragtext1 = new wxTextDataObject(wxT(""));
   dragtext2 = new wxTextDataObject(wxT(""));
   dragtarget1 = new ExtImportPrefsDropTarget(dragtext1);
   dragtarget2 = new ExtImportPrefsDropTarget(dragtext2);
   dragtarget1->SetPrefs (this);
   dragtarget2->SetPrefs (this);
   Populate();
}

ExtImportPrefs::~ExtImportPrefs()
{
}

/// Creates the dialog and its contents.
void ExtImportPrefs::Populate()
{
   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   // Use 'eIsCreatingFromPrefs' so that the GUI is
   // initialised with values from gPrefs.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
}

void ExtImportPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);

   S.TieCheckBox(_("A&ttempt to use filter in OpenFile dialog first"),
         wxT("/ExtendedImport/OverrideExtendedImportByOpenFileDialogChoice"),
         true);
   S.StartStatic(_("Rules to choose import filters"), 1);
   {
      S.SetSizerProportion(1);
      S.StartHorizontalLay (wxEXPAND, 1);
      {
         bool fillRuleTable = false;
         if (RuleTable == NULL)
         {
            RuleTable = new Grid(S.GetParent(),EIPRuleTable);

            RuleTable->SetColLabelSize(RuleTable->GetDefaultRowSize());
#if EXTIMPORT_MIME_SUPPORT
            RuleTable->CreateGrid (0, 2, wxGrid::wxGridSelectRows);
#else
            RuleTable->CreateGrid (0, 1, wxGrid::wxGridSelectRows);
#endif
            RuleTable->DisableDragColMove ();
            RuleTable->DisableDragRowSize ();
            RuleTable->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
            RuleTable->SetColLabelValue (0, _("File extensions"));
#if EXTIMPORT_MIME_SUPPORT
            RuleTable->SetColLabelValue (1, _("Mime-types"));
#endif
            RuleTable->SetRowLabelSize (0);
            RuleTable->SetSelectionMode (wxGrid::wxGridSelectRows);
            RuleTable->AutoSizeColumns ();

            RuleTable->SetDropTarget (dragtarget1);
            RuleTable->EnableDragCell (true);
            fillRuleTable = true;
         }
         S.AddWindow(RuleTable, wxEXPAND | wxALL);

         PluginList = S.Id(EIPPluginList).AddListControl ();

         if (fillRuleTable)
         {
            PluginList->SetSingleStyle (wxLC_REPORT, true);
            PluginList->SetSingleStyle (wxLC_SINGLE_SEL, true);
            PluginList->InsertColumn (0, _("Importer order"));
            PluginList->SetDropTarget (dragtarget2);

            ExtImportItems *items = Importer::Get().GetImportItems();
            for (unsigned int i = 0; i < items->Count(); i++)
               AddItemToTable (i, &(*items)[i]);
            if (items->Count() > 0)
            {
               RuleTable->SelectRow(0);
               RuleTable->SetGridCursor(0,0);
            }
         }
      }
      S.EndHorizontalLay();
      S.StartHorizontalLay (wxSHRINK, 0);
      {
          MoveRuleUp = S.Id (EIPMoveRuleUp).AddButton (_("Move rule &up"));
          MoveRuleDown = S.Id (EIPMoveRuleDown).AddButton
                (_("Move rule &down"));
          MoveFilterUp = S.Id (EIPMoveFilterUp).AddButton
                (_("Move f&ilter up"));
          MoveFilterDown = S.Id (EIPMoveFilterDown).AddButton
                (_("Move &filter down"));
      }
      S.EndHorizontalLay();
      S.StartHorizontalLay (wxSHRINK, 0);
      {
          AddRule = S.Id (EIPAddRule).AddButton (_("&Add new rule"));
          DelRule = S.Id (EIPDelRule).AddButton (_("De&lete selected rule"));
      }
      S.EndHorizontalLay();
   }
   S.EndStatic();
   Layout();
   Fit();
   SetMinSize(GetSize());
}

bool ExtImportPrefs::Apply()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   return true;
}

void ExtImportPrefs::OnPluginKeyDown(wxListEvent& event)
{
   for (int i = 0; i < 1; i++)
   {
#ifdef __WXMAC__
      if (!mFakeKeyEvent && !wxGetKeyState(WXK_COMMAND))
         break;
#else
      if (!mFakeKeyEvent && !wxGetKeyState(WXK_CONTROL))
         break;
#endif

      if (DoOnPluginKeyDown (event.GetKeyCode()))
         event.Skip();
   }
}

void ExtImportPrefs::SwapPluginRows (int row1, int row2)
{
   wxString t, t2;
   long d, d2;
   ImportPlugin *ip1, *ip2;

   ExtImportItems *items = Importer::Get().GetImportItems();
   ExtImportItem *item = &(*items)[last_selected];

   t = PluginList->GetItemText (row1);
   d = PluginList->GetItemData (row1);
   d2 = PluginList->GetItemData (row2);
   PluginList->SetItemText (row1, PluginList->GetItemText (row2));
   PluginList->SetItemText (row2, t);
   if (d == -1 || d2 == -1)
   {
      PluginList->SetItemData (row1, PluginList->GetItemData (row2));
      PluginList->SetItemData (row2, d);
      if (d == -1)
      {
         item->divider = row2;
      }
      else if (d2 == -1)
      {
         item->divider = row1;
      }
   }
   else
   {
      ip1 = item->filter_objects[d];
      ip2 = item->filter_objects[d2];
      item->filter_objects[d] = ip2;
      item->filter_objects[d2] = ip1;
      t = item->filters[d];
      t2 = item->filters[d2];
      item->filters[d] = t2;
      item->filters[d2] = t;
   }
}

bool ExtImportPrefs::DoOnPluginKeyDown (int code)
{
   if (code != WXK_UP && code != WXK_DOWN)
         return false;

   long itemIndex = -1;
   long itemIndex2 = -1;
   itemIndex = PluginList->GetNextItem(itemIndex,
         wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   if (itemIndex == -1)
         return false;

   if (last_selected == -1)
         return false;

   ExtImportItems *items = Importer::Get().GetImportItems();
   ExtImportItem *item = &(*items)[last_selected];

   if (code == WXK_UP && itemIndex == 0)
      return false;
   else if (code == WXK_DOWN && itemIndex == PluginList->GetItemCount() - 1)
      return false;

   if (code == WXK_UP)
   {
      itemIndex2 = itemIndex - 1;
   }
   else if (code == WXK_DOWN)
   {
      itemIndex2 = itemIndex + 1;
   }
   SwapPluginRows (itemIndex, itemIndex2);
   if (mFakeKeyEvent)
   {
      PluginList->SetItemState (itemIndex, 0, wxLIST_STATE_SELECTED);
      PluginList->SetItemState (itemIndex2, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
   }
   int fcount = item->filter_objects.Count();
   if (item->divider >= fcount)
   {
      item->divider = -1;
   }
   if (item->divider < -1)
      item->divider = item->filter_objects.Count() - 1;

   return true;
}


void ExtImportPrefs::SwapRows (int row1, int row2)
{
   int t;
   ExtImportItem *t1, *t2;
   wxString ts;
   if (row1 == row2)
      return;
   if (row1 > row2)
   {
      t = row1;
      row1 = row2;
      row2 = t;
   }
   ExtImportItems *items = Importer::Get().GetImportItems();
   t1 = items->Detach(row1);
   t2 = items->Detach(row1);
   items->Insert (t1, row1);
   items->Insert (t2, row1);
   for (int i = 0; i < RuleTable->GetNumberCols(); i++)
   {
      ts = RuleTable->GetCellValue (row2, i);
      RuleTable->SetCellValue (row2, i, RuleTable->GetCellValue (row1, i));
      RuleTable->SetCellValue (row1, i, ts);
   }
}

void ExtImportPrefs::OnPluginBeginDrag(wxListEvent& WXUNUSED(event))
{
   wxDropSource dragSource(this);
   dragtext2->SetText(wxT(""));
   dragSource.SetData(*dragtext2);
   mDragFocus = PluginList;
   wxDragResult result = dragSource.DoDragDrop(TRUE);
   mDragFocus = NULL;
   switch (result)
   {
      case wxDragCopy:
      case wxDragMove:
      case wxDragNone:
         return;
         break;
      default:
         break;
   }
}

void ExtImportPrefs::OnRuleTableKeyDown(wxKeyEvent& event)
{
   int mods = event.GetModifiers();
   if (mods & wxMOD_CMD && (event.GetKeyCode() == WXK_UP ||
          event.GetKeyCode() == WXK_DOWN))
   {
      DoOnRuleTableKeyDown (event.GetKeyCode());
   }
   else
   {
      event.Skip();
   }
}

void ExtImportPrefs::DoOnRuleTableKeyDown (int keycode)
{
   int selrow = RuleTable->GetGridCursorRow ();
   wxString ts;
   if (keycode == WXK_UP)
   {
      if (selrow <= 0)
         return;
      SwapRows (selrow - 1, selrow);
      RuleTable->MoveCursorUp (false);
      RuleTable->SelectRow (selrow - 1);
   }
   else if (keycode == WXK_DOWN)
   {
      if (selrow == RuleTable->GetNumberRows() - 1)
         return;
      SwapRows (selrow, selrow + 1);
      RuleTable->MoveCursorDown (false);
      RuleTable->SelectRow (selrow + 1);
   }
}

void ExtImportPrefs::OnRuleTableSelect (wxGridEvent& event)
{
   int toprow;
   event.Skip();
   if (!event.Selecting() || mStopRecursiveSelection)
      return;

   toprow = event.GetRow();
   if (toprow < 0)
      return;

   DoOnRuleTableSelect (toprow);
}

void ExtImportPrefs::OnRuleTableSelectRange (wxGridRangeSelectEvent& event)
{
   int toprow;
   event.Skip();
   if (!event.Selecting() || mStopRecursiveSelection)
      return;

   toprow = event.GetTopRow();
   if (toprow < 0)
      return;

   DoOnRuleTableSelect (toprow);
   mStopRecursiveSelection = true;
   RuleTable->SelectRow (toprow);
   mStopRecursiveSelection = false;
   RuleTable->SetGridCursor (toprow, 0);
}

void ExtImportPrefs::DoOnRuleTableSelect (int toprow)
{
   ExtImportItems *items = Importer::Get().GetImportItems();

   if (toprow < 0 || toprow > (int)items->GetCount())
   {
      return;
   }

   ExtImportItem *item = &(*items)[toprow];
   PluginList->DeleteAllItems();

   int fcount;
   fcount = item->filters.Count();
   int shift = 0;
   for (int i = 0; i < fcount; i++)
   {
      if (item->divider == i)
      {
         PluginList->InsertItem (i, _("Unused filters:"));
         PluginList->SetItemData (i, -1);
         shift = 1;
      }
      if (item->filter_objects[i] != NULL)
      {
         PluginList->InsertItem (i + shift,
               item->filter_objects[i]->GetPluginFormatDescription());
      }
      else
      {
         PluginList->InsertItem (i + shift, item->filters[i]);
      }
      PluginList->SetItemData (i + shift, i);
   }
   if (item->divider == -1)
   {
      PluginList->InsertItem (fcount, _("Unused filters:"));
      PluginList->SetItemData (fcount, -1);
   }
   wxListItem info;
   info.SetId (0);
   info.SetColumn (0);
   info.SetStateMask (wxLIST_STATE_SELECTED);
   info.SetState (wxLIST_STATE_SELECTED);
   info.SetMask (wxLIST_MASK_STATE);
   PluginList->SetItem (info);
   PluginList->SetColumnWidth (0, wxLIST_AUTOSIZE);
   last_selected = toprow;
}

void ExtImportPrefs::OnRuleTableEdit (wxGridEvent& event)
{
   int row = event.GetRow();
   int col = event.GetCol();
   ExtImportItems *items = Importer::Get().GetImportItems();
   ExtImportItem *item = &(*items)[row];
   RuleTable->SaveEditControlValue();

   wxString val = RuleTable->GetCellValue (row, col);
   int fixSpaces = wxNO;
   bool askedAboutSpaces = false;
   wxArrayString vals;
   wxString delims(wxT(":"));
   Importer::Get().StringToList (val, delims, vals);
   switch (col)
   {
   case 0:
      item->extensions.Clear();
      break;
   case 1:
      item->mime_types.Clear();
      break;
   }

   for (size_t i = 0; i < vals.Count(); i++)
   {

      wxString trimmed = vals[i];
      trimmed.Trim().Trim(false);
      if (trimmed.Cmp(vals[i]) != 0)
      {
         if (!askedAboutSpaces)
         {
            fixSpaces = wxMessageBox(_(
"There are space characters (spaces, newlines, tabs or linefeeds) in one of \
the items. They are likely to break the pattern matching. Unless you know \
what you are doing, it is recommended to trim spaces. Do you want \
Audacity to trim spaces for you?"
            ),_("Spaces detected"), wxYES_NO);
            askedAboutSpaces = true;
         }
         if (fixSpaces != wxYES)
         {
            trimmed = vals[i];
         }
         else
         {
            vals[i] = trimmed;
         }
      }
      switch (col)
      {
      case 0:
         item->extensions.Add (trimmed);
         break;
      case 1:
         item->mime_types.Add (trimmed);
         break;
      }
   }
   if (fixSpaces == wxYES)
   {
      wxString vals_as_string;
      for (size_t i = 0; i < vals.Count(); i++)
      {
         if (i > 0)
            vals_as_string.Append (wxT(":"));
         vals_as_string.Append (vals[i]);
      }
      RuleTable->SetCellValue (row, col, vals_as_string);
   }

   RuleTable->AutoSizeColumns ();
}

void ExtImportPrefs::AddItemToTable (int index, ExtImportItem *item)
{
   wxString extensions, mime_types;
   if (item->extensions.Count() > 0)
   {
      extensions.Append (item->extensions[0]);
      for (unsigned int i = 1; i < item->extensions.Count(); i++)
      {
         extensions.Append (wxT(":"));
         extensions.Append (item->extensions[i]);
      }
   }
   if (item->mime_types.Count() > 0)
   {
      mime_types.Append (item->mime_types[0]);
      for (unsigned int i = 1; i < item->mime_types.Count(); i++)
      {
         mime_types.Append (wxT(":"));
         mime_types.Append (item->mime_types[i]);
      }
   }

   RuleTable->InsertRows (index, 1);
   RuleTable->SetCellValue (index, 0, extensions);
#if EXTIMPORT_MIME_SUPPORT
   RuleTable->SetCellValue (index, 1, mime_types);
#endif
   RuleTable->AutoSizeColumns ();
}

void ExtImportPrefs::OnAddRule(wxCommandEvent& WXUNUSED(event))
{
   ExtImportItems *items = Importer::Get().GetImportItems();
   ExtImportItem *item = Importer::Get().CreateDefaultImportItem();
   items->Add (item);

   AddItemToTable (RuleTable->GetNumberRows (), item);
   RuleTable->SelectRow(RuleTable->GetNumberRows () - 1);
   RuleTable->SetGridCursor (RuleTable->GetNumberRows () - 1, 0);
   RuleTable->SetFocus();
}

void ExtImportPrefs::OnDelRule(wxCommandEvent& WXUNUSED(event))
{
   if (last_selected < 0)
      return;
   ExtImportItems *items = Importer::Get().GetImportItems();

   int msgres = wxMessageBox (_("Do you really want to delete selected rule?"),
      _("Rule deletion confirmation"), wxYES_NO, RuleTable);
   if (msgres == wxNO || msgres != wxYES)
      return;

   RuleTable->DeleteRows (last_selected);
   items->RemoveAt (last_selected);
   RuleTable->AutoSizeColumns ();
   if (last_selected >= RuleTable->GetNumberRows ())
      last_selected = RuleTable->GetNumberRows () - 1;
   if (last_selected >= 0)
   {
      RuleTable->SelectRow(last_selected);
      RuleTable->SetGridCursor (last_selected, 0);
   }
}

void ExtImportPrefs::OnRuleMoveUp(wxCommandEvent& WXUNUSED(event))
{
   DoOnRuleTableKeyDown (WXK_UP);
}

void ExtImportPrefs::OnRuleMoveDown(wxCommandEvent& WXUNUSED(event))
{
   DoOnRuleTableKeyDown (WXK_DOWN);
}

void ExtImportPrefs::FakeOnPluginKeyDown (int keycode)
{
   wxListEvent fakeevent(wxEVT_COMMAND_LIST_KEY_DOWN, EIPPluginList);
   fakeevent.SetEventObject(this);
   fakeevent.m_code = keycode;
   mFakeKeyEvent = true;
   GetEventHandler()->ProcessEvent (fakeevent);
   mFakeKeyEvent = false;
}

void ExtImportPrefs::OnFilterMoveUp(wxCommandEvent& WXUNUSED(event))
{
   FakeOnPluginKeyDown (WXK_UP);
}

void ExtImportPrefs::OnFilterMoveDown(wxCommandEvent& WXUNUSED(event))
{
   FakeOnPluginKeyDown (WXK_DOWN);
}


void ExtImportPrefs::OnRuleTableCellClick (wxGridEvent& event)
{
   int row = event.GetRow();
   RuleTable->SelectRow (row, false);
   RuleTable->SetGridCursor (row, 0);

   wxDropSource dragSource(this);
   dragtext1->SetText(wxT(""));
   dragSource.SetData(*dragtext1);
   mDragFocus = RuleTable;
   wxDragResult result = dragSource.DoDragDrop(TRUE);
   mDragFocus = NULL;
   switch (result)
   {
      case wxDragCopy: /* copy the data */
      case wxDragMove:
      case wxDragNone:
         return;
         break;
      default:         /* do nothing */ break;
   }

   event.Skip();
}

ExtImportPrefsDropTarget::ExtImportPrefsDropTarget (wxDataObject *dataObject)
{
   SetDataObject (dataObject);
   mPrefs = NULL;
}

ExtImportPrefsDropTarget::~ExtImportPrefsDropTarget ()
{
}

void ExtImportPrefsDropTarget::SetPrefs (ExtImportPrefs *prefs)
{
   mPrefs = prefs;
}

wxDragResult ExtImportPrefsDropTarget::OnData(wxCoord  WXUNUSED(x), wxCoord  WXUNUSED(y),
      wxDragResult def)
{
   return def;
}

#if defined(__WXMSW__)
/* wxListCtrl::FindItem() in wxPoint()-taking mode works only for lists in
 * Small/Large-icon mode
 * wxListCtrl::HitTest() on Windows only hits item label rather than its whole
 * row, which makes it difficult to drag over items with short or non-existent
 * labels.
 */
long wxCustomFindItem(wxListCtrl *list, int x, int y)
{
   long count = list->GetItemCount();
   wxRect r;
   for (long i = 0; i < count; i++)
   {
      if (list->GetItemRect (i, r))
      {
         if (r.Contains (x, y))
            return i;
      }
   }
   return -1;
}
#endif

bool ExtImportPrefsDropTarget::OnDrop(wxCoord x, wxCoord y)
{
   if (mPrefs == NULL)
      return false;
   wxListCtrl *PluginList = mPrefs->GetPluginList();
   Grid *RuleTable = mPrefs->GetRuleTable();
   if (mPrefs->GetDragFocus() == RuleTable)
   {
      if (RuleTable->YToRow (y -
            RuleTable->GetColLabelSize ()) == wxNOT_FOUND)
         return false;
   }
   else if (mPrefs->GetDragFocus() == PluginList)
   {
#if defined(__WXMSW__)
      long item = wxCustomFindItem (PluginList, x, y);
#else
      int flags = 0;
      long item = PluginList->HitTest (wxPoint (x, y), flags, NULL);
#endif
      if (item < 0)
         return false;
   }

   return true;
}

wxDragResult ExtImportPrefsDropTarget::OnEnter(wxCoord x, wxCoord y,
      wxDragResult def)
{
   return OnDragOver(x, y, def);
}

wxDragResult ExtImportPrefsDropTarget::OnDragOver(wxCoord x, wxCoord y,
      wxDragResult WXUNUSED(def))
{
   if (mPrefs == NULL)
      return wxDragNone;
   wxListCtrl *PluginList = mPrefs->GetPluginList();
   Grid *RuleTable = mPrefs->GetRuleTable();
   if (mPrefs->GetDragFocus() == RuleTable)
   {
      int row;
      row = RuleTable->YToRow (y -
         RuleTable->GetColLabelSize ());
      if (row == wxNOT_FOUND)
         return wxDragNone;


      int cRow = RuleTable->GetGridCursorRow ();
      if (row != cRow)
      {
         mPrefs->SwapRows (cRow, row);
         RuleTable->SetGridCursor (row, 0);
         RuleTable->SelectRow (row);
      }
   }
   else if (mPrefs->GetDragFocus() == PluginList)
   {
#if defined(__WXMSW__)
      long item = wxCustomFindItem (PluginList, x, y);
#else
      int flags = 0;
      long item = PluginList->HitTest (wxPoint (x, y), flags, NULL);
#endif
      if (item < 0)
         return wxDragNone;

      long selected = -1;
      selected = PluginList->GetNextItem(selected,
            wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
      if (selected == -1)
            return wxDragNone;

      if (item != selected)
      {
         mPrefs->SwapPluginRows(selected, item);
         PluginList->SetItemState (selected, 0, wxLIST_STATE_SELECTED);
         PluginList->SetItemState (item, wxLIST_STATE_SELECTED,
               wxLIST_STATE_SELECTED);
      }
   }
   return wxDragCopy;
}

void ExtImportPrefsDropTarget::OnLeave()
{
}

void ExtImportPrefsDropTarget::SetDataObject(wxDataObject* data)
{
   this->m_dataObject = data;
}
