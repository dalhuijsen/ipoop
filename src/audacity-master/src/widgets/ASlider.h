/**********************************************************************

  Audacity: A Digital Audio Editor

  ASlider.h

  Dominic Mazzoni

  This class is a custom slider.

**********************************************************************/

#ifndef __AUDACITY_SLIDER__
#define __AUDACITY_SLIDER__

#include <wx/defs.h>
#include <wx/window.h>
#include <wx/dialog.h>
#include <wx/panel.h>

#if wxUSE_ACCESSIBILITY
#include <wx/access.h>
#endif

class wxBitmap;
class wxCursor;
class wxImage;
class wxSize;
class wxPoint;
class wxTextCtrl;
class wxButton;

class Ruler;

//
// Predefined slider types (mStyle)
//
#define FRAC_SLIDER 1    // 0.0...1.0
#define DB_SLIDER 2      // -36...36 dB
#define PAN_SLIDER 3     // -1.0...1.0
#define SPEED_SLIDER 4  // 0.01 ..3.0
#ifdef EXPERIMENTAL_MIDI_OUT
#define VEL_SLIDER 5    // -50..50
#endif

#define DB_MIN -36.0f
#define DB_MAX 36.0f
#define FRAC_MIN 0.0f
#define FRAC_MAX 1.0f
#define SPEED_MIN 0.01f
#define SPEED_MAX 3.0f
#define VEL_MIN -50.0f
#define VEL_MAX 50.0f

// Customizable slider only: If stepValue is STEP_CONTINUOUS,
// every value on the slider between minValue and maxValue
// will be possible
//
#define STEP_CONTINUOUS 0.0f

//
// Lightweight slider - i.e. a slider that doesn't appear in
// its own window, but rather draws itself inside an existing
// window (used inside Track Labels).  The ASlider class,
// which uses this class, is below.
//

class LWSlider
{
   friend class ASlider;
   friend class ASliderAx;

 public:

   // MM: Construct customizable slider
   LWSlider(wxWindow * parent,
            wxString name,
            const wxPoint &pos,
            const wxSize &size,
            float minValue,
            float maxValue,
            float stepValue,
            bool canUseShift,
            int style,
            bool heavyweight=false,
            bool popup=true,
            int orientation = wxHORIZONTAL); // wxHORIZONTAL or wxVERTICAL. wxVERTICAL is currently only for DB_SLIDER.

   // Construct predefined slider
   LWSlider(wxWindow * parent,
            wxString name,
            const wxPoint &pos,
            const wxSize &size,
            int style,
            bool heavyweight=false,
            bool popup=true,
            int orientation = wxHORIZONTAL); // wxHORIZONTAL or wxVERTICAL. wxVERTICAL is currently only for DB_SLIDER.

   void Init(wxWindow * parent,
             wxString name,
             const wxPoint &pos,
             const wxSize &size,
             float minValue,
             float maxValue,
             float stepValue,
             bool canUseShift,
             int style,
             bool heavyweight,
             bool popup,
             float speed,
             int orientation = wxHORIZONTAL); // wxHORIZONTAL or wxVERTICAL. wxVERTICAL is currently only for DB_SLIDER.

   virtual ~LWSlider();

   wxWindowID GetId();
   void SetId(wxWindowID id);

   void SetDefaultValue(float value);
   void SetDefaultShortcut(bool value);

   void GetScroll(float & line, float & page);
   void SetScroll(float line, float page);

   float Get(bool convert = true);
   void Set(float value);
#ifdef EXPERIMENTAL_MIDI_OUT
   void SetStyle(int style);
#endif
   void Increase(float steps);
   void Decrease(float steps);

   // If set to less than 1.0, moving the mouse one pixel will move
   // the slider by less than 1 unit
   void SetSpeed(float speed);

   void Move(const wxPoint &newpos);

   void OnPaint(wxDC &dc, bool selected);
   void OnSize(wxSizeEvent & event);
   void OnMouseEvent(wxMouseEvent & event);
   void OnKeyEvent(wxKeyEvent & event);
   void Refresh();

   void RecreateTipWin();

   bool ShowDialog();
   bool ShowDialog(wxPoint pos);

   void SetEnabled(bool enabled);
   bool GetEnabled();

   static void DeleteSharedTipPanel();

 private:

   void FormatPopWin();
   void SetPopWinPosition();
   void CreatePopWin();
   void Draw();

   bool DoShowDialog(wxPoint pos);

   void SendUpdate( float newValue );

   int ValueToPosition(float val);
   float DragPositionToValue(int fromPos, bool shiftDown);
   float ClickPositionToValue(int fromPos, bool shiftDown);

   wxWindow* GetToolTipParent() const;

   wxWindow *mParent;

   wxString maxTipLabel; //string with the max num of chars for tip

   int mStyle;
   int mOrientation; // wxHORIZONTAL or wxVERTICAL. wxVERTICAL is currently only for DB_SLIDER.

   bool mHW; // is it really heavyweight (in a window)
   bool mPopup; // should display dialog on double click

   int mLeft;
   int mTop;

   int mWidth;                  //In pixels
   int mHeight;                 //In pixels

   // for (mOrientation == wxHORIZONTAL)
   int mCenterY;

   int mLeftX;
   int mRightX;
   int mWidthX;

   // for (mOrientation == wxVERTICAL) //v Vertical PAN_SLIDER currently not handled, forced to horizontal.
   int mCenterX;

   int mTopY;
   int mBottomY; // low values at bottom
   int mHeightY;


   int mThumbWidth;             //In pixels
   int mThumbHeight;            //In pixels

   float mClickValue;
   int mClickPos; // position in x if (mOrientation == wxHORIZONTAL), else in y

   float mMinValue;
   float mMaxValue;
   float mStepValue;
   float mSpeed;

