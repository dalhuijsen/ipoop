/**********************************************************************

  Audacity: A Digital Audio Editor

  Meter.cpp

  Dominic Mazzoni
  Vaughan Johnson

  2004.06.25 refresh rate limited to 30mS, by ChackoN

*******************************************************************//**

\class Meter
\brief VU Meter, for displaying recording/playback level

  This is a bunch of common code that can display many different
  forms of VU meters and other displays.

  But note that a lot of later code here assumes these are
  MeterToolBar meters, e.g., Meter::StartMonitoring,
  so these are not as generic/common as originally intended.

*//****************************************************************//**

\class MeterBar
\brief A struct used by Meter to hold the position of one bar.

*//****************************************************************//**

\class MeterUpdateMsg
\brief Message used to update the Meter

*//****************************************************************//**

\class MeterUpdateQueue
\brief Queue of MeterUpdateMsg used to feed the Meter.

*//******************************************************************/

#include "../Audacity.h"
#include "../AudacityApp.h"

#include <wx/defs.h>
#include <wx/dialog.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/intl.h>
#include <wx/menu.h>
#include <wx/settings.h>
#include <wx/textdlg.h>
#include <wx/numdlg.h>
#include <wx/radiobut.h>
#include <wx/tooltip.h>
#include <wx/msgdlg.h>

#include <math.h>

#include "Meter.h"

#include "../AudioIO.h"
#include "../AColor.h"
#include "../ImageManipulation.h"
#include "../Project.h"
#include "../toolbars/MeterToolBar.h"
#include "../toolbars/ControlToolBar.h"
#include "../Prefs.h"

#include "../Theme.h"
#include "../AllThemeResources.h"
#include "../Experimental.h"
#include "../widgets/valnum.h"

/* Updates to the meter are passed accross via meter updates, each contained in
 * a MeterUpdateMsg object */
wxString MeterUpdateMsg::toString()
{
wxString output;  // somewhere to build up a string in
output = wxString::Format(wxT("Meter update msg: %i channels, %i samples\n"), \
      kMaxMeterBars, numFrames);
for (int i = 0; i<kMaxMeterBars; i++)
   {  // for each channel of the meters
   output += wxString::Format(wxT("%f peak, %f rms "), peak[i], rms[i]);
   if (clipping[i])
      output += wxString::Format(wxT("clipped "));
   else
      output += wxString::Format(wxT("no clip "));
   output += wxString::Format(wxT("%i head, %i tail\n"), headPeakCount[i], tailPeakCount[i]);
   }
return output;
}

wxString MeterUpdateMsg::toStringIfClipped()
{
   for (int i = 0; i<kMaxMeterBars; i++)
   {
      if (clipping[i] || (headPeakCount[i] > 0) || (tailPeakCount[i] > 0))
         return toString();
   }
   return wxT("");
}

//
// The Meter passes itself messages via this queue so that it can
// communicate between the audio thread and the GUI thread.
// This class is as simple as possible in order to be thread-safe
// without needing mutexes.
//

MeterUpdateQueue::MeterUpdateQueue(int maxLen):
   mBufferSize(maxLen)
{
   mBuffer = new MeterUpdateMsg[mBufferSize];
   Clear();
}

// destructor
MeterUpdateQueue::~MeterUpdateQueue()
{
   delete[] mBuffer;
}

void MeterUpdateQueue::Clear()
{
   mStart = 0;
   mEnd = 0;
}

// Add a message to the end of the queue.  Return false if the
// queue was full.
bool MeterUpdateQueue::Put(MeterUpdateMsg &msg)
{
   int len = (mEnd + mBufferSize - mStart) % mBufferSize;

   // Never completely fill the queue, because then the
   // state is ambiguous (mStart==mEnd)
   if (len >= mBufferSize-1)
      return false;

   //wxLogDebug(wxT("Put: %s"), msg.toString().c_str());

   mBuffer[mEnd] = msg;
   mEnd = (mEnd+1)%mBufferSize;

   return true;
}

// Get the next message from the start of the queue.
// Return false if the queue was empty.
bool MeterUpdateQueue::Get(MeterUpdateMsg &msg)
{
   int len = (mEnd + mBufferSize - mStart) % mBufferSize;

   if (len == 0)
      return false;

   msg = mBuffer[mStart];
   mStart = (mStart+1)%mBufferSize;

   return true;
}

//
// Meter class
//

#include "../../images/SpeakerMenu.xpm"
#include "../../images/MicMenu.xpm"

// How many pixels between items?
const static int gap = 2;

// Event used to notify all meters of preference changes
DEFINE_EVENT_TYPE(EVT_METER_PREFERENCES_CHANGED);

const static wxChar *PrefStyles[] =
{
   wxT("AutomaticStereo"),
   wxT("HorizontalStereo"),
   wxT("VerticalStereo")
};

enum {
   OnMeterUpdateID = 6000,
   OnMonitorID,
   OnPreferencesID
};

BEGIN_EVENT_TABLE(Meter, wxPanel)
   EVT_TIMER(OnMeterUpdateID, Meter::OnMeterUpdate)
   EVT_MOUSE_EVENTS(Meter::OnMouse)
   EVT_CONTEXT_MENU(Meter::OnContext)
   EVT_KEY_DOWN(Meter::OnKeyDown)
   EVT_KEY_UP(Meter::OnKeyUp)
   EVT_SET_FOCUS(Meter::OnSetFocus)
   EVT_KILL_FOCUS(Meter::OnKillFocus)
   EVT_ERASE_BACKGROUND(Meter::OnErase)
   EVT_PAINT(Meter::OnPaint)
   EVT_SIZE(Meter::OnSize)
   EVT_MENU(OnMonitorID, Meter::OnMonitor)
   EVT_MENU(OnPreferencesID, Meter::OnPreferences)
END_EVENT_TABLE()

IMPLEMENT_CLASS(Meter, wxPanel)

Meter::Meter(AudacityProject *project,
             wxWindow* parent, wxWindowID id,
             bool isInput,
             const wxPoint& pos /*= wxDefaultPosition*/,
             const wxSize& size /*= wxDefaultSize*/,
             Style style /*= HorizontalStereo*/,
             float fDecayRate /*= 60.0f*/)
: wxPanel(parent, id, pos, size, wxTAB_TRAVERSAL | wxNO_BORDER | wxWANTS_CHARS),
   mProject(project),
   mQueue(1024),
   mWidth(size.x),
   mHeight(size.y),
   mIsInput(isInput),
   mDesiredStyle(style),
   mGradient(true),
   mDB(true),
   mDBRange(ENV_DB_RANGE),
   mDecay(true),
   mDecayRate(fDecayRate),
   mClip(true),
   mNumPeakSamplesToClip(3),
   mPeakHoldDuration(3),
   mT(0),
   mRate(0),
   mMonitoring(false),
   mActive(false),
   mNumBars(0),
   mLayoutValid(false),
   mBitmap(NULL),
   mIcon(NULL),
   mAccSilent(false)
{
   mStyle = mDesiredStyle;

   mIsFocused = false;

#if wxUSE_ACCESSIBILITY
   SetAccessible(new MeterAx(this));
#endif

   UpdatePrefs();

   wxColour backgroundColour =
      wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
   mBkgndBrush = wxBrush(backgroundColour, wxSOLID);

   mPeakPeakPen = wxPen(theTheme.Colour( clrMeterPeak),        1, wxSOLID);
   mDisabledPen = wxPen(theTheme.Colour( clrMeterDisabledPen), 1, wxSOLID);

   // Register for our preference update event
   wxTheApp->Connect(EVT_METER_PREFERENCES_CHANGED,
                     wxCommandEventHandler(Meter::OnMeterPrefsUpdated),
                     NULL,
                     this);

   if (mIsInput) {
      wxTheApp->Connect(EVT_AUDIOIO_MONITOR,
                        wxCommandEventHandler(Meter::OnAudioIOStatus),
                        NULL,
                        this);
      wxTheApp->Connect(EVT_AUDIOIO_CAPTURE,
                        wxCommandEventHandler(Meter::OnAudioIOStatus),
                        NULL,
                        this);

      mPen       = wxPen(   theTheme.Colour( clrMeterInputPen         ), 1, wxSOLID);
      mBrush     = wxBrush( theTheme.Colour( clrMeterInputBrush       ), wxSOLID);
      mRMSBrush  = wxBrush( theTheme.Colour( clrMeterInputRMSBrush    ), wxSOLID);
      mClipBrush = wxBrush( theTheme.Colour( clrMeterInputClipBrush   ), wxSOLID);
//      mLightPen  = wxPen(   theTheme.Colour( clrMeterInputLightPen    ), 1, wxSOLID);
//      mDarkPen   = wxPen(   theTheme.Colour( clrMeterInputDarkPen     ), 1, wxSOLID);
   }
   else {
      // Register for AudioIO events
      wxTheApp->Connect(EVT_AUDIOIO_PLAYBACK,
                        wxCommandEventHandler(Meter::OnAudioIOStatus),
                        NULL,
                        this);

      mPen       = wxPen(   theTheme.Colour( clrMeterOutputPen        ), 1, wxSOLID);
      mBrush     = wxBrush( theTheme.Colour( clrMeterOutputBrush      ), wxSOLID);
      mRMSBrush  = wxBrush( theTheme.Colour( clrMeterOutputRMSBrush   ), wxSOLID);
      mClipBrush = wxBrush( theTheme.Colour( clrMeterOutputClipBrush  ), wxSOLID);
//      mLightPen  = wxPen(   theTheme.Colour( clrMeterOutputLightPen   ), 1, wxSOLID);
//      mDarkPen   = wxPen(   theTheme.Colour( clrMeterOutputDarkPen    ), 1, wxSOLID);
   }

//   mDisabledBkgndBrush = wxBrush(theTheme.Colour( clrMeterDisabledBrush), wxSOLID);
   // No longer show a difference in the background colour when not monitoring.
   // We have the tip instead.
   mDisabledBkgndBrush = mBkgndBrush;
   
   // MixerTrackCluster style has no menu, so disallows SetStyle, so never needs icon.
   if (mStyle != MixerTrackCluster)
   {
      if(mIsInput)
      {
         mIcon = new wxBitmap(MicMenuNarrow_xpm);
      }
      else
      {
         mIcon = new wxBitmap(SpeakerMenuNarrow_xpm);
      }
   }

   mRuler.SetFonts(GetFont(), GetFont(), GetFont());
   mRuler.SetFlip(true);
   mRuler.SetLabelEdges(true);

   mTimer.SetOwner(this, OnMeterUpdateID);
   // TODO: Yikes.  Hard coded sample rate.
   // JKC: I've looked at this, and it's benignish.  It just means that the meter
   // balistics are right for 44KHz and a bit more frisky than they should be
   // for higher sample rates.
   Reset(44100.0, true);
}

