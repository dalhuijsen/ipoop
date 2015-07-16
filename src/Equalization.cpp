/**********************************************************************

   Audacity: A Digital Audio Editor

   EffectEqualization.cpp

   Mitch Golden
   Vaughan Johnson (Preview)
   Martyn Shaw (FIR filters, response curve, graphic EQ)

*******************************************************************//**

   \file Equalization.cpp
   \brief Implements EffectEqualiztaion, EqualizationDialog,
   EqualizationPanel, EQCurve and EQPoint.

*//****************************************************************//**


   \class EffectEqualization
   \brief An Effect that modifies volume in different frequency bands.

   Performs filtering, using an FFT to do a FIR filter.
   It lets the user draw an arbitrary envelope (using the same
   envelope editing code that is used to edit the track's
   amplitude envelope).

   Also allows the curve to be specified with a series of 'graphic EQ'
   sliders.

   The filter is applied using overlap/add of Hanning windows.

   Clone of the FFT Filter effect, no longer part of Audacity.

*//****************************************************************//**

   \class EqualizationPanel
   \brief EqualizationPanel is used with EqualizationDialog and controls
   a graph for EffectEqualization.  We should look at amalgamating the
   various graphing code, such as provided by FreqWindow and FilterPanel.

*//****************************************************************//**

   \class EQCurve
   \brief EQCurve is used with EffectEqualization.

*//****************************************************************//**

   \class EQPoint
   \brief EQPoint is used with EQCurve and hence EffectEqualization.

*//*******************************************************************/


#include "../Audacity.h"

#include <math.h>
#include <vector>

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/brush.h>
#include <wx/button.h>  // not really needed here
#include <wx/dcmemory.h>
#include <wx/event.h>
#include <wx/image.h>
#include <wx/intl.h>
#include <wx/choice.h>
#include <wx/radiobut.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/textdlg.h>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
#include <wx/tooltip.h>
#include <wx/utils.h>

#include "../Experimental.h"
#include "../AColor.h"
#include "../ShuttleGui.h"
#include "../PlatformCompatibility.h"
#include "../FileNames.h"
#include "../Envelope.h"
#include "../widgets/LinkingHtmlWindow.h"
#include "../widgets/ErrorDialog.h"
#include "../FFT.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../WaveTrack.h"
#include "../widgets/Ruler.h"
#include "../xml/XMLFileReader.h"
#include "../Theme.h"
#include "../AllThemeResources.h"
#include "../WaveTrack.h"
#include "../float_cast.h"

#include "FileDialog.h"

#include "Equalization.h"

#ifdef EXPERIMENTAL_EQ_SSE_THREADED
#include "Equalization48x.h"
#endif


enum
{
   ID_Length = 10000,
   ID_dBMax,
   ID_dBMin,
   ID_Clear,
   ID_Invert,
   ID_Draw,
   ID_Graphic,
   ID_Interp,
   ID_Linear,
   ID_Grid,
   ID_Curve,
   ID_Manage,
   ID_Delete,
#ifdef EXPERIMENTAL_EQ_SSE_THREADED
   ID_DefaultMath,
   ID_SSE,
   ID_SSEThreaded,
   ID_AVX,
   ID_AVXThreaded,
   ID_Bench,
#endif
   ID_Slider,   // needs to come last
};

enum kInterpolations
{
   kBspline,
   kCosine,
   kCubic,
   kNumInterpolations
};

static const wxString kInterpStrings[kNumInterpolations] =
{
   /* i18n-hint: Technical term for a kind of curve.*/
   XO("B-spline"),
   XO("Cosine"),
   XO("Cubic")
};

static const double kThirdOct[] =
{
   20., 25., 31., 40., 50., 63., 80., 100., 125., 160., 200.,
   250., 315., 400., 500., 630., 800., 1000., 1250., 1600., 2000.,
   2500., 3150., 4000., 5000., 6300., 8000., 10000., 12500., 16000., 20000.,
};

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name          Type        Key                     Def      Min      Max      Scale
Param( FilterLength, int,     XO("FilterLength"),        4001,    21,      8191,    0      );
Param( CurveName,    wxChar*, XO("CurveName"),           wxT("unnamed"), wxT(""), wxT(""), wxT(""));
Param( InterpLin,    bool,    XO("InterpolateLin"),      false,   false,   true,    false  );
Param( InterpMeth,   int,     XO("InterpolationMethod"), 0,       0,       0,       0      );
Param( DrawMode,     bool,    wxT(""),                   true,    false,   true,    false  );
Param( DrawGrid,     bool,    wxT(""),                   true,    false,   true,    false  );
Param( dBMin,        float,   wxT(""),                   -30.0,   -120.0,  -10.0,   0      );
Param( dBMax,        float,   wxT(""),                   30.0,    0.0,     60.0,    0      );

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY( EQPointArray );
WX_DEFINE_OBJARRAY( EQCurveArray );

///----------------------------------------------------------------------------
// EffectEqualization
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(EffectEqualization, wxEvtHandler)
   EVT_SIZE( EffectEqualization::OnSize )

   EVT_SLIDER( ID_Length, EffectEqualization::OnSliderM )
   EVT_SLIDER( ID_dBMax, EffectEqualization::OnSliderDBMAX )
   EVT_SLIDER( ID_dBMin, EffectEqualization::OnSliderDBMIN )
   EVT_COMMAND_RANGE(ID_Slider,
                     ID_Slider + NUMBER_OF_BANDS - 1,
                     wxEVT_COMMAND_SLIDER_UPDATED,
                     EffectEqualization::OnSlider)
   EVT_CHOICE( ID_Interp, EffectEqualization::OnInterp )

   EVT_CHOICE( ID_Curve, EffectEqualization::OnCurve )
   EVT_BUTTON( ID_Manage, EffectEqualization::OnManage )
   EVT_BUTTON( ID_Clear, EffectEqualization::OnClear )
   EVT_BUTTON( ID_Invert, EffectEqualization::OnInvert )

   EVT_RADIOBUTTON(ID_Draw, EffectEqualization::OnDrawMode)
   EVT_RADIOBUTTON(ID_Graphic, EffectEqualization::OnGraphicMode)
   EVT_CHECKBOX(ID_Linear, EffectEqualization::OnLinFreq)
   EVT_CHECKBOX(ID_Grid, EffectEqualization::OnGridOnOff)

#ifdef EXPERIMENTAL_EQ_SSE_THREADED
   EVT_RADIOBUTTON(ID_DefaultMath, EffectEqualization::OnProcessingRadio)
   EVT_RADIOBUTTON(ID_SSE, EffectEqualization::OnProcessingRadio)
   EVT_RADIOBUTTON(ID_SSEThreaded, EffectEqualization::OnProcessingRadio)
   EVT_RADIOBUTTON(ID_AVX, EffectEqualization::OnProcessingRadio)
   EVT_RADIOBUTTON(ID_AVXThreaded, EffectEqualization::OnProcessingRadio)
   EVT_BUTTON(ID_Bench, EffectEqualization::OnBench)
#endif
END_EVENT_TABLE()

EffectEqualization::EffectEqualization()
{
   hFFT = InitializeFFT(windowSize);
   mFFTBuffer = new float[windowSize];
   mFilterFuncR = new float[windowSize];
   mFilterFuncI = new float[windowSize];

   SetLinearEffectFlag(true);

#ifdef EXPERIMENTAL_EQ_SSE_THREADED
   mEffectEqualization48x=NULL;
#endif

   mM = DEF_FilterLength;
   mLin = DEF_InterpLin;
   mInterp = DEF_InterpMeth;
   mCurveName = DEF_CurveName;

   GetPrivateConfig(GetCurrentSettingsGroup(), wxT("dBMin"), mdBMin, DEF_dBMin);
   GetPrivateConfig(GetCurrentSettingsGroup(), wxT("dBMax"), mdBMax, DEF_dBMax);
   GetPrivateConfig(GetCurrentSettingsGroup(), wxT("DrawMode"), mDrawMode, DEF_DrawMode);
   GetPrivateConfig(GetCurrentSettingsGroup(), wxT("DrawGrid"), mDrawGrid, DEF_DrawGrid);

   for (int i = 0; i < kNumInterpolations; i++)
   {
      mInterpolations.Add(wxGetTranslation(kInterpStrings[i]));
   }

   mLogEnvelope = new Envelope();
   mLogEnvelope->SetInterpolateDB(false);
   mLogEnvelope->Mirror(false);
   mLogEnvelope->SetRange(MIN_dBMin, MAX_dBMax); // MB: this is the highest possible range

   mLinEnvelope = new Envelope();
   mLinEnvelope->SetInterpolateDB(false);
   mLinEnvelope->Mirror(false);
   mLinEnvelope->SetRange(MIN_dBMin, MAX_dBMax); // MB: this is the highest possible range

   mEnvelope = (mLin ? mLinEnvelope : mLogEnvelope);

   mWindowSize = windowSize;

   mCurve = NULL;
   mDirty = false;
   mDisallowCustom = false;

   // Load the EQ curves
   LoadCurves();

   // Note: initial curve is set in TransferDataToWindow

   mBandsInUse = NUMBER_OF_BANDS;
   //double loLog = log10(mLoFreq);
   //double stepLog = (log10(mHiFreq) - loLog)/((double)NUM_PTS-1.);
   for(int i=0; i<NUM_PTS-1; i++)
      mWhens[i] = (double)i/(NUM_PTS-1.);
   mWhens[NUM_PTS-1] = 1.;
   mWhenSliders[NUMBER_OF_BANDS] = 1.;
   mEQVals[NUMBER_OF_BANDS] = 0.;

#ifdef EXPERIMENTAL_EQ_SSE_THREADED
   bool useSSE;
   GetPrivateConfig(GetCurrentSettingsGroup(), wxT("/SSE/GUI"), useSSE, false);
   if(useSSE && !mEffectEqualization48x)
      mEffectEqualization48x=new EffectEqualization48x;
   else
      if(!useSSE && mEffectEqualization48x) {
         delete mEffectEqualization48x;
         mEffectEqualization48x=NULL;
      }
   mBench=false;
#endif
}


EffectEqualization::~EffectEqualization()
{
   if(mLogEnvelope)
      delete mLogEnvelope;
   mLogEnvelope = NULL;
   if(mLinEnvelope)
      delete mLinEnvelope;
   mLinEnvelope = NULL;

   if(hFFT)
      EndFFT(hFFT);
   hFFT = NULL;
   if(mFFTBuffer)
      delete[] mFFTBuffer;
   mFFTBuffer = NULL;
   if(mFilterFuncR)
      delete[] mFilterFuncR;
   if(mFilterFuncI)
      delete[] mFilterFuncI;
   mFilterFuncR = NULL;
   mFilterFuncI = NULL;
#ifdef EXPERIMENTAL_EQ_SSE_THREADED
   if(mEffectEqualization48x)
      delete mEffectEqualization48x;
#endif
}

// IdentInterface implementation

wxString EffectEqualization::GetSymbol()
{
   return EQUALIZATION_PLUGIN_SYMBOL;
}

wxString EffectEqualization::GetDescription()
{
   return XO("Adjusts the volume levels of particular frequencies");
}

// EffectIdentInterface implementation

EffectType EffectEqualization::GetType()
{
   return EffectTypeProcess;
}

// EffectClientInterface implementation

bool EffectEqualization::GetAutomationParameters(EffectAutomationParameters & parms)
{
   parms.Write(KEY_FilterLength, mM);
   parms.Write(KEY_CurveName, mCurveName);
   parms.Write(KEY_InterpLin, mLin);
   parms.WriteEnum(KEY_InterpMeth, mInterp, wxArrayString(kNumInterpolations, kInterpStrings));

   return true;
}

bool EffectEqualization::SetAutomationParameters(EffectAutomationParameters & parms)
{
   // Pretty sure the interpolation name shouldn't have been interpreted when
   // specified in chains, but must keep it that way for compatibility.
   wxArrayString interpolations(mInterpolations);
   for (int i = 0; i < kNumInterpolations; i++)
   {
      interpolations.Add(kInterpStrings[i]);
   }

   ReadAndVerifyInt(FilterLength);
   ReadAndVerifyString(CurveName);
   ReadAndVerifyBool(InterpLin);
   ReadAndVerifyEnum(InterpMeth, interpolations);

   mM = FilterLength;
   mCurveName = CurveName;
   mLin = InterpLin;
   mInterp = InterpMeth;

   if (InterpMeth >= kNumInterpolations)
   {
      InterpMeth -= kNumInterpolations;
   }

   mEnvelope = (mLin ? mLinEnvelope : mLogEnvelope);

   return true;
}

bool EffectEqualization::LoadFactoryDefaults()
{
   mdBMin = DEF_dBMin;
   mdBMax = DEF_dBMax;
   mDrawMode = DEF_DrawMode;
   mDrawGrid = DEF_DrawGrid;

   return Effect::LoadFactoryDefaults();
}

// EffectUIClientInterface implementation

bool EffectEqualization::ValidateUI()
{
   // If editing a batch chain, we don't want to be using the unnamed curve so
   // we offer to save it.
   while (mDisallowCustom && mCurveName.IsSameAs(wxT("unnamed")))
   {
      wxMessageBox(_("To use this EQ curve in a batch chain, please choose a new name for it.\nChoose the 'Save/Manage Curves...' button and rename the 'unnamed' curve, then use that one."),
         _("EQ Curve needs a different name"),
         wxOK | wxCENTRE,
         mUIParent);
      return false;
   }

   SetPrivateConfig(GetCurrentSettingsGroup(), wxT("dBMin"), mdBMin);
   SetPrivateConfig(GetCurrentSettingsGroup(), wxT("dBMax"), mdBMax);
   SetPrivateConfig(GetCurrentSettingsGroup(), wxT("DrawMode"), mDrawMode);
   SetPrivateConfig(GetCurrentSettingsGroup(), wxT("DrawGrid"), mDrawGrid);

   return true;
}

// Effect implementation

bool EffectEqualization::Startup()
{
   wxString base = wxT("/Effects/Equalization/");

   // Migrate settings from 2.1.0 or before

   // Already migrated, so bail
   if (gPrefs->Exists(base + wxT("Migrated")))
   {
      return true;
   }

   // Load the old "current" settings
   if (gPrefs->Exists(base))
   {
      // These get saved to the current preset
      gPrefs->Read(base + wxT("FilterLength"), &mM, 4001);
      if ((mM < 21) || (mM > 8191)) {  // corrupted Prefs?
         mM = 4001;  //default
      }
      gPrefs->Read(base + wxT("CurveName"), &mCurveName, wxT("unnamed"));
      gPrefs->Read(base + wxT("Lin"), &mLin, false);
      gPrefs->Read(base + wxT("Interp"), &mInterp, 0);

      SaveUserPreset(GetCurrentSettingsGroup());

      // These persist across preset changes
      double dBMin;
      gPrefs->Read(base + wxT("dBMin"), &dBMin, -30.0);
      if ((dBMin < -120) || (dBMin > -10)) {  // corrupted Prefs?
         dBMin = -30;  //default
      }
      mdBMin = dBMin;
      SetPrivateConfig(GetCurrentSettingsGroup(), wxT("dBMin"), mdBMin);

      double dBMax;
      gPrefs->Read(base + wxT("dBMax"), &dBMax, 30.);
      if ((dBMax < 0) || (dBMax > 60)) {  // corrupted Prefs?
         dBMax = 30;  //default
      }
      mdBMax = dBMax;
      SetPrivateConfig(GetCurrentSettingsGroup(), wxT("dBMax"), mdBMax);

      gPrefs->Read(base + wxT("DrawMode"), &mDrawMode, true);
      SetPrivateConfig(GetCurrentSettingsGroup(), wxT("DrawMode"), mDrawMode);

      gPrefs->Read(base + wxT("DrawGrid"), &mDrawGrid, true);
      SetPrivateConfig(GetCurrentSettingsGroup(), wxT("DrawGrid"), mDrawGrid);

      // Do not migrate again
      gPrefs->Write(base + wxT("Migrated"), true);
      gPrefs->Flush();
   }

   return true;
}

