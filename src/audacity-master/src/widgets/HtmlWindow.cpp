/**********************************************************************

  Audacity: A Digital Audio Editor

  HtmlWindow.cpp

  Leland Lucius

*******************************************************************//**

\file HtmlWindow.cpp

  Implements HtmlWindow

*//*******************************************************************//**

\class HtmlWindow
\brief The widget to the left of a ToolBar that allows it to be dragged
around to new positions.

*//**********************************************************************/

#include "../Audacity.h"

#include <wx/defs.h>

#include "HtmlWindow.h"

////////////////////////////////////////////////////////////
/// Methods for HtmlWindow
////////////////////////////////////////////////////////////

//
// Constructor
//
HtmlWindow::HtmlWindow(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name)
: wxHtmlWindow(parent, id, pos, size, style, name)
{
#if wxUSE_ACCESSIBILITY
   SetAccessible( new HtmlWindowAx( this ) );
#endif
}

//
// Destructor
//
HtmlWindow::~HtmlWindow()
{
}

#if wxUSE_ACCESSIBILITY

HtmlWindowAx::HtmlWindowAx( wxWindow *window ):
   wxWindowAccessible( window )
{
}

HtmlWindowAx::~HtmlWindowAx()
{
}

// Retrieves the address of an IDispatch interface for the specified child.
// All objects must support this property.
wxAccStatus HtmlWindowAx::GetChild( int childId, wxAccessible** child )
{
   if( childId == wxACC_SELF )
   {
      *child = this;
   }
   else
   {
      *child = NULL;
   }

   return wxACC_OK;
}

// Gets the number of children.
wxAccStatus HtmlWindowAx::GetChildCount(int* childCount)
{
   *childCount = 0;

   return wxACC_OK;
}

// Gets the default action for this object (0) or > 0 (the action for
// a child).  Return wxACC_OK even if there is no action. actionName
// is the action, or the empty string if there is no action.  The
// retrieved string describes the action that is performed on an
// object, not what the object does as a result. For example, a
// toolbar button that prints a document has a default action of
// "Press" rather than "Prints the current document."
wxAccStatus HtmlWindowAx::GetDefaultAction(int WXUNUSED(childId), wxString* actionName)
{
   actionName->Clear();

   return wxACC_OK;
}

// Returns the description for this object or a child.
wxAccStatus HtmlWindowAx::GetDescription( int WXUNUSED(childId), wxString *description )
{
   description->Clear();

   return wxACC_OK;
}

// Gets the window with the keyboard focus.
// If childId is 0 and child is NULL, no object in
// this subhierarchy has the focus.
// If this object has the focus, child should be 'this'.
wxAccStatus HtmlWindowAx::GetFocus(int* childId, wxAccessible** child)
{
   *childId = 0;
   *child = this;

   return wxACC_OK;
}

// Returns help text for this object or a child, similar to tooltip text.
wxAccStatus HtmlWindowAx::GetHelpText( int WXUNUSED(childId), wxString *helpText )
{
   helpText->Clear();

   return wxACC_OK;
}

// Returns the keyboard shortcut for this object or child.
// Return e.g. ALT+K
wxAccStatus HtmlWindowAx::GetKeyboardShortcut( int WXUNUSED(childId), wxString *shortcut )
{
   shortcut->Clear();

   return wxACC_OK;
}

// Returns the rectangle for this object (id = 0) or a child element (id > 0).
// rect is in screen coordinates.
wxAccStatus HtmlWindowAx::GetLocation( wxRect& rect, int WXUNUSED(elementId) )
{
   HtmlWindow *hw = wxDynamicCast( GetWindow(), HtmlWindow );

   rect = hw->GetRect();
   rect.SetPosition( hw->GetParent()->ClientToScreen( rect.GetPosition() ) );

   return wxACC_OK;
}

// Gets the name of the specified object.
wxAccStatus HtmlWindowAx::GetName(int WXUNUSED(childId), wxString* name)
{
   HtmlWindow *hw = wxDynamicCast( GetWindow(), HtmlWindow );

   *name = hw->GetName();
   if( name->IsEmpty() )
   {
      *name = hw->GetLabel();
   }

   return wxACC_OK;
}

// Returns a role constant.
wxAccStatus HtmlWindowAx::GetRole(int WXUNUSED(childId), wxAccRole* role)
{
   *role = wxROLE_SYSTEM_STATICTEXT;

   return wxACC_OK;
}

// Gets a variant representing the selected children
// of this object.
// Acceptable values:
// - a null variant (IsNull() returns TRUE)
// - a list variant (GetType() == wxT("list"))
// - an integer representing the selected child element,
//   or 0 if this object is selected (GetType() == wxT("long"))
// - a "void*" pointer to a wxAccessible child object
wxAccStatus HtmlWindowAx::GetSelections( wxVariant * WXUNUSED(selections) )
{
   return wxACC_NOT_IMPLEMENTED;
}

// Returns a state constant.
wxAccStatus HtmlWindowAx::GetState(int WXUNUSED(childId), long* state)
{
   HtmlWindow *hw = wxDynamicCast( GetWindow(), HtmlWindow );

   *state = wxACC_STATE_SYSTEM_FOCUSABLE;

   *state |= ( hw == wxWindow::FindFocus() ? wxACC_STATE_SYSTEM_FOCUSED : 0 );

   return wxACC_OK;
}

// Returns a localized string representing the value for the object
// or child.
wxAccStatus HtmlWindowAx::GetValue(int WXUNUSED(childId), wxString* strValue)
{
   HtmlWindow *hw = wxDynamicCast( GetWindow(), HtmlWindow );

   *strValue = hw->ToText();

   return wxACC_OK;
}

#endif