void Meter::Clear()
{
   mQueue.Clear();
}

Meter::~Meter()
{
   if (mIsInput)
   {
      // Unregister for AudioIO events
      wxTheApp->Disconnect(EVT_AUDIOIO_MONITOR,
                           wxCommandEventHandler(Meter::OnAudioIOStatus),
                           NULL,
                           this);
      wxTheApp->Disconnect(EVT_AUDIOIO_CAPTURE,
                           wxCommandEventHandler(Meter::OnAudioIOStatus),
                           NULL,
                           this);
   }
   else
   {
      wxTheApp->Disconnect(EVT_AUDIOIO_PLAYBACK,
                           wxCommandEventHandler(Meter::OnAudioIOStatus),
                           NULL,
                           this);
   }

   // Unregister for our preference update event
   wxTheApp->Disconnect(EVT_METER_PREFERENCES_CHANGED,
                        wxCommandEventHandler(Meter::OnMeterPrefsUpdated),
                        NULL,
                        this);

   // LLL:  This prevents a crash during termination if monitoring
   //       is active.
   if (gAudioIO->IsMonitoring())
      gAudioIO->StopStream();
   if (mIcon)
      delete mIcon;
   if (mBitmap)
      delete mBitmap;
}

void Meter::UpdatePrefs()
{
   mDBRange = gPrefs->Read(wxT("/GUI/EnvdBRange"), ENV_DB_RANGE);

   mMeterRefreshRate = gPrefs->Read(Key(wxT("RefreshRate")), 30);
   mGradient = gPrefs->Read(Key(wxT("Bars")), wxT("Gradient")) == wxT("Gradient");
   mDB = gPrefs->Read(Key(wxT("Type")), wxT("dB")) == wxT("dB");
   mMeterDisabled = gPrefs->Read(Key(wxT("Disabled")), (long)0);

   if (mDesiredStyle != MixerTrackCluster)
   {
      wxString style = gPrefs->Read(Key(wxT("Style")));
      if (style == wxT("AutomaticStereo"))
      {
         mDesiredStyle = AutomaticStereo;
      }
      else if (style == wxT("HorizontalStereo"))
      {
         mDesiredStyle = HorizontalStereo;
      }
      else if (style == wxT("VerticalStereo"))
      {
         mDesiredStyle = VerticalStereo;
      }
      else
      {
         mDesiredStyle = AutomaticStereo;
      }
   }

   // Set the desired orientation (resets ruler orientation)
   SetActiveStyle(mDesiredStyle);

   // Reset to ensure new size is retrieved when language changes
   mLeftSize = wxSize(0, 0);
   mRightSize = wxSize(0, 0);

   Reset(mRate, false);

   mLayoutValid = false;
}

void Meter::OnErase(wxEraseEvent & WXUNUSED(event))
{
   // Ignore it to prevent flashing
}

void Meter::OnPaint(wxPaintEvent & WXUNUSED(event))
{
   wxDC *paintDC = wxAutoBufferedPaintDCFactory(this);
   wxDC & destDC = *paintDC;

   if (mLayoutValid == false)
   {
      if (mBitmap)
      {
         delete mBitmap;
      }
   
      // Create a new one using current size and select into the DC
      mBitmap = new wxBitmap(mWidth, mHeight);
      wxMemoryDC dc;
      dc.SelectObject(*mBitmap);
   
      // Go calculate all of the layout metrics
      HandleLayout(dc);
   
      // Start with a clean background
      // LLL:  Should research USE_AQUA_THEME usefulness...
#ifndef USE_AQUA_THEME
#ifdef EXPERIMENTAL_THEMING
      if( !mMeterDisabled )
      {
         mBkgndBrush.SetColour( GetParent()->GetBackgroundColour() );
      }
#endif
   
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.SetBrush(mBkgndBrush);
      dc.DrawRectangle(0, 0, mWidth, mHeight);
#endif
   
      // MixerTrackCluster style has no icon or L/R labels
      if (mStyle != MixerTrackCluster)
      {
         dc.DrawBitmap(*mIcon, mIconRect.GetPosition(), true);
         dc.SetFont(GetFont());
         dc.DrawText(mLeftText, mLeftTextPos.x, mLeftTextPos.y);
         dc.DrawText(mRightText, mRightTextPos.x, mRightTextPos.y);
      }
   
      // Setup the colors for the 3 sections of the meter bars
      wxColor green(117, 215, 112);
      wxColor yellow(255, 255, 0);
      wxColor red(255, 0, 0);
   
      // Draw the meter bars at maximum levels
      for (int i = 0; i < mNumBars; i++)
      {
         // Give it a recessed look
         AColor::Bevel(dc, false, mBar[i].b);
   
         // Draw the clip indicator bevel
         if (mClip)
         {
            AColor::Bevel(dc, false, mBar[i].rClip);
         }
   
         // Cache bar rect
         wxRect r = mBar[i].r;
   
         if (mGradient)
         {
            // Calculate the size of the two gradiant segments of the meter
            double gradw;
            double gradh;
            if (mDB)
            {
               gradw = (double) r.GetWidth() / mDBRange * 6.0;
               gradh = (double) r.GetHeight() / mDBRange * 6.0;
            }
            else
            {
               gradw = (double) r.GetWidth() / 100 * 25;
               gradh = (double) r.GetHeight() / 100 * 25;
            }
   
            if (mBar[i].vert)
            {
               // Draw the "critical" segment (starts at top of meter and works down)
               r.SetHeight(gradh);
               dc.GradientFillLinear(r, red, yellow, wxSOUTH);
   
               // Draw the "warning" segment
               r.SetTop(r.GetBottom());
               dc.GradientFillLinear(r, yellow, green, wxSOUTH);
   
               // Draw the "safe" segment
               r.SetTop(r.GetBottom());
               r.SetBottom(mBar[i].r.GetBottom());
               dc.SetPen(*wxTRANSPARENT_PEN);
               dc.SetBrush(green);
               dc.DrawRectangle(r);
            }
            else
            {
               // Draw the "safe" segment
               r.SetWidth(r.GetWidth() - (int) (gradw + gradw + 0.5));
               dc.SetPen(*wxTRANSPARENT_PEN);
               dc.SetBrush(green);
               dc.DrawRectangle(r);
   
               // Draw the "warning"  segment
               r.SetLeft(r.GetRight() + 1);
               r.SetWidth(floor(gradw));
               dc.GradientFillLinear(r, green, yellow);
   
               // Draw the "critical" segment
               r.SetLeft(r.GetRight() + 1);
               r.SetRight(mBar[i].r.GetRight());
               dc.GradientFillLinear(r, yellow, red);
            }
#ifdef EXPERIMENTAL_METER_LED_STYLE
            if (!mBar[i].vert)
            {
               wxRect r = mBar[i].r;
               wxPen BackgroundPen;
               BackgroundPen.SetColour( wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE) );
               dc.SetPen( BackgroundPen );
               int i;
               for(i=0;i<r.width;i++)
               {
                  // 2 pixel spacing between the LEDs
                  if( (i%7)<2 ){
                     dc.DrawLine( i+r.x, r.y, i+r.x, r.y+r.height );
                  } else {
                     // The LEDs have triangular ends.  
                     // This code shapes the ends.
                     int j = abs( (i%7)-4);
                     dc.DrawLine( i+r.x, r.y, i+r.x, r.y+j +1);
                     dc.DrawLine( i+r.x, r.y+r.height-j, i+r.x, r.y+r.height );
                  }
               }
            }
#endif
         }
      }
   
      // Draw the ruler
      mRuler.Draw(dc);   

      // Bitmap created...unselect
      dc.SelectObject(wxNullBitmap);
   }

   // Copy predrawn bitmap to the dest DC
   destDC.DrawBitmap(*mBitmap, 0, 0);

   // Go draw the meter bars, Left & Right channels using current levels
   for (int i = 0; i < mNumBars; i++)
   {
      DrawMeterBar(destDC, &mBar[i]);
   }

   // We can have numbers over the bars, in which case we have to draw them each time.
   if (mStyle == HorizontalStereoCompact || mStyle == VerticalStereoCompact)
   {
      mRuler.Draw(destDC);
   }

   // Let the user know they can click to start monitoring
   if( mIsInput && !mActive )
   {
      destDC.SetFont( GetFont() );
      wxArrayString texts;

      texts.Add( _("Click to Start Monitoring") );
      texts.Add( _("Click for Monitoring") );
      texts.Add( _("Click to Start") );
      texts.Add( _("Click") );

      for( size_t i = 0, cnt = texts.GetCount(); i < cnt; i++ )
      {
         wxString Text = wxT(" ") + texts[i] + wxT(" ");
         wxSize Siz = destDC.GetTextExtent( Text );
         Siz.SetWidth( Siz.GetWidth() + gap );
         Siz.SetHeight( Siz.GetHeight() + gap );

         if( mBar[0].vert)
         {
            if( Siz.GetWidth() < mBar[0].r.GetHeight() )
            {
               wxRect r( mBar[1].b.GetLeft() - (int) (Siz.GetHeight() / 2.0) + 0.5,
                           mBar[0].r.GetTop() + (int) ((mBar[0].r.GetHeight() - Siz.GetWidth()) / 2.0) + 0.5,
                           Siz.GetHeight(),
                           Siz.GetWidth() );

               destDC.SetBrush( *wxWHITE_BRUSH );
               destDC.SetPen( *wxGREY_PEN );
               destDC.DrawRectangle( r );
               destDC.SetBackgroundMode( wxTRANSPARENT );
               r.SetTop( r.GetBottom() + (gap / 2) );
               destDC.DrawRotatedText( Text, r.GetPosition(), 90 );
               break;
            }
         }
         else
         {
            if( Siz.GetWidth() < mBar[0].r.GetWidth() )
            {
               wxRect r( mBar[0].r.GetLeft() + (int) ((mBar[0].r.GetWidth() - Siz.GetWidth()) / 2.0) + 0.5,
                         mBar[1].b.GetTop() - (int) (Siz.GetHeight() / 2.0) + 0.5,
                         Siz.GetWidth(),
                         Siz.GetHeight() );

               destDC.SetBrush( *wxWHITE_BRUSH );
               destDC.SetPen( *wxGREY_PEN );
               destDC.DrawRectangle( r );
               destDC.SetBackgroundMode( wxTRANSPARENT );
               r.SetLeft( r.GetLeft() + (gap / 2) );
               r.SetTop( r.GetTop() + (gap / 2));
               destDC.DrawText( Text, r.GetPosition() );
               break;
            }
         }
      }
   }