bool EffectEqualization::Init()
{
   int selcount = 0;
   double rate = 0.0;
   TrackListIterator iter(GetActiveProject()->GetTracks());
   Track *t = iter.First();
   while (t) {
      if (t->GetSelected() && t->GetKind() == Track::Wave) {
         WaveTrack *track = (WaveTrack *)t;
         if (selcount==0) {
            rate = track->GetRate();
         }
         else {
            if (track->GetRate() != rate) {
               wxMessageBox(_("To apply Equalization, all selected tracks must have the same sample rate."));
               return(false);
            }
         }
         selcount++;
      }
      t = iter.Next();
   }

   mHiFreq = rate / 2.0;
   mLoFreq = loFreqI;

   mBandsInUse = 0;
   while (kThirdOct[mBandsInUse] <= mHiFreq) {
      mBandsInUse++;
      if (mBandsInUse == NUMBER_OF_BANDS)
         break;
   }

   mEnvelope = (mLin ? mLinEnvelope : mLogEnvelope);

   CalcFilter();

   return(true);
}

bool EffectEqualization::Process()
{
#ifdef EXPERIMENTAL_EQ_SSE_THREADED
   if(mEffectEqualization48x)
      if(mBench) {
         mBench=false;
         return mEffectEqualization48x->Benchmark(this);
      } else
         return mEffectEqualization48x->Process(this);
#endif
   this->CopyInputTracks(); // Set up mOutputTracks.
   bool bGoodResult = true;

   SelectedTrackListOfKindIterator iter(Track::Wave, mOutputTracks);
   WaveTrack *track = (WaveTrack *) iter.First();
   int count = 0;
   while (track) {
      double trackStart = track->GetStartTime();
      double trackEnd = track->GetEndTime();
      double t0 = mT0 < trackStart? trackStart: mT0;
      double t1 = mT1 > trackEnd? trackEnd: mT1;

      if (t1 > t0) {
         sampleCount start = track->TimeToLongSamples(t0);
         sampleCount end = track->TimeToLongSamples(t1);
         sampleCount len = (sampleCount)(end - start);

         if (!ProcessOne(count, track, start, len))
         {
            bGoodResult = false;
            break;
         }
      }

      track = (WaveTrack *) iter.Next();
      count++;
   }

   this->ReplaceProcessedTracks(bGoodResult);
   return bGoodResult;
}

bool EffectEqualization::PopulateUI(wxWindow *parent)
{
   mUIParent = parent;
   mUIParent->PushEventHandler(this);

   LoadUserPreset(GetCurrentSettingsGroup());

   ShuttleGui S(mUIParent, eIsCreating);
   PopulateOrExchange(S);

   return true;
}

bool EffectEqualization::CloseUI()
{
   mUIParent->RemoveEventHandler(this);

   mUIParent = NULL;

   return true;
}

void EffectEqualization::PopulateOrExchange(ShuttleGui & S)
{
   wxWindow *parent = S.GetParent();

   wxStaticText *txt;
   wxButton *btn;

   // Create the base sizer
   szrV = new wxBoxSizer( wxVERTICAL );
   szrV->AddSpacer(10);

   // -------------------------------------------------------------------
   // EQ panel and sliders for vertical scale
   // -------------------------------------------------------------------
   szr1 = new wxFlexGridSizer( 4, 0, 0 );
   szr1->AddGrowableCol( 2, 0 );
   szr1->AddGrowableRow( 0, 0 );
   szr1->SetFlexibleDirection( wxBOTH );

   szr2 = new wxBoxSizer( wxVERTICAL );
   mdBMaxSlider = new wxSlider(parent, ID_dBMax, DEF_dBMax, MIN_dBMax, MAX_dBMax,
      wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL|wxSL_INVERSE);
   szr2->Add( mdBMaxSlider, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 4 );
   mdBMinSlider = new wxSlider(parent, ID_dBMin, DEF_dBMin, MIN_dBMin, MAX_dBMin,
      wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL|wxSL_INVERSE);
   szr2->Add( mdBMinSlider, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 4 );
   szr1->Add( szr2, 0, wxEXPAND|wxALIGN_CENTRE|wxALL, 4 );

#if wxUSE_ACCESSIBILITY
   mdBMaxSlider->SetName(_("Max dB"));
   mdBMaxSlider->SetAccessible(new SliderAx(mdBMaxSlider, wxString(wxT("%d ")) + _("dB")));
   mdBMinSlider->SetName(_("Min dB"));
   mdBMinSlider->SetAccessible(new SliderAx(mdBMinSlider, wxString(wxT("%d ")) + _("dB")));
#endif

   mdBRuler = new RulerPanel(parent, wxID_ANY);
   mdBRuler->ruler.SetBounds(0, 0, 100, 100); // Ruler can't handle small sizes
   mdBRuler->ruler.SetOrientation(wxVERTICAL);
   mdBRuler->ruler.SetRange(60.0, -120.0);
   mdBRuler->ruler.SetFormat(Ruler::LinearDBFormat);
   mdBRuler->ruler.SetUnits(_("dB"));
   mdBRuler->ruler.SetLabelEdges(true);
   mdBRuler->ruler.mbTicksAtExtremes = true;
   int w, h;
   mdBRuler->ruler.GetMaxSize(&w, NULL);
   mdBRuler->SetSize(wxSize(w, 150));  // height needed for wxGTK

   szr4 = new wxBoxSizer( wxVERTICAL );
   szr4->AddSpacer(PANELBORDER); // vertical space for panel border
   szr4->Add( mdBRuler, 1, wxEXPAND|wxALIGN_LEFT|wxALL );
   szr4->AddSpacer(PANELBORDER); // vertical space for panel border
   szr1->Add( szr4, 0, wxEXPAND|wxALIGN_LEFT|wxALL );

   mPanel = new EqualizationPanel(this, parent);
   szr1->Add( mPanel, 1, wxEXPAND|wxALIGN_CENTRE);
   szr3 = new wxBoxSizer( wxVERTICAL );
   szr1->Add( szr3, 0, wxALIGN_CENTRE|wxRIGHT, 0);   //spacer for last EQ

   /// Next row of wxFlexGridSizer
   szr1->Add(1, 1); // horizontal spacer
   szr1->Add(1, 1); // horizontal spacer

   mFreqRuler  = new RulerPanel(parent, wxID_ANY);
   mFreqRuler->ruler.SetBounds(0, 0, 100, 100); // Ruler can't handle small sizes
   mFreqRuler->ruler.SetOrientation(wxHORIZONTAL);
   mFreqRuler->ruler.SetLog(true);
   mFreqRuler->ruler.SetRange(mLoFreq, mHiFreq);
   mFreqRuler->ruler.SetFormat(Ruler::IntFormat);
   mFreqRuler->ruler.SetUnits(_("Hz"));
   mFreqRuler->ruler.SetFlip(true);
   mFreqRuler->ruler.SetLabelEdges(true);
   mFreqRuler->ruler.mbTicksAtExtremes = true;
   mFreqRuler->ruler.GetMaxSize(NULL, &h);
   mFreqRuler->SetMinSize(wxSize(-1, h));
   szr5 = new wxBoxSizer( wxHORIZONTAL );
   szr5->AddSpacer(PANELBORDER); // horizontal space for panel border
   szr5->Add( mFreqRuler, 1, wxEXPAND|wxALIGN_LEFT);
   szr5->AddSpacer(PANELBORDER); // horizontal space for panel border
   szr1->Add( szr5, 0, wxEXPAND|wxALIGN_LEFT|wxALL );
   szr1->Layout();

   szrV->Add( szr1, 1, wxEXPAND|wxALIGN_CENTER|wxALL, 0 );

   // -------------------------------------------------------------------
   // Graphic EQ - parent gets laid out horizontally in onSize
   // -------------------------------------------------------------------

   szrG = new wxBoxSizer( wxHORIZONTAL  );
   szrG->Add(0, 0, 0); // horizontal spacer, will be used to position LH EQ slider
   for (int i = 0; (i < NUMBER_OF_BANDS) && (kThirdOct[i] <= mHiFreq); ++i)
   {
      mSliders[i] = new wxSlider(parent, ID_Slider + i, 0, -20, +20,
         wxDefaultPosition, wxSize(20, 124), wxSL_VERTICAL|
         wxSL_INVERSE);
      szrG->Add( mSliders[i], 0, wxEXPAND|wxALIGN_CENTER );
      szrG->Add(0, 0, 0); // horizontal spacer - used to put EQ sliders in correct position
      mSliders[i]->Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(EffectEqualization::OnErase));
      mEQVals[i] = 0.;

#if wxUSE_ACCESSIBILITY
      wxString name;
      if( kThirdOct[i] < 1000.)
         name.Printf(wxString(wxT("%d ")) + _("Hz"), (int) kThirdOct[i]);
      else
         name.Printf(wxString(wxT("%g ")) + _("kHz"), kThirdOct[i]/1000.);
      mSliders[i]->SetName(name);
      mSliders[i]->SetAccessible(new SliderAx(mSliders[i], wxString(wxT("%d ")) + _("dB")));
#endif
   }
   szrV->Add( szrG, 0, wxEXPAND|wxALIGN_LEFT|wxALL, 0 );

   wxSizerItem *EQslider = szrG->GetItem((size_t)1);
   wxSize EQsliderSize = EQslider->GetSize();   //size of the sliders
   szr3->SetMinSize(EQsliderSize.x/2, -1);   //extra gap for last slider

   // -------------------------------------------------------------------
   // Graphic or curve drawing?
   // -------------------------------------------------------------------
   szrH = new wxBoxSizer( wxHORIZONTAL );

   mDraw = new wxRadioButton(
      parent, ID_Draw, _("&Draw Curves"),
      wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
   mDraw->SetName(_("Draw Curves"));
   szrH->Add( mDraw, 0, wxRIGHT, 10 );

   mGraphic = new wxRadioButton(
      parent, ID_Graphic, _("&Graphic EQ"),
      wxDefaultPosition, wxDefaultSize, 0 );
   mGraphic->SetName(_("Graphic EQ"));
   szrH->Add( mGraphic, 0, wxRIGHT, 4 );

   mInterpChoice = new wxChoice(parent, ID_Interp,
      wxDefaultPosition, wxDefaultSize, mInterpolations);

   mInterpChoice->SetSelection(0);
   szrI = new wxBoxSizer( wxHORIZONTAL );
   szrI->Add( mInterpChoice, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 40 );
   szrH->Add( szrI );

   szrL = new wxBoxSizer( wxHORIZONTAL );
   mLinFreq = new wxCheckBox(parent, ID_Linear, _("Li&near Frequency Scale"));
   mLinFreq->SetName(_("Linear Frequency Scale"));
   szrL->Add( mLinFreq, 0 );
   szrH->Add(szrL);  // either szrI or szrL are visible, not both.

   // -------------------------------------------------------------------
   // Filter length grouping
   // -------------------------------------------------------------------

   // length of filter (M) label
   txt = new wxStaticText(parent, wxID_ANY, _("Length of &Filter:"));
   szrH->Add( txt, 0 );

   // length of filter (M) slider
   mMSlider = new wxSlider(parent, ID_Length, (mM -1)/2, 10, 4095,
      wxDefaultPosition, wxSize(200, -1), wxSL_HORIZONTAL);
   mMSlider->SetName(_("Length of Filter"));
   szrH->Add( mMSlider, 0, wxEXPAND );

   wxString label;
   label.Printf( wxT("%d"), mM );
   mMText = new wxStaticText(parent, wxID_ANY, label);
   mMText->SetName(label); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   szrH->Add( mMText, 0 );

   // Add the length / graphic / draw grouping
   szrV->Add( szrH, 0, wxALIGN_CENTER | wxALL, 4 );

   // -------------------------------------------------------------------
   // Curve management grouping
   // -------------------------------------------------------------------
   szrC = new wxBoxSizer( wxHORIZONTAL );   //szrC is for the curves bits

   txt = new wxStaticText( parent, wxID_ANY, _("&Select Curve:") );
   szrC->Add( txt, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, 4 );

   mCurve = new wxChoice( parent, ID_Curve );
   szrC->Add( mCurve, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, 4 );

   mManage = new wxButton( parent, ID_Manage, _("S&ave/Manage Curves...") );
   mManage->SetName(_("Save and Manage Curves"));
   szrC->Add( mManage, 0, wxALIGN_CENTRE|wxLEFT, 4 );

   btn = new wxButton( parent, ID_Clear, _("Fla&tten"));
   szrC->Add( btn, 0, wxALIGN_CENTRE | wxALL, 4 );
   btn = new wxButton( parent, ID_Invert, _("&Invert"));
   szrC->Add( btn, 0, wxALIGN_CENTRE | wxALL, 4 );
   mGridOnOff = new wxCheckBox(parent, ID_Grid, _("G&rids"),
      wxDefaultPosition, wxDefaultSize,
#if defined(__WXGTK__)
      // Fixes bug #662
      wxALIGN_LEFT);
#else
      wxALIGN_RIGHT);
#endif
   mGridOnOff->SetName(_("Grids"));
   szrC->Add( mGridOnOff, 0, wxALIGN_CENTRE | wxALL, 4 );

   szrV->Add( szrC, 0, wxALIGN_CENTER | wxALL, 0 );