   float mScrollLine;
   float mScrollPage;

   float mCurrentValue;

   bool mDefaultShortcut;
   float mDefaultValue;

   bool mCanUseShift;

   wxWindowID mID;

   //since we only need to show one tip at a time, just share one instance over all sliders.
   static wxWindow* sharedTipPanel;

   Ruler* mpRuler;

   bool mIsDragging;

   wxBitmap *mBitmap;
   wxBitmap *mThumbBitmap;

   // AD: True if this object owns *mThumbBitmap (sometimes mThumbBitmap points
   // to an object we shouldn't delete) -- once we get theming totally right
   // this should go away
   bool mThumbBitmapAllocated;

   wxString mName;

   bool mEnabled;
};

class ASlider :public wxPanel
{
   friend class ASliderAx;

 public:
   ASlider( wxWindow * parent,
            wxWindowID id,
            wxString name,
            const wxPoint & pos,
            const wxSize & size,
            int style = FRAC_SLIDER,
            bool popup = true,
            bool canUseShift = true,
            float stepValue = STEP_CONTINUOUS,
            int orientation = wxHORIZONTAL);
   virtual ~ASlider();

   void GetScroll(float & line, float & page);
   void SetScroll(float line, float page);

   float Get( bool convert = true );
   void Set(float value);
#ifdef EXPERIMENTAL_MIDI_OUT
   void SetStyle(int style);
#endif

   void Increase(float steps);
   void Decrease(float steps);
   bool ShowDialog(wxPoint pos = wxPoint(-1, -1));

   void SetSpeed(float speed);

   void OnErase(wxEraseEvent & event);
   void OnPaint(wxPaintEvent & event);
   void OnSize(wxSizeEvent & event);
   void OnMouseEvent(wxMouseEvent & event);
   void OnCaptureLost(wxMouseCaptureLostEvent & event);
   void OnKeyEvent(wxKeyEvent &event);
   void OnSlider(wxCommandEvent &event);
   void OnSetFocus(wxFocusEvent & event);
   void OnKillFocus(wxFocusEvent & event);

   void RecreateTipWin();

   // Overrides of the wxWindow functions with the same semantics
   bool Enable(bool enable = true);
   bool IsEnabled() const;

 private:
   LWSlider *mLWSlider;
   bool mSliderIsFocused;

 protected:
   int mStyle;

 public:
    DECLARE_EVENT_TABLE()
};



#define SLIDER_DIALOG_TEXTCTRL 100


// This is a modal dialog that contains an ASlider
// and a text-entry box which can be used to set the
// value of a slider.
class SliderDialog: public wxDialog
{
 public:
   SliderDialog(wxWindow * parent, wxWindowID id,
                const wxString & title,
                wxPoint position,
                wxSize size,
                int style,
                float value,
                float line,
                float page);
   ~SliderDialog();

   float Get();

 private:
   bool TransferDataToWindow();
   bool TransferDataFromWindow();

   void OnSlider(wxCommandEvent &event);

   ASlider * mSlider;
   wxTextCtrl * mTextCtrl;
   int mStyle;

 public:
   DECLARE_EVENT_TABLE()
};


#if wxUSE_ACCESSIBILITY

class ASliderAx: public wxWindowAccessible
{
public:
   ASliderAx(wxWindow * window);

   virtual ~ ASliderAx();

   // Retrieves the address of an IDispatch interface for the specified child.
   // All objects must support this property.
   virtual wxAccStatus GetChild( int childId, wxAccessible** child );

   // Gets the number of children.
   virtual wxAccStatus GetChildCount(int* childCount);

   // Gets the default action for this object (0) or > 0 (the action for a child).
   // Return wxACC_OK even if there is no action. actionName is the action, or the empty
   // string if there is no action.
   // The retrieved string describes the action that is performed on an object,
   // not what the object does as a result. For example, a toolbar button that prints
   // a document has a default action of "Press" rather than "Prints the current document."
   virtual wxAccStatus GetDefaultAction( int childId, wxString *actionName );

   // Returns the description for this object or a child.
   virtual wxAccStatus GetDescription( int childId, wxString *description );

   // Gets the window with the keyboard focus.
   // If childId is 0 and child is NULL, no object in
   // this subhierarchy has the focus.
   // If this object has the focus, child should be 'this'.
   virtual wxAccStatus GetFocus( int *childId, wxAccessible **child );

   // Returns help text for this object or a child, similar to tooltip text.
   virtual wxAccStatus GetHelpText( int childId, wxString *helpText );

   // Returns the keyboard shortcut for this object or child.
   // Return e.g. ALT+K
   virtual wxAccStatus GetKeyboardShortcut( int childId, wxString *shortcut );

   // Returns the rectangle for this object (id = 0) or a child element (id > 0).
   // rect is in screen coordinates.
   virtual wxAccStatus GetLocation( wxRect& rect, int elementId );

   // Gets the name of the specified object.
   virtual wxAccStatus GetName( int childId, wxString *name );

   // Returns a role constant.
   virtual wxAccStatus GetRole( int childId, wxAccRole *role );

   // Gets a variant representing the selected children
   // of this object.
   // Acceptable values:
   // - a null variant (IsNull() returns TRUE)
   // - a list variant (GetType() == wxT("list"))
   // - an integer representing the selected child element,
   //   or 0 if this object is selected (GetType() == wxT("long"))
   // - a "void*" pointer to a wxAccessible child object
   virtual wxAccStatus GetSelections( wxVariant *selections );

   // Returns a state constant.
   virtual wxAccStatus GetState(int childId, long* state);

   // Returns a localized string representing the value for the object
   // or child.
   virtual wxAccStatus GetValue(int childId, wxString* strValue);

};

#endif // wxUSE_ACCESSIBILITY

#endif