#if defined(__WXMSW__) || defined(__WXGTK__)
   if (mIsFocused)
   {
      wxRect r = mIconRect;
      AColor::DrawFocus(destDC, r.Inflate(1, 1));
   }
#endif

   delete paintDC;
}

void Meter::OnSize(wxSizeEvent & WXUNUSED(event))
{
   GetClientSize(&mWidth, &mHeight);

   mLayoutValid = false;
}

void Meter::OnMouse(wxMouseEvent &evt)
{
   if (mStyle == MixerTrackCluster) // MixerTrackCluster style has no menu.
      return;

  #if wxUSE_TOOLTIPS // Not available in wxX11
   if (evt.Leaving()){
      GetActiveProject()->TP_DisplayStatusMessage(wxT(""));
   }
   else if (evt.Entering()) {
      // Display the tooltip in the status bar
      wxToolTip * pTip = this->GetToolTip();
      if( pTip ) {
         wxString tipText = pTip->GetTip();
         GetActiveProject()->TP_DisplayStatusMessage(tipText);
      }
   }
  #endif

   if (evt.RightDown() ||
       (evt.ButtonDown() && mIconRect.Contains(evt.m_x, evt.m_y)))
   {
      wxMenu *menu = new wxMenu();
      // Note: these should be kept in the same order as the enum
      if (mIsInput) {
         wxMenuItem *mi;
         if (mMonitoring)
            mi = menu->Append(OnMonitorID, _("Stop Monitoring"));
         else
            mi = menu->Append(OnMonitorID, _("Start Monitoring"));
         mi->Enable(!mActive || mMonitoring);
      }

      menu->Append(OnPreferencesID, _("Preferences..."));

      if (evt.RightDown()) {
         ShowMenu(evt.GetPosition());
      }
      else {
         ShowMenu(wxPoint(mIconRect.x + 1, mIconRect.y + mIconRect.height + 1));
      }

      delete menu;
   }
   else if (evt.LeftDown()) {
      if (mIsInput) {
         if (mActive && !mMonitoring) {
            Reset(mRate, true);
         }
         else {
            StartMonitoring();
         }
      }
      else {
         Reset(mRate, true);
      }
   }
}

void Meter::OnContext(wxContextMenuEvent &evt)
{
#if defined(__WXMSW__)
   if (mHadKeyDown)
#endif
   if (mStyle != MixerTrackCluster) // MixerTrackCluster style has no menu.
   {
      ShowMenu(wxPoint(mIconRect.x + 1, mIconRect.y + mIconRect.height + 1));
   }
   else
   {
      evt.Skip();
   }

#if defined(__WXMSW__)
   mHadKeyDown = false;
#endif
}

void Meter::OnKeyDown(wxKeyEvent &evt)
{
   switch (evt.GetKeyCode())
   {
   // These are handled in the OnKeyUp handler because, on Windows at least, the
   // key up event will be passed on to the menu if we show it here.  This causes
   // the default sound to be heard if assigned.
   //
   // But, again on Windows, when the user selects a menu item, it is handled by
   // the menu and the key up event is passed along to our OnKeyUp() handler, so
   // we have to ignore it, otherwise we'd just show the menu again.
   case WXK_RETURN:
   case WXK_NUMPAD_ENTER:
   case WXK_WINDOWS_MENU:
   case WXK_MENU:
#if defined(__WXMSW__)
      mHadKeyDown = true;
#endif
      break;
   case WXK_RIGHT:
      Navigate(wxNavigationKeyEvent::IsForward);
      break;
   case WXK_LEFT:
      Navigate(wxNavigationKeyEvent::IsBackward);
      break;
   case WXK_TAB:
      if (evt.ShiftDown())
         Navigate(wxNavigationKeyEvent::IsBackward);
      else
         Navigate(wxNavigationKeyEvent::IsForward);
      break;
   default:
      evt.Skip();
      break;
   }
}

void Meter::OnKeyUp(wxKeyEvent &evt)
{
   switch (evt.GetKeyCode())
   {
   case WXK_RETURN:
   case WXK_NUMPAD_ENTER:
#if defined(__WXMSW__)
      if (mHadKeyDown)
#endif
      if (mStyle != MixerTrackCluster) // MixerTrackCluster style has no menu.
      {
         ShowMenu(wxPoint(mIconRect.x + 1, mIconRect.y + mIconRect.height + 1));
      }
#if defined(__WXMSW__)
      mHadKeyDown = false;
#endif
      break;
   default:
      evt.Skip();
      break;
   }
}

void Meter::OnSetFocus(wxFocusEvent & WXUNUSED(evt))
{
   mIsFocused = true;
   Refresh(false);
}

void Meter::OnKillFocus(wxFocusEvent & WXUNUSED(evt))
{
   mIsFocused = false;
   Refresh(false);
}

void Meter::SetStyle(Style newStyle)
{
   if (mStyle != newStyle && mDesiredStyle == AutomaticStereo)
   {
      SetActiveStyle(newStyle);

      mLayoutValid = false;

      Refresh(false);
   }
}

void Meter::Reset(double sampleRate, bool resetClipping)
{
   mT = 0;
   mRate = sampleRate;
   for (int j = 0; j < kMaxMeterBars; j++)
   {
      ResetBar(&mBar[j], resetClipping);
   }

   // wxTimers seem to be a little unreliable - sometimes they stop for
   // no good reason, so this "primes" it every now and then...
   mTimer.Stop();

   // While it's stopped, empty the queue
   mQueue.Clear();

   mLayoutValid = false;

   mTimer.Start(1000 / mMeterRefreshRate);

   Refresh(false);
}

static float floatMax(float a, float b)
{
   return a>b? a: b;
}

static int intmin(int a, int b)
{
   return a<b? a: b;
}

static int intmax(int a, int b)
{
   return a>b? a: b;
}