#ifdef EXPERIMENTAL_EQ_SSE_THREADED
   // -------------------------------------------------------------------
   // Processing routine selection
   // -------------------------------------------------------------------
   if(mEffectEqualization48x) {
      szrM = new wxBoxSizer( wxHORIZONTAL );
      txt = new wxStaticText( parent, wxID_ANY, _("&Processing: ") );
      szrM->Add( txt, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, 4 );

      mMathProcessingType[0] = new wxRadioButton(
         parent, ID_DefaultMath, _("D&efault"),
         wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
      mMathProcessingType[0]->SetName(_("Default"));
      szrM->Add( mMathProcessingType[0], 0, wxRIGHT, 10 );

      mMathProcessingType[1] = new wxRadioButton(
         parent, ID_SSE, _("&SSE"),
         wxDefaultPosition, wxDefaultSize, 0 );
      mMathProcessingType[1]->SetName(_("SSE"));
      szrM->Add( mMathProcessingType[1], 0, wxRIGHT, 4 );

      mMathProcessingType[2] = new wxRadioButton(
         parent, ID_SSE_Threaded, _("SSE &Threaded"),
         wxDefaultPosition, wxDefaultSize, 0 );
      mMathProcessingType[2]->SetName(_("SSE"));
      szrM->Add( mMathProcessingType[2], 0, wxRIGHT, 4 );

      mMathProcessingType[3] = new wxRadioButton(
         parent, ID_AVX, _("A&VX"),
         wxDefaultPosition, wxDefaultSize, 0 );
      mMathProcessingType[3]->SetName(_("AVX"));
      szrM->Add( mMathProcessingType[3], 0, wxRIGHT, 4 );

      mMathProcessingType[4] = new wxRadioButton(
         parent, ID_AVX_Threaded, _("AV&X Threaded"),
         wxDefaultPosition, wxDefaultSize, 0 );
      mMathProcessingType[4]->SetName(_("AVX Threaded"));
      szrM->Add( mMathProcessingType[4], 0, wxRIGHT, 4 );

      if(!EffectEqualization48x::GetMathCaps()->SSE) {
         mMathProcessingType[1]->Disable();
         mMathProcessingType[2]->Disable();
      }
      if(true) { //!EffectEqualization48x::GetMathCaps()->AVX) { not implemented
         mMathProcessingType[3]->Disable();
         mMathProcessingType[4]->Disable();
      }
      // update the control state
      mMathProcessingType[0]->SetValue(true);
      int mathPath=EffectEqualization48x::GetMathPath();
      if(mathPath&MATH_FUNCTION_SSE) {
         mMathProcessingType[1]->SetValue(true);
         if(mathPath&MATH_FUNCTION_THREADED)
            mMathProcessingType[2]->SetValue(true);
      }
      if(false) { //mathPath&MATH_FUNCTION_AVX) { not implemented
         mMathProcessingType[3]->SetValue(true);
         if(mathPath&MATH_FUNCTION_THREADED)
            mMathProcessingType[4]->SetValue(true);
      }
      btn = new wxButton( parent, ID_Bench, _("&Bench"));
      szrM->Add( btn, 0, wxRIGHT, 4 );

      szrV->Add( szrM, 0, wxALIGN_CENTER | wxALL, 0 );
   }
#endif

   wxGetTopLevelParent(parent)->SetAutoLayout(true);

   szrV->Show(szrG, true);
   szrH->Show(szrI, true);
   szrH->Show(szrL, false);

   parent->SetSizer(szrV);
   szrV->SetSizeHints(parent);
   
   szrL->SetMinSize(szrI->GetSize());

   szrV->Show(szrC, true);
   szrV->Show(szrG, false);
   szrH->Show(szrI, false);
   szrH->Show(szrL, true);

   return;
}

//
// Populate the window with relevant variables
//
bool EffectEqualization::TransferDataToWindow()
{
   // Start with a clean slate
   Flatten();
   mDirty = false;

   // Set log or lin freq scale (affects interpolation as well)
   mLinFreq->SetValue( mLin );
   wxCommandEvent dummyEvent;
   OnLinFreq(dummyEvent);  // causes a CalcFilter

   mGridOnOff->SetValue( mDrawGrid ); // checks/unchecks the box on the interface

   mMSlider->SetValue((mM-1)/2);
   mM = 0;                        // force refresh in TransferDataFromWindow()

   mdBMinSlider->SetValue((int)mdBMin);
   mdBMin = 0;                     // force refresh in TransferDataFromWindow()

   mdBMaxSlider->SetValue((int)mdBMax);
   mdBMax = 0;                    // force refresh in TransferDataFromWindow()

   // Reload the curve names
   UpdateCurves();

   // Set graphic interpolation mode
   mInterpChoice->SetSelection(mInterp);

   // Set Graphic (Fader) or Draw mode
   if (mDrawMode)
   {
      mDraw->SetValue(true);
      UpdateDraw();
   }
   else
   {
      mGraphic->SetValue(true);
      UpdateGraphic();
   }

   TransferDataFromWindow();

   mUIParent->Layout();
   wxGetTopLevelParent(mUIParent)->Layout();

   return true;
}

//
// Retrieve data from the window
//
bool EffectEqualization::TransferDataFromWindow()
{
   wxString tip;

   bool rr = false;
   float dB = (float) mdBMinSlider->GetValue();
   if (dB != mdBMin) {
      rr = true;
      mdBMin = dB;
      tip.Printf(wxString(wxT("%d ")) + _("dB"),(int)mdBMin);
      mdBMinSlider->SetToolTip(tip);
   }

   dB = (float) mdBMaxSlider->GetValue();
   if (dB != mdBMax) {
      rr = true;
      mdBMax = dB;
      tip.Printf(wxString(wxT("%d ")) + _("dB"),(int)mdBMax);
      mdBMaxSlider->SetToolTip(tip);
   }

   // Refresh ruler if values have changed
   if (rr) {
      int w1, w2, h;
      mdBRuler->ruler.GetMaxSize(&w1, &h);
      mdBRuler->ruler.SetRange(mdBMax, mdBMin);
      mdBRuler->ruler.GetMaxSize(&w2, &h);
      if( w1 != w2 )   // Reduces flicker
      {
         mdBRuler->SetSize(wxSize(w2,h));
         szr1->Layout();
         LayoutEQSliders();
         szrG->Layout();
         mFreqRuler->Refresh(false);
      }
      mdBRuler->Refresh(false);

      mPanel->Refresh(false);
   }

   int m = 2 * mMSlider->GetValue() + 1;   // odd numbers only
   if (m != mM) {
      mM = m;
      mPanel->ForceRecalc();

      tip.Printf(wxT("%d"), mM);
      mMText->SetLabel(tip);
      mMText->SetName(mMText->GetLabel()); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
      mMSlider->SetToolTip(tip);
   }

   return true;
}

// EffectEqualization implementation

bool EffectEqualization::ProcessOne(int count, WaveTrack * t,
                                    sampleCount start, sampleCount len)
{
   // create a new WaveTrack to hold all of the output, including 'tails' each end
   AudacityProject *p = GetActiveProject();
   WaveTrack *output = p->GetTrackFactory()->NewWaveTrack(floatSample, t->GetRate());

   int L = windowSize - (mM - 1);   //Process L samples at a go
   sampleCount s = start;
   sampleCount idealBlockLen = t->GetMaxBlockSize() * 4;
   if (idealBlockLen % L != 0)
      idealBlockLen += (L - (idealBlockLen % L));

   float *buffer = new float[idealBlockLen];

   float *window1 = new float[windowSize];
   float *window2 = new float[windowSize];
   float *thisWindow = window1;
   float *lastWindow = window2;

   sampleCount originalLen = len;

   int i,j;
   for(i=0; i<windowSize; i++)
      lastWindow[i] = 0;

   TrackProgress(count, 0.);
   bool bLoopSuccess = true;
   int wcopy = 0;
   int offset = (mM - 1)/2;

   while(len)
   {
      sampleCount block = idealBlockLen;
      if (block > len)
         block = len;

      t->Get((samplePtr)buffer, floatSample, s, block);

      for(i=0; i<block; i+=L)   //go through block in lumps of length L
      {
         wcopy = L;
         if (i + wcopy > block)   //if last lump would exceed block
            wcopy = block - i;   //shorten it
         for(j=0; j<wcopy; j++)
            thisWindow[j] = buffer[i+j];   //copy the L (or remaining) samples
         for(j=wcopy; j<windowSize; j++)
            thisWindow[j] = 0;   //this includes the padding

         Filter(windowSize, thisWindow);

         // Overlap - Add
         for(j=0; (j<mM-1) && (j<wcopy); j++)
            buffer[i+j] = thisWindow[j] + lastWindow[L + j];
         for(j=mM-1; j<wcopy; j++)
            buffer[i+j] = thisWindow[j];

         float *tempP = thisWindow;
         thisWindow = lastWindow;
         lastWindow = tempP;
      }  //next i, lump of this block

      output->Append((samplePtr)buffer, floatSample, block);
      len -= block;
      s += block;

      if (TrackProgress(count, (s-start)/(double)originalLen))
      {
         bLoopSuccess = false;
         break;
      }
   }

   if(bLoopSuccess)
   {
      // mM-1 samples of 'tail' left in lastWindow, get them now
      if(wcopy < (mM-1)) {
         // Still have some overlap left to process
         // (note that lastWindow and thisWindow have been exchanged at this point
         //  so that 'thisWindow' is really the window prior to 'lastWindow')
         for(j=0; j<mM-1-wcopy; j++)
            buffer[j] = lastWindow[wcopy + j] + thisWindow[L + wcopy + j];
         // And fill in the remainder after the overlap
         for( ; j<mM-1; j++)
            buffer[j] = lastWindow[wcopy + j];
      } else {
         for(j=0; j<mM-1; j++)
            buffer[j] = lastWindow[wcopy + j];
      }
      output->Append((samplePtr)buffer, floatSample, mM-1);
      output->Flush();

      // now move the appropriate bit of the output back to the track
      // (this could be enhanced in the future to use the tails)
      double offsetT0 = t->LongSamplesToTime((sampleCount)offset);
      double lenT = t->LongSamplesToTime(originalLen);
      // 'start' is the sample offset in 't', the passed in track
      // 'startT' is the equivalent time value
      // 'output' starts at zero
      double startT = t->LongSamplesToTime(start);

      //output has one waveclip for the total length, even though
      //t might have whitespace seperating multiple clips
      //we want to maintain the original clip structure, so
      //only paste the intersections of the new clip.

      //Find the bits of clips that need replacing
      std::vector<std::pair<double, double> > clipStartEndTimes;
      std::vector<std::pair<double, double> > clipRealStartEndTimes; //the above may be truncated due to a clip being partially selected
      for (WaveClipList::compatibility_iterator it=t->GetClipIterator(); it; it=it->GetNext())
      {
         WaveClip *clip;
         double clipStartT;
         double clipEndT;

         clip = it->GetData();
         clipStartT = clip->GetStartTime();
         clipEndT = clip->GetEndTime();
         if( clipEndT <= startT )
            continue;   // clip is not within selection
         if( clipStartT >= startT + lenT )
            continue;   // clip is not within selection

         //save the actual clip start/end so that we can rejoin them after we paste.
         clipRealStartEndTimes.push_back(std::pair<double,double>(clipStartT,clipEndT));

         if( clipStartT < startT )  // does selection cover the whole clip?
            clipStartT = startT; // don't copy all the new clip
         if( clipEndT > startT + lenT )  // does selection cover the whole clip?
            clipEndT = startT + lenT; // don't copy all the new clip

         //save them
         clipStartEndTimes.push_back(std::pair<double,double>(clipStartT,clipEndT));
      }
      //now go thru and replace the old clips with new
      for(unsigned int i=0;i<clipStartEndTimes.size();i++)
      {
         Track *toClipOutput;
         //remove the old audio and get the new
         t->Clear(clipStartEndTimes[i].first,clipStartEndTimes[i].second);
         output->Copy(clipStartEndTimes[i].first-startT+offsetT0,clipStartEndTimes[i].second-startT+offsetT0, &toClipOutput);
         if(toClipOutput)
         {
            //put the processed audio in
            bool bResult = t->Paste(clipStartEndTimes[i].first, toClipOutput);
            wxASSERT(bResult); // TO DO: Actually handle this.
            //if the clip was only partially selected, the Paste will have created a split line.  Join is needed to take care of this
            //This is not true when the selection is fully contained within one clip (second half of conditional)
            if( (clipRealStartEndTimes[i].first  != clipStartEndTimes[i].first ||
               clipRealStartEndTimes[i].second != clipStartEndTimes[i].second) &&
               !(clipRealStartEndTimes[i].first <= startT &&
               clipRealStartEndTimes[i].second >= startT+lenT) )
               t->Join(clipRealStartEndTimes[i].first,clipRealStartEndTimes[i].second);
            delete toClipOutput;
         }
      }
   }

   delete[] buffer;
   delete[] window1;
   delete[] window2;
   delete output;

   return bLoopSuccess;
}

bool EffectEqualization::CalcFilter()
{
   double loLog = log10(mLoFreq);
   double hiLog = log10(mHiFreq);
   double denom = hiLog - loLog;

   double delta = mHiFreq / ((double)(mWindowSize/2.));
   double val0;
   double val1;

   bool lin = mDrawMode && mLin;

   if( lin )
   {
      val0 = mLinEnvelope->GetValue(0.0);   //no scaling required - saved as dB
      val1 = mLinEnvelope->GetValue(1.0);
   }
   else
   {
      val0 = mLogEnvelope->GetValue(0.0);   //no scaling required - saved as dB
      val1 = mLogEnvelope->GetValue(1.0);
   }
   mFilterFuncR[0] = val0;
   double freq = delta;

   int i;
   for(i=1; i<=mWindowSize/2; i++)
   {
      double when;
      if( lin )
         when = freq/mHiFreq;
      else
         when = (log10(freq) - loLog)/denom;
      if(when < 0.)
      {
         mFilterFuncR[i] = val0;
      }
      else  if(when > 1.0)
      {
         mFilterFuncR[i] = val1;
      }
      else
      {
         if( lin )
            mFilterFuncR[i] = mLinEnvelope->GetValue(when);
         else
            mFilterFuncR[i] = mLogEnvelope->GetValue(when);
      }
      freq += delta;
   }
   mFilterFuncR[mWindowSize/2] = val1;

   mFilterFuncR[0] = (float)(pow(10., mFilterFuncR[0]/20.));
   for(i=1;i<mWindowSize/2;i++)
   {
      mFilterFuncR[i] = (float)(pow(10., mFilterFuncR[i]/20.));
      mFilterFuncR[mWindowSize-i]=mFilterFuncR[i];   //Fill entire array
   }
   mFilterFuncR[i] = (float)(pow(10., mFilterFuncR[i]/20.));   //do last one

   //transfer to time domain to do the padding and windowing
   float *outr = new float[mWindowSize];
   float *outi = new float[mWindowSize];
#ifdef EXPERIMENTAL_USE_REALFFTF
   InverseRealFFT(mWindowSize, mFilterFuncR, NULL, outr); // To time domain
#else
   FFT(mWindowSize,true,mFilterFuncR,NULL,outr,outi);   //To time domain
#endif

   for(i=0;i<=(mM-1)/2;i++)
   {  //Windowing - could give a choice, fixed for now - MJS
      //      double mult=0.54-0.46*cos(2*M_PI*(i+(mM-1)/2.0)/(mM-1));   //Hamming
      //Blackman
      double mult=0.42-0.5*cos(2*M_PI*(i+(mM-1)/2.0)/(mM-1))+.08*cos(4*M_PI*(i+(mM-1)/2.0)/(mM-1));
      outr[i]*=mult;
      if(i!=0){
         outr[mWindowSize-i]*=mult;
      }
   }
   for(;i<=mWindowSize/2;i++)
   {   //Padding
      outr[i]=0;
      outr[mWindowSize-i]=0;
   }
   float *tempr = new float[mM];
   for(i=0;i<(mM-1)/2;i++)
   {   //shift so that padding on right
      tempr[(mM-1)/2+i]=outr[i];
      tempr[i]=outr[mWindowSize-(mM-1)/2+i];
   }
   tempr[(mM-1)/2+i]=outr[i];

   for(i=0;i<mM;i++)
   {   //and copy useful values back
      outr[i]=tempr[i];
   }
   for(i=mM;i<mWindowSize;i++)
   {   //rest is padding
      outr[i]=0.;
   }

   //Back to the frequency domain so we can use it
   RealFFT(mWindowSize,outr,mFilterFuncR,mFilterFuncI);

   delete[] outr;
   delete[] outi;
   delete[] tempr;

   return TRUE;
}

void EffectEqualization::Filter(sampleCount len, float *buffer)
{
   int i;
   float re,im;
   // Apply FFT
   RealFFTf(buffer, hFFT);
   //FFT(len, false, inr, NULL, outr, outi);

   // Apply filter
   // DC component is purely real
   mFFTBuffer[0] = buffer[0] * mFilterFuncR[0];
   for(i=1; i<(len/2); i++)
   {
      re=buffer[hFFT->BitReversed[i]  ];
      im=buffer[hFFT->BitReversed[i]+1];
      mFFTBuffer[2*i  ] = re*mFilterFuncR[i] - im*mFilterFuncI[i];
      mFFTBuffer[2*i+1] = re*mFilterFuncI[i] + im*mFilterFuncR[i];
   }
   // Fs/2 component is purely real
   mFFTBuffer[1] = buffer[1] * mFilterFuncR[len/2];

   // Inverse FFT and normalization
   InverseRealFFTf(mFFTBuffer, hFFT);
   ReorderToTime(hFFT, mFFTBuffer, buffer);
}

//
// Load external curves with fallback to default, then message
//
void EffectEqualization::LoadCurves(wxString fileName, bool append)
{
   // Construct normal curve filename
   //
   // LLL:  Wouldn't you know that as of WX 2.6.2, there is a conflict
   //       between wxStandardPaths and wxConfig under Linux.  The latter
   //       creates a normal file as "$HOME/.audacity", while the former
   //       expects the ".audacity" portion to be a directory.
   // MJS:  I don't know what the above means, or if I have broken it.
   wxFileName fn;
   if(fileName == wxT(""))
      fn = wxFileName( FileNames::DataDir(), wxT("EQCurves.xml") );
   else
      fn = wxFileName(fileName); // user is loading a specific set of curves

   // If requested file doesn't exist...
   if( !fn.FileExists() )
   {
      // look in data dir first, in case the user has their own defaults (maybe downloaded ones)
      fn = wxFileName( FileNames::DataDir(), wxT("EQDefaultCurves.xml") );
      if( !fn.FileExists() )
      {  // Default file not found in the data dir.  Fall back to Resources dir.
         // See http://docs.wxwidgets.org/trunk/classwx_standard_paths.html#5514bf6288ee9f5a0acaf065762ad95d
         static wxString resourcesDir;
         resourcesDir = wxStandardPaths::Get().GetResourcesDir();
         fn = wxFileName( resourcesDir, wxT("EQDefaultCurves.xml") );
      }
      if( !fn.FileExists() )
      {
         wxString errorMessage;
         errorMessage.Printf(_("EQCurves.xml and EQDefaultCurves.xml were not found on your system.\nPlease press 'help' to visit the download page.\n\nSave the curves at %s"), FileNames::DataDir().c_str());
         ShowErrorDialog(mUIParent, _("EQCurves.xml and EQDefaultCurves.xml missing"),
            errorMessage, wxT("http://wiki.audacityteam.org/wiki/EQCurvesDownload"), false);
         // Have another go at finding EQCurves.xml in the data dir, in case 'help' helped
         fn = wxFileName( FileNames::DataDir(), wxT("EQDefaultCurves.xml") );
         if( !fn.FileExists() )
         {
            mCurves.Add( _("unnamed") );   // we still need a default curve to use
            return;
         }
      }
   }

   EQCurve tempCustom(wxT("temp"));
   if( append == false) // Start from scratch
      mCurves.Clear();
   else  // appending so copy and remove 'unnamed', to replace later
   {
      tempCustom.points = mCurves.Last().points;
      mCurves.RemoveAt(mCurves.Count()-1);
   }

   // Load the curves
   XMLFileReader reader;
   if( !reader.Parse( this, fn.GetFullPath() ) )
   {
      wxString msg;
      /* i18n-hint: EQ stands for 'Equalization'.*/
      msg.Printf(_("Error Loading EQ Curves from file:\n%s\nError message says:\n%s"), fn.GetFullPath().c_str(), reader.GetErrorStr().c_str());
      // Inform user of load failure
      wxMessageBox( msg,
         _("Error Loading EQ Curves"),
         wxOK | wxCENTRE);
      mCurves.Add( _("unnamed") );  // we always need a default curve to use
      return;
   }

   // Move "unnamed" to end, if it exists in current language.
   int numCurves = mCurves.GetCount();
   int curve;
   EQCurve tempUnnamed(wxT("tempUnnamed"));
   for( curve = 0; curve < numCurves-1; curve++ )
   {
      if( mCurves[curve].Name == _("unnamed") )
      {
         tempUnnamed.points = mCurves[curve].points;
         mCurves.RemoveAt(curve);
         mCurves.Add( _("unnamed") );   // add 'unnamed' back at the end
         mCurves.Last().points = tempUnnamed.points;
      }
   }

   if( mCurves.Last().Name != _("unnamed") )
      mCurves.Add( _("unnamed") );   // we always need a default curve to use
   if( append == true )
   {
      mCurves.Last().points = tempCustom.points;
   }

   return;
}

//
// Save curves to external file
//
void EffectEqualization::SaveCurves(wxString fileName)
{
   wxFileName fn;
   if( fileName == wxT(""))
   {
      // Construct default curve filename
      //
      // LLL:  Wouldn't you know that as of WX 2.6.2, there is a conflict
      //       between wxStandardPaths and wxConfig under Linux.  The latter
      //       creates a normal file as "$HOME/.audacity", while the former
      //       expects the ".audacity" portion to be a directory.
      fn = wxFileName( FileNames::DataDir(), wxT("EQCurves.xml") );

      // If the directory doesn't exist...
      if( !fn.DirExists() )
      {
         // Attempt to create it
         if( !fn.Mkdir( fn.GetPath(), 511, wxPATH_MKDIR_FULL ) )
         {
            // MkDir() will emit message
            return;
         }
      }
   }
   else
      fn = wxFileName(fileName);

   // Create/Open the file
   XMLFileWriter eqFile;

   try
   {
      eqFile.Open( fn.GetFullPath(), wxT("wb") );

      // Write the curves
      WriteXML( eqFile );

      // Close the file
      eqFile.Close();
   }
   catch (XMLFileWriterException* pException)
   {
      wxMessageBox(wxString::Format(
         _("Couldn't write to file \"%s\": %s"),
         fn.GetFullPath().c_str(), pException->GetMessage().c_str()),
         _("Error Saving Equalization Curves"), wxICON_ERROR, mUIParent);

      delete pException;
   }
}

//
// Make the passed curve index the active one
//
void EffectEqualization::setCurve(int currentCurve)
{
   // Set current choice
   Select( currentCurve );

   wxASSERT( currentCurve < (int) mCurves.GetCount() );
   bool changed = false;

   if( mLin )   // linear freq mode?
   {
      Envelope *env = mLinEnvelope;
      env->Flatten(0.);
      env->SetTrackLen(1.0);

      if( mCurves[currentCurve].points.GetCount() )
      {
         double when, value;
         int i;
         int nCurvePoints = mCurves[currentCurve].points.GetCount();
         for(i=0;i<nCurvePoints;i++)
         {
            when = mCurves[currentCurve].points[i].Freq / mHiFreq;
            value = mCurves[currentCurve].points[i].dB;
            if(when <= 1)
               env->Insert(when, value);
            else
               break;
         }
         if ( i != nCurvePoints) // there are more points at higher freqs
         {
            when = 1.;  // set the RH end to the next highest point
            value = mCurves[currentCurve].points[nCurvePoints-1].dB;
            env->Insert(when, value);
            changed = true;
         }
      }
   }
   else
   {
      Envelope *env = mLogEnvelope;
      env->Flatten(0.);
      env->SetTrackLen(1.0);

      if( mCurves[currentCurve].points.GetCount() )
      {
         double when, value;
         double loLog = log10(20.);
         double hiLog = log10(mHiFreq);
         double denom = hiLog - loLog;
         int i;
         int nCurvePoints = mCurves[currentCurve].points.GetCount();

         for(i=0;i<nCurvePoints;i++)
         {
            if( mCurves[currentCurve].points[i].Freq >= 20)
            {
               when = (log10(mCurves[currentCurve].points[i].Freq) - loLog)/denom;
               value = mCurves[currentCurve].points[i].dB;
               if(when <= 1)
                  env->Insert(when, value);
               else
               {  // we have a point beyond fs/2.  Insert it so that env code can use it.
                  // but just this one, we have no use for the rest
                  env->SetTrackLen(when); // can't Insert if the envelope isn't long enough
                  env->Insert(when, value);
                  break;
               }
            }
            else
            {  //get the first point as close as we can to the last point requested
               changed = true;
               //double f = mCurves[currentCurve].points[i].Freq;
               //double v = mCurves[currentCurve].points[i].dB;
               mLogEnvelope->Insert(0., mCurves[currentCurve].points[i].dB);
            }
         }
      }
   }
   if(changed) // not all points were loaded so switch to unnamed
      EnvelopeUpdated();
   mPanel->ForceRecalc();
}

void EffectEqualization::setCurve()
{
   setCurve((int) mCurves.GetCount()-1);
}

void EffectEqualization::setCurve(wxString curveName)
{
   unsigned i = 0;
   for( i = 0; i < mCurves.GetCount(); i++ )
      if( curveName == mCurves[ i ].Name )
         break;
   if( i == mCurves.GetCount())
   {
      wxMessageBox( _("Requested curve not found, using 'unnamed'"), _("Curve not found"), wxOK|wxICON_ERROR );
      setCurve((int) mCurves.GetCount()-1);
   }
   else
      setCurve( i );
}

//
// Set new curve selection and manage state of delete button
//
void EffectEqualization::Select( int curve )
{
   // Set current choice
   mCurve->SetSelection( curve );
   mCurveName = mCurves[ curve ].Name;
}

//
// Capture updated envelope
//
void EffectEqualization::EnvelopeUpdated()
{
   if (mDraw && mLin)
   {
      EnvelopeUpdated(mLinEnvelope, true);
   }
   else
   {
      EnvelopeUpdated(mLogEnvelope, false);
   }
}

void EffectEqualization::EnvelopeUpdated(Envelope *env, bool lin)
{
   // Allocate and populate point arrays
   int numPoints = env->GetNumberOfPoints();
   double *when = new double[ numPoints ];
   double *value = new double[ numPoints ];
   env->GetPoints( when, value, numPoints );

   // Clear the unnamed curve
   int curve = mCurves.GetCount()-1;
   mCurves[ curve ].points.Clear();

   if(lin)
   {
      // Copy and convert points
      int point;
      for( point = 0; point < numPoints; point++ )
      {
         double freq = when[ point ] * mHiFreq;
         double db = value[ point ];

         // Add it to the curve
         mCurves[ curve ].points.Add( EQPoint( freq, db ) );
      }
   }
   else
   {
      double loLog = log10( 20. );
      double hiLog = log10( mHiFreq );
      double denom = hiLog - loLog;

      // Copy and convert points
      int point;
      for( point = 0; point < numPoints; point++ )
      {
         double freq = pow( 10., ( ( when[ point ] * denom ) + loLog ));
         double db = value[ point ];

         // Add it to the curve
         mCurves[ curve ].points.Add( EQPoint( freq, db ) );
      }
   }
   // Remember that we've updated the unnamed curve
   mDirty = true;

   // set 'unnamed' as the selected curve
   Select( (int) mCurves.GetCount()-1 );

   // Clean up
   delete [] when;
   delete [] value;
}

//
// Flatten the curve
//
void EffectEqualization::Flatten()
{
   mLogEnvelope->Flatten(0.);
   mLogEnvelope->SetTrackLen(1.0);
   mLinEnvelope->Flatten(0.);
   mLinEnvelope->SetTrackLen(1.0);
   mPanel->ForceRecalc();
   if( !mDrawMode )
   {
      for( int i=0; i< mBandsInUse; i++)
      {
         mSliders[i]->SetValue(0);
         mSlidersOld[i] = 0;
         mEQVals[i] = 0.;

         wxString tip;
         if( kThirdOct[i] < 1000.)
            tip.Printf( wxT("%dHz\n%.1fdB"), (int)kThirdOct[i], 0. );
         else
            tip.Printf( wxT("%gkHz\n%.1fdB"), kThirdOct[i]/1000., 0. );
         mSliders[i]->SetToolTip(tip);
      }
   }
   EnvelopeUpdated();
}

//
// Process XML tags and handle the ones we recognize
//
bool EffectEqualization::HandleXMLTag(const wxChar *tag, const wxChar **attrs)
{
   // May want to add a version strings...
   if( !wxStrcmp( tag, wxT("equalizationeffect") ) )
   {
      return true;
   }

   // Located a new curve
   if( !wxStrcmp(tag, wxT("curve") ) )
   {
      // Process the attributes
      while( *attrs )
      {
         // Cache attr/value and bump to next
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         // Create a new curve and name it
         if( !wxStrcmp( attr, wxT("name") ) )
         {
            const wxString strValue = value;
            if (!XMLValueChecker::IsGoodString(strValue))
               return false;
            // check for a duplicate name and add (n) if there is one
            int n = 0;
            wxString strValueTemp = strValue;
            bool exists;
            do
            {
               exists = false;
               for(size_t i=0;i<mCurves.GetCount();i++)
               {
                  if(n>0)
                     strValueTemp.Printf(wxT("%s (%d)"),strValue.c_str(),n);
                  if(mCurves[i].Name == strValueTemp)
                  {
                     exists = true;
                     break;
                  }
               }
               n++;
            }
            while(exists == true);

            mCurves.Add( EQCurve( strValueTemp ) );
         }
      }

      // Tell caller it was processed
      return true;
   }

   // Located a new point
   if( !wxStrcmp( tag, wxT("point") ) )
   {
      // Set defaults in case attributes are missing
      double f = 0.0;
      double d = 0.0;

      // Process the attributes
      double dblValue;
      while( *attrs )
      {   // Cache attr/value and bump to next
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         const wxString strValue = value;

         // Get the frequency
         if( !wxStrcmp( attr, wxT("f") ) )
         {
            if (!XMLValueChecker::IsGoodString(strValue) ||
               !Internat::CompatibleToDouble(strValue, &dblValue))
               return false;
            f = dblValue;
         }
         // Get the dB
         else if( !wxStrcmp( attr, wxT("d") ) )
         {
            if (!XMLValueChecker::IsGoodString(strValue) ||
               !Internat::CompatibleToDouble(strValue, &dblValue))
               return false;
            d = dblValue;
         }
      }

      // Create a new point
      mCurves[ mCurves.GetCount() - 1 ].points.Add( EQPoint( f, d ) );

      // Tell caller it was processed
      return true;
   }

   // Tell caller we didn't understand the tag
   return false;
}

//
// Return handler for recognized tags
//
XMLTagHandler *EffectEqualization::HandleXMLChild(const wxChar *tag)
{
   if( !wxStrcmp( tag, wxT("equalizationeffect") ) )
   {
      return this;
   }

   if( !wxStrcmp( tag, wxT("curve") ) )
   {
      return this;
   }

   if( !wxStrcmp( tag, wxT("point") ) )
   {
      return this;
   }

   return NULL;
}