static float ClipZeroToOne(float z)
{
   if (z > 1.0)
      return 1.0;
   else if (z < 0.0)
      return 0.0;
   else
      return z;
}

static float ToDB(float v, float range)
{
   double db;
   if (v > 0)
      db = 20 * log10(fabs(v));
   else
      db = -999;
   return ClipZeroToOne((db + range) / range);
}

void Meter::UpdateDisplay(int numChannels, int numFrames, float *sampleData)
{
   int i, j;
   float *sptr = sampleData;
   int num = intmin(numChannels, mNumBars);
   MeterUpdateMsg msg;

   memset(&msg, 0, sizeof(msg));
   msg.numFrames = numFrames;

   for(i=0; i<numFrames; i++) {
      for(j=0; j<num; j++) {
         msg.peak[j] = floatMax(msg.peak[j], fabs(sptr[j]));
         msg.rms[j] += sptr[j]*sptr[j];

         // In addition to looking for mNumPeakSamplesToClip peaked
         // samples in a row, also send the number of peaked samples
         // at the head and tail, in case there's a run of peaked samples
         // that crosses block boundaries
         if (fabs(sptr[j])>=MAX_AUDIO) {
            if (msg.headPeakCount[j]==i)
               msg.headPeakCount[j]++;
            msg.tailPeakCount[j]++;
            if (msg.tailPeakCount[j] > mNumPeakSamplesToClip)
               msg.clipping[j] = true;
         }
         else
            msg.tailPeakCount[j] = 0;
      }
      sptr += numChannels;
   }
   for(j=0; j<mNumBars; j++)
      msg.rms[j] = sqrt(msg.rms[j]/numFrames);

   mQueue.Put(msg);
}

// Vaughan, 2010-11-29: This not currently used. See comments in MixerTrackCluster::UpdateMeter().
//void Meter::UpdateDisplay(int numChannels, int numFrames,
//                           // Need to make these double-indexed arrays if we handle more than 2 channels.
//                           float* maxLeft, float* rmsLeft,
//                           float* maxRight, float* rmsRight,
//                           const sampleCount kSampleCount)
//{
//   int i, j;
//   int num = intmin(numChannels, mNumBars);
//   MeterUpdateMsg msg;
//
//   msg.numFrames = kSampleCount;
//   for(j=0; j<mNumBars; j++) {
//      msg.peak[j] = 0.0;
//      msg.rms[j] = 0.0;
//      msg.clipping[j] = false;
//      msg.headPeakCount[j] = 0;
//      msg.tailPeakCount[j] = 0;
//   }
//
//   for(i=0; i<numFrames; i++) {
//      for(j=0; j<num; j++) {
//         msg.peak[j] = floatMax(msg.peak[j], ((j == 0) ? maxLeft[i] : maxRight[i]));
//         msg.rms[j] = floatMax(msg.rms[j], ((j == 0) ? rmsLeft[i] : rmsRight[i]));
//
//         // In addition to looking for mNumPeakSamplesToClip peaked
//         // samples in a row, also send the number of peaked samples
//         // at the head and tail, in case there's a run
//         // of peaked samples that crosses block boundaries.
//         if (fabs((j == 0) ? maxLeft[i] : maxRight[i]) >= MAX_AUDIO)
//         {
//            if (msg.headPeakCount[j]==i)
//               msg.headPeakCount[j]++;
//            msg.tailPeakCount[j]++;
//            if (msg.tailPeakCount[j] > mNumPeakSamplesToClip)
//               msg.clipping[j] = true;
//         }
//         else
//            msg.tailPeakCount[j] = 0;
//      }
//   }
//
//   mQueue.Put(msg);
//}

void Meter::OnMeterUpdate(wxTimerEvent & WXUNUSED(event))
{
   MeterUpdateMsg msg;
   int numChanges = 0;
#ifdef AUTOMATED_INPUT_LEVEL_ADJUSTMENT
   double maxPeak = 0.0;
   bool discarded = false;
#endif

   // We shouldn't receive any events if the meter is disabled, but clear it to be safe
   if (mMeterDisabled) {
      mQueue.Clear();
      return;
   }

   // There may have been several update messages since the last
   // time we got to this function.  Catch up to real-time by
   // popping them off until there are none left.  It is necessary
   // to process all of them, otherwise we won't handle peaks and
   // peak-hold bars correctly.
   while(mQueue.Get(msg)) {
      numChanges++;
      double deltaT = msg.numFrames / mRate;
      int j;

      mT += deltaT;
      for(j=0; j<mNumBars; j++) {
         mBar[j].isclipping = false;

         //
         if (mDB) {
            msg.peak[j] = ToDB(msg.peak[j], mDBRange);
            msg.rms[j] = ToDB(msg.rms[j], mDBRange);
         }

         if (mDecay) {
            if (mDB) {
               float decayAmount = mDecayRate * deltaT / mDBRange;
               mBar[j].peak = floatMax(msg.peak[j],
                                       mBar[j].peak - decayAmount);
            }
            else {
               double decayAmount = mDecayRate * deltaT;
               double decayFactor = pow(10.0, -decayAmount/20);
               mBar[j].peak = floatMax(msg.peak[j],
                                       mBar[j].peak * decayFactor);
            }
         }
         else
            mBar[j].peak = msg.peak[j];

         // This smooths out the RMS signal
         float smooth = pow(0.9, (double)msg.numFrames/1024.0);
         mBar[j].rms = mBar[j].rms * smooth + msg.rms[j] * (1.0 - smooth);

         if (mT - mBar[j].peakHoldTime > mPeakHoldDuration ||
             mBar[j].peak > mBar[j].peakHold) {
            mBar[j].peakHold = mBar[j].peak;
            mBar[j].peakHoldTime = mT;
         }

         if (mBar[j].peak > mBar[j].peakPeakHold )
            mBar[j].peakPeakHold = mBar[j].peak;

         if (msg.clipping[j] ||
             mBar[j].tailPeakCount+msg.headPeakCount[j] >=
             mNumPeakSamplesToClip){
            mBar[j].clipping = true;
            mBar[j].isclipping = true;
         }

         mBar[j].tailPeakCount = msg.tailPeakCount[j];
#ifdef AUTOMATED_INPUT_LEVEL_ADJUSTMENT
         if (mT > gAudioIO->AILAGetLastDecisionTime()) {
            discarded = false;
            maxPeak = msg.peak[j] > maxPeak ? msg.peak[j] : maxPeak;
            printf("%f@%f ", msg.peak[j], mT);
         }
         else {
            discarded = true;
            printf("%f@%f discarded\n", msg.peak[j], mT);
         }
#endif
      }
   } // while

   if (numChanges > 0) {
      #ifdef AUTOMATED_INPUT_LEVEL_ADJUSTMENT
         if (gAudioIO->AILAIsActive() && mIsInput && !discarded) {
            gAudioIO->AILAProcess(maxPeak);
            putchar('\n');
         }
      #endif
      RepaintBarsNow();
   }
}

float Meter::GetMaxPeak() const
{
   int j;
   float maxPeak = 0.;

   for(j=0; j<mNumBars; j++)
      maxPeak = mBar[j].peak > maxPeak ? mBar[j].peak : maxPeak;

   return(maxPeak);
}