//
// Write all of the curves to the XML file
//
void EffectEqualization::WriteXML(XMLWriter &xmlFile)
{
   // Start our heirarchy
   xmlFile.StartTag( wxT( "equalizationeffect" ) );

   // Write all curves
   int numCurves = mCurves.GetCount();
   int curve;
   for( curve = 0; curve < numCurves; curve++ )
   {
      // Start a new curve
      xmlFile.StartTag( wxT( "curve" ) );
      xmlFile.WriteAttr( wxT( "name" ), mCurves[ curve ].Name );

      // Write all points
      int numPoints = mCurves[ curve ].points.GetCount();
      int point;
      for( point = 0; point < numPoints; point++ )
      {
         // Write new point
         xmlFile.StartTag( wxT( "point" ) );
         xmlFile.WriteAttr( wxT( "f" ), mCurves[ curve ].points[ point ].Freq, 12 );
         xmlFile.WriteAttr( wxT( "d" ), mCurves[ curve ].points[ point ].dB, 12 );
         xmlFile.EndTag( wxT( "point" ) );
      }

      // Terminate curve
      xmlFile.EndTag( wxT( "curve" ) );
   }

   // Terminate our heirarchy
   xmlFile.EndTag( wxT( "equalizationeffect" ) );
}

///////////////////////////////////////////////////////////////////////////////
//
// All EffectEqualization methods beyond this point interact with the UI, so
// can't be called while the UI is not displayed.
//
///////////////////////////////////////////////////////////////////////////////

void EffectEqualization::LayoutEQSliders()
{
   //layout the Graphic EQ sizer here
   wxSize szrGSize = szrG->GetSize();   //total size we have to play with
   wxSize szr2Size = szr2->GetSize();   //size of the dBMax/Min sliders
   wxSizerItem *ruler = szr1->GetItem((size_t)1);
   wxSize rulerSize = ruler->GetSize();   //and the ruler
   wxSizerItem *EQslider = szrG->GetItem((size_t)1);
   wxSize EQsliderSize = EQslider->GetSize();   //size of the sliders

#if defined(__WXMAC__)
   // LL: 2010-01-04 - Don't know why, but on the Mac, the rightmost sliders
   // will wind up off the edge of the window since they get spaced out too
   // much.  Somewhere, there's an extra 2 pixels in slider width that's not
   // being accounted for.  (I guess)
   EQsliderSize.x += 2;
#endif

   int start, w, range, so_far;
   start = szr2Size.x + rulerSize.x + 12;   //inc ruler & mPanel border (4+4 + 4)
   szrG->SetItemMinSize((size_t)0, start - EQsliderSize.x/2, -1);   //set 1st spacer so that 1st slider aligned with ruler
   range = szrGSize.x - EQsliderSize.x/2 - start;
   so_far = start + EQsliderSize.x/2;

   double loLog = log10(mLoFreq);
   double hiLog = log10(mHiFreq);
   double denom = hiLog - loLog;
   for (int i = 1; (i < NUMBER_OF_BANDS) && (kThirdOct[i] <= mHiFreq); ++i)   //go along the spacers
   {
      float posn = range*(log10(kThirdOct[i])-loLog)/denom;   //centre of this slider, from start
      w = start + ((int)(posn+.5)) - EQsliderSize.x/2;   //LH edge of slider, from 0
      w = w - so_far;   //gap needed to put it here
      szrG->SetItemMinSize((size_t)(i*2), w, -1);   //set spacers so that sliders aligned with ruler
      so_far += (w + EQsliderSize.x);
   }

   mUIParent->RefreshRect(wxRect(szrG->GetPosition(), szrGSize));
}

void EffectEqualization::UpdateCurves()
{
   // Reload the curve names
   mCurve->Clear();
   for (size_t i = 0, cnt = mCurves.GetCount(); i < cnt; i++)
   {
      mCurve->Append(mCurves[ i ].Name);
   }
   mCurve->SetStringSelection(mCurveName);

   // Allow the control to resize
   mCurve->SetSizeHints(-1, -1);

   // Set initial curve
   setCurve( mCurveName );
}

void EffectEqualization::UpdateDraw()
{
   int numPoints = mLogEnvelope->GetNumberOfPoints();
   double *when = new double[ numPoints ];
   double *value = new double[ numPoints ];
   double deltadB = 0.1;
   double dx, dy, dx1, dy1, err;

   mLogEnvelope->GetPoints( when, value, numPoints );

   // set 'unnamed' as the selected curve
   EnvelopeUpdated();

   bool flag = true;
   while (flag)
   {
      flag = false;
      int numDeleted = 0;
      mLogEnvelope->GetPoints( when, value, numPoints );
      for(int j=0;j<numPoints-2;j++)
      {
         dx = when[j+2+numDeleted] - when[j+numDeleted];
         dy = value[j+2+numDeleted] - value[j+numDeleted];
         dx1 = when[j+numDeleted+1] - when[j+numDeleted];
         dy1 = dy * dx1 / dx;
         err = fabs(value[j+numDeleted+1] - (value[j+numDeleted] + dy1));
         if( err < deltadB )
         {   // within < deltadB dB?
            mLogEnvelope->Delete(j+1);
            numPoints--;
            numDeleted++;
            flag = true;
         }
      }
   }
   delete [] when;
   delete [] value;

   if(mLin)
   {
      EnvLogToLin();
      mEnvelope = mLinEnvelope;
      mFreqRuler->ruler.SetLog(false);
      mFreqRuler->ruler.SetRange(0, mHiFreq);
   }

   szrV->Show(szrG,false);
   szrH->Show(szrI,false);
   szrH->Show(szrL,true);
   mUIParent->Layout();
   wxGetTopLevelParent(mUIParent)->Layout();
   mPanel->ForceRecalc();     // it may have changed slightly due to the deletion of points
}

void EffectEqualization::UpdateGraphic()
{
   double loLog = log10(mLoFreq);
   double hiLog = log10(mHiFreq);
   double denom = hiLog - loLog;

   if(mLin)  //going from lin to log freq scale
   {  // add some extra points to the linear envelope for the graphic to follow
      double step = pow(2., 1./12.);   // twelve steps per octave
      double when,value;
      for(double freq=10.; freq<mHiFreq; freq*=step)
      {
         when = freq/mHiFreq;
         value = mLinEnvelope->GetValue(when);
         mLinEnvelope->Insert(when, value);
      }

      EnvLinToLog();
      mEnvelope = mLogEnvelope;
      mFreqRuler->ruler.SetLog(true);
      mFreqRuler->ruler.SetRange(mLoFreq, mHiFreq);
   }

   for (int i = 0; i < mBandsInUse; i++)
   {
      if( kThirdOct[i] == mLoFreq )
         mWhenSliders[i] = 0.;
      else
         mWhenSliders[i] = (log10(kThirdOct[i])-loLog)/denom;
      mEQVals[i] = mLogEnvelope->GetValue(mWhenSliders[i]);    //set initial values of sliders
      if( mEQVals[i] > 20.)
         mEQVals[i] = 20.;
      if( mEQVals[i] < -20.)
         mEQVals[i] = -20.;
   }
   ErrMin();                  //move sliders to minimise error
   for (int i = 0; i < mBandsInUse; i++)
   {
      mSliders[i]->SetValue(lrint(mEQVals[i])); //actually set slider positions
      mSlidersOld[i] = mSliders[i]->GetValue();
      wxString tip;
      if( kThirdOct[i] < 1000.)
         tip.Printf( wxT("%dHz\n%.1fdB"), (int)kThirdOct[i], mEQVals[i] );
      else
         tip.Printf( wxT("%gkHz\n%.1fdB"), kThirdOct[i]/1000., mEQVals[i] );
      mSliders[i]->SetToolTip(tip);
   }

   szrV->Show(szrG,true);  // eq sliders
   szrH->Show(szrI,true);  // interpolation choice
   szrH->Show(szrL,false); // linear freq checkbox
   mUIParent->Layout();
   wxGetTopLevelParent(mUIParent)->Layout();
//   mUIParent->Layout();    // Make all sizers get resized first
   LayoutEQSliders();      // Then layout sliders
   mUIParent->Layout();
   wxGetTopLevelParent(mUIParent)->Layout();
//   mUIParent->Layout();    // And layout again to resize dialog

   wxSize wsz = mUIParent->GetSize();
   wxSize ssz = szrV->GetSize();
   if (ssz.x > wsz.x || ssz.y > wsz.y)
   {
      mUIParent->Fit();
   }

   GraphicEQ(mLogEnvelope);
   mDrawMode = false;
}

void EffectEqualization::EnvLogToLin(void)
{
   int numPoints = mLogEnvelope->GetNumberOfPoints();
   if( numPoints == 0 )
   {
      return;
   }

   double *when = new double[ numPoints ];
   double *value = new double[ numPoints ];

   mLinEnvelope->Flatten(0.);
   mLinEnvelope->SetTrackLen(1.0);
   mLogEnvelope->GetPoints( when, value, numPoints );
   mLinEnvelope->Move(0., value[0]);
   double loLog = log10(20.);
   double hiLog = log10(mHiFreq);
   double denom = hiLog - loLog;

   for( int i=0; i < numPoints; i++)
      mLinEnvelope->Insert(pow( 10., ((when[i] * denom) + loLog))/mHiFreq , value[i]);
   mLinEnvelope->Move(1., value[numPoints-1]);

   delete [] when;
   delete [] value;
}

void EffectEqualization::EnvLinToLog(void)
{
   int numPoints = mLinEnvelope->GetNumberOfPoints();
   if( numPoints == 0 )
   {
      return;
   }

   double *when = new double[ numPoints ];
   double *value = new double[ numPoints ];

   mLogEnvelope->Flatten(0.);
   mLogEnvelope->SetTrackLen(1.0);
   mLinEnvelope->GetPoints( when, value, numPoints );
   mLogEnvelope->Move(0., value[0]);
   double loLog = log10(20.);
   double hiLog = log10(mHiFreq);
   double denom = hiLog - loLog;
   bool changed = false;

   for( int i=0; i < numPoints; i++)
   {
      if( when[i]*mHiFreq >= 20 )
      {
         mLogEnvelope->Insert((log10(when[i]*mHiFreq)-loLog)/denom , value[i]);
      }
      else
      {  //get the first point as close as we can to the last point requested
         changed = true;
         double v = value[i];
         mLogEnvelope->Insert(0., v);
      }
   }
   mLogEnvelope->Move(1., value[numPoints-1]);

   delete [] when;
   delete [] value;

   if(changed)
      EnvelopeUpdated(mLogEnvelope, false);
}

void EffectEqualization::ErrMin(void)
{
   double vals[NUM_PTS];
   int i;
   double error = 0.0;
   double oldError = 0.0;
   double mEQValsOld = 0.0;
   double correction = 1.6;
   bool flag;
   int j=0;
   Envelope *testEnvelope;
   testEnvelope = new Envelope();
   testEnvelope->SetInterpolateDB(false);
   testEnvelope->Mirror(false);
   testEnvelope->SetRange(-120.0, 60.0);
   testEnvelope->Flatten(0.);
   testEnvelope->SetTrackLen(1.0);
   testEnvelope->CopyFrom(mLogEnvelope, 0.0, 1.0);

   for(i=0; i < NUM_PTS; i++)
      vals[i] = testEnvelope->GetValue(mWhens[i]);

   //   Do error minimisation
   error = 0.;
   GraphicEQ(testEnvelope);
   for(i=0; i < NUM_PTS; i++)   //calc initial error
   {
      double err = vals[i] - testEnvelope->GetValue(mWhens[i]);
      error += err*err;
   }
   oldError = error;
   while( j < mBandsInUse*12 )  //loop over the sliders a number of times
   {
      i = j%mBandsInUse;       //use this slider
      if( (j > 0) & (i == 0) )   // if we've come back to the first slider again...
      {
         if( correction > 0 )
            correction = -correction;     //go down
         else
            correction = -correction/2.;  //go up half as much
      }
      flag = true;   // check if we've hit the slider limit
      do
      {
         oldError = error;
         mEQValsOld = mEQVals[i];
         mEQVals[i] += correction;    //move fader value
         if( mEQVals[i] > 20. )
         {
            mEQVals[i] = 20.;
            flag = false;
         }
         if( mEQVals[i] < -20. )
         {
            mEQVals[i] = -20.;
            flag = false;
         }
         GraphicEQ(testEnvelope);         //calculate envelope
         error = 0.;
         for(int k=0; k < NUM_PTS; k++)  //calculate error
         {
            double err = vals[k] - testEnvelope->GetValue(mWhens[k]);
            error += err*err;
         }
      }
      while( (error < oldError) & flag );
      if( error > oldError )
      {
         mEQVals[i] = mEQValsOld;   //last one didn't work
         error = oldError;
      }
      else
         oldError = error;
      if( error < .0025 * mBandsInUse)
         break;   // close enuff
      j++;  //try next slider
   }
   if( error > .0025 * mBandsInUse ) // not within 0.05dB on each slider, on average
   {
      Select( (int) mCurves.GetCount()-1 );
      EnvelopeUpdated(testEnvelope, false);
   }
   delete testEnvelope;
}