wxFont Meter::GetFont() const
{
   int fontSize = 10;
#if defined __WXMSW__
   fontSize = 8;
#endif

   return wxFont(fontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
}

void Meter::ResetBar(MeterBar *b, bool resetClipping)
{
   b->peak = 0.0;
   b->rms = 0.0;
   b->peakHold = 0.0;
   b->peakHoldTime = 0.0;
   if (resetClipping)
   {
      b->clipping = false;
      b->peakPeakHold = 0.0;
   }
   b->isclipping = false;
   b->tailPeakCount = 0;
}

bool Meter::IsClipping() const
{
   for (int c = 0; c < kMaxMeterBars; c++)
      if (mBar[c].isclipping)
         return true;
   return false;
}

void Meter::SetActiveStyle(Style newStyle)
{
   mStyle = newStyle;

   // Set dummy ruler bounds so width/height can be retrieved
   // NOTE: Make sure the Right and Bottom values are large enough to
   //       ensure full width/height of digits get calculated.
   mRuler.SetBounds(0, 0, 500, 500);

   if (mDB)
   {
      mRuler.SetFormat(Ruler::LinearDBFormat);
      if (mStyle == HorizontalStereo || mStyle == HorizontalStereoCompact)
      {
         mRuler.SetOrientation(wxHORIZONTAL);
         mRuler.SetRange(-mDBRange, 0);
      }
      else
      {
         mRuler.SetOrientation(wxVERTICAL);
         mRuler.SetRange(0, -mDBRange);
      }
   }
   else
   {
      mRuler.SetFormat(Ruler::RealFormat);
      if (mStyle == HorizontalStereo || mStyle == HorizontalStereoCompact)
      {
         mRuler.SetOrientation(wxHORIZONTAL);
         mRuler.SetRange(0, 1);
      }
      else
      {
         mRuler.SetOrientation(wxVERTICAL);
         mRuler.SetRange(1, 0);
      }
   }

   mRuler.GetMaxSize(&mRulerWidth, &mRulerHeight);
}

void Meter::SetBarAndClip(int iBar, bool vert)
{
   // Save the orientation
   mBar[iBar].vert = vert;

   // Create the bar rectangle and educe to fit inside the bevel
   mBar[iBar].r = mBar[iBar].b;
   mBar[iBar].r.x += 1;
   mBar[iBar].r.width -= 1;
   mBar[iBar].r.y += 1;
   mBar[iBar].r.height -= 1;

   if (vert)
   {
      if (mClip)
      {
         // Create the clip rectangle
         mBar[iBar].rClip = mBar[iBar].b;
         mBar[iBar].rClip.height = 3;

         // Make room for the clipping indicator
         mBar[iBar].b.y += 3 + gap;
         mBar[iBar].b.height -= 3 + gap;
         mBar[iBar].r.y += 3 + gap;
         mBar[iBar].r.height -= 3 + gap;
      }
   }
   else
   {
      if (mClip)
      {
         // Make room for the clipping indicator
         mBar[iBar].b.width -= 4;
         mBar[iBar].r.width -= 4;

         // Create the indicator rectangle
         mBar[iBar].rClip = mBar[iBar].b;
         mBar[iBar].rClip.x = mBar[iBar].b.GetRight() + 1 + gap; // +1 for bevel
         mBar[iBar].rClip.width = 3;
      }
   }
}

void Meter::HandleLayout(wxDC &dc)
{
   // Refresh to reflect any language changes
   /* i18n-hint: One-letter abbreviation for Left, in VU Meter */
   mLeftText = _("L");
   /* i18n-hint: One-letter abbreviation for Right, in VU Meter */
   mRightText = _("R");

   dc.SetFont(GetFont());
   int iconWidth = 0;
   int iconHeight = 0;
   int width = mWidth;
   int height = mHeight;
   int left = 0;
   int top = 0;
   int barw;
   int barh;
   int lside;
   int rside;

   // MixerTrackCluster has no L/R labels or icon
   if (mStyle != MixerTrackCluster)
   {
      if (mDesiredStyle == AutomaticStereo)
      {
         SetActiveStyle(width > height ? HorizontalStereo : VerticalStereo);
      }
   
      if (mStyle == HorizontalStereoCompact || mStyle == HorizontalStereo)
      {
         SetActiveStyle(height < 50 ? HorizontalStereoCompact : HorizontalStereo);
      }
      else if (mStyle == VerticalStereoCompact || mStyle == VerticalStereo)
      {
         SetActiveStyle(width < 100 ? VerticalStereoCompact : VerticalStereo);
      }
   
      iconWidth = mIcon->GetWidth();
      iconHeight = mIcon->GetHeight();
      if (mLeftSize.GetWidth() == 0)  // Not yet initialized to dc.
      {
         dc.GetTextExtent(mLeftText, &mLeftSize.x, &mLeftSize.y);
         dc.GetTextExtent(mRightText, &mRightSize.x, &mRightSize.y);
      }
   }

   int ltxtWidth = mLeftSize.GetWidth();
   int ltxtHeight = mLeftSize.GetHeight();
   int rtxtWidth = mRightSize.GetWidth();
   int rtxtHeight = mRightSize.GetHeight();

   switch (mStyle)
   {
   default:
      wxPrintf(wxT("Style not handled yet!\n"));
      break;
   case MixerTrackCluster:
      // width is now the entire width of the meter canvas
      width -= mRulerWidth + left;

      // height is now the entire height of the meter canvas
      height -= top + gap;
 
      // barw is half of the canvas while allowing for a gap between meters
      barw = (width - gap) / 2;

      // barh is now the height of the canvas
      barh = height;

      // We always have 2 bars
      mNumBars = 2;

      // Save dimensions of the left bevel
      mBar[0].b = wxRect(left, top, barw, barh);

      // Save dimensions of the right bevel
      mBar[1].b = mBar[0].b;
      mBar[1].b.SetLeft(mBar[0].b.GetRight() + 1 + gap); // +1 for right edge

      // Set bar and clipping indicator dimensions
      SetBarAndClip(0, true);
      SetBarAndClip(1, true);

      mRuler.SetBounds(mBar[1].r.GetRight() + 1,   // +1 for the bevel
                       mBar[1].r.GetTop(),
                       mWidth,
                       mBar[1].r.GetBottom());
      mRuler.OfflimitsPixels(0, 0);
      break;
   case VerticalStereo:
      // Determine required width of each side;
      lside = intmax(iconWidth, ltxtWidth);
      rside = intmax(mRulerWidth, rtxtWidth);

      // left is now the right edge of the icon or L label
      left = lside;

      // Ensure there's a margin between top edge of window and the meters
      top = gap;

      // Position the icon
      mIconRect.SetX(left - iconWidth);
      mIconRect.SetY(top);
      mIconRect.SetWidth(iconWidth);
      mIconRect.SetHeight(iconHeight);

      // Position the L/R labels
      mLeftTextPos = wxPoint(left - ltxtWidth - gap, height - gap - ltxtHeight);
      mRightTextPos = wxPoint(width - rside - gap, height - gap - rtxtHeight);

      // left is now left edge of left bar
      left += gap;

      // width is now the entire width of the meter canvas
      width -= gap + rside + gap + left;

      // height is now the entire height of the meter canvas
      height -= top + gap;
 
      // barw is half of the canvas while allowing for a gap between meters
      barw = (width - gap) / 2;

      // barh is now the height of the canvas
      barh = height;

      // We always have 2 bars
      mNumBars = 2;

      // Save dimensions of the left bevel
      mBar[0].b = wxRect(left, top, barw, barh);

      // Save dimensions of the right bevel
      mBar[1].b = mBar[0].b;
      mBar[1].b.SetLeft(mBar[0].b.GetRight() + 1 + gap); // +1 for right edge

      // Set bar and clipping indicator dimensions
      SetBarAndClip(0, true);
      SetBarAndClip(1, true);

      mRuler.SetBounds(mBar[1].r.GetRight() + 1,   // +1 for the bevel
                       mBar[1].r.GetTop(),
                       mWidth,
                       mBar[1].r.GetBottom());
      mRuler.OfflimitsPixels(mRightTextPos.y - gap, mBar[1].r.GetBottom());
      break;
   case VerticalStereoCompact:
      // Ensure there's a margin between top edge of window and the meters
      top = gap;

      // Position the icon
      mIconRect.SetX((width - iconWidth) / 2);
      mIconRect.SetY(top);
      mIconRect.SetWidth(iconWidth);
      mIconRect.SetHeight(iconHeight);

      // top is now the top of the bar
      top += iconHeight + gap;

      // height is now the entire height of the meter canvas
      height -= top + gap + ltxtHeight + gap;

      // barw is half of the canvas while allowing for a gap between meters
      barw = (width / 2) - gap;

      // barh is now the height of the canvas
      barh = height;

      // We always have 2 bars
      mNumBars = 2;

      // Save dimensions of the left bevel
      mBar[0].b = wxRect(left, top, barw, barh);

      // Save dimensions of the right bevel
      mBar[1].b = mBar[0].b;
      mBar[1].b.SetLeft(mBar[0].b.GetRight() + 1 + gap); // +1 for right edge

      // Set bar and clipping indicator dimensions
      SetBarAndClip(0, true);
      SetBarAndClip(1, true);

      // L/R is centered horizontally under each bar
      mLeftTextPos = wxPoint(mBar[0].b.GetLeft() + ((mBar[0].b.GetWidth() - ltxtWidth) / 2), top + barh + gap);
      mRightTextPos = wxPoint(mBar[1].b.GetLeft() + ((mBar[1].b.GetWidth() - rtxtWidth) / 2), top + barh + gap);

      mRuler.SetBounds((mWidth - mRulerWidth) / 2,
                       mBar[1].r.GetTop(),
                       (mWidth - mRulerWidth) / 2,
                       mBar[1].r.GetBottom());
      mRuler.OfflimitsPixels(0, 0);
      break;
   case HorizontalStereo:
      // Ensure there's a margin between left edge of window and items
      left = gap;

      // Add a gap between bottom of icon and bottom of window
      height -= gap;

      // Create icon rectangle
      mIconRect.SetX(left);
      mIconRect.SetY(height - iconHeight);
      mIconRect.SetWidth(iconWidth);
      mIconRect.SetHeight(iconHeight);

      // Make sure there's room for icon and gap between the bottom of the meter and icon
      height -= iconHeight + gap;

      // L/R is centered vertically and to the left of a each bar
      mLeftTextPos = wxPoint(left, (height / 4) - ltxtHeight / 2);
      mRightTextPos = wxPoint(left, (height * 3 / 4) - rtxtHeight / 2);

      // Add width of widest of the L/R characters
      left += intmax(ltxtWidth, rtxtWidth); //, iconWidth);

      // Add gap between L/R and meter bevel
      left += gap;

      // width is now the entire width of the meter canvas
      width -= left;

      // barw is now the width of the canvas minus gap between canvas and right window edge
      barw = width - gap;

      // barh is half of the canvas while allowing for a gap between meters
      barh = (height - gap) / 2;

      // We always have 2 bars
      mNumBars = 2;

      // Save dimensions of the top bevel
      mBar[0].b = wxRect(left, top, barw, barh);

      // Save dimensions of the bottom bevel
      mBar[1].b = mBar[0].b;
      mBar[1].b.SetTop(mBar[0].b.GetBottom() + 1 + gap); // +1 for bottom edge

      // Set bar and clipping indicator dimensions
      SetBarAndClip(0, false);
      SetBarAndClip(1, false);

      mRuler.SetBounds(mBar[1].r.GetLeft(),
                       mBar[1].r.GetBottom() + 1, // +1 to fit below bevel
                       mBar[1].r.GetRight(),
                       mHeight - mBar[1].r.GetBottom() + 1);
      mRuler.OfflimitsPixels(0, mIconRect.GetRight() - 4);
      break;
   case HorizontalStereoCompact:
      // Ensure there's a margin between left edge of window and items
      left = gap;

      // Create icon rectangle
      mIconRect.SetX(left);
      mIconRect.SetY((height - iconHeight) / 2);
      mIconRect.SetWidth(iconWidth);
      mIconRect.SetHeight(iconHeight);

      // Add width of icon and gap between icon and L/R
      left += iconWidth + gap;

      // L/R is centered vertically and to the left of a each bar
      mLeftTextPos = wxPoint(left, (height / 4) - (ltxtHeight / 2));
      mRightTextPos = wxPoint(left, (height * 3 / 4) - (ltxtHeight / 2));

      // Add width of widest of the L/R characters and a gap between labels and meter bevel
      left += intmax(ltxtWidth, rtxtWidth) + gap;

      // width is now the entire width of the meter canvas
      width -= left;

      // barw is now the width of the canvas minus gap between canvas and window edge
      barw = width - gap;

      // barh is half of the canvas while allowing for a gap between meters
      barh = (height - gap) / 2;

      // We always have 2 bars
      mNumBars = 2;

      // Save dimensions of the top bevel
      mBar[0].b = wxRect(left, top, barw, barh);

      // Save dimensions of the bottom bevel
      // Since the bars butt up against the window's top and bottom edges, we need
      // to include an extra pixel in the bottom bar when the window height and
      // meter height do not exactly match.
      mBar[1].b = mBar[0].b;
      mBar[1].b.SetTop(mBar[0].b.GetBottom() + 1 + gap); // +1 for bottom bevel
      mBar[1].b.SetHeight(mHeight - mBar[1].b.GetTop() - 1); // +1 for bottom bevel

      // Add clipping indicators - do after setting bar/bevel dimensions above
      SetBarAndClip(0, false);
      SetBarAndClip(1, false);

      mRuler.SetBounds(mBar[1].r.GetLeft(),
                       mBar[1].b.GetTop() - (mRulerHeight / 2),
                       mBar[1].r.GetRight(),
                       mBar[1].b.GetTop() - (mRulerHeight / 2));
      mRuler.OfflimitsPixels(0, 0);
      break;
   }

   mLayoutValid = true;
}

void Meter::RepaintBarsNow()
{
   if (mLayoutValid)
   {
#if defined(__WXMSW__)
      wxClientDC clientDC(this);
      wxBufferedDC dc(&clientDC, *mBitmap);
#else
      wxClientDC dc(this);
#endif

      for (int i = 0; i < mNumBars; i++)
      {
         DrawMeterBar(dc, &mBar[i]);
      }

#if defined(__WXMAC__) || defined(__WXGTK__)
      // Due to compositing or antialiasing on the Mac, we have to make
      // sure all remnants of the previous ruler text is completely gone.
      // Otherwise, we get a strange bolding effect.
      //
      // Since redrawing the rulers above wipe out most of the ruler, the
      // only thing that is left is the bits between the bars.
      if (mStyle == HorizontalStereoCompact)
      {
         dc.SetPen(*wxTRANSPARENT_PEN);
         dc.SetBrush(mBkgndBrush);
         dc.DrawRectangle(mBar[0].b.GetLeft(),
                          mBar[0].b.GetBottom() + 1,
                          mBar[0].b.GetWidth(),
                          mBar[1].b.GetTop() - mBar[0].b.GetBottom() - 1);
         AColor::Bevel(dc, false, mBar[0].b);
         AColor::Bevel(dc, false, mBar[1].b);
      }
      else if (mStyle == VerticalStereoCompact)
      {
         dc.SetPen(*wxTRANSPARENT_PEN);
         dc.SetBrush(mBkgndBrush);
         dc.DrawRectangle(mBar[0].b.GetRight() + 1,
                          mBar[0].b.GetTop(),
                          mBar[1].b.GetLeft() - mBar[0].b.GetRight() - 1,
                          mBar[0].b.GetHeight());
         AColor::Bevel(dc, false, mBar[0].b);
         AColor::Bevel(dc, false, mBar[1].b);
      }
#endif

#if defined(__WXMSW__) || defined(__WXGTK__)
      if (mIsFocused)
      {
         wxRect r = mIconRect;
         AColor::DrawFocus(dc, r.Inflate(1, 1));
      }
#endif

      // Compact style requires redrawing ruler
      if (mStyle == HorizontalStereoCompact || mStyle == VerticalStereoCompact)
      {
          mRuler.Draw(dc);
      }
   }
}

void Meter::DrawMeterBar(wxDC &dc, MeterBar *bar)
{
   // Cache some metrics
   wxCoord x = bar->r.GetLeft();
   wxCoord y = bar->r.GetTop();
   wxCoord w = bar->r.GetWidth();
   wxCoord h = bar->r.GetHeight();
   wxCoord ht;
   wxCoord wd;

   // Setup for erasing the background
   dc.SetPen(*wxTRANSPARENT_PEN);
   dc.SetBrush(mMeterDisabled ? mDisabledBkgndBrush : mBkgndBrush);

   if (mGradient)
   {
      // Map the predrawn bitmap into the source DC
      wxMemoryDC srcDC;
      srcDC.SelectObject(*mBitmap);

      if (bar->vert)
      {
         // Copy as much of the predrawn meter bar as is required for the
         // current peak.
         // (h - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         ht = (int)(bar->peak * (h - 1) + 0.5);

         // Blank out the rest
         if (h - ht)
         {
            // ht includes peak value...not really needed but doesn't hurt
            dc.DrawRectangle(x, y, w, h - ht);
         }

         // Copy as much of the predrawn meter bar as is required for the
         // current peak.
         // +/-1 to include the peak position
         if (ht)
         {
            dc.Blit(x, y + h - ht - 1, w, ht + 1, &srcDC, x, y + h - ht - 1);
         }

         // Draw the "recent" peak hold line using the predrawn meter bar so that
         // it will be the same color as the original level.
         // (h - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         ht = (int)(bar->peakHold * (h - 1) + 0.5);
         if (ht > 1)
         {
            dc.Blit(x, y + h - ht - 1, w, 2, &srcDC, x, y + h - ht - 1);
         }

         // Draw the "maximum" peak hold line
         // (h - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         dc.SetPen(mPeakPeakPen);
         ht = (int)(bar->peakPeakHold * (h - 1) + 0.5);
         if (ht > 0)
         {
            AColor::Line(dc, x, y + h - ht - 1, x + w - 1, y + h - ht - 1);
            if (ht > 1)
            {
               AColor::Line(dc, x, y + h - ht, x + w - 1, y + h - ht);
            }
         }
      }
      else
      {
         // Calculate the peak position
         // (w - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         wd = (int)(bar->peak * (w - 1) + 0.5);

         // Blank out the rest
         if (w - wd)
         {
            // wd includes peak value...not really needed but doesn't hurt
            dc.DrawRectangle(x + wd, y, w - wd, h);
         }

         // Copy as much of the predrawn meter bar as is required for the
         // current peak.  But, only blit() if there's something to copy
         // to prevent display corruption.
         // +1 to include peak position
         if (wd)
         {
            dc.Blit(x, y, wd + 1, h, &srcDC, x, y);
         }

         // Draw the "recent" peak hold line using the predrawn meter bar so that
         // it will be the same color as the original level.
         // -1 to give a 2 pixel width
         wd = (int)(bar->peakHold * (w - 1) + 0.5);
         if (wd > 1)
         {
            dc.Blit(x + wd - 1, y, 2, h, &srcDC, x + wd, y);
         }

         // Draw the "maximum" peak hold line using a themed color
         // (w - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         dc.SetPen(mPeakPeakPen);
         wd = (int)(bar->peakPeakHold * (w - 1) + 0.5);
         if (wd > 0)
         {
            AColor::Line(dc, x + wd, y, x + wd, y + h - 1);
            if (wd > 1)
            {
               AColor::Line(dc, x + wd - 1, y, x + wd - 1, y + h - 1);
            }
         }
      }

      // No longer need the source DC, so unselect the predrawn bitmap
      srcDC.SelectObject(wxNullBitmap);
   }
   else
   {
      if (bar->vert)
      {
         // Calculate the peak position
         // (h - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         ht = (int)(bar->peak * (h - 1) + 0.5);

         // Blank out the rest
         if (h - ht)
         {
            // ht includes peak value...not really needed but doesn't hurt
            dc.DrawRectangle(x, y, w, h - ht);
         }

         // Draw the peak level
         // +/-1 to include the peak position
         dc.SetPen(*wxTRANSPARENT_PEN);
         dc.SetBrush(mMeterDisabled ? mDisabledBkgndBrush : mBrush);
         if (ht)
         {
            dc.DrawRectangle(x, y + h - ht - 1, w, ht + 1);
         }

         // Draw the "recent" peak hold line
         // (h - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         dc.SetPen(mPen);
         int ht = (int)(bar->peakHold * (h - 1) + 0.5);
         if (ht > 0)
         {
            AColor::Line(dc, x, y + h - ht - 1, x + w - 1, y + h - ht - 1);
            if (ht > 1)
            {
               AColor::Line(dc, x, y + h - ht, x + w - 1, y + h - ht);
            }
         }

         // Calculate the rms position
         // (h - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         // +1 to include the rms position
         ht = (int)(bar->rms * (h - 1) + 0.5);

         // Draw the RMS level
         dc.SetPen(*wxTRANSPARENT_PEN);
         dc.SetBrush(mMeterDisabled ? mDisabledBkgndBrush : mRMSBrush);
         if (ht)
         {
            dc.DrawRectangle(x, y + h - ht - 1, w, ht + 1);
         }

         // Draw the "maximum" peak hold line
         // (h - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         dc.SetPen(mPeakPeakPen);
         ht = (int)(bar->peakPeakHold * (h - 1) + 0.5);
         if (ht > 0)
         {
            AColor::Line(dc, x, y + h - ht - 1, x + w - 1, y + h - ht - 1);
            if (ht > 1)
            {
               AColor::Line(dc, x, y + h - ht, x + w - 1, y + h - ht);
            }
         }
      }
      else
      {
         // Calculate the peak position
         // (w - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         wd = (int)(bar->peak * (w - 1) + 0.5);

         // Blank out the rest
         if (w - wd)
         {
            // wd includes peak value...not really needed but doesn't hurt
            dc.DrawRectangle(x + wd, y, w - wd, h);
         }

         // Draw the peak level
         // +1 to include peak position
         dc.SetPen(*wxTRANSPARENT_PEN);
         dc.SetBrush(mMeterDisabled ? mDisabledBkgndBrush : mBrush);
         if (wd)
         {
            dc.DrawRectangle(x, y, wd + 1, h);
         }

         // Draw the "recent" peak hold line
         // (w - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         dc.SetPen(mPen);
         wd = (int)(bar->peakHold * (w - 1) + 0.5);
         if (wd > 0)
         {
            AColor::Line(dc, x + wd, y, x + wd, y + h - 1);
            if (wd > 1)
            {
               AColor::Line(dc, x + wd - 1, y, x + wd - 1, y + h - 1);
            }
         }

         // Calculate the rms position
         // (w - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         wd = (int)(bar->rms * (w - 1) + 0.5);

         // Draw the rms level 
         // +1 to include the rms position
         dc.SetPen(*wxTRANSPARENT_PEN);
         dc.SetBrush(mMeterDisabled ? mDisabledBkgndBrush : mRMSBrush);
         if (wd)
         {
            dc.DrawRectangle(x, y, wd + 1, h);
         }

         // Draw the "maximum" peak hold line using a themed color
         // (w - 1) corresponds to the mRuler.SetBounds() in HandleLayout()
         dc.SetPen(mPeakPeakPen);
         wd = (int)(bar->peakPeakHold * (w - 1) + 0.5);
         if (wd > 0)
         {
            AColor::Line(dc, x + wd, y, x + wd, y + h - 1);
            if (wd > 1)
            {
               AColor::Line(dc, x + wd - 1, y, x + wd - 1, y + h - 1);
            }
         }
      }
   }

   // If meter had a clipping indicator, draw or erase it
   // LLL:  At least I assume that's what "mClip" is supposed to be for as
   //       it is always "true".
   if (mClip)
   {
      if (bar->clipping)
      {
         dc.SetBrush(mClipBrush);
      }
      else
      {
         dc.SetBrush(mMeterDisabled ? mDisabledBkgndBrush : mBkgndBrush);
      }
      dc.SetPen(*wxTRANSPARENT_PEN);
      wxRect r(bar->rClip.GetX() + 1,
               bar->rClip.GetY() + 1,
               bar->rClip.GetWidth() - 1,
               bar->rClip.GetHeight() - 1);
      dc.DrawRectangle(r);
   }
}

bool Meter::IsMeterDisabled() const
{
   return mMeterDisabled != 0;
}

void Meter::StartMonitoring()
{
   bool start = !mMonitoring;

   if (gAudioIO->IsMonitoring()){
      gAudioIO->StopStream();
   } 

   if (start && !gAudioIO->IsBusy()){
      AudacityProject *p = GetActiveProject();
      if (p){
         gAudioIO->StartMonitoring(p->GetRate());
      }

      mLayoutValid = false;

      Refresh(false);
   }
}

void Meter::OnAudioIOStatus(wxCommandEvent &evt)
{
   evt.Skip();

   AudacityProject *p = (AudacityProject *) evt.GetEventObject();

   mActive = false;
   if (evt.GetInt() != 0)
   {
      if (p == mProject)
      {
         mActive = true;

         mTimer.Start(1000 / mMeterRefreshRate);

         if (evt.GetEventType() == EVT_AUDIOIO_MONITOR)
         {
            mMonitoring = mActive;
         }
      }
      else
      {
         mTimer.Stop();

         mMonitoring = false;
      }
   }
   else
   {
      mTimer.Stop();

      mMonitoring = false;
   }

   // Only refresh is we're the active meter
   if (IsShownOnScreen())
   {
      Refresh(false);
   }
}

// SaveState() and RestoreState() exist solely for purpose of recreating toolbars
// They should really be quering the project for current audio I/O state, but there
// isn't a clear way of doing that just yet.  (It should NOT query AudioIO.)
void *Meter::SaveState()
{
   bool *state = new bool[2];
   state[0] = mMonitoring;
   state[1] = mActive;

   return state;
}

void Meter::RestoreState(void *state)
{
   bool *s = (bool *)state;
   mMonitoring = s[0];
   mActive = s[1];

   if (mActive)
   {
      mTimer.Start(1000 / mMeterRefreshRate);
   }

   delete [] s;
}

//
// Pop-up menu
//

void Meter::ShowMenu(const wxPoint & pos)
{
   wxMenu *menu = new wxMenu();
   // Note: these should be kept in the same order as the enum
   if (mIsInput) {
      wxMenuItem *mi;
      if (mMonitoring)
         mi = menu->Append(OnMonitorID, _("Stop Monitoring"));
      else
         mi = menu->Append(OnMonitorID, _("Start Monitoring"));
      mi->Enable(!mActive || mMonitoring);
   }

   menu->Append(OnPreferencesID, _("Preferences..."));

   mAccSilent = true;      // temporarily make screen readers say (close to) nothing on focus events

   PopupMenu(menu, pos);

   /* if stop/start monitoring was chosen in the menu, then by this point
   OnMonitoring has been called and variables which affect the accessibility
   name have been updated so it's now ok for screen readers to read the name of
   the button */
   mAccSilent = false;
#if wxUSE_ACCESSIBILITY
   GetAccessible()->NotifyEvent(wxACC_EVENT_OBJECT_FOCUS,
                                this,
                                wxOBJID_CLIENT,
                                wxACC_SELF);
#endif

   delete menu;
}

void Meter::OnMonitor(wxCommandEvent & WXUNUSED(event))
{
   StartMonitoring();
}

void Meter::OnMeterPrefsUpdated(wxCommandEvent & evt)
{
   evt.Skip();

   UpdatePrefs();

   Refresh(false);
}

void Meter::OnPreferences(wxCommandEvent & WXUNUSED(event))
{
   wxTextCtrl *rate;
   wxRadioButton *gradient;
   wxRadioButton *rms;
   wxRadioButton *db;
   wxRadioButton *linear;
   wxRadioButton *automatic;
   wxRadioButton *horizontal;
   wxRadioButton *vertical;
   int meterRefreshRate = mMeterRefreshRate;

   wxString title(mIsInput ? _("Recording Meter Preferences") : _("Playback Meter Preferences"));

   // Dialog is a child of the project, rather than of the toolbar.
   // This determines where it pops up.

   wxDialog dlg(GetActiveProject(), wxID_ANY, title);
   dlg.SetName(dlg.GetTitle());
   ShuttleGui S(&dlg, eIsCreating);
   S.StartVerticalLay();
   {
      S.StartStatic(_("Refresh Rate"), 0);
      {
         S.AddFixedText(_("Higher refresh rates make the meter show more frequent\nchanges. A rate of 30 per second or less should prevent\nthe meter affecting audio quality on slower machines."));
         S.StartHorizontalLay();
         {
            rate = S.AddTextBox(_("Meter refresh rate per second [1-100]: "),
                                wxString::Format(wxT("%d"), meterRefreshRate),
                                10);
            rate->SetName(_("Meter refresh rate per second [1-100]"));
            IntegerValidator<long> vld(&mMeterRefreshRate);
            vld.SetRange(0, 100);
            rate->SetValidator(vld);
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      S.StartHorizontalLay();
      {
        S.StartStatic(_("Meter Style"), 0);
        {
           S.StartVerticalLay();
           {
              gradient = S.AddRadioButton(_("Gradient"));
              gradient->SetName(_("Gradient"));
              gradient->SetValue(mGradient);

              rms = S.AddRadioButtonToGroup(_("RMS"));
              rms->SetName(_("RMS"));
              rms->SetValue(!mGradient);
           }
           S.EndVerticalLay();
        }
        S.EndStatic();

        S.StartStatic(_("Meter Type"), 0);
        {
           S.StartVerticalLay();
           {
              db = S.AddRadioButton(_("dB"));
              db->SetName(_("dB"));
              db->SetValue(mDB);

              linear = S.AddRadioButtonToGroup(_("Linear"));
              linear->SetName(_("Linear"));
              linear->SetValue(!mDB);
           }
           S.EndVerticalLay();
        }
        S.EndStatic();

        S.StartStatic(_("Orientation"), 1);
        {
           S.StartVerticalLay();
           {
              automatic = S.AddRadioButton(_("Automatic"));
              automatic->SetName(_("Automatic"));
              automatic->SetValue(mDesiredStyle == AutomaticStereo);

              horizontal = S.AddRadioButtonToGroup(_("Horizontal"));
              horizontal->SetName(_("Horizontal"));
              horizontal->SetValue(mDesiredStyle == HorizontalStereo);

              vertical = S.AddRadioButtonToGroup(_("Vertical"));
              vertical->SetName(_("Vertical"));
              vertical->SetValue(mDesiredStyle == VerticalStereo);
           }
           S.EndVerticalLay();
        }
        S.EndStatic();
      }
      S.EndHorizontalLay();
      S.AddStandardButtons();
   }
   S.EndVerticalLay();
   dlg.Layout();
   dlg.Fit();

   dlg.CenterOnParent();

   if (dlg.ShowModal() == wxID_OK)
   {
      wxArrayString style;
      style.Add(wxT("AutomaticStereo"));
      style.Add(wxT("HorizontalStereo"));
      style.Add(wxT("VerticalStereo"));

      int s = 0;
      s = automatic->GetValue() ? 0 : s;
      s = horizontal->GetValue() ? 1 : s;
      s = vertical->GetValue() ? 2 : s;

      gPrefs->Write(Key(wxT("Style")), style[s]);
      gPrefs->Write(Key(wxT("Bars")), gradient->GetValue() ? wxT("Gradient") : wxT("RMS"));
      gPrefs->Write(Key(wxT("Type")), db->GetValue() ? wxT("dB") : wxT("Linear"));
      gPrefs->Write(Key(wxT("RefreshRate")), rate->GetValue());

      gPrefs->Flush();

      // Currently, there are 2 playback meters and 2 record meters and any number of 
      // mixerboard meters, so we have to send out an preferences updated message to
      // ensure they all update themselves.
      wxCommandEvent e(EVT_METER_PREFERENCES_CHANGED);
      e.SetEventObject(this);
      GetParent()->GetEventHandler()->ProcessEvent(e);
   }
}

wxString Meter::Key(const wxString & key) const
{
   if (mStyle == MixerTrackCluster)
   {
      return wxT("/Meter/Mixerboard/") + key;
   }

   if (mIsInput)
   {
      return wxT("/Meter/Input/") + key;
   }

   return wxT("/Meter/Output/") + key;
}

#if wxUSE_ACCESSIBILITY

MeterAx::MeterAx(wxWindow *window):
   wxWindowAccessible(window)
{
}

MeterAx::~MeterAx()
{
}

// Performs the default action. childId is 0 (the action for this object)
// or > 0 (the action for a child).
// Return wxACC_NOT_SUPPORTED if there is no default action for this
// window (e.g. an edit control).
wxAccStatus MeterAx::DoDefaultAction(int WXUNUSED(childId))
{
   Meter *m = wxDynamicCast(GetWindow(), Meter);

   if (m && m->mIsInput)
   {
      m->StartMonitoring();
   }

   return wxACC_OK;
}

// Retrieves the address of an IDispatch interface for the specified child.
// All objects must support this property.
wxAccStatus MeterAx::GetChild(int childId, wxAccessible** child)
{
   if (childId == wxACC_SELF)
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
wxAccStatus MeterAx::GetChildCount(int* childCount)
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
wxAccStatus MeterAx::GetDefaultAction(int WXUNUSED(childId), wxString* actionName)
{
   *actionName = _("Press");

   return wxACC_OK;
}

// Returns the description for this object or a child.
wxAccStatus MeterAx::GetDescription(int WXUNUSED(childId), wxString *description)
{
   description->Clear();

   return wxACC_NOT_SUPPORTED;
}

// Gets the window with the keyboard focus.
// If childId is 0 and child is NULL, no object in
// this subhierarchy has the focus.
// If this object has the focus, child should be 'this'.
wxAccStatus MeterAx::GetFocus(int* childId, wxAccessible** child)
{
   *childId = 0;
   *child = this;

   return wxACC_OK;
}

// Returns help text for this object or a child, similar to tooltip text.
wxAccStatus MeterAx::GetHelpText(int WXUNUSED(childId), wxString *helpText)
{
   helpText->Clear();

   return wxACC_NOT_SUPPORTED;
}

// Returns the keyboard shortcut for this object or child.
// Return e.g. ALT+K
wxAccStatus MeterAx::GetKeyboardShortcut(int WXUNUSED(childId), wxString *shortcut)
{
   shortcut->Clear();

   return wxACC_OK;
}

// Returns the rectangle for this object (id = 0) or a child element (id > 0).
// rect is in screen coordinates.
wxAccStatus MeterAx::GetLocation(wxRect & rect, int WXUNUSED(elementId))
{
   Meter *m = wxDynamicCast(GetWindow(), Meter);

   rect = m->mIconRect;
   rect.SetPosition(m->ClientToScreen(rect.GetPosition()));

   return wxACC_OK;
}

// Gets the name of the specified object.
wxAccStatus MeterAx::GetName(int WXUNUSED(childId), wxString* name)
{
   Meter *m = wxDynamicCast(GetWindow(), Meter);

   if (m->mAccSilent)
   {
      *name = wxT("");     // Jaws reads nothing, and nvda reads "unknown"
   }
   else
   {

      *name = m->GetName();
      if (name->IsEmpty())
      {
         *name = m->GetLabel();
      }

      if (name->IsEmpty())
      {
         *name = _("Meter");
      }

      if (m->mMonitoring)
      {
         // translations of strings such as " Monitoring " did not
         // always retain the leading space. Therefore a space has
         // been added to ensure at least one space, and stop
         // words from being merged
         *name += wxT(" ") + wxString::Format(_(" Monitoring "));
      }
      else if (m->mActive)
      {
         *name += wxT(" ") + wxString::Format(_(" Active "));
      }

      float peak = 0.;
      bool clipped = false;
      for (int i = 0; i < m->mNumBars; i++)
      {
         peak = wxMax(peak, m->mBar[i].peakPeakHold);
         if (m->mBar[i].clipping)
            clipped = true;
      }

      if (m->mDB)
      {
         *name += wxT(" ") + wxString::Format(_(" Peak %2.f dB"), (peak * m->mDBRange) - m->mDBRange);
      }
      else
      {
         *name += wxT(" ") + wxString::Format(_(" Peak %.2f "), peak);
      }

      if (clipped)
      {
         *name += wxT(" ") + wxString::Format(_(" Clipped "));
      }
   }

   return wxACC_OK;
}

// Returns a role constant.
wxAccStatus MeterAx::GetRole(int WXUNUSED(childId), wxAccRole* role)
{
   Meter *m = wxDynamicCast(GetWindow(), Meter);

   if (m->mAccSilent)
      *role = wxROLE_NONE;    // Jaws and nvda both read nothing
   else
      *role = wxROLE_SYSTEM_BUTTONDROPDOWN;

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
wxAccStatus MeterAx::GetSelections(wxVariant * WXUNUSED(selections))
{
   return wxACC_NOT_IMPLEMENTED;
}

// Returns a state constant.
wxAccStatus MeterAx::GetState(int WXUNUSED(childId), long* state)
{
   Meter *m = wxDynamicCast( GetWindow(), Meter );

   *state = wxACC_STATE_SYSTEM_FOCUSABLE;

   // Do not use mButtonIsFocused is not set until after this method
   // is called.
   *state |= ( m == wxWindow::FindFocus() ? wxACC_STATE_SYSTEM_FOCUSED : 0 );

   return wxACC_OK;
}

// Returns a localized string representing the value for the object
// or child.
wxAccStatus MeterAx::GetValue(int WXUNUSED(childId), wxString* WXUNUSED(strValue))
{
   return wxACC_NOT_SUPPORTED;
}

#endif