void EffectEqualization::GraphicEQ(Envelope *env)
{
   // JKC: 'value' is for height of curve.
   // The 0.0 initial value would only get used if NUM_PTS were 0.
   double value = 0.0;
   double dist, span, s;

   env->Flatten(0.);
   env->SetTrackLen(1.0);

   switch( mInterp )
   {
   case kBspline:  // B-spline
      {
         int minF = 0;
         for(int i=0; i<NUM_PTS; i++)
         {
            while( (mWhenSliders[minF] <= mWhens[i]) & (minF < mBandsInUse) )
               minF++;
            minF--;
            if( minF < 0 ) //before first slider
            {
               dist = mWhens[i] - mWhenSliders[0];
               span = mWhenSliders[1] - mWhenSliders[0];
               s = dist/span;
               if( s < -1.5 )
                  value = 0.;
               else if( s < -.5 )
                  value = mEQVals[0]*(s + 1.5)*(s + 1.5)/2.;
               else
                  value = mEQVals[0]*(.75 - s*s) + mEQVals[1]*(s + .5)*(s + .5)/2.;
            }
            else
            {
               if( mWhens[i] > mWhenSliders[mBandsInUse-1] )   //after last fader
               {
                  dist = mWhens[i] - mWhenSliders[mBandsInUse-1];
                  span = mWhenSliders[mBandsInUse-1] - mWhenSliders[mBandsInUse-2];
                  s = dist/span;
                  if( s > 1.5 )
                     value = 0.;
                  else if( s > .5 )
                     value = mEQVals[mBandsInUse-1]*(s - 1.5)*(s - 1.5)/2.;
                  else
                     value = mEQVals[mBandsInUse-1]*(.75 - s*s) +
                     mEQVals[mBandsInUse-2]*(s - .5)*(s - .5)/2.;
               }
               else  //normal case
               {
                  dist = mWhens[i] - mWhenSliders[minF];
                  span = mWhenSliders[minF+1] - mWhenSliders[minF];
                  s = dist/span;
                  if(s < .5 )
                  {
                     value = mEQVals[minF]*(0.75 - s*s);
                     if( minF+1 < mBandsInUse )
                        value += mEQVals[minF+1]*(s+.5)*(s+.5)/2.;
                     if( minF-1 >= 0 )
                        value += mEQVals[minF-1]*(s-.5)*(s-.5)/2.;
                  }
                  else
                  {
                     value = mEQVals[minF]*(s-1.5)*(s-1.5)/2.;
                     if( minF+1 < mBandsInUse )
                        value += mEQVals[minF+1]*(.75-(1.-s)*(1.-s));
                     if( minF+2 < mBandsInUse )
                        value += mEQVals[minF+2]*(s-.5)*(s-.5)/2.;
                  }
               }
            }
            if(mWhens[i]<=0.)
               env->Move( 0., value );
            env->Insert( mWhens[i], value );
         }
         env->Move( 1., value );
         break;
      }

   case kCosine:  // Cosine squared
      {
         int minF = 0;
         for(int i=0; i<NUM_PTS; i++)
         {
            while( (mWhenSliders[minF] <= mWhens[i]) & (minF < mBandsInUse) )
               minF++;
            minF--;
            if( minF < 0 ) //before first slider
            {
               dist = mWhenSliders[0] - mWhens[i];
               span = mWhenSliders[1] - mWhenSliders[0];
               if( dist < span )
                  value = mEQVals[0]*(1. + cos(M_PI*dist/span))/2.;
               else
                  value = 0.;
            }
            else
            {
               if( mWhens[i] > mWhenSliders[mBandsInUse-1] )   //after last fader
               {
                  span = mWhenSliders[mBandsInUse-1] - mWhenSliders[mBandsInUse-2];
                  dist = mWhens[i] - mWhenSliders[mBandsInUse-1];
                  if( dist < span )
                     value = mEQVals[mBandsInUse-1]*(1. + cos(M_PI*dist/span))/2.;
                  else
                     value = 0.;
               }
               else  //normal case
               {
                  span = mWhenSliders[minF+1] - mWhenSliders[minF];
                  dist = mWhenSliders[minF+1] - mWhens[i];
                  value = mEQVals[minF]*(1. + cos(M_PI*(span-dist)/span))/2. +
                     mEQVals[minF+1]*(1. + cos(M_PI*dist/span))/2.;
               }
            }
            if(mWhens[i]<=0.)
               env->Move( 0., value );
            env->Insert( mWhens[i], value );
         }
         env->Move( 1., value );
         break;
      }

   case kCubic:  // Cubic Spline
      {
         double y2[NUMBER_OF_BANDS+1];
         mEQVals[mBandsInUse] = mEQVals[mBandsInUse-1];
         spline(mWhenSliders, mEQVals, mBandsInUse+1, y2);
         for(double xf=0; xf<1.; xf+=1./NUM_PTS)
         {
            env->Insert(xf, splint(mWhenSliders, mEQVals, mBandsInUse+1, y2, xf));
         }
         break;
      }
   }

   mPanel->ForceRecalc();
}

void EffectEqualization::spline(double x[], double y[], int n, double y2[])
{
   int i;
   double p, sig, *u = new double[n];

   y2[0] = 0.;  //
   u[0] = 0.;   //'natural' boundary conditions
   for(i=1;i<n-1;i++)
   {
      sig = ( x[i] - x[i-1] ) / ( x[i+1] - x[i-1] );
      p = sig * y2[i-1] + 2.;
      y2[i] = (sig - 1.)/p;
      u[i] = ( y[i+1] - y[i] ) / ( x[i+1] - x[i] ) - ( y[i] - y[i-1] ) / ( x[i] - x[i-1] );
      u[i] = (6.*u[i]/( x[i+1] - x[i-1] ) - sig * u[i-1]) / p;
   }
   y2[n-1] = 0.;
   for(i=n-2;i>=0;i--)
      y2[i] = y2[i]*y2[i+1] + u[i];

   delete [] u;
}

double EffectEqualization::splint(double x[], double y[], int n, double y2[], double xr)
{
   double a, b, h;
   static double xlast = 0.;   // remember last x value requested
   static int k = 0;           // and which interval we were in

   if( xr < xlast )
      k = 0;                   // gone back to start, (or somewhere to the left)
   xlast = xr;
   while( (x[k] <= xr) && (k < n-1) )
      k++;
   k--;
   h = x[k+1] - x[k];
   a = ( x[k+1] - xr )/h;
   b = (xr - x[k])/h;
   return( a*y[k]+b*y[k+1]+((a*a*a-a)*y2[k]+(b*b*b-b)*y2[k+1])*h*h/6.);
}

void EffectEqualization::OnSize(wxSizeEvent & event)
{
   mUIParent->Layout();

   if (!mDrawMode)
   {
      LayoutEQSliders();
   }

   event.Skip();
}

void EffectEqualization::OnErase(wxEraseEvent & WXUNUSED(event))
{
   // Ignore it
}

void EffectEqualization::OnSlider(wxCommandEvent & event)
{
   wxSlider *s = (wxSlider *)event.GetEventObject();
   for (int i = 0; i < mBandsInUse; i++)
   {
      if( s == mSliders[i])
      {
         int posn = mSliders[i]->GetValue();
         if( wxGetKeyState(WXK_SHIFT) )
         {
            if( posn > mSlidersOld[i] )
               mEQVals[i] += (float).1;
            else
               if( posn < mSlidersOld[i] )
                  mEQVals[i] -= .1f;
         }
         else
            mEQVals[i] += (posn - mSlidersOld[i]);
         if( mEQVals[i] > 20. )
            mEQVals[i] = 20.;
         if( mEQVals[i] < -20. )
            mEQVals[i] = -20.;
         int newPosn = (int)mEQVals[i];
         mSliders[i]->SetValue( newPosn );
         mSlidersOld[i] = newPosn;
         wxString tip;
         if( kThirdOct[i] < 1000.)
            tip.Printf( wxT("%dHz\n%.1fdB"), (int)kThirdOct[i], mEQVals[i] );
         else
            tip.Printf( wxT("%gkHz\n%.1fdB"), kThirdOct[i]/1000., mEQVals[i] );
         s->SetToolTip(tip);
         break;
      }
   }
   GraphicEQ(mLogEnvelope);
   EnvelopeUpdated();
}

void EffectEqualization::OnInterp(wxCommandEvent & WXUNUSED(event))
{
   if(mGraphic->GetValue())
   {
      GraphicEQ(mLogEnvelope);
      EnvelopeUpdated();
   }
   mInterp = mInterpChoice->GetSelection();
}

void EffectEqualization::OnDrawMode(wxCommandEvent & WXUNUSED(event))
{
   UpdateDraw();

   mDrawMode = true;
}

void EffectEqualization::OnGraphicMode(wxCommandEvent & WXUNUSED(event))
{
   UpdateGraphic();

   mDrawMode = false;
}

void EffectEqualization::OnSliderM(wxCommandEvent & WXUNUSED(event))
{
   TransferDataFromWindow();
   mPanel->ForceRecalc();
}

void EffectEqualization::OnSliderDBMIN(wxCommandEvent & WXUNUSED(event))
{
   TransferDataFromWindow();
}

void EffectEqualization::OnSliderDBMAX(wxCommandEvent & WXUNUSED(event))
{
   TransferDataFromWindow();
}

//
// New curve was selected
//
void EffectEqualization::OnCurve(wxCommandEvent & WXUNUSED(event))
{
   // Select new curve
   setCurve( mCurve->GetCurrentSelection() );
   if( !mDrawMode )
      UpdateGraphic();
}

//
// User wants to modify the list in some way
//
void EffectEqualization::OnManage(wxCommandEvent & WXUNUSED(event))
{
   EditCurvesDialog d(mUIParent, this, mCurve->GetSelection());
   d.ShowModal();

   // Reload the curve names
   UpdateCurves();

   // Allow control to resize
   mUIParent->Layout();
}

void EffectEqualization::OnClear(wxCommandEvent & WXUNUSED(event))
{
   Flatten();
}

void EffectEqualization::OnInvert(wxCommandEvent & WXUNUSED(event)) // Inverts any curve
{
   if(!mDrawMode)   // Graphic (Slider) mode. Invert the sliders.
   {
      for (int i = 0; i < mBandsInUse; i++)
      {
         mEQVals[i] = -mEQVals[i];
         int newPosn = (int)mEQVals[i];
         mSliders[i]->SetValue( newPosn );
         mSlidersOld[i] = newPosn;

         wxString tip;
         if( kThirdOct[i] < 1000.)
            tip.Printf( wxT("%dHz\n%.1fdB"), (int)kThirdOct[i], mEQVals[i] );
         else
            tip.Printf( wxT("%gkHz\n%.1fdB"), kThirdOct[i]/1000., mEQVals[i] );
         mSliders[i]->SetToolTip(tip);
      }
      GraphicEQ(mLogEnvelope);
   }
   else  // Draw mode.  Invert the points.
   {
      bool lin;   // refers to the 'log' or 'lin' of the frequency scale, not the amplitude
      int numPoints; // number of points in the curve/envelope

      // determine if log or lin curve is the current one
      // and find out how many points are in the curve
      if(mLin)  // lin freq scale and so envelope
      {
         lin = true;
         numPoints = mLinEnvelope->GetNumberOfPoints();
      }
      else
      {
         lin = false;
         numPoints = mLogEnvelope->GetNumberOfPoints();
      }

      if( numPoints == 0 )
         return;

      double *when = new double[ numPoints ];
      double *value = new double[ numPoints ];

      if(lin)
         mLinEnvelope->GetPoints( when, value, numPoints );
      else
         mLogEnvelope->GetPoints( when, value, numPoints );

      // invert the curve
      for( int i=0; i < numPoints; i++)
      {
         if(lin)
            mLinEnvelope->Move(when[i] , -value[i]);
         else
            mLogEnvelope->Move(when[i] , -value[i]);
      }

      delete [] when;
      delete [] value;

      // copy it back to the other one (just in case)
      if(lin)
         EnvLinToLog();
      else
         EnvLogToLin();
   }

   // and update the display etc
   mPanel->ForceRecalc();
   EnvelopeUpdated();
}

void EffectEqualization::OnGridOnOff(wxCommandEvent & WXUNUSED(event))
{
   mDrawGrid = mGridOnOff->IsChecked();
   mPanel->Refresh(false);
}

void EffectEqualization::OnLinFreq(wxCommandEvent & WXUNUSED(event))
{
   mLin = mLinFreq->IsChecked();
   if(mLin)  //going from log to lin freq scale
   {
      mFreqRuler->ruler.SetLog(false);
      mFreqRuler->ruler.SetRange(0, mHiFreq);
      EnvLogToLin();
      mEnvelope = mLinEnvelope;
      mLin = true;
   }
   else  //going from lin to log freq scale
   {
      mFreqRuler->ruler.SetLog(true);
      mFreqRuler->ruler.SetRange(mLoFreq, mHiFreq);
      EnvLinToLog();
      mEnvelope = mLogEnvelope;
      mLin = false;
   }
   mFreqRuler->Refresh(false);
   mPanel->ForceRecalc();
}

#ifdef EXPERIMENTAL_EQ_SSE_THREADED

void EffectEqualization::OnProcessingRadio(wxCommandEvent & event)
{
   int testEvent=event.GetId();
   switch(testEvent)
   {
   case defaultMathRadioID: EffectEqualization48x::SetMathPath(MATH_FUNCTION_ORIGINAL);
      break;
   case sSERadioID: EffectEqualization48x::SetMathPath(MATH_FUNCTION_SSE);
      break;
   case sSEThreadedRadioID: EffectEqualization48x::SetMathPath(MATH_FUNCTION_THREADED | MATH_FUNCTION_SSE);
      break;
   case aVXRadioID: testEvent=2;
      break;
   case aVXThreadedRadioID: testEvent=2;
      break;
   }

};

void EffectEqualization::OnBench( wxCommandEvent & event)
{
   m_pEffect->mBench=true;
   OnOk(event);
}

#endif

//----------------------------------------------------------------------------
// EqualizationPanel
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(EqualizationPanel, wxPanel)
   EVT_PAINT(EqualizationPanel::OnPaint)
   EVT_MOUSE_EVENTS(EqualizationPanel::OnMouseEvent)
   EVT_MOUSE_CAPTURE_LOST(EqualizationPanel::OnCaptureLost)
   EVT_SIZE(EqualizationPanel::OnSize)
END_EVENT_TABLE()

EqualizationPanel::EqualizationPanel(EffectEqualization *effect, wxWindow *parent)
:  wxPanel(parent)
{
   mParent = parent;
   mEffect = effect;
   
   mOutr = NULL;
   mOuti = NULL;

   mBitmap = NULL;
   mWidth = 0;
   mHeight = 0;

   mEffect->mEnvelope->Flatten(0.);
   mEffect->mEnvelope->Mirror(false);
   mEffect->mEnvelope->SetTrackLen(1.0);

   ForceRecalc();
}

EqualizationPanel::~EqualizationPanel()
{
   if (mBitmap)
      delete mBitmap;
   if (mOuti)
      delete [] mOuti;
   if (mOutr)
      delete [] mOutr;
}

void EqualizationPanel::ForceRecalc()
{
   mRecalcRequired = true;
   Refresh(false);
}

void EqualizationPanel::Recalc()
{
   if (mOutr)
      delete [] mOutr;
   mOutr = new float[mEffect->mWindowSize];

   if (mOuti)
      delete [] mOuti;
   mOuti = new float[mEffect->mWindowSize];

   mEffect->CalcFilter();   //to calculate the actual response
#ifdef EXPERIMENTAL_USE_REALFFTF
   InverseRealFFT(mEffect->mWindowSize, mEffect->mFilterFuncR, mEffect->mFilterFuncI, mOutr);
#else
   FFT(mWindowSize,true,mFilterFuncR,mFilterFuncI,mOutr,mOuti);   //work out FIR response - note mOuti will be all zeros
#endif // EXPERIMENTAL_USE_REALFFTF
}

void EqualizationPanel::OnSize(wxSizeEvent &  WXUNUSED(event))
{
   Refresh( false );
}

void EqualizationPanel::OnPaint(wxPaintEvent &  WXUNUSED(event))
{
   wxPaintDC dc(this);
   if(mRecalcRequired) {
      Recalc();
      mRecalcRequired = false;
   }
   int width, height;
   GetSize(&width, &height);

   if (!mBitmap || mWidth!=width || mHeight!=height)
   {
      if (mBitmap)
         delete mBitmap;

      mWidth = width;
      mHeight = height;
      mBitmap = new wxBitmap(mWidth, mHeight);
   }

   wxBrush bkgndBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));

   wxMemoryDC memDC;
   memDC.SelectObject(*mBitmap);

   wxRect bkgndRect;
   bkgndRect.x = 0;
   bkgndRect.y = 0;
   bkgndRect.width = mWidth;
   bkgndRect.height = mHeight;
   memDC.SetBrush(bkgndBrush);
   memDC.SetPen(*wxTRANSPARENT_PEN);
   memDC.DrawRectangle(bkgndRect);

   bkgndRect.y = mHeight;
   memDC.DrawRectangle(bkgndRect);

   wxRect border;
   border.x = 0;
   border.y = 0;
   border.width = mWidth;
   border.height = mHeight;

   memDC.SetBrush(*wxWHITE_BRUSH);
   memDC.SetPen(*wxBLACK_PEN);
   memDC.DrawRectangle(border);

   mEnvRect = border;
   mEnvRect.Deflate(PANELBORDER, PANELBORDER);

   // Pure blue x-axis line
   memDC.SetPen(wxPen(theTheme.Colour( clrGraphLines ), 1, wxSOLID));
   int center = (int) (mEnvRect.height * mEffect->mdBMax/(mEffect->mdBMax-mEffect->mdBMin) + .5);
   AColor::Line(memDC,
      mEnvRect.GetLeft(), mEnvRect.y + center,
      mEnvRect.GetRight(), mEnvRect.y + center);

   // Draw the grid, if asked for.  Do it now so it's underneath the main plots.
   if( mEffect->mDrawGrid )
   {
      mEffect->mFreqRuler->ruler.DrawGrid(memDC, mEnvRect.height, true, true, PANELBORDER, PANELBORDER);
      mEffect->mdBRuler->ruler.DrawGrid(memDC, mEnvRect.width, true, true, PANELBORDER, PANELBORDER);
   }

   // Med-blue envelope line
   memDC.SetPen(wxPen(theTheme.Colour( clrGraphLines ), 3, wxSOLID));

   // Draw envelope
   double *values = new double[mEnvRect.width];
   mEffect->mEnvelope->GetValues(values, mEnvRect.width, 0.0, 1.0/mEnvRect.width);
   int x, y, xlast = 0, ylast = 0;
   bool off = false, off1 = false;
   for(int i=0; i<mEnvRect.width; i++)
   {
      x = mEnvRect.x + i;
      y = lrint(mEnvRect.height*((mEffect->mdBMax-values[i])/(mEffect->mdBMax-mEffect->mdBMin)) + .25 ); //needs more optimising, along with'what you get'?
      if( y >= mEnvRect.height)
      {
         y = mEnvRect.height - 1;
         off = true;
      }
      else
      {
         off = false;
         off1 = false;
      }
      if ( (i != 0) & (!off1) )
      {
         AColor::Line(memDC, xlast, ylast,
            x, mEnvRect.y + y);
      }
      off1 = off;
      xlast = x;
      ylast = mEnvRect.y + y;
   }
   delete[] values;

   //Now draw the actual response that you will get.
   //mFilterFunc has a linear scale, window has a log one so we have to fiddle about
   memDC.SetPen(wxPen(theTheme.Colour( clrResponseLines ), 1, wxSOLID));
   double scale = (double)mEnvRect.height/(mEffect->mdBMax-mEffect->mdBMin);   //pixels per dB
   double yF;   //gain at this freq
   double delta = mEffect->mHiFreq/(((double)mEffect->mWindowSize/2.));   //size of each freq bin

   bool lin = mEffect->mDraw && mEffect->mLin;   // log or lin scale?

   double loLog = log10(mEffect->mLoFreq);
   double step = lin ? mEffect->mHiFreq : (log10(mEffect->mHiFreq) - loLog);
   step /= ((double)mEnvRect.width-1.);
   double freq;   //actual freq corresponding to x position
   int halfM = (mEffect->mM-1)/2;
   int n;   //index to mFreqFunc
   for(int i=0; i<mEnvRect.width; i++)
   {
      x = mEnvRect.x + i;
      freq = lin ? step*i : pow(10., loLog + i*step);   //Hz
      if( ( lin ? step : (pow(10., loLog + (i+1)*step)-freq) ) < delta)
      {   //not enough resolution in FFT
         // set up for calculating cos using recurrance - faster than calculating it directly each time
         double theta = M_PI*freq/mEffect->mHiFreq;   //radians, normalized
         double wtemp = sin(0.5 * theta);
         double wpr = -2.0 * wtemp * wtemp;
         double wpi = -1.0 * sin(theta);
         double wr = cos(theta*halfM);
         double wi = sin(theta*halfM);

         yF = 0.;
         for(int j=0;j<halfM;j++)
         {
            yF += 2. * mOutr[j] * wr;  // This works for me, compared to the previous version.  Compare wr to cos(theta*(halfM-j)).  Works for me.  Keep everything as doubles though.
            // do recurrance
            wr = (wtemp = wr) * wpr - wi * wpi + wr;
            wi = wi * wpr + wtemp * wpi + wi;
         }
         yF += mOutr[halfM];
         yF = fabs(yF);
         if(yF!=0.)
            yF = 20.0*log10(yF);   //20 here as an amplitude
         else
            yF = mEffect->mdBMin;
      }
      else
      {   //use FFT, it has enough resolution
         n = (int)(freq/delta + .5);
         if(pow(mEffect->mFilterFuncR[n],2)+pow(mEffect->mFilterFuncI[n],2)!=0.)
            yF = 10.0*log10(pow(mEffect->mFilterFuncR[n],2)+pow(mEffect->mFilterFuncI[n],2));   //10 here, a power
         else
            yF = mEffect->mdBMin;
      }
      if(yF < mEffect->mdBMin)
         yF = mEffect->mdBMin;
      yF = center-scale*yF;
      if(yF>mEnvRect.height)
         yF = mEnvRect.height - 1;
      if(yF<0.)
         yF=0.;
      y = (int)(yF+.5);

      if (i != 0)
      {
         AColor::Line(memDC, xlast, ylast, x, mEnvRect.y + y);
      }
      xlast = x;
      ylast = mEnvRect.y + y;
   }

   memDC.SetPen(*wxBLACK_PEN);
   if( mEffect->mDraw->GetValue() )
      mEffect->mEnvelope->DrawPoints(memDC, mEnvRect, 0.0, mEnvRect.width-1, false, mEffect->mdBMin, mEffect->mdBMax);

   dc.Blit(0, 0, mWidth, mHeight,
      &memDC, 0, 0, wxCOPY, FALSE);
}

void EqualizationPanel::OnMouseEvent(wxMouseEvent & event)
{
   if (!mEffect->mDrawMode)
   {
      return;
   }

   if (event.ButtonDown() && !HasCapture())
   {
      CaptureMouse();
   }

   if (mEffect->mEnvelope->MouseEvent(event, mEnvRect, 0.0, mEnvRect.width, false,
      mEffect->mdBMin, mEffect->mdBMax))
   {
      mEffect->EnvelopeUpdated();
      ForceRecalc();
   }

   if (event.ButtonUp() && HasCapture())
   {
      ReleaseMouse();
   }
}

void EqualizationPanel::OnCaptureLost(wxMouseCaptureLostEvent & WXUNUSED(event))
{
   if (HasCapture())
   {
      ReleaseMouse();
   }
}

//----------------------------------------------------------------------------
// EditCurvesDialog
//----------------------------------------------------------------------------
// Note that the 'modified' curve used to be called 'custom' but is now called 'unnamed'
// Some things that deal with 'unnamed' curves still use, for example, 'mCustomBackup' as variable names.
/// Constructor

BEGIN_EVENT_TABLE(EditCurvesDialog, wxDialog)
   EVT_BUTTON(UpButtonID, EditCurvesDialog::OnUp)
   EVT_BUTTON(DownButtonID, EditCurvesDialog::OnDown)
   EVT_BUTTON(RenameButtonID, EditCurvesDialog::OnRename)
   EVT_BUTTON(DeleteButtonID, EditCurvesDialog::OnDelete)
   EVT_BUTTON(ImportButtonID, EditCurvesDialog::OnImport)
   EVT_BUTTON(ExportButtonID, EditCurvesDialog::OnExport)
   EVT_BUTTON(LibraryButtonID, EditCurvesDialog::OnLibrary)
   EVT_BUTTON(DefaultsButtonID, EditCurvesDialog::OnDefaults)
   EVT_BUTTON(wxID_OK, EditCurvesDialog::OnOK)
END_EVENT_TABLE()

EditCurvesDialog::EditCurvesDialog(wxWindow * parent, EffectEqualization * effect, int position):
wxDialog(parent, wxID_ANY, _("Manage Curves List"),
         wxDefaultPosition, wxDefaultSize,
         wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
   SetLabel(_("Manage Curves"));         // Provide visual label
   SetName(_("Manage Curves List"));     // Provide audible label
   mParent = parent;
   mEffect = effect;
   mPosition = position;
   // make a copy of mEffect->mCurves here to muck about with.
   mEditCurves.Clear();
   for (unsigned int i = 0; i < mEffect->mCurves.GetCount(); i++)
   {
      mEditCurves.Add(mEffect->mCurves[i].Name);
      mEditCurves[i].points = mEffect->mCurves[i].points;
   }

   Populate();
}

EditCurvesDialog::~EditCurvesDialog()
{
}

/// Creates the dialog and its contents.
void EditCurvesDialog::Populate()
{
   //------------------------- Main section --------------------
   ShuttleGui S(this, eIsCreating);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
}

/// Defines the dialog and does data exchange with it.
void EditCurvesDialog::PopulateOrExchange(ShuttleGui & S)
{
   S.StartHorizontalLay(wxEXPAND);
   {
      S.StartStatic(_("&Curves"), 1);
      {
         S.SetStyle(wxSUNKEN_BORDER | wxLC_REPORT | wxLC_HRULES | wxLC_VRULES );
         mList = S.Id(CurvesListID).AddListControlReportMode();
         mList->InsertColumn(0, _("Curve Name"), wxLIST_FORMAT_RIGHT);
      }
      S.EndStatic();
      S.StartVerticalLay(0);
      {
         S.Id(UpButtonID).AddButton(_("Move &Up"), wxALIGN_LEFT);
         S.Id(DownButtonID).AddButton(_("Move &Down"), wxALIGN_LEFT);
         S.Id(RenameButtonID).AddButton(_("&Rename..."), wxALIGN_LEFT);
         S.Id(DeleteButtonID).AddButton(_("D&elete..."), wxALIGN_LEFT);
         S.Id(ImportButtonID).AddButton(_("I&mport..."), wxALIGN_LEFT);
         S.Id(ExportButtonID).AddButton(_("E&xport..."), wxALIGN_LEFT);
         S.Id(LibraryButtonID).AddButton(_("&Get More..."), wxALIGN_LEFT);
         S.Id(DefaultsButtonID).AddButton(_("De&faults"), wxALIGN_LEFT);
      }
      S.EndVerticalLay();
   }
   S.EndHorizontalLay();
   S.AddStandardButtons();
   S.StartStatic(_("Help"));
   S.AddConstTextBox(wxT(""), _("Rename 'unnamed' to save a new entry.\n'OK' saves all changes, 'Cancel' doesn't."));
   S.EndStatic();
   PopulateList(mPosition);
   Fit();

   return;
}

void EditCurvesDialog::PopulateList(int position)
{
   mList->DeleteAllItems();
   for (unsigned int i = 0; i < mEditCurves.GetCount(); i++)
      mList->InsertItem(i, mEditCurves[i].Name);
   mList->SetColumnWidth(0, wxLIST_AUTOSIZE);
   int curvesWidth = mList->GetColumnWidth(0);
   mList->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
   int headerWidth = mList->GetColumnWidth(0);
   mList->SetColumnWidth(0, wxMax(headerWidth, curvesWidth));
   // use 'position' to set focus
   mList->EnsureVisible(position);
   mList->SetItemState(position, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
}

void EditCurvesDialog::OnUp(wxCommandEvent & WXUNUSED(event))
{
   long item = mList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   if ( item == -1 )
      return;  // no items selected
   if( item == 0 )
      item = mList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); // top item selected, can't move up
   int state;
   while( item != -1 )
   {
      if ( item == mList->GetItemCount()-1)
      {  // 'unnamed' always stays at the bottom
         wxMessageBox(_("'unnamed' always stays at the bottom of the list"), _("'unnamed' is special"));   // these could get tedious!
         return;
      }
      state = mList->GetItemState(item-1, wxLIST_STATE_SELECTED);
      if ( state != wxLIST_STATE_SELECTED )
      { // swap this with one above but only if it isn't selected
         EQCurve temp(wxT("temp"));
         temp.Name = mEditCurves[item].Name;
         temp.points = mEditCurves[item].points;
         mEditCurves[item].Name = mEditCurves[item-1].Name;
         mEditCurves[item].points = mEditCurves[item-1].points;
         mEditCurves[item-1].Name = temp.Name;
         mEditCurves[item-1].points = temp.points;
         wxString sTemp = mList->GetItemText(item);
         mList->SetItem(item, 0, mList->GetItemText(item-1));
         mList->SetItem(item-1, 0, sTemp);
         mList->SetItemState(item, 0, wxLIST_STATE_SELECTED);
         mList->SetItemState(item-1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
      }
      item = mList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   }
}

void EditCurvesDialog::OnDown(wxCommandEvent & WXUNUSED(event))
{  // looks harder than OnUp as we need to seek backwards up the list, hence GetPreviousItem
   long item = GetPreviousItem(mList->GetItemCount());
   if( item == -1 )
      return;  // nothing selected
   int state;
   while( item != -1 )
   {
      if( (item != mList->GetItemCount()-1) && (item != mList->GetItemCount()-2) )
      {  // can't move 'unnamed' down, or the one above it
         state = mList->GetItemState(item+1, wxLIST_STATE_SELECTED);
         if ( state != wxLIST_STATE_SELECTED )
         { // swap this with one below but only if it isn't selected
            EQCurve temp(wxT("temp"));
            temp.Name = mEditCurves[item].Name;
            temp.points = mEditCurves[item].points;
            mEditCurves[item].Name = mEditCurves[item+1].Name;
            mEditCurves[item].points = mEditCurves[item+1].points;
            mEditCurves[item+1].Name = temp.Name;
            mEditCurves[item+1].points = temp.points;
            wxString sTemp = mList->GetItemText(item);
            mList->SetItem(item, 0, mList->GetItemText(item+1));
            mList->SetItem(item+1, 0, sTemp);
            mList->SetItemState(item, 0, wxLIST_STATE_SELECTED);
            mList->SetItemState(item+1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
         }
      }
      item = GetPreviousItem(item);
   }
}

long EditCurvesDialog::GetPreviousItem(long item)  // wx doesn't have this
{
   long lastItem = -1;
   long itemTemp = mList->GetNextItem(-1, wxLIST_NEXT_ALL,
      wxLIST_STATE_SELECTED);
   while( (itemTemp != -1) && (itemTemp < item) )
   {
      lastItem = itemTemp;
      itemTemp = mList->GetNextItem(itemTemp, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   }
   return lastItem;
}

// Rename curve/curves
void EditCurvesDialog::OnRename(wxCommandEvent & WXUNUSED(event))
{
   wxString name;
   int numCurves = mEditCurves.GetCount();
   int curve = 0;

   // Setup list of characters that aren't allowed
   wxArrayString exclude;
   exclude.Add( wxT("<") );
   exclude.Add( wxT(">") );
   exclude.Add( wxT("'") );
   exclude.Add( wxT("\"") );

   // Get the first one to be renamed
   long item = mList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   long firstItem = item;  // for reselection with PopulateList
   while(item >= 0)
   {
      // Prompt the user until a valid name is enter or cancelled
      bool overwrite = false;
      bool bad = true;
      while( bad )   // Check for an unacceptable duplicate
      {   // Show the dialog and bail if the user cancels
         bad = false;
         // build the dialog
         wxTextEntryDialog dlg( this,
            _("Rename '") + mEditCurves[ item ].Name + _("' to..."),
            _("Rename...") );
         dlg.SetTextValidator( wxFILTER_EXCLUDE_CHAR_LIST );
         dlg.SetName( _("Rename '") + mEditCurves[ item ].Name );
         wxTextValidator *tv = dlg.GetTextValidator();
         tv->SetExcludes( exclude );   // Tell the validator about excluded chars
         if( dlg.ShowModal() == wxID_CANCEL )
         {
            bad = true;
            break;
         }

         // Extract the name from the dialog
         name = dlg.GetValue();

         // Search list of curves for a duplicate name
         for( curve = 0; curve < numCurves; curve++ )
         {
            wxString temp = mEditCurves[ curve ].Name;
            if( name.IsSameAs( mEditCurves[ curve ].Name )) // case sensitive
            {
               bad = true;
               if( curve == item )  // trying to rename a curve with the same name
               {
                  wxMessageBox( _("Name is the same as the original one"), _("Same name"), wxOK );
                  break;
               }
               int answer = wxMessageBox( _("Overwrite existing curve '") + name +_("'?"),
                  _("Curve exists"), wxYES_NO );
               if (answer == wxYES)
               {
                  bad = false;
                  overwrite = true; // we are going to overwrite the one with this name
                  break;
               }
            }
         }
         if( name == wxT("") || name == wxT("unnamed") )
            bad = true;
      }

      // if bad, we cancelled the rename dialog, so nothing to do.
      if( bad == true )
         ;
      else if(overwrite){
         // Overwrite another curve.
         // JKC: because 'overwrite' is true, 'curve' is the number of the curve that
         // we are about to overwrite.
         mEditCurves[ curve ].Name = name;
         mEditCurves[ curve ].points = mEditCurves[ item ].points;
         // if renaming the unnamed item, then select it,
         // otherwise get rid of the item we've renamed.
         if( item == (numCurves-1) )
            mList->SetItem(curve, 0, name);
         else
         {
            mEditCurves.RemoveAt( item );
            numCurves--;
         }
      }
      else if( item == (numCurves-1) ) // renaming 'unnamed'
      {  // Create a new entry
         mEditCurves.Add( EQCurve( wxT("unnamed") ) );
         // Copy over the points
         mEditCurves[ numCurves ].points = mEditCurves[ numCurves - 1 ].points;
         // Give the original unnamed entry the new name
         mEditCurves[ numCurves - 1 ].Name = name;
         numCurves++;
      }
      else  // just rename (the 'normal' case)
      {
         mEditCurves[ item ].Name = name;
         mList->SetItem(item, 0, name);
      }
      // get next selected item
      item = mList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   }

   PopulateList(firstItem);  // Note: only saved to file when you OK out of the dialog
   return;
}

// Delete curve/curves
void EditCurvesDialog::OnDelete(wxCommandEvent & WXUNUSED(event))
{
   // We could could count them here
   // And then put in a 'Delete N items?' prompt.

#if 0 // 'one at a time' prompt code
   // Get the first one to be deleted
   long item = mList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   // Take care, mList and mEditCurves will get out of sync as curves are deleted
   int deleted = 0;
   long highlight = -1;

   while(item >= 0)
   {
      if(item == mList->GetItemCount()-1)   //unnamed
      {
         wxMessageBox(_("You cannot delete the 'unnamed' curve."),
            _("Can't delete 'unnamed'"), wxOK | wxCENTRE, this);
      }
      else
      {
         // Create the prompt
         wxString quest;
         quest = wxString(_("Delete '")) + mEditCurves[ item-deleted ].Name + _("' ?");

         // Ask for confirmation before removal
         int ans = wxMessageBox( quest, _("Confirm Deletion"), wxYES_NO | wxCENTRE, this );
         if( ans == wxYES )
         {  // Remove the curve from the array
            mEditCurves.RemoveAt( item-deleted );
            deleted++;
         }
         else
            highlight = item-deleted;  // if user presses 'No', select that curve
      }
      // get next selected item
      item = mList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   }

   if(highlight == -1)
      PopulateList(mEditCurves.GetCount()-1);   // set 'unnamed' as the selected curve
   else
      PopulateList(highlight);   // user said 'No' to deletion
#else // 'delete all N' code
   int count = mList->GetSelectedItemCount();
   long item = mList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   // Create the prompt
   wxString quest;
   if( count > 1 )
      quest.Printf(_("Delete ") + wxString(wxT("%d ")) + _("items?"), count);
   else
      if( count == 1 )
         quest = wxString(_("Delete '")) + mEditCurves[ item ].Name + _("' ?");
      else
         return;
   // Ask for confirmation before removal
   int ans = wxMessageBox( quest, _("Confirm Deletion"), wxYES_NO | wxCENTRE, this );
   if( ans == wxYES )
   {  // Remove the curve(s) from the array
      // Take care, mList and mEditCurves will get out of sync as curves are deleted
      int deleted = 0;
      while(item >= 0)
      {
         if(item == mList->GetItemCount()-1)   //unnamed
         {
            wxMessageBox(_("You cannot delete the 'unnamed' curve, it is special."),
               _("Can't delete 'unnamed'"), wxOK | wxCENTRE, this);
         }
         else
         {
            mEditCurves.RemoveAt( item-deleted );
            deleted++;
         }
         item = mList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
      }
      PopulateList(mEditCurves.GetCount()-1);   // set 'unnamed' as the selected curve
   }
#endif
}

void EditCurvesDialog::OnImport( wxCommandEvent & WXUNUSED(event))
{
   FileDialog filePicker(this, _("Choose an EQ curve file"), FileNames::DataDir(), wxT(""), _("xml files (*.xml;*.XML)|*.xml;*.XML"));
   wxString fileName = wxT("");
   if( filePicker.ShowModal() == wxID_CANCEL)
      return;
   else
      fileName = filePicker.GetPath();
   // Use EqualizationDialog::LoadCurves to read into (temporary) mEditCurves
   // This may not be the best OOP way of doing it, but I don't know better (MJS)
   EQCurveArray temp;
   temp = mEffect->mCurves;   // temp copy of the main dialog curves
   mEffect->mCurves = mEditCurves;  // copy EditCurvesDialog to main interface
   mEffect->LoadCurves(fileName, true);   // use main interface to load imported curves
   mEditCurves = mEffect->mCurves;  // copy back to this interface
   mEffect->mCurves = temp;   // and reset the main interface how it was
   PopulateList(0);  // update the EditCurvesDialog dialog
   return;
}

void EditCurvesDialog::OnExport( wxCommandEvent & WXUNUSED(event))
{
   FileDialog filePicker(this, _("Export EQ curves as..."), FileNames::DataDir(), wxT(""), wxT("*.XML"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER);   // wxFD_CHANGE_DIR?
   wxString fileName = wxT("");
   if( filePicker.ShowModal() == wxID_CANCEL)
      return;
   else
      fileName = filePicker.GetPath();

   EQCurveArray temp;
   temp = mEffect->mCurves;   // backup the parent's curves
   EQCurveArray exportCurves;   // Copy selected curves to export
   exportCurves.Clear();
   long item = mList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   int i=0;
   while(item >= 0)
   {
      if(item != mList->GetItemCount()-1)   // not 'unnamed'
      {
         exportCurves.Add(mEditCurves[item].Name);
         exportCurves[i].points = mEditCurves[item].points;
         i++;
      }
      else
         wxMessageBox(_("You cannot export 'unnamed' curve, it is special."), _("Cannot Export 'unnamed'"));
      // get next selected item
      item = mList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   }
   if(i>0)
   {
      mEffect->mCurves = exportCurves;
      mEffect->SaveCurves(fileName);
      mEffect->mCurves = temp;
      wxString message;
      message.Printf(_("%d curves exported to %s"), i, fileName.c_str());
      wxMessageBox(message, _("Curves exported"));
   }
   else
      wxMessageBox(_("No curves exported"), _("No curves exported"));
}

void EditCurvesDialog::OnLibrary( wxCommandEvent & WXUNUSED(event))
{
   wxLaunchDefaultBrowser(wxT("http://wiki.audacityteam.org/wiki/EQCurvesDownload"));
}

void EditCurvesDialog::OnDefaults( wxCommandEvent & WXUNUSED(event))
{
   EQCurveArray temp;
   temp = mEffect->mCurves;
   // we expect this to fail in LoadCurves (due to a lack of path) and handle that there
   mEffect->LoadCurves( wxT("EQDefaultCurves.xml") );
   mEditCurves = mEffect->mCurves;
   mEffect->mCurves = temp;
   PopulateList(0);  // update the EditCurvesDialog dialog
}

#ifdef EXPERIMENTAL_EQ_SSE_THREADED

void EqualizationDialog::OnProcessingRadio(wxCommandEvent & event)
{
   int testEvent=event.GetId();
   switch(testEvent)
   {
   case defaultMathRadioID: EffectEqualization48x::SetMathPath(MATH_FUNCTION_ORIGINAL);
      break;
   case sSERadioID: EffectEqualization48x::SetMathPath(MATH_FUNCTION_SSE);
      break;
   case sSEThreadedRadioID: EffectEqualization48x::SetMathPath(MATH_FUNCTION_THREADED | MATH_FUNCTION_SSE);
      break;
   case aVXRadioID: testEvent=2;
      break;
   case aVXThreadedRadioID: testEvent=2;
      break;
   }

};

void EqualizationDialog::OnBench( wxCommandEvent & event)
{
   m_pEffect->mBench=true;
   OnOk(event);
}

#endif

void EditCurvesDialog::OnOK(wxCommandEvent & WXUNUSED(event))
{
   // Make a backup of the current curves
   wxString backupPlace = wxFileName( FileNames::DataDir(), wxT("EQBackup.xml") ).GetFullPath();
   mEffect->SaveCurves(backupPlace);
   // Load back into the main dialog
   mEffect->mCurves.Clear();
   for (unsigned int i = 0; i < mEditCurves.GetCount(); i++)
   {
      mEffect->mCurves.Add(mEditCurves[i].Name);
      mEffect->mCurves[i].points = mEditCurves[i].points;
   }
   mEffect->SaveCurves();
   mEffect->LoadCurves();
//   mEffect->CreateChoice();
   wxGetTopLevelParent(mEffect->mUIParent)->Layout();
//   mEffect->mUIParent->Layout();

   // Select something sensible
   long item = mList->GetNextItem(-1,
      wxLIST_NEXT_ALL,
      wxLIST_STATE_SELECTED);
   if (item == -1)
      item = mList->GetItemCount()-1;   // nothing selected, default to 'unnamed'
   mEffect->setCurve(item);
   EndModal(true);
}

#if wxUSE_ACCESSIBILITY

SliderAx::SliderAx( wxWindow * window, wxString fmt ):
wxWindowAccessible( window )
{
   mParent = window;
   mFmt = fmt;
}

SliderAx::~SliderAx()
{
}

// Retrieves the address of an IDispatch interface for the specified child.
// All objects must support this property.
wxAccStatus SliderAx::GetChild( int childId, wxAccessible** child )
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
wxAccStatus SliderAx::GetChildCount(int* childCount)
{
   *childCount = 3;

   return wxACC_OK;
}

// Gets the default action for this object (0) or > 0 (the action for a child).
// Return wxACC_OK even if there is no action. actionName is the action, or the empty
// string if there is no action.
// The retrieved string describes the action that is performed on an object,
// not what the object does as a result. For example, a toolbar button that prints
// a document has a default action of "Press" rather than "Prints the current document."
wxAccStatus SliderAx::GetDefaultAction( int WXUNUSED(childId), wxString *actionName )
{
   actionName->Clear();

   return wxACC_OK;
}

// Returns the description for this object or a child.
wxAccStatus SliderAx::GetDescription( int WXUNUSED(childId), wxString *description )
{
   description->Clear();

   return wxACC_OK;
}

// Gets the window with the keyboard focus.
// If childId is 0 and child is NULL, no object in
// this subhierarchy has the focus.
// If this object has the focus, child should be 'this'.
wxAccStatus SliderAx::GetFocus(int* childId, wxAccessible** child)
{
   *childId = 0;
   *child = this;

   return wxACC_OK;
}

// Returns help text for this object or a child, similar to tooltip text.
wxAccStatus SliderAx::GetHelpText( int WXUNUSED(childId), wxString *helpText )
{
   helpText->Clear();

   return wxACC_OK;
}

// Returns the keyboard shortcut for this object or child.
// Return e.g. ALT+K
wxAccStatus SliderAx::GetKeyboardShortcut( int WXUNUSED(childId), wxString *shortcut )
{
   shortcut->Clear();

   return wxACC_OK;
}

// Returns the rectangle for this object (id = 0) or a child element (id > 0).
// rect is in screen coordinates.
wxAccStatus SliderAx::GetLocation( wxRect& rect, int WXUNUSED(elementId) )
{
   wxSlider *s = wxDynamicCast( GetWindow(), wxSlider );

   rect = s->GetRect();
   rect.SetPosition( s->GetParent()->ClientToScreen( rect.GetPosition() ) );

   return wxACC_OK;
}

// Gets the name of the specified object.
wxAccStatus SliderAx::GetName(int WXUNUSED(childId), wxString* name)
{
   wxSlider *s = wxDynamicCast( GetWindow(), wxSlider );

   *name = s->GetName();

   return wxACC_OK;
}

// Returns a role constant.
wxAccStatus SliderAx::GetRole(int childId, wxAccRole* role)
{
   switch( childId )
   {
   case 0:
      *role = wxROLE_SYSTEM_SLIDER;
      break;

   case 1:
   case 3:
      *role = wxROLE_SYSTEM_PUSHBUTTON;
      break;

   case 2:
      *role = wxROLE_SYSTEM_INDICATOR;
      break;
   }

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
wxAccStatus SliderAx::GetSelections( wxVariant * WXUNUSED(selections) )
{
   return wxACC_NOT_IMPLEMENTED;
}

// Returns a state constant.
wxAccStatus SliderAx::GetState(int childId, long* state)
{
   wxSlider *s = wxDynamicCast( GetWindow(), wxSlider );

   switch( childId )
   {
   case 0:
      *state = wxACC_STATE_SYSTEM_FOCUSABLE;
      break;

   case 1:
      if( s->GetValue() == s->GetMin() )
      {
         *state = wxACC_STATE_SYSTEM_INVISIBLE;
      }
      break;

   case 3:
      if( s->GetValue() == s->GetMax() )
      {
         *state = wxACC_STATE_SYSTEM_INVISIBLE;
      }
      break;
   }

   // Do not use mSliderIsFocused is not set until after this method
   // is called.
   *state |= ( s == wxWindow::FindFocus() ? wxACC_STATE_SYSTEM_FOCUSED : 0 );

   return wxACC_OK;
}

// Returns a localized string representing the value for the object
// or child.
wxAccStatus SliderAx::GetValue(int childId, wxString* strValue)
{
   wxSlider *s = wxDynamicCast( GetWindow(), wxSlider );

   if( childId == 0 )
   {
      strValue->Printf( mFmt, s->GetValue() );

      return wxACC_OK;
   }

   return wxACC_NOT_SUPPORTED;
}

#endif

