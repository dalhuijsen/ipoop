/**********************************************************************

  Audacity: A Digital Audio Editor

  VSTEffect.cpp

  Dominic Mazzoni

  This class implements a VST Plug-in effect.  The plug-in must be
  loaded in a platform-specific way and passed into the constructor,
  but from here this class handles the interfacing.

**********************************************************************/

//#define VST_DEBUG
//#define DEBUG_VST

// *******************************************************************
// WARNING:  This is NOT 64-bit safe
// *******************************************************************

#if defined(BUILDING_AUDACITY)
#include "../../Audacity.h"
#include "../../PlatformCompatibility.h"

// Make the main function private
#define MODULEMAIN_SCOPE static
#else
#define MODULEMAIN_SCOPE
#define USE_VST 1
#endif

#if USE_VST

#include <limits.h>
#include <stdio.h>

#include <wx/app.h>
#include <wx/defs.h>
#include <wx/buffer.h>
#include <wx/busyinfo.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/dcclient.h>
#include <wx/dialog.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/frame.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/module.h>
#include <wx/msgdlg.h>
#include <wx/process.h>
#include <wx/progdlg.h>
#include <wx/recguard.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/scrolwin.h>
#include <wx/sstream.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>

#if defined(__WXMAC__)
#include <dlfcn.h>
#include <wx/mac/private.h>
#elif defined(__WXMSW__)
#include <wx/dynlib.h>
#include <wx/msw/seh.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi")
#else
// Includes for GTK are later since they cause conflicts with our class names
#endif

// TODO:  Unfortunately we have some dependencies on Audacity provided 
//        dialogs, widgets and other stuff.  This will need to be cleaned up.

#include "FileDialog.h"
#include "../../FileNames.h"
#include "../../Internat.h"
#include "../../PlatformCompatibility.h"
#include "../../ShuttleGui.h"
#include "../../effects/Effect.h"
#include "../../widgets/NumericTextCtrl.h"
#include "../../widgets/valnum.h"
#include "../../xml/XMLFileReader.h"
#include "../../xml/XMLWriter.h"

#include "audacity/ConfigInterface.h"

// Must include after ours since we have a lot of name collisions
#if defined(__WXGTK__)
#include <dlfcn.h>
#define Region XRegion     // Conflicts with Audacity's Region structure
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#undef Region
#endif

#include "VSTEffect.h"

// NOTE:  To debug the subprocess, use wxLogDebug and, on Windows, Debugview
//        from TechNet (Sysinternals).

// ============================================================================
//
// Module registration entry point
//
// This is the symbol that Audacity looks for when the module is built as a
// dynamic library.
//
// When the module is builtin to Audacity, we use the same function, but it is
// declared static so as not to clash with other builtin modules.
//
// ============================================================================
MODULEMAIN_SCOPE ModuleInterface *AudacityModule(ModuleManagerInterface *moduleManager,
                                                 const wxString *path)
{
   // Create our effects module and register
   return new VSTEffectsModule(moduleManager, path);
}

#if defined(BUILDING_AUDACITY)
// ============================================================================
//
// Register this as a builtin module
// 
// We also take advantage of the fact that wxModules are initialized before
// the wxApp::OnInit() method is called.  We check to see if Audacity was
// executed to scan a VST effect in a different process.
//
// ============================================================================
DECLARE_BUILTIN_MODULE(VSTBuiltin);

class VSTSubEntry : public wxModule
{
public:
   bool OnInit()
   {
      // Have we been started to check a plugin?
      if (wxTheApp && wxTheApp->argc == 3 && wxStrcmp(wxTheApp->argv[1], VSTCMDKEY) == 0)
      {
         // NOTE:  This can really hide failures, which is what we want for those pesky
         //        VSTs that are bad or that our support isn't currect.  But, it can also
         //        hide Audacity failures in the subprocess, so if you're having an unruley
         //        VST or odd Audacity failures, comment it out and you might get more info.
         //wxHandleFatalExceptions();
         VSTEffectsModule::Check(wxTheApp->argv[2]);

         // Returning false causes default processing to display a message box, but we don't
         // want that so disable logging.
         wxLog::EnableLogging(false);

         return false;
      }

      return true;
   };

   void OnExit() {};

   DECLARE_DYNAMIC_CLASS(VSTSubEntry);
};
IMPLEMENT_DYNAMIC_CLASS(VSTSubEntry, wxModule);

#endif

//----------------------------------------------------------------------------
// VSTSubProcess
//----------------------------------------------------------------------------
#define OUTPUTKEY wxT("<VSTLOADCHK>-")
enum InfoKeys
{
   kKeySubIDs,
   kKeyBegin,
   kKeyName,
   kKeyPath,
   kKeyVendor,
   kKeyVersion,
   kKeyDescription,
   kKeyEffectType,
   kKeyInteractive,
   kKeyAutomatable,
   kKeyEnd
};

class VSTSubProcess : public wxProcess,
                      public EffectIdentInterface
{
public:
   VSTSubProcess()
   {
      Redirect();
   }

   // EffectClientInterface implementation

   wxString GetPath()
   {
      return mPath;
   }

   wxString GetSymbol()
   {
      return mName;
   }

   wxString GetName()
   {
      return GetSymbol();
   }

   wxString GetVendor()
   {
      return mVendor;
   }

   wxString GetVersion()
   {
      return mVersion;
   }

   wxString GetDescription()
   {
      return mDescription;
   }

   wxString GetFamily()
   {
      return VSTPLUGINTYPE;
   }

   EffectType GetType()
   {
      return mType;
   }

   bool IsInteractive()
   {
      return mInteractive;
   }

   bool IsDefault()
   {
      return false;
   }

   bool IsLegacy()
   {
      return false;
   }

   bool SupportsRealtime()
   {
      return mType == EffectTypeProcess;
   }

   bool SupportsAutomation()
   {
      return mAutomatable;
   }

public:
   wxString mPath;
   wxString mName;
   wxString mVendor;
   wxString mVersion;
   wxString mDescription;
   EffectType mType;
   bool mInteractive;
   bool mAutomatable;
};

// ============================================================================
//
// VSTEffectsModule
//
// ============================================================================
VSTEffectsModule::VSTEffectsModule(ModuleManagerInterface *moduleManager,
                                   const wxString *path)
{
   mModMan = moduleManager;
   if (path)
   {
      mPath = *path;
   }
}

VSTEffectsModule::~VSTEffectsModule()
{
   mPath = wxEmptyString;
}

// ============================================================================
// IdentInterface implementation
// ============================================================================

wxString VSTEffectsModule::GetPath()
{
   return mPath;
}

wxString VSTEffectsModule::GetSymbol()
{
   return XO("VST Effects");
}

wxString VSTEffectsModule::GetName()
{
   return GetSymbol();
}

wxString VSTEffectsModule::GetVendor()
{
   return XO("The Audacity Team");
}

wxString VSTEffectsModule::GetVersion()
{
   // This "may" be different if this were to be maintained as a separate DLL
   return AUDACITY_VERSION_STRING;
}

wxString VSTEffectsModule::GetDescription()
{
   return XO("Adds the ability to use VST effects in Audacity.");
}

// ============================================================================
// ModuleInterface implementation
// ============================================================================

bool VSTEffectsModule::Initialize()
{
   // Nothing to do here
   return true;
}

void VSTEffectsModule::Terminate()
{
   // Nothing to do here
   return;
}

bool VSTEffectsModule::AutoRegisterPlugins(PluginManagerInterface & WXUNUSED(pm))
{
   // We don't auto-register
   return true;
}

wxArrayString VSTEffectsModule::FindPlugins(PluginManagerInterface & pm)
{
   wxArrayString pathList;
   wxArrayString files;

   // Check for the VST_PATH environment variable
   wxString vstpath = wxString::FromUTF8(getenv("VST_PATH"));
   if (!vstpath.empty())
   {
      wxStringTokenizer tok(vstpath);
      while (tok.HasMoreTokens())
      {
         pathList.Add(tok.GetNextToken());
      }
   }

#if defined(__WXMAC__)  
#define VSTPATH wxT("/Library/Audio/Plug-Ins/VST")

   // Look in ~/Library/Audio/Plug-Ins/VST and /Library/Audio/Plug-Ins/VST
   pathList.Add(wxGetHomeDir() + wxFILE_SEP_PATH + VSTPATH);
   pathList.Add(VSTPATH);

   // Recursively search all paths for Info.plist files.  This will identify all
   // bundles.
   pm.FindFilesInPathList(wxT("Info.plist"), pathList, files, true);

   // Remove the 'Contents/Info.plist' portion of the names
   for (size_t i = 0; i < files.GetCount(); i++)
   {
      files[i] = wxPathOnly(wxPathOnly(files[i]));
      if (!files[i].EndsWith(wxT(".vst")))
      {
         files.RemoveAt(i--);
      }
   }

#elif defined(__WXMSW__)

   TCHAR dpath[MAX_PATH];
   TCHAR tpath[MAX_PATH];
   DWORD len;

   // Try HKEY_CURRENT_USER registry key first
   len = WXSIZEOF(tpath);
   if (SHRegGetUSValue(wxT("Software\\VST"),
                       wxT("VSTPluginsPath"),
                       NULL,
                       tpath,
                       &len,
                       FALSE,
                       NULL,
                       0) == ERROR_SUCCESS)
   {
      tpath[len] = 0;
      dpath[0] = 0;
      ExpandEnvironmentStrings(tpath, dpath, WXSIZEOF(dpath));
      pathList.Add(dpath);
   }

   // Then try HKEY_LOCAL_MACHINE registry key
   len = WXSIZEOF(tpath);
   if (SHRegGetUSValue(wxT("Software\\VST"),
                       wxT("VSTPluginsPath"),
                       NULL,
                       tpath,
                       &len,
                       TRUE,
                       NULL,
                       0) == ERROR_SUCCESS)
   {
      tpath[len] = 0;
      dpath[0] = 0;
      ExpandEnvironmentStrings(tpath, dpath, WXSIZEOF(dpath));
      pathList.Add(dpath);
   }

   // Add the default path last
   dpath[0] = 0;
   ExpandEnvironmentStrings(wxT("%ProgramFiles%\\Steinberg\\VSTPlugins"),
                            dpath,
                            WXSIZEOF(dpath));
   pathList.Add(dpath);

   // Recursively scan for all DLLs
   pm.FindFilesInPathList(wxT("*.dll"), pathList, files, true);

#else

   // Nothing specified in the VST_PATH environment variable...provide defaults
   if (vstpath.IsEmpty())
   {
      // We add this "non-default" one
      pathList.Add(wxT(LIBDIR) wxT("/vst"));

      // These are the defaults used by other hosts
      pathList.Add(wxT("/usr/lib/vst"));
      pathList.Add(wxT("/usr/local/lib/vst"));
      pathList.Add(wxGetHomeDir() + wxFILE_SEP_PATH + wxT(".vst"));
   }

   // Recursively scan for all shared objects
   pm.FindFilesInPathList(wxT("*.so"), pathList, files, true);

#endif

   return files;
}

bool VSTEffectsModule::RegisterPlugin(PluginManagerInterface & pm, const wxString & path)
{
   // TODO:  Fix this for external usage
   wxString cmdpath = PlatformCompatibility::GetExecutablePath();

   wxString effectIDs = wxT("0;");
   wxStringTokenizer effectTzr(effectIDs, wxT(";"));

   wxProgressDialog *progress = NULL;
   size_t idCnt = 0;
   size_t idNdx = 0;

   bool valid = false;
   bool cont = true;

   while (effectTzr.HasMoreTokens() && cont)
   {
      wxString effectID = effectTzr.GetNextToken();

      wxString cmd;
      cmd.Printf(wxT("\"%s\" %s \"%s;%s\""), cmdpath.c_str(), VSTCMDKEY, path.c_str(), effectID.c_str());

      VSTSubProcess *proc = new VSTSubProcess();
      try
      {
         int flags = wxEXEC_SYNC | wxEXEC_NODISABLE;
#if defined(__WXMSW__)
         flags += wxEXEC_NOHIDE;
#endif
         wxExecute(cmd, flags, proc);
      }
      catch (...)
      {
         wxLogMessage(_("VST plugin registration failed for %s\n"), path.c_str());
         delete proc;
         return false;
      }

      wxString output;
      wxStringOutputStream ss(&output);
      proc->GetInputStream()->Read(ss);

      int keycount = 0;
      bool haveBegin = false;
      wxStringTokenizer tzr(output, wxT("\n"));
      while (tzr.HasMoreTokens())
      {
         wxString line = tzr.GetNextToken();

         // Our output may follow any output the plugin may have written.
         if (!line.StartsWith(OUTPUTKEY))
         {
            continue;
         }

         long key;
         if (!line.Mid(wxStrlen(OUTPUTKEY)).BeforeFirst(wxT('=')).ToLong(&key))
         {
            continue;
         }
         wxString val = line.AfterFirst(wxT('=')).BeforeFirst(wxT('\r'));

         switch (key)
         {
            case kKeySubIDs:
               effectIDs = val;
               effectTzr.Reinit(effectIDs);
               idCnt = effectTzr.CountTokens();
               if (idCnt > 3)
               {
                  progress = new wxProgressDialog(_("Scanning Shell VST"),
                                                  wxString::Format(_("Registering %d of %d: %-64.64s"), 0, idCnt, proc->GetName().c_str()),
                                                  idCnt,
                                                  NULL,
                                                  wxPD_APP_MODAL |
                                                  wxPD_AUTO_HIDE |
                                                  wxPD_CAN_ABORT |
                                                  wxPD_ELAPSED_TIME |
                                                  wxPD_ESTIMATED_TIME |
                                                  wxPD_REMAINING_TIME);
                  progress->Show();
               }
            break;

            case kKeyBegin:
               haveBegin = true;
               keycount++;
            break;

            case kKeyName:
               proc->mName = val;
               keycount++;
            break;

            case kKeyPath:
               proc->mPath = val;
               keycount++;
            break;

            case kKeyVendor:
               proc->mVendor = val;
               keycount++;
            break;

            case kKeyVersion:
               proc->mVersion = val;
               keycount++;
            break;

            case kKeyDescription:
               proc->mDescription = val;
               keycount++;
            break;

            case kKeyEffectType:
               long type;
               val.ToLong(&type);
               proc->mType = (EffectType) type;
               keycount++;
            break;

            case kKeyInteractive:
               proc->mInteractive = val.IsSameAs(wxT("1"));
               keycount++;
            break;

            case kKeyAutomatable:
               proc->mAutomatable = val.IsSameAs(wxT("1"));
               keycount++;
            break;

            case kKeyEnd:
            {
               if (!haveBegin || ++keycount != kKeyEnd)
               {
                  keycount = 0;
                  haveBegin = false;
                  continue;
               }

               bool skip = false;
               if (progress)
               {
                  idNdx++;
                  cont = progress->Update(idNdx,
                                          wxString::Format(_("Registering %d of %d: %-64.64s"), idNdx, idCnt, proc->GetName().c_str()));
               }

               if (!skip && cont)
               {
                  valid = true;
                  pm.RegisterPlugin(this, proc);
               }
            }
            break;

            default:
               keycount = 0;
               haveBegin = false;
            break;
         }
      }

      delete proc;
   }

   if (progress)
   {
      delete progress;
   }

   return valid;
}

bool VSTEffectsModule::IsPluginValid(const wxString & path)
{
   wxString realPath = path.BeforeFirst(wxT(';'));
   return wxFileName::FileExists(realPath) || wxFileName::DirExists(realPath);
}

IdentInterface *VSTEffectsModule::CreateInstance(const wxString & path)
{
   // For us, the ID is simply the path to the effect
   return new VSTEffect(path);
}

void VSTEffectsModule::DeleteInstance(IdentInterface *instance)
{
   VSTEffect *effect = dynamic_cast<VSTEffect *>(instance);
   if (effect)
   {
      delete effect;
   }
}

// ============================================================================
// ModuleEffectInterface implementation
// ============================================================================

// ============================================================================
// VSTEffectsModule implementation
// ============================================================================

// static
//
// Called from reinvokation of Audacity or DLL to check in a separate process
void VSTEffectsModule::Check(const wxChar *path)
{
   VSTEffect *effect = new VSTEffect(path);
   if (effect)
   {
      if (effect->SetHost(NULL))
      {
         wxArrayInt effectIDs = effect->GetEffectIDs();
         wxString out;

         if (effectIDs.GetCount() > 0)
         {
            wxString subids;

            for (size_t i = 0, cnt = effectIDs.GetCount(); i < cnt; i++)
            {
               subids += wxString::Format(wxT("%d;"), effectIDs[i]);
            }

            out = wxString::Format(wxT("%s%d=%s\n"), OUTPUTKEY, kKeySubIDs, subids.RemoveLast().c_str());
         }
         else
         {
            out += wxString::Format(wxT("%s%d=%s\n"), OUTPUTKEY, kKeyBegin, wxEmptyString);
            out += wxString::Format(wxT("%s%d=%s\n"), OUTPUTKEY, kKeyPath, effect->GetPath().c_str());
            out += wxString::Format(wxT("%s%d=%s\n"), OUTPUTKEY, kKeyName, effect->GetName().c_str());
            out += wxString::Format(wxT("%s%d=%s\n"), OUTPUTKEY, kKeyVendor, effect->GetVendor().c_str());
            out += wxString::Format(wxT("%s%d=%s\n"), OUTPUTKEY, kKeyVersion, effect->GetVersion().c_str());
            out += wxString::Format(wxT("%s%d=%s\n"), OUTPUTKEY, kKeyDescription, effect->GetDescription().c_str());
            out += wxString::Format(wxT("%s%d=%d\n"), OUTPUTKEY, kKeyEffectType, effect->GetType());
            out += wxString::Format(wxT("%s%d=%d\n"), OUTPUTKEY, kKeyInteractive, effect->IsInteractive());
            out += wxString::Format(wxT("%s%d=%d\n"), OUTPUTKEY, kKeyAutomatable, effect->SupportsAutomation());
            out += wxString::Format(wxT("%s%d=%s\n"), OUTPUTKEY, kKeyEnd, wxEmptyString);
         }

         // We want to output info in one chunk to prevent output
         // from the effect intermixing with the info
         const wxCharBuffer buf = out.ToUTF8();
         fwrite(buf, 1, strlen(buf), stdout);
         fflush(stdout);
      }

      delete effect;
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// VSTEffectOptionsDialog
//
///////////////////////////////////////////////////////////////////////////////

class VSTEffectOptionsDialog:public wxDialog
{
public:
   VSTEffectOptionsDialog(wxWindow * parent, EffectHostInterface *host);
   virtual ~VSTEffectOptionsDialog();

   void PopulateOrExchange(ShuttleGui & S);

   void OnOk(wxCommandEvent & evt);

private:
   EffectHostInterface *mHost;
   int mBufferSize;
   bool mUseLatency;
   bool mUseGUI;

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(VSTEffectOptionsDialog, wxDialog)
   EVT_BUTTON(wxID_OK, VSTEffectOptionsDialog::OnOk)
END_EVENT_TABLE()

VSTEffectOptionsDialog::VSTEffectOptionsDialog(wxWindow * parent, EffectHostInterface *host)
:  wxDialog(parent, wxID_ANY, wxString(_("VST Effect Options")))
{
   mHost = host;

   mHost->GetSharedConfig(wxT("Options"), wxT("BufferSize"), mBufferSize, 8192);
   mHost->GetSharedConfig(wxT("Options"), wxT("UseLatency"), mUseLatency, true);
   mHost->GetSharedConfig(wxT("Options"), wxT("UseGUI"), mUseGUI, true);

   ShuttleGui S(this, eIsCreating);
   PopulateOrExchange(S);
}

VSTEffectOptionsDialog::~VSTEffectOptionsDialog()
{
}

void VSTEffectOptionsDialog::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(5);
   S.StartHorizontalLay(wxEXPAND, 1);
   {
      S.StartVerticalLay(false);
      {
         S.StartStatic(_("Buffer Size"));
         {
            IntegerValidator<int> vld(&mBufferSize);
            vld.SetRange(8, 1048576 * 1);

            S.AddVariableText(wxString() +
               _("The buffer size controls the number of samples sent to the effect ") +
               _("on each iteration. Smaller values will cause slower processing and ") +
               _("some effects require 8192 samples or less to work properly. However ") +
               _("most effects can accept large buffers and using them will greatly ") +
               _("reduce processing time."))->Wrap(650);

            S.StartHorizontalLay(wxALIGN_LEFT);
            {
               wxTextCtrl *t;
               t = S.TieNumericTextBox(_("&Buffer Size (8 to 1048576 samples):"),
                                       mBufferSize,
                                       12);
               t->SetMinSize(wxSize(100, -1));
               t->SetValidator(vld);
            }
            S.EndHorizontalLay();
         }
         S.EndStatic();

         S.StartStatic(_("Latency Compensation"));
         {
            S.AddVariableText(wxString() +
               _("As part of their processing, some VST effects must delay returning ") +
               _("audio to Audacity. When not compensating for this delay, you will ") +
               _("notice that small silences have been inserted into the audio. ") +
               _("Enabling this option will provide that compensation, but it may ") +
               _("not work for all VST effects."))->Wrap(650);

            S.StartHorizontalLay(wxALIGN_LEFT);
            {
               S.TieCheckBox(_("Enable &compensation"),
                             mUseLatency);
            }
            S.EndHorizontalLay();
         }
         S.EndStatic();

         S.StartStatic(_("Graphical Mode"));
         {
            S.AddVariableText(wxString() +
               _("Most VST effects have a graphical interface for setting parameter values.") +
               _(" A basic text-only method is also available. ") +
               _(" Reopen the effect for this to take effect."))->Wrap(650);
            S.TieCheckBox(_("Enable &graphical interface"),
                          mUseGUI);
         }
         S.EndStatic();
      }
      S.EndVerticalLay();
   }
   S.EndHorizontalLay();

   S.AddStandardButtons();

   Layout();
   Fit();
   Center();
}

void VSTEffectOptionsDialog::OnOk(wxCommandEvent & WXUNUSED(evt))
{
   if (!Validate())
   {
      return;
   }

   ShuttleGui S(this, eIsGettingFromDialog);
   PopulateOrExchange(S);

   mHost->SetSharedConfig(wxT("Options"), wxT("BufferSize"), mBufferSize);
   mHost->SetSharedConfig(wxT("Options"), wxT("UseLatency"), mUseLatency);
   mHost->SetSharedConfig(wxT("Options"), wxT("UseGUI"), mUseGUI);

   EndModal(wxID_OK);
}

///////////////////////////////////////////////////////////////////////////////
//
// VSTEffectTimer
//
///////////////////////////////////////////////////////////////////////////////

class VSTEffectTimer : public wxTimer
{
public:
   VSTEffectTimer(VSTEffect *effect)
   :  wxTimer(),
      mEffect(effect)
   {
   }

   ~VSTEffectTimer()
   {
   }

   void Notify()
   {
      mEffect->OnTimer();
   }

private:
   VSTEffect *mEffect;
};

///////////////////////////////////////////////////////////////////////////////
//
// VSTEffect
//
///////////////////////////////////////////////////////////////////////////////

enum
{
   ID_Duration = 20000,
   ID_Sliders = 21000,
};

DEFINE_LOCAL_EVENT_TYPE(EVT_SIZEWINDOW);
DEFINE_LOCAL_EVENT_TYPE(EVT_UPDATEDISPLAY);

BEGIN_EVENT_TABLE(VSTEffect, wxEvtHandler)
   EVT_COMMAND_RANGE(ID_Sliders, ID_Sliders + 999, wxEVT_COMMAND_SLIDER_UPDATED, VSTEffect::OnSlider)

   // Events from the audioMaster callback
   EVT_COMMAND(wxID_ANY, EVT_SIZEWINDOW, VSTEffect::OnSizeWindow)
END_EVENT_TABLE()

#if defined(__WXMAC__)

// To use, change the SDK in the project to at least 10.5
extern void DebugPrintControlHierarchy(WindowRef inWindow);
extern void DebugPrintWindowList(void);

// Event handler to track when the mouse enters/exits the various view
static const EventTypeSpec trackingEventList[] =
{                             
   {kEventClassControl, kEventControlTrackingAreaEntered},
   {kEventClassControl, kEventControlTrackingAreaExited},
};                            
                              
pascal OSStatus VSTEffect::TrackingEventHandler(EventHandlerCallRef handler, EventRef event, void *data)
{
   return ((VSTEffect *)data)->OnTrackingEvent(event);
}

OSStatus VSTEffect::OnTrackingEvent(EventRef event)
{
   OSStatus result = eventNotHandledErr;
   
   if (GetEventKind(event) == kEventControlTrackingAreaEntered)
   {
      // Should we save the existing cursor???
      SetThemeCursor(kThemeArrowCursor);
   }

   if (GetEventKind(event) == kEventControlTrackingAreaExited)
   {
      // Possibly restore a saved cursor
   }

   return result;
}

// ----------------------------------------------------------------------------
// Most of the following is used to deal with VST effects that create an overlay
// window on top of ours.  This is usually done because Cocoa is being used
// instead of Carbon.
//
// That works just fine...usually.  But, we display the effect in a modal dialog
// box and, since that overlay window is just another window in the application,
// the modality of the dialog causes the overlay to be disabled and the user
// can't interact with the effect.
//
// Examples of these effects would be BlueCat's Freeware Pack and GRM Tools,
// though I'm certain there are other's out there.  Anything JUCE based would
// affected...that's what GRM Tools uses.
//
// So, to work around the problem (without moving to Cocoa or wxWidgets 3.x),
// we install an event handler if the overlay is detected.  This handler and
// the companion handler on our window use the kEventWindowGetClickModality
// event to tell the system that events can be passed to our window and the
// overlay window.
//
// In addition, there's some window state management that must be dealt with
// to keep our window from becoming unhightlighted when the floater is clicked.
// ----------------------------------------------------------------------------

// Events to be captured in the overlay window
static const EventTypeSpec OverlayEventList[] =
{
#if 0
   { kEventClassMouse,  kEventMouseDown },
   { kEventClassMouse,  kEventMouseUp },
   { kEventClassMouse,  kEventMouseMoved },
   { kEventClassMouse,  kEventMouseDragged },
   { kEventClassMouse,  kEventMouseEntered },
   { kEventClassMouse,  kEventMouseExited },
   { kEventClassMouse,  kEventMouseWheelMoved },
#endif
};

// Overlay window event handler callback thunk
pascal OSStatus VSTEffect::OverlayEventHandler(EventHandlerCallRef handler, EventRef event, void *data)
{
   return ((VSTEffect *)data)->OnOverlayEvent(handler, event);
}

// Overlay window event handler
OSStatus VSTEffect::OnOverlayEvent(EventHandlerCallRef handler, EventRef event)
{
   // Get the current window in front of all the rest of the non-floaters.
   WindowRef frontwin = FrontNonFloatingWindow();

   // Get the target window of the event
   WindowRef evtwin = 0;
   GetEventParameter(event,
                     kEventParamDirectObject,
                     typeWindowRef,
                     NULL,
                     sizeof(evtwin),
                     NULL,
                     &evtwin);

#if defined(DEBUG_VST)
   int cls = GetEventClass(event);
   printf("OVERLAY class %4.4s kind %d ewin %p owin %p mwin %p anf %p fnf %p\n",
      &cls,
      GetEventKind(event),
      evtwin,
      mOverlayRef,
      mWindowRef,
      ActiveNonFloatingWindow(),
      frontwin);
#endif

   bool block = false;
   WindowModality kind;
   WindowRef ref = NULL;
   GetWindowModality(frontwin, &kind, &ref);

   switch (kind)
   {
      case kWindowModalityNone:
      {
         // Allow
      }
      break;

      case kWindowModalityWindowModal:
      {
         if (ref == mWindowRef || ref == mOverlayRef)
         {
            block = true;
         }
      }
      break;

      case kWindowModalitySystemModal:
      case kWindowModalityAppModal:
      {
         if (frontwin != mWindowRef && frontwin != mOverlayRef)
         {
            block = true;
         }
      }
      break;
   }

   // We must block mouse events because plugins still act on mouse
   // movement and drag events, even if they are supposed to be disabled
   // due to other modal dialogs (like when Load or Settings are clicked).
   if (GetEventClass(event) == kEventClassMouse)
   {
      if (block)
      {
         return noErr;
      }
      
      return eventNotHandledErr;
   }

   // Only kEventClassWindow events at this point
   switch (GetEventKind(event))
   {
      // The system is asking if the target of an upcoming event
      // should be passed to the overlay window or not.
      //
      // We allow it when the overlay window or our window is the
      // curret top window.  Any other windows would mean that a
      // modal dialog box has been opened on top and we should block.
      case kEventWindowGetClickModality:
      {
         // Announce the event may need blocking
         HIModalClickResult res = block ? kHIModalClickIsModal | kHIModalClickAnnounce : 0;

         // Set the return parameters
         SetEventParameter(event,
                           kEventParamWindowModality,
                           typeWindowRef,
                           sizeof(kind),
                           &kind);

         SetEventParameter(event,
                           kEventParamModalWindow,
                           typeWindowRef,
                           sizeof(ref),
                           &ref);

         SetEventParameter(event,
                           kEventParamModalClickResult,
                           typeModalClickResult,
                           sizeof(res),
                           &res);

         return noErr;
      }
      break;
   }

   return eventNotHandledErr;
}

// Events to be captured in the our window
static const EventTypeSpec WindowEventList[] =
{
   { kEventClassWindow, kEventWindowGetClickModality },
   { kEventClassWindow, kEventWindowShown },
   { kEventClassWindow, kEventWindowClose },
#if 0
   { kEventClassMouse,  kEventMouseDown },
   { kEventClassMouse,  kEventMouseUp },
   { kEventClassMouse,  kEventMouseMoved },
   { kEventClassMouse,  kEventMouseDragged },
   { kEventClassMouse,  kEventMouseEntered },
   { kEventClassMouse,  kEventMouseExited },
   { kEventClassMouse,  kEventMouseWheelMoved },
#endif
};

// Our window event handler callback thunk
pascal OSStatus VSTEffect::WindowEventHandler(EventHandlerCallRef handler, EventRef event, void *data)
{
   return ((VSTEffect *)data)->OnWindowEvent(handler, event);
}

// Our window event handler
OSStatus VSTEffect::OnWindowEvent(EventHandlerCallRef handler, EventRef event)
{
   // Get the current window in from of all the rest non-floaters.
   WindowRef frontwin = FrontNonFloatingWindow();

   // Get the target window of the event
   WindowRef evtwin = 0;
   GetEventParameter(event,
                     kEventParamDirectObject,
                     typeWindowRef,
                     NULL,
                     sizeof(evtwin),
                     NULL,
                     &evtwin);

#if defined(DEBUG_VST)
   int cls = GetEventClass(event);
   printf("WINDOW class %4.4s kind %d ewin %p owin %p mwin %p anf %p fnf %p\n",
      &cls,
      GetEventKind(event),
      evtwin,
      mOverlayRef,
      mWindowRef,
      ActiveNonFloatingWindow(),
      frontwin);
#endif

   bool block = false;
   WindowModality kind;
   WindowRef ref = NULL;
   GetWindowModality(frontwin, &kind, &ref);

   switch (kind)
   {
      case kWindowModalityNone:
      {
         // Allow
      }
      break;

      case kWindowModalityWindowModal:
      {
         if (ref == mWindowRef || ref == mOverlayRef)
         {
            block = true;
         }
      }
      break;

      case kWindowModalitySystemModal:
      case kWindowModalityAppModal:
      {
         if (frontwin != mWindowRef && frontwin != mOverlayRef)
         {
            block = true;
         }
      }
      break;
   }

   // We must block mouse events because plugins still act on mouse
   // movement and drag events, even if they are supposed to be disabled
   // due to other modal dialogs (like when Load or Settings are clicked).
   if (GetEventClass(event) == kEventClassMouse)
   {
      if (block)
      {
         return noErr;
      }

      return eventNotHandledErr;
   }

   // Only kEventClassWindow events at this point
   switch (GetEventKind(event))
   {
      // If we don't capture the close event, Audacity will crash at termination
      // since the window is still on the wxWidgets toplevel window lists, but
      // it has already been deleted from the system.
      case kEventWindowClose:
      {
         RemoveHandler();
         mDialog->Close();
         return noErr;
      }
      break;

      // This is where we determine if the effect has created a window above
      // ours.  Since the overlay is created on top of our window, we look at
      // the topmost window to see if it is different that ours.  If so, then
      // we assume an overlay has been created and install the event handler
      // on the overlay.
      case kEventWindowShown:
      {
         // Have an overlay?
         WindowRef newprev = GetPreviousWindow(mWindowRef);

         if (newprev != mPreviousRef)
         {
            // We have an overlay
            mOverlayRef = newprev;

            // Set our window's activatino scope to make sure it alway
            // stays active.
            SetWindowActivationScope(mWindowRef,
                                     kWindowActivationScopeIndependent);

            // Install the overlay handler
            mOverlayEventHandlerUPP = NewEventHandlerUPP(OverlayEventHandler);
            InstallWindowEventHandler(mOverlayRef,
                                      mOverlayEventHandlerUPP,
                                      GetEventTypeCount(OverlayEventList),
                                      OverlayEventList,
                                      this,
                                      &mOverlayEventHandlerRef);

            ControlRef root = HIViewGetRoot(mOverlayRef);
            HIViewRef view;
            HIViewFindByID(root, kHIViewWindowContentID, &view);
            InstallControlEventHandler(root,
                                       mTrackingHandlerUPP,
                                       GetEventTypeCount(trackingEventList),
                                       trackingEventList,
                                       this,
                                       &mOverlayRootTrackingHandlerRef);
            InstallControlEventHandler(view,
                                       mTrackingHandlerUPP,
                                       GetEventTypeCount(trackingEventList),
                                       trackingEventList,
                                       this,
                                       &mOverlayViewTrackingHandlerRef);
            HIViewNewTrackingArea(root, NULL, 0, NULL);
            HIViewNewTrackingArea(view, NULL, 0, NULL);
         }
      }
      break;

      // The system is asking if the target of an upcoming event
      // should be passed to the overlay window or not.
      //
      // We allow it when the overlay window or our window is the
      // curret top window.  Any other windows would mean that a
      // modal dialog box has been opened on top and we should block.
      case kEventWindowGetClickModality:
      {
         // Announce the event may need blocking
         HIModalClickResult res = block ? kHIModalClickIsModal | kHIModalClickAnnounce : 0;

         // Set the return parameters
         SetEventParameter(event,
                           kEventParamWindowModality,
                           typeWindowRef,
                           sizeof(kind),
                           &kind);

         SetEventParameter(event,
                           kEventParamModalWindow,
                           typeWindowRef,
                           sizeof(ref),
                           &ref);

         SetEventParameter(event,
                           kEventParamModalClickResult,
                           typeModalClickResult,
                           sizeof(res),
                           &res);

         if (mOverlayRef)
         {
            // If the front window is the overlay, then make our window
            // the selected one so that the mouse click go to it instead.
            WindowRef act = ActiveNonFloatingWindow();
            if (frontwin == mOverlayRef || act == NULL || act == mOverlayRef)
            {
               SelectWindow(mWindowRef);
            }
         }

         return noErr;
      }
      break;
   }

   return eventNotHandledErr;
}
#endif

#if defined(__WXGTK__)

static int trappedErrorCode = 0;
static int X11TrapHandler(Display *, XErrorEvent *err)
{
    return 0;
}
#endif

// Needed to support shell plugins...sucks, but whatcha gonna do???
intptr_t VSTEffect::mCurrentEffectID;

typedef AEffect *(*vstPluginMain)(audioMasterCallback audioMaster);

intptr_t VSTEffect::AudioMaster(AEffect * effect,
                                int32_t opcode,
                                int32_t index,
                                intptr_t value,
                                void * ptr,
                                float opt)
{
   VSTEffect *vst = (effect ? (VSTEffect *) effect->ptr2 : NULL);

   // Handles operations during initialization...before VSTEffect has had a
   // chance to set its instance pointer.
   switch (opcode)
   {
      case audioMasterVersion:
         return (intptr_t) 2400;

      case audioMasterCurrentId:
         return mCurrentEffectID;

      case audioMasterGetVendorString:
         strcpy((char *) ptr, "Audacity Team");    // Do not translate, max 64 + 1 for null terminator
         return 1;

      case audioMasterGetProductString:
         strcpy((char *) ptr, "Audacity");         // Do not translate, max 64 + 1 for null terminator
         return 1;

      case audioMasterGetVendorVersion:
         return (intptr_t) (AUDACITY_VERSION << 24 |
                            AUDACITY_RELEASE << 16 |
                            AUDACITY_REVISION << 8 |
                            AUDACITY_MODLEVEL);

      // Some (older) effects depend on an effIdle call when requested.  An
      // example is the Antress Modern plugins which uses the call to update
      // the editors display when the program (preset) changes.
      case audioMasterNeedIdle:
         if (vst)
         {
            vst->NeedIdle();
            return 1;
         }
         return 0;

      // We would normally get this if the effect editor is dipslayed and something "major"
      // has changed (like a program change) instead of multiple automation calls.
      // Since we don't do anything with the parameters while the editor is displayed,
      // there's no need for us to do anything.
      case audioMasterUpdateDisplay:
         if (vst)
         {
            vst->UpdateDisplay();
            return 1;
         }
         return 0;

      // Return the current time info.
      case audioMasterGetTime:
         if (vst)
         {
            return (intptr_t) vst->GetTimeInfo();
         }
         return 0;

      // Inputs, outputs, or initial delay has changed...all we care about is initial delay.
      case audioMasterIOChanged:
         if (vst)
         {
            vst->SetBufferDelay(effect->initialDelay);
            return 1;
         }
         return 0;

      case audioMasterGetSampleRate:
         if (vst)
         {
            return (intptr_t) vst->GetSampleRate();
         }
         return 0;

      case audioMasterIdle:
         wxYieldIfNeeded();
         return 1;

      case audioMasterGetCurrentProcessLevel:
         if (vst)
         {
            return vst->GetProcessLevel();
         }
         return 0;

      case audioMasterGetLanguage:
         return kVstLangEnglish;

      // We always replace, never accumulate
      case audioMasterWillReplaceOrAccumulate:
         return 1;

      // Resize the window to accommodate the effect size
      case audioMasterSizeWindow:
         if (vst)
         {
            vst->SizeWindow(index, value);
         }
         return 1;

      case audioMasterCanDo:
      {
         char *s = (char *) ptr;
         if (strcmp(s, "acceptIOChanges") == 0 ||
            strcmp(s, "sendVstTimeInfo") == 0 ||
            strcmp(s, "startStopProcess") == 0 ||
            strcmp(s, "shellCategory") == 0 ||
            strcmp(s, "sizeWindow") == 0)
         {
            return 1;
         }

#if defined(VST_DEBUG)
#if defined(__WXMSW__)
         wxLogDebug(wxT("VST canDo: %s"), wxString::FromAscii((char *)ptr).c_str());
#else
         wxPrintf(wxT("VST canDo: %s\n"), wxString::FromAscii((char *)ptr).c_str());
#endif
#endif

         return 0;
      }

      case audioMasterBeginEdit:
      case audioMasterEndEdit:
         return 0;

      case audioMasterAutomate:
         if (vst)
         {
            vst->Automate(index, opt);
         }
         return 0;

      // We're always connected (sort of)
      case audioMasterPinConnected:

      // We don't do MIDI yet
      case audioMasterWantMidi:
      case audioMasterProcessEvents:

         // Don't need to see any messages about these
         return 0;
   }

#if defined(VST_DEBUG)
#if defined(__WXMSW__)
   wxLogDebug(wxT("vst: %p opcode: %d index: %d value: %p ptr: %p opt: %f user: %p"),
              effect, (int) opcode, (int) index, (void *) value, ptr, opt, vst);
#else
   wxPrintf(wxT("vst: %p opcode: %d index: %d value: %p ptr: %p opt: %f user: %p\n"),
            effect, (int) opcode, (int) index, (void *) value, ptr, opt, vst);
#endif
#endif

   return 0;
}

VSTEffect::VSTEffect(const wxString & path, VSTEffect *master)
:  mPath(path),
   mMaster(master)
{
   mHost = NULL;
   mModule = NULL;
   mAEffect = NULL;
   mDialog = NULL;

   mTimer = new VSTEffectTimer(this);
   mTimerGuard = 0;

   mInteractive = false;
   mAudioIns = 0;
   mAudioOuts = 0;
   mMidiIns = 0;
   mMidiOuts = 0;
   mSampleRate = 44100;
   mBlockSize = 0;
   mBufferDelay = 0;
   mProcessLevel = 1;         // in GUI thread
   mHasPower = false;
   mWantsIdle = false;
   mWantsEditIdle = false;
   mUserBlockSize = 8192;
   mBlockSize = mUserBlockSize;
   mUseLatency = true;
   mReady = false;

   memset(&mTimeInfo, 0, sizeof(mTimeInfo));
   mTimeInfo.samplePos = 0.0;
   mTimeInfo.sampleRate = 44100.0;  // this is a bogus value, but it's only for the display
   mTimeInfo.nanoSeconds = wxGetLocalTimeMillis().ToDouble();
   mTimeInfo.tempo = 120.0;
   mTimeInfo.timeSigNumerator = 4;
   mTimeInfo.timeSigDenominator = 4;
   mTimeInfo.flags = kVstTempoValid | kVstNanosValid;

   // UI

   mNames = NULL;
   mSliders = NULL;
   mDisplays = NULL;
   mLabels = NULL;
   mContainer = NULL;

#if defined(__WXMAC__)
   mOverlayRef = 0;
   mOverlayEventHandlerUPP = 0;
   mOverlayEventHandlerRef = 0;

   mWindowRef = 0;
   mWindowEventHandlerUPP = 0;
   mWindowEventHandlerRef = 0;

   mRootTrackingHandlerRef = 0;
   mViewTrackingHandlerRef = 0;
   mSubviewTrackingHandlerRef = 0;
   mOverlayRootTrackingHandlerRef = 0;
   mOverlayViewTrackingHandlerRef = 0;

#elif defined(__WXMSW__)
   mHwnd = 0;
#else
   mXdisp = 0;
   mXwin = 0;
#endif

   // If we're a slave then go ahead a load immediately
   if (mMaster)
   {
      Load();
   }
}

VSTEffect::~VSTEffect()
{
   if (mDialog)
   {
      mDialog->Close();
   }

   if (mNames)
   {
      delete [] mNames;
   }

   if (mSliders)
   {
      delete [] mSliders;
   }

   if (mDisplays)
   {
      delete [] mDisplays;
   }

   if (mLabels)
   {
      delete [] mLabels;
   }

   Unload();
}

// ============================================================================
// IdentInterface Implementation
// ============================================================================

wxString VSTEffect::GetPath()
{
   return mPath;
}

wxString VSTEffect::GetSymbol()
{
   return mName;
}

wxString VSTEffect::GetName()
{
   return GetSymbol();
}

wxString VSTEffect::GetVendor()
{
   return mVendor;
}

wxString VSTEffect::GetVersion()
{
   wxString version;

   bool skipping = true;
   for (int i = 0, s = 0; i < 4; i++, s += 8)
   {
      int dig = (mVersion >> s) & 0xff;
      if (dig != 0 || !skipping)
      {
         version += !skipping ? wxT(".") : wxT("");
         version += wxString::Format(wxT("%d"), dig);
         skipping = false;
      }
   }

   return version;
}

wxString VSTEffect::GetDescription()
{
   // VST does have a product string opcode and sum effects return a short
   // description, but most do not or they just return the name again.  So,
   // try to provide some sort of useful information.
   mDescription = XO("Audio In: ") +
                  wxString::Format(wxT("%d"), mAudioIns) +
                  XO(", Audio Out: ") +
                  wxString::Format(wxT("%d"), mAudioOuts);

   return mDescription;
}

// ============================================================================
// EffectIdentInterface Implementation
// ============================================================================

EffectType VSTEffect::GetType()
{
   if (mAudioIns == 0 && mAudioOuts == 0 && mMidiIns == 0 && mMidiOuts == 0)
   {
      return EffectTypeNone;
   }

   if (mAudioIns == 0 && mMidiIns == 0)
   {
      return EffectTypeGenerate;
   }

   if (mAudioOuts == 0 && mMidiOuts == 0)
   {
      return EffectTypeAnalyze;
   }

   return EffectTypeProcess;
}


wxString VSTEffect::GetFamily()
{
   return VSTPLUGINTYPE;
}

bool VSTEffect::IsInteractive()
{
   return mInteractive;
}

bool VSTEffect::IsDefault()
{
   return false;
}

bool VSTEffect::IsLegacy()
{
   return false;
}

bool VSTEffect::SupportsRealtime()
{
   return GetType() == EffectTypeProcess;
}

bool VSTEffect::SupportsAutomation()
{
   return mAutomatable;
}

// ============================================================================
// EffectClientInterface Implementation
// ============================================================================

bool VSTEffect::SetHost(EffectHostInterface *host)
{
   mHost = host;

   if (!mAEffect)
   {
      Load();
   }

   if (!mAEffect)
   {
      return false;
   }

   // If we have a master then there's no need to load settings since the master will feed
   // us everything we need.
   if (mMaster)
   {
      return true;
   }

   if (mHost)
   {
      mHost->GetSharedConfig(wxT("Options"), wxT("BufferSize"), mUserBlockSize, 8192);
      mHost->GetSharedConfig(wxT("Options"), wxT("UseLatency"), mUseLatency, true);

      mBlockSize = mUserBlockSize;

      bool haveDefaults;
      mHost->GetPrivateConfig(mHost->GetFactoryDefaultsGroup(), wxT("Initialized"), haveDefaults, false);
      if (!haveDefaults)
      {
         SaveParameters(mHost->GetFactoryDefaultsGroup());
         mHost->SetPrivateConfig(mHost->GetFactoryDefaultsGroup(), wxT("Initialized"), true);
      }

      LoadParameters(mHost->GetCurrentSettingsGroup());
   }

   return true;
}

int VSTEffect::GetAudioInCount()
{
   return mAudioIns;
}

int VSTEffect::GetAudioOutCount()
{
   return mAudioOuts;
}

int VSTEffect::GetMidiInCount()
{
   return mMidiIns;
}

int VSTEffect::GetMidiOutCount()
{
   return mMidiOuts;
}

sampleCount VSTEffect::SetBlockSize(sampleCount maxBlockSize)
{
   if (mUserBlockSize > maxBlockSize)
   {
      mBlockSize = maxBlockSize;
   }
   else
   {
      mBlockSize = mUserBlockSize;
   }

   return mBlockSize;
}

void VSTEffect::SetSampleRate(sampleCount rate)
{
   mSampleRate = (float) rate;
}

sampleCount VSTEffect::GetLatency()
{
   if (mUseLatency)
   {
      // ??? Threading issue ???
      sampleCount delay = mBufferDelay;
      mBufferDelay = 0;
      return delay;
   }

   return 0;
}

sampleCount VSTEffect::GetTailSize()
{
   return 0;
}

bool VSTEffect::IsReady()
{
   return mReady;
}

bool VSTEffect::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames WXUNUSED(chanMap))
{
   // Initialize time info
   memset(&mTimeInfo, 0, sizeof(mTimeInfo));
   mTimeInfo.sampleRate = mSampleRate;
   mTimeInfo.nanoSeconds = wxGetLocalTimeMillis().ToDouble();
   mTimeInfo.tempo = 120.0;
   mTimeInfo.timeSigNumerator = 4;
   mTimeInfo.timeSigDenominator = 4;
   mTimeInfo.flags = kVstTempoValid | kVstNanosValid | kVstTransportPlaying;

   // Set processing parameters...power must be off for this
   callDispatcher(effSetSampleRate, 0, 0, NULL, mSampleRate);
   callDispatcher(effSetBlockSize, 0, mBlockSize, NULL, 0.0);

   // Turn on the power
   PowerOn();

   // Set the initial buffer delay
   SetBufferDelay(mAEffect->initialDelay);

   mReady = true;

   return true;
}

bool VSTEffect::ProcessFinalize()
{
   mReady = false;

   PowerOff();

   return true;
}

sampleCount VSTEffect::ProcessBlock(float **inBlock, float **outBlock, sampleCount blockLen)
{
   // Only call the effect if there's something to do...some do not like zero-length block
   if (blockLen)
   {
      // Go let the plugin moleste the samples
      callProcessReplacing(inBlock, outBlock, blockLen);

      // And track the position
      mTimeInfo.samplePos += ((double) blockLen / mTimeInfo.sampleRate);
   }

   return blockLen;
}

int VSTEffect::GetChannelCount()
{
   return mNumChannels;
}

void VSTEffect::SetChannelCount(int numChannels)
{
   mNumChannels = numChannels;
}

bool VSTEffect::RealtimeInitialize()
{
   mMasterIn = new float *[mAudioIns];
   for (int i = 0; i < mAudioIns; i++)
   {
      mMasterIn[i] = new float[mBlockSize];
      memset(mMasterIn[i], 0, mBlockSize * sizeof(float));
   }

   mMasterOut = new float *[mAudioOuts];
   for (int i = 0; i < mAudioOuts; i++)
   {
      mMasterOut[i] = new float[mBlockSize];
   }

   return ProcessInitialize(0, NULL);
}

bool VSTEffect::RealtimeAddProcessor(int numChannels, float sampleRate)
{
   VSTEffect *slave = new VSTEffect(mPath, this);
   mSlaves.Add(slave);

   slave->SetBlockSize(mBlockSize);
   slave->SetChannelCount(numChannels);
   slave->SetSampleRate(sampleRate);

   int clen = 0;
   if (mAEffect->flags & effFlagsProgramChunks)
   {
      void *chunk = NULL;

      clen = (int) callDispatcher(effGetChunk, 0, 0, &chunk, 0.0);
      if (clen != 0)
      {
         slave->callSetChunk(false, clen, chunk);
      }
   }

   if (clen == 0)
   {
      callDispatcher(effBeginSetProgram, 0, 0, NULL, 0.0);

      for (int i = 0; i < mAEffect->numParams; i++)
      {
         slave->callSetParameter(i, callGetParameter(i));
      }

      callDispatcher(effEndSetProgram, 0, 0, NULL, 0.0);
   }

   return slave->ProcessInitialize(0, NULL);
}

bool VSTEffect::RealtimeFinalize()
{
   for (size_t i = 0, cnt = mSlaves.GetCount(); i < cnt; i++)
   {
      mSlaves[i]->ProcessFinalize();
      delete mSlaves[i];
   }
   mSlaves.Clear();

   for (int i = 0; i < mAudioIns; i++)
   {
      delete [] mMasterIn[i];
   }
   delete [] mMasterIn;

   for (int i = 0; i < mAudioOuts; i++)
   {
      delete [] mMasterOut[i];
   }
   delete [] mMasterOut;

   return ProcessFinalize();
}

bool VSTEffect::RealtimeSuspend()
{
   PowerOff();

   for (size_t i = 0, cnt = mSlaves.GetCount(); i < cnt; i++)
   {
      mSlaves[i]->PowerOff();
   }

   return true;
}

bool VSTEffect::RealtimeResume()
{
   PowerOn();

   for (size_t i = 0, cnt = mSlaves.GetCount(); i < cnt; i++)
   {
      mSlaves[i]->PowerOn();
   }

   return true;
}

bool VSTEffect::RealtimeProcessStart()
{
   for (int i = 0; i < mAudioIns; i++)
   {
      memset(mMasterIn[i], 0, mBlockSize * sizeof(float));
   }

   mNumSamples = 0;

   return true;
}

sampleCount VSTEffect::RealtimeProcess(int group, float **inbuf, float **outbuf, sampleCount numSamples)
{
   wxASSERT(numSamples <= mBlockSize);

   for (int c = 0; c < mAudioIns; c++)
   {
      for (sampleCount s = 0; s < numSamples; s++)
      {
         mMasterIn[c][s] += inbuf[c][s];
      }
   }
   mNumSamples = wxMax(numSamples, mNumSamples);

   return mSlaves[group]->ProcessBlock(inbuf, outbuf, numSamples);
}

bool VSTEffect::RealtimeProcessEnd()
{
   ProcessBlock(mMasterIn, mMasterOut, mNumSamples);

   return true;
}

//
// Some history...
//
// Before we ran into the Antress plugin problem with buffer size limitations,
// (see below) we just had a plain old effect loop...get the input samples, pass
// them to the effect, save the output samples.
//
// But, the hack I put in to limit the buffer size to only 8k (normally 512k or so)
// severely impacted performance.  So, Michael C. added some intermediate buffering
// that sped things up quite a bit and this is how things have worked for quite a
// while.  It still didn't get the performance back to the pre-hack stage, but it
// was a definite benefit.
//
// History over...
//
// I've recently (May 2014) tried newer versions of the Antress effects and they
// no longer seem to have a problem with buffer size.  So, I've made a bit of a
// compromise...I've made the buffer size user configurable.  Should have done this
// from the beginning.  I've left the default 8k, just in case, but now the user
// can set the buffering based on their specific setup and needs.
//
// And at the same time I added buffer delay compensation, which allows Audacity
// to account for latency introduced by some effects.  This is based on information
// provided by the effect, so it will not work with all effects since they don't
// all provide the information (kn0ck0ut is one).
//
bool VSTEffect::ShowInterface(wxWindow *parent, bool forceModal)
{
   if (mDialog)
   {
      mDialog->Close(true);
      return false;
   }

   //   mProcessLevel = 1;      // in GUI thread

   // Set some defaults since some VSTs need them...these will be reset when
   // normal or realtime processing begins
   if (!IsReady())
   {
      mSampleRate = 44100;
      mBlockSize = 8192;
      ProcessInitialize(0, NULL);
   }

   mDialog = mHost->CreateUI(parent, this);
   if (!mDialog)
   {
      return false;
   }
   mDialog->CentreOnParent();

   if (SupportsRealtime() && !forceModal)
   {
      mDialog->Show();

      return false;
   }

   bool res = mDialog->ShowModal() != 0;
   mDialog = NULL;

   return res;
}

bool VSTEffect::GetAutomationParameters(EffectAutomationParameters & parms)
{
   for (int i = 0; i < mAEffect->numParams; i++)
   {
      wxString name = GetString(effGetParamName, i);
      if (name.IsEmpty())
      {
         name.Printf(wxT("parm_%d"), i);
      }

      float value = callGetParameter(i);
      if (!parms.Write(name, value))
      {
         return false;
      }
   }

   return true;
}

bool VSTEffect::SetAutomationParameters(EffectAutomationParameters & parms)
{
   size_t slaveCnt = mSlaves.GetCount();

   callDispatcher(effBeginSetProgram, 0, 0, NULL, 0.0);
   for (int i = 0; i < mAEffect->numParams; i++)
   {
      wxString name = GetString(effGetParamName, i);
      if (name.IsEmpty())
      {
         name.Printf(wxT("parm_%d"), i);
      }

      double d = 0.0;
      if (!parms.Read(name, &d))
      {
         return false;
      }

      if (d >= -1.0 && d <= 1.0)
      {
         callSetParameter(i, d);
         for (size_t i = 0; i < slaveCnt; i++)
         {
            mSlaves[i]->callSetParameter(i, d);
         }
      }
   }
   callDispatcher(effEndSetProgram, 0, 0, NULL, 0.0);

   return true;
}


bool VSTEffect::LoadUserPreset(const wxString & name)
{
   if (!LoadParameters(name))
   {
      return false;
   }

   RefreshParameters();

   return true;
}

bool VSTEffect::SaveUserPreset(const wxString & name)
{
   return SaveParameters(name);
}

wxArrayString VSTEffect::GetFactoryPresets()
{
   wxArrayString progs; 

   // Some plugins, like Guitar Rig 5, only report 128 programs while they have hundreds.  While
   // I was able to come up with a hack in the Guitar Rig case to gather all of the program names
   // it would not let me set a program outside of the first 128.
   for (int i = 0; i < mAEffect->numPrograms; i++)
   {
      progs.Add(GetString(effGetProgramNameIndexed, i));
   }

   return progs;
}

bool VSTEffect::LoadFactoryPreset(int id)
{
   callSetProgram(id);

   RefreshParameters();

   return true;
}

bool VSTEffect::LoadFactoryDefaults()
{
   if (!LoadParameters(mHost->GetFactoryDefaultsGroup()))
   {
      return false;
   }

   RefreshParameters();

   return true;
}

// ============================================================================
// EffectUIClientInterface implementation
// ============================================================================

void VSTEffect::SetHostUI(EffectUIHostInterface *host)
{
   mUIHost = host;
}

bool VSTEffect::PopulateUI(wxWindow *parent)
{
   mDialog = (wxDialog *) wxGetTopLevelParent(parent);
   mParent = parent;

   mParent->PushEventHandler(this);

   // Determine if the VST editor is supposed to be used or not
   mHost->GetSharedConfig(wxT("Options"),
                          wxT("UseGUI"),
                          mGui,
                          true);
   mGui = mAEffect->flags & effFlagsHasEditor ? mGui : false;

   // Must use the GUI editor if parameters aren't provided
   if (mAEffect->numParams == 0)
   {
      mGui = true;
   }

   // Build the appropriate dialog type
   if (mGui)
   {
      BuildFancy();
   }
   else
   {
      BuildPlain();
   }

   return true;
}

bool VSTEffect::IsGraphicalUI()
{
   return mGui;
}

bool VSTEffect::ValidateUI()
{
   if (!mParent->Validate() || !mParent->TransferDataFromWindow())
   {
      return false;
   }

   if (GetType() == EffectTypeGenerate)
   {
      mHost->SetDuration(mDuration->GetValue());
   }

   return true;
}

bool VSTEffect::HideUI()
{
   return true;
}

bool VSTEffect::CloseUI()
{
   mParent->RemoveEventHandler(this);

   PowerOff();

   NeedEditIdle(false);

   RemoveHandler();

   if (mNames)
   {
      delete [] mNames;
      mNames = NULL;
   }

   if (mSliders)
   {
      delete [] mSliders;
      mSliders = NULL;
   }

   if (mDisplays)
   {
      delete [] mDisplays;
      mDisplays = NULL;
   }

   if (mLabels)
   {
      delete [] mLabels;
      mLabels = NULL;
   }

   mUIHost = NULL;
   mParent = NULL;
   mDialog = NULL;

   return true;
}

bool VSTEffect::CanExportPresets()
{
   return true;
}

void VSTEffect::ExportPresets()
{
   wxString path;

   // Ask the user for the real name
   //
   // Passing a valid parent will cause some effects dialogs to malfunction
   // upon returning from the FileSelector().
   path = FileSelector(_("Save VST Preset As:"),
                       FileNames::DataDir(),
                       wxEmptyString,
                       wxT("xml"),
                       wxT("Standard VST bank file (*.fxb)|*.fxb|Standard VST program file (*.fxp)|*.fxp|Audacity VST preset file (*.xml)|*.xml"),
                       wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER,
                       NULL);

   // User canceled...
   if (path.IsEmpty())
   {
      return;
   }

   wxFileName fn(path);
   wxString ext = fn.GetExt();
   if (ext.CmpNoCase(wxT("fxb")) == 0)
   {
      SaveFXB(fn);
   }
   else if (ext.CmpNoCase(wxT("fxp")) == 0)
   {
      SaveFXP(fn);
   }
   else if (ext.CmpNoCase(wxT("xml")) == 0)
   {
      SaveXML(fn);
   }
   else
   {
      // This shouldn't happen, but complain anyway
      wxMessageBox(_("Unrecognized file extension."),
                   _("Error Saving VST Presets"),
                   wxOK | wxCENTRE,
                   mParent);

      return;
   }
}

//
// Load an "fxb", "fxp" or Audacuty "xml" file
//
// Based on work by Sven Giermann
//
void VSTEffect::ImportPresets()
{
   wxString path;

   // Ask the user for the real name
   path = FileSelector(_("Load VST Preset:"),
                       FileNames::DataDir(),
                       wxEmptyString,
                       wxT("xml"),
                       wxT("VST preset files (*.fxb; *.fxp; *.xml)|*.fxb;*.fxp;*.xml"),
                       wxFD_OPEN | wxRESIZE_BORDER,
                       mParent);

   // User canceled...
   if (path.IsEmpty())
   {
      return;
   }

   wxFileName fn(path);
   wxString ext = fn.GetExt();
   bool success = false;
   if (ext.CmpNoCase(wxT("fxb")) == 0)
   {
      success = LoadFXB(fn);
   }
   else if (ext.CmpNoCase(wxT("fxp")) == 0)
   {
      success = LoadFXP(fn);
   }
   else if (ext.CmpNoCase(wxT("xml")) == 0)
   {
      success = LoadXML(fn);
   }
   else
   {
      // This shouldn't happen, but complain anyway
      wxMessageBox(_("Unrecognized file extension."),
                   _("Error Loading VST Presets"),
                   wxOK | wxCENTRE,
                   mParent);

         return;
   }

   if (!success)
   {
      wxMessageBox(_("Unable to load presets file."),
                   _("Error Loading VST Presets"),
                   wxOK | wxCENTRE,
                   mParent);

      return;
   }

   RefreshParameters();

   return;
}

bool VSTEffect::HasOptions()
{
   return true;
}

void VSTEffect::ShowOptions()
{
   VSTEffectOptionsDialog dlg(mParent, mHost);
   if (dlg.ShowModal())
   {
      // Reinitialize configuration settings
      mHost->GetSharedConfig(wxT("Options"), wxT("BufferSize"), mUserBlockSize, 8192);
      mHost->GetSharedConfig(wxT("Options"), wxT("UseLatency"), mUseLatency, true);
   }
}

// ============================================================================
// VSTEffect implementation
// ============================================================================

bool VSTEffect::Load()
{
   vstPluginMain pluginMain;
   bool success = false;

   long effectID = 0;
   wxString realPath = mPath.BeforeFirst(wxT(';'));
   mPath.AfterFirst(wxT(';')).ToLong(&effectID);
   mCurrentEffectID = (intptr_t) effectID;

   mModule = NULL;
   mAEffect = NULL;

#if defined(__WXMAC__)
   // Start clean
   mBundleRef = NULL;

   // Don't really know what this should be initialize to
   mResource = -1;

   // Convert the path to a CFSTring
   wxMacCFStringHolder path(realPath);

   // Convert the path to a URL
   CFURLRef urlRef =
      CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                    path,
                                    kCFURLPOSIXPathStyle,
                                    true);
   if (urlRef == NULL)
   {
      return false;
   }

   // Create the bundle using the URL
   CFBundleRef bundleRef = CFBundleCreate(kCFAllocatorDefault, urlRef);

   // Done with the URL
   CFRelease(urlRef);

   // Bail if the bundle wasn't created
   if (bundleRef == NULL) 
   {
      return false;
   }

   // Retrieve a reference to the executable
   CFURLRef exeRef = CFBundleCopyExecutableURL(bundleRef);
   if (exeRef == NULL)
   {
      CFRelease(bundleRef);
      return false;
   }

   // Convert back to path
   UInt8 exePath[PLATFORM_MAX_PATH];
   Boolean good = CFURLGetFileSystemRepresentation(exeRef, true, exePath, sizeof(exePath));

   // Done with the executable reference
   CFRelease(exeRef);

   // Bail if we couldn't resolve the executable path
   if (good == FALSE)
   {
      CFRelease(bundleRef);
      return false;
   }

   // Attempt to open it
   mModule = dlopen((char *) exePath, RTLD_NOW | RTLD_LOCAL);
   if (mModule == NULL)
   {
      CFRelease(bundleRef);
      return false;
   }

   // Try to locate the new plugin entry point
   pluginMain = (vstPluginMain) dlsym(mModule, "VSTPluginMain");

   // If not found, try finding the old entry point
   if (pluginMain == NULL)
   {
      pluginMain = (vstPluginMain) dlsym(mModule, "main_macho");
   }

   // Must not be a VST plugin
   if (pluginMain == NULL)
   {
      dlclose(mModule);
      mModule = NULL;
      CFRelease(bundleRef);
      return false;
   }

   // Need to keep the bundle reference around so we can map the
   // resources.
   mBundleRef = bundleRef;

   // Open the resource map ... some plugins (like GRM Tools) need this.
   mResource = (int) CFBundleOpenBundleResourceMap(bundleRef);

#elif defined(__WXMSW__)

   {
      wxLogNull nolog;

      // Try to load the library
      wxDynamicLibrary *lib = new wxDynamicLibrary(realPath);
      if (!lib) 
      {
         return false;
      }

      // Bail if it wasn't successful
      if (!lib->IsLoaded())
      {
         delete lib;
         return false;
      }

      // Try to find the entry point, while suppressing error messages
      pluginMain = (vstPluginMain) lib->GetSymbol(wxT("VSTPluginMain"));
      if (pluginMain == NULL)
      {
         pluginMain = (vstPluginMain) lib->GetSymbol(wxT("main"));
         if (pluginMain == NULL)
         {
            delete lib;
            return false;
         }
      }

      // Save the library reference
      mModule = lib;
   }

#else

   // Attempt to load it
   //
   // Spent a few days trying to figure out why some VSTs where running okay and
   // others were hit or miss.  The cause was that we export all of Audacity's
   // symbols and some of the loaded libraries were picking up Audacity's and 
   // not their own.
   //
   // So far, I've only seen this issue on Linux, but we might just be getting
   // lucky on the Mac and Windows.  The sooner we stop exporting everything
   // the better.
   //
   // To get around the problem, I just added the RTLD_DEEPBIND flag to the load
   // and that "basically" puts Audacity last when the loader needs to resolve
   // symbols.
   //
   // Once we define a proper external API, the flags can be removed.
   void *lib = dlopen((const char *)wxString(realPath).ToUTF8(), RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
   if (!lib) 
   {
      return false;
   }

   // Try to find the entry point, while suppressing error messages
   pluginMain = (vstPluginMain) dlsym(lib, "VSTPluginMain");
   if (pluginMain == NULL)
   {
      pluginMain = (vstPluginMain) dlsym(lib, "main");
      if (pluginMain == NULL)
      {
         dlclose(lib);
         return false;
      }
   }

   // Save the library reference
   mModule = lib;

#endif

   // Initialize the plugin
   try
   {
      mAEffect = pluginMain(VSTEffect::AudioMaster);
   }
   catch (...)
   {
      wxLogMessage(_("VST plugin initialization failed\n"));
      mAEffect = NULL;
   }

   // Was it successful?
   if (mAEffect)
   {
      // Save a reference to ourselves
      //
      // Note:  Some hosts use "user" and some use "ptr2/resvd2".  It might
      //        be worthwhile to check if user is NULL before using it and
      //        then falling back to "ptr2/resvd2".
      mAEffect->ptr2 = this;

      // Give the plugin an initial sample rate and blocksize
      callDispatcher(effSetSampleRate, 0, 0, NULL, 48000.0);
      callDispatcher(effSetBlockSize, 0, 512, NULL, 0);

      // Ask the plugin to identify itself...might be needed for older plugins
      callDispatcher(effIdentify, 0, 0, NULL, 0);

      // Open the plugin
      callDispatcher(effOpen, 0, 0, NULL, 0.0);

      // Set it again in case plugin ignored it before the effOpen
      callDispatcher(effSetSampleRate, 0, 0, NULL, 48000.0);
      callDispatcher(effSetBlockSize, 0, 512, NULL, 0);

      // Ensure that it looks like a plugin and can deal with ProcessReplacing
      // calls.  Also exclude synths for now.
      if (mAEffect->magic == kEffectMagic &&
         !(mAEffect->flags & effFlagsIsSynth) &&
         mAEffect->flags & effFlagsCanReplacing)
      {
         mName = GetString(effGetEffectName);
         if (mName.length() == 0)
         {
            mName = GetString(effGetProductString);
            if (mName.length() == 0)
            {
               wxFileName f(realPath);
               mName = f.GetName();
            }
         }
         mVendor = GetString(effGetVendorString);
         mVersion = wxINT32_SWAP_ON_LE(callDispatcher(effGetVendorVersion, 0, 0, NULL, 0));
         if (mVersion == 0)
         {
            mVersion = wxINT32_SWAP_ON_LE(mAEffect->version);
         }

         if (mAEffect->flags & effFlagsHasEditor || mAEffect->numParams != 0)
         {
            mInteractive = true;
         }

         mAudioIns = mAEffect->numInputs;
         mAudioOuts = mAEffect->numOutputs;

         mMidiIns = 0;
         mMidiOuts = 0;

         // Check to see if parameters can be automated.  This isn't a gaurantee
         // since it could be that the effect simply doesn't support the opcode.
         mAutomatable = false;
         for (int i = 0; i < mAEffect->numParams; i++)
         {
            if (callDispatcher(effCanBeAutomated, 0, i, NULL, 0.0))
            {
               mAutomatable = true;
               break;
            }
         }

         // Make sure we start out with a valid program selection
         // I've found one plugin (SoundHack +morphfilter) that will
         // crash Audacity when saving the initial default parameters
         // with this.
         callSetProgram(0);

         // Pretty confident that we're good to go
         success = true;
      }
   }

   if (!success)
   {
      Unload();
   }

   return success;
}

void VSTEffect::Unload()
{
   if (mDialog)
   {
      CloseUI();
   }

   if (mTimer)
   {
      mTimer->Stop();
      delete mTimer;
      mTimer = NULL;
   }

   if (mAEffect)
   {
      // Turn the power off
      PowerOff();

      // Finally, close the plugin
      callDispatcher(effClose, 0, 0, NULL, 0.0);
      mAEffect = NULL;
   }

   if (mModule)
   {
#if defined(__WXMAC__)

      if (mResource != -1)
      {
         CFBundleCloseBundleResourceMap((CFBundleRef) mBundleRef, mResource);
         mResource = -1;
      }

      if (mBundleRef != NULL)
      {
         CFRelease((CFBundleRef) mBundleRef);
         mBundleRef = NULL;
      }

      dlclose(mModule);

#elif defined(__WXMSW__)

      delete (wxDynamicLibrary *) mModule;

#else

      dlclose(mModule);

#endif

      mModule = NULL;
      mAEffect = NULL;
   }
}

wxArrayInt VSTEffect::GetEffectIDs()
{
   wxArrayInt effectIDs;

   // Are we a shell?
   if ((VstPlugCategory) callDispatcher(effGetPlugCategory, 0, 0, NULL, 0) == kPlugCategShell)
   {
      char name[64];
      int effectID;

      effectID = (int) callDispatcher(effShellGetNextPlugin, 0, 0, &name, 0);
      while (effectID)
      {
         effectIDs.Add(effectID);
         effectID = (int) callDispatcher(effShellGetNextPlugin, 0, 0, &name, 0);
      }
   }

   return effectIDs;
}

bool VSTEffect::LoadParameters(const wxString & group)
{
   wxString value;

   VstPatchChunkInfo info = {1, mAEffect->uniqueID, mAEffect->version, mAEffect->numParams};
   mHost->GetPrivateConfig(group, wxT("UniqueID"), info.pluginUniqueID, info.pluginUniqueID);
   mHost->GetPrivateConfig(group, wxT("Version"), info.pluginVersion, info.pluginVersion);
   mHost->GetPrivateConfig(group, wxT("Elements"), info.numElements, info.numElements);

   if ((info.pluginUniqueID != mAEffect->uniqueID) ||
       (info.pluginVersion != mAEffect->version) ||
       (info.numElements != mAEffect->numParams))
   {
      return false;
   }

   if (mHost->GetPrivateConfig(group, wxT("Chunk"), value, wxEmptyString))
   {
      char *buf = new char[value.length() / 4 * 3];

      int len = VSTEffect::b64decode(value, buf);
      if (len)
      {
         callSetChunk(true, len, buf, &info);
      }
      delete [] buf;

      return true;
   }

   wxString parms;
   if (!mHost->GetPrivateConfig(group, wxT("Parameters"), parms, wxEmptyString))
   {
      return false;
   }

   EffectAutomationParameters eap;
   if (!eap.SetParameters(parms))
   {
      return false;
   }

   return SetAutomationParameters(eap);
}

bool VSTEffect::SaveParameters(const wxString & group)
{
   mHost->SetPrivateConfig(group, wxT("UniqueID"), mAEffect->uniqueID);
   mHost->SetPrivateConfig(group, wxT("Version"), mAEffect->version);
   mHost->SetPrivateConfig(group, wxT("Elements"), mAEffect->numParams);

   if (mAEffect->flags & effFlagsProgramChunks)
   {
      void *chunk = NULL;
      int clen = (int) callDispatcher(effGetChunk, 1, 0, &chunk, 0.0);
      if (clen <= 0)
      {
         return false;
      }

      mHost->SetPrivateConfig(group, wxT("Chunk"), VSTEffect::b64encode(chunk, clen));
      return true;
   }

   EffectAutomationParameters eap;
   if (!GetAutomationParameters(eap))
   {
      return false;
   }

   wxString parms;
   if (!eap.GetParameters(parms))
   {
      return false;
   }

   return mHost->SetPrivateConfig(group, wxT("Parameters"), parms);
}

void VSTEffect::OnTimer()
{
   wxRecursionGuard guard(mTimerGuard);

   // Ignore it if we're recursing
   if (guard.IsInside())
   {
      return;
   }

   if (mWantsIdle)
   {
      int ret = callDispatcher(effIdle, 0, 0, NULL, 0.0);
      if (!ret)
      {
         mWantsIdle = false;
      }
   }

   if (mWantsEditIdle)
   {
      callDispatcher(effEditIdle, 0, 0, NULL, 0.0);
   }
}

void VSTEffect::NeedIdle()
{
   mWantsIdle = true;
   mTimer->Start(100);
}

void VSTEffect::NeedEditIdle(bool state)
{
   mWantsEditIdle = state;
   mTimer->Start(100);
}

VstTimeInfo *VSTEffect::GetTimeInfo()
{
   mTimeInfo.nanoSeconds = wxGetLocalTimeMillis().ToDouble();
   return &mTimeInfo;
}

float VSTEffect::GetSampleRate()
{
   return mTimeInfo.sampleRate;
}

int VSTEffect::GetProcessLevel()
{
   return mProcessLevel;
}

void VSTEffect::PowerOn()
{
   if (!mHasPower)
   {
      // Turn the power on
      callDispatcher(effMainsChanged, 0, 1, NULL, 0.0);

      // Tell the effect we're going to start processing
      callDispatcher(effStartProcess, 0, 0, NULL, 0.0);

      // Set state
      mHasPower = true;
   }
}

void VSTEffect::PowerOff()
{
   if (mHasPower)
   {
      // Tell the effect we're going to stop processing
      callDispatcher(effStopProcess, 0, 0, NULL, 0.0);

      // Turn the power off
      callDispatcher(effMainsChanged, 0, 0, NULL, 0.0);

      // Set state
      mHasPower = false;
   }
}

void VSTEffect::SizeWindow(int w, int h)
{
   // Queue the event to make the resizes smoother
   if (mParent)
   {
      wxCommandEvent sw(EVT_SIZEWINDOW);
      sw.SetInt(w);
      sw.SetExtraLong(h);
      mParent->GetEventHandler()->AddPendingEvent(sw);
   }

   return;
}

void VSTEffect::UpdateDisplay()
{
#if 0
   // Tell the dialog to refresh effect information
   if (mParent)
   {
      wxCommandEvent ud(EVT_UPDATEDISPLAY);
      mParent->GetEventHandler()->AddPendingEvent(ud);
   }
#endif
   return;
}

void VSTEffect::Automate(int index, float value)
{
   // Just ignore it if we're a slave
   if (mMaster)
   {
      return;
   }

   for (size_t i = 0, cnt = mSlaves.GetCount(); i < cnt; i++)
   {
      mSlaves[i]->callSetParameter(index, value);
   }

   return;
}

void VSTEffect::SetBufferDelay(int samples)
{
   // We do not support negative delay
   if (samples >= 0 && mUseLatency)
   {
      mBufferDelay = samples;
   }

   return;
}

int VSTEffect::GetString(wxString & outstr, int opcode, int index)
{
   char buf[256];

   memset(buf, 0, sizeof(buf));

   callDispatcher(opcode, index, 0, buf, 0.0);

   outstr = wxString::FromUTF8(buf);

   return 0;
}

wxString VSTEffect::GetString(int opcode, int index)
{
   wxString str;

   GetString(str, opcode, index);

   return str;
}

void VSTEffect::SetString(int opcode, const wxString & str, int index)
{
   char buf[256];
   strcpy(buf, str.Left(255).ToUTF8());

   callDispatcher(opcode, index, 0, buf, 0.0);
}

intptr_t VSTEffect::callDispatcher(int opcode,
                                   int index, intptr_t value, void *ptr, float opt)
{
   // Needed since we might be in the dispatcher when the timer pops
   wxCRIT_SECT_LOCKER(locker, mDispatcherLock);
   return mAEffect->dispatcher(mAEffect, opcode, index, value, ptr, opt);
}

void VSTEffect::callProcessReplacing(float **inputs,
                                     float **outputs, int sampleframes)
{
   mAEffect->processReplacing(mAEffect, inputs, outputs, sampleframes);
}

float VSTEffect::callGetParameter(int index)
{
   return mAEffect->getParameter(mAEffect, index);
}

void VSTEffect::callSetParameter(int index, float value)
{
   if (callDispatcher(effCanBeAutomated, 0, index, NULL, 0.0))
   {
      mAEffect->setParameter(mAEffect, index, value);

      for (size_t i = 0, cnt = mSlaves.GetCount(); i < cnt; i++)
      {
         mSlaves[i]->callSetParameter(index, value);
      }
   }
}

void VSTEffect::callSetProgram(int index)
{
   callDispatcher(effBeginSetProgram, 0, 0, NULL, 0.0);

   callDispatcher(effSetProgram, 0, index, NULL, 0.0);
   for (size_t i = 0, cnt = mSlaves.GetCount(); i < cnt; i++)
   {
      mSlaves[i]->callSetProgram(index);
   }

   callDispatcher(effEndSetProgram, 0, 0, NULL, 0.0);
}

void VSTEffect::callSetChunk(bool isPgm, int len, void *buf)
{
   VstPatchChunkInfo info;

   memset(&info, 0, sizeof(info));
   info.version = 1;
   info.pluginUniqueID = mAEffect->uniqueID;
   info.pluginVersion = mAEffect->version;
   info.numElements = isPgm ? mAEffect->numParams : mAEffect->numPrograms;

   callSetChunk(isPgm, len, buf, &info);
}

void VSTEffect::callSetChunk(bool isPgm, int len, void *buf, VstPatchChunkInfo *info)
{
   if (isPgm)
   {
      // Ask the effect if this is an acceptable program
      if (callDispatcher(effBeginLoadProgram, 0, 0, info, 0.0) == -1)
      {
         return;
      }
   }
   else
   {
      // Ask the effect if this is an acceptable bank
      if (callDispatcher(effBeginLoadBank, 0, 0, info, 0.0) == -1)
      {
         return;
      }
   }

   callDispatcher(effBeginSetProgram, 0, 0, NULL, 0.0);
   callDispatcher(effSetChunk, isPgm ? 1 : 0, len, buf, 0.0);
   callDispatcher(effEndSetProgram, 0, 0, NULL, 0.0);

   for (size_t i = 0, cnt = mSlaves.GetCount(); i < cnt; i++)
   {
      mSlaves[i]->callSetChunk(isPgm, len, buf, info);
   }
}

////////////////////////////////////////////////////////////////////////////////
// Base64 en/decoding
//
// Original routines marked as public domain and found at:
//
// http://en.wikibooks.org/wiki/Algorithm_implementation/Miscellaneous/Base64
//
////////////////////////////////////////////////////////////////////////////////

// Lookup table for encoding
const static wxChar cset[] = wxT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
const static char padc = wxT('=');

wxString VSTEffect::b64encode(const void *in, int len)
{
   unsigned char *p = (unsigned char *) in;
   wxString out;

   unsigned long temp;
   for (int i = 0; i < len / 3; i++)
   {
      temp  = (*p++) << 16; //Convert to big endian
      temp += (*p++) << 8;
      temp += (*p++);
      out += cset[(temp & 0x00FC0000) >> 18];
      out += cset[(temp & 0x0003F000) >> 12];
      out += cset[(temp & 0x00000FC0) >> 6];
      out += cset[(temp & 0x0000003F)];
   }

   switch (len % 3)
   {
      case 1:
         temp  = (*p++) << 16; //Convert to big endian
         out += cset[(temp & 0x00FC0000) >> 18];
         out += cset[(temp & 0x0003F000) >> 12];
         out += padc;
         out += padc;
      break;

      case 2:
         temp  = (*p++) << 16; //Convert to big endian
         temp += (*p++) << 8;
         out += cset[(temp & 0x00FC0000) >> 18];
         out += cset[(temp & 0x0003F000) >> 12];
         out += cset[(temp & 0x00000FC0) >> 6];
         out += padc;
      break;
   }

   return out;
}

int VSTEffect::b64decode(wxString in, void *out)
{
   int len = in.length();
   unsigned char *p = (unsigned char *) out;

   if (len % 4)  //Sanity check
   {
      return 0;
   }

   int padding = 0;
   if (len)
   {
      if (in[len - 1] == padc)
      {
         padding++;
      }

      if (in[len - 2] == padc)
      {
         padding++;
      }
   }

   //const char *a = in.mb_str();
   //Setup a vector to hold the result
   unsigned long temp = 0; //Holds decoded quanta
   int i = 0;
   while (i < len)
   {
      for (int quantumPosition = 0; quantumPosition < 4; quantumPosition++)
      {
         unsigned char c = in[i];
         temp <<= 6;

         if (c >= 0x41 && c <= 0x5A)
         {
            temp |= c - 0x41;
         }
         else if (c >= 0x61 && c <= 0x7A)
         {
            temp |= c - 0x47;
         }
         else if (c >= 0x30 && c <= 0x39)
         {
            temp |= c + 0x04;
         }
         else if (c == 0x2B)
         {
            temp |= 0x3E;
         }
         else if (c == 0x2F)
         {
            temp |= 0x3F;
         }
         else if (c == padc)
         {
            switch (len - i)
            {
               case 1: //One pad character
                  *p++ = (temp >> 16) & 0x000000FF;
                  *p++ = (temp >> 8) & 0x000000FF;
                  return p - (unsigned char *) out;
               case 2: //Two pad characters
                  *p++ = (temp >> 10) & 0x000000FF;
                  return p - (unsigned char *) out;
            }
         }
         i++;
      }
      *p++ = (temp >> 16) & 0x000000FF;
      *p++ = (temp >> 8) & 0x000000FF;
      *p++ = temp & 0x000000FF;
   }

   return p - (unsigned char *) out;
}

void VSTEffect::RemoveHandler()
{
#if defined(__WXMAC__)
   if (mWindowRef)
   {
      callDispatcher(effEditClose, 0, 0, mWindowRef, 0.0);
      mWindowRef = 0;
   }

   if (mOverlayViewTrackingHandlerRef)
   {
      ::RemoveEventHandler(mOverlayViewTrackingHandlerRef);
      mOverlayViewTrackingHandlerRef = 0;
   }

   if (mOverlayRootTrackingHandlerRef)
   {
      ::RemoveEventHandler(mOverlayRootTrackingHandlerRef);
      mOverlayRootTrackingHandlerRef = 0;
   }

   if (mOverlayEventHandlerRef)
   {
      ::RemoveEventHandler(mOverlayEventHandlerRef);
      mOverlayEventHandlerRef = 0;
   }

   if (mOverlayEventHandlerUPP)
   {
      DisposeEventHandlerUPP(mOverlayEventHandlerUPP);
      mOverlayEventHandlerUPP = 0;
   }

   if (mSubviewTrackingHandlerRef)
   {
      ::RemoveEventHandler(mSubviewTrackingHandlerRef);
      mSubviewTrackingHandlerRef = 0;
   }

   if (mViewTrackingHandlerRef)
   {
      ::RemoveEventHandler(mViewTrackingHandlerRef);
      mViewTrackingHandlerRef = 0;
   }

   if (mRootTrackingHandlerRef)
   {
      ::RemoveEventHandler(mRootTrackingHandlerRef);
      mRootTrackingHandlerRef = 0;
   }

   if (mTrackingHandlerUPP)
   {
      DisposeEventHandlerUPP(mTrackingHandlerUPP);
      mTrackingHandlerUPP = 0;
   }

   if (mWindowEventHandlerRef)
   {
      ::RemoveEventHandler(mWindowEventHandlerRef);
      mWindowEventHandlerRef = 0;
      mDialog->MacInstallTopLevelWindowEventHandler();
   }

   if (mWindowEventHandlerUPP)
   {
      DisposeEventHandlerUPP(mWindowEventHandlerUPP);
      mWindowEventHandlerUPP = 0;
   }
#elif defined(__WXMSW__)
   if (mHwnd)
   {
      callDispatcher(effEditClose, 0, 0, mHwnd, 0.0);
      mHwnd = 0;
   }
#else
   if (mXwin)
   {
      callDispatcher(effEditClose, 0, (intptr_t)mXdisp, (void *)mXwin, 0.0);
      mXdisp = 0;
      mXwin = 0;
   }
#endif
}

void VSTEffect::BuildFancy()
{
   struct
   {
      short top, left, bottom, right;
   } *rect;

   // Turn the power on...some effects need this when the editor is open
   PowerOn();

   // Some effects like to have us get their rect before opening them.
   callDispatcher(effEditGetRect, 0, 0, &rect, 0.0);

#if defined(__WXMAC__)
   // Retrieve the current window and the one above it.  The window list
   // is kept in top-most to bottom-most order, so we'll use that to
   // determine if another window was opened above ours.
   mWindowRef = (WindowRef) mDialog->MacGetWindowRef();
   mPreviousRef = GetPreviousWindow(mWindowRef);

   // Install the event handler on our window
   mWindowEventHandlerUPP = NewEventHandlerUPP(WindowEventHandler);
   InstallWindowEventHandler(mWindowRef,
                             mWindowEventHandlerUPP,
                             GetEventTypeCount(WindowEventList),
                             WindowEventList,
                             this,
                             &mWindowEventHandlerRef);

   // Find the content view within our window
   ControlRef root = HIViewGetRoot(mWindowRef);
   HIViewRef view;
   HIViewFindByID(root, kHIViewWindowContentID, &view);

   // And ask the effect to add it's GUI
   callDispatcher(effEditOpen, 0, 0, mWindowRef, 0.0);

   // Get the subview it created
   HIViewRef subview = HIViewGetFirstSubview(view);
   if (subview == NULL)
   {
      // Doesn't seem the effect created the subview, so switch
      // to the plain dialog.  This can happen when an effect
      // uses the content view directly.  As of this time, we
      // will not try to support those and fall back to the
      // textual interface.
      mGui = false;
      RemoveHandler();
      BuildPlain();
      return;
   }

   // Install the tracking event handler on our views
   mTrackingHandlerUPP = NewEventHandlerUPP(VSTEffect::TrackingEventHandler);
   InstallControlEventHandler(root,
                              mTrackingHandlerUPP,
                              GetEventTypeCount(trackingEventList),
                              trackingEventList,
                              this,
                              &mRootTrackingHandlerRef);
   InstallControlEventHandler(view,
                              mTrackingHandlerUPP,
                              GetEventTypeCount(trackingEventList),
                              trackingEventList,
                              this,
                              &mViewTrackingHandlerRef);
   InstallControlEventHandler(subview,
                              mTrackingHandlerUPP,
                              GetEventTypeCount(trackingEventList),
                              trackingEventList,
                              this,
                              &mSubviewTrackingHandlerRef);
   HIViewNewTrackingArea(root, NULL, 0, NULL);
   HIViewNewTrackingArea(view, NULL, 0, NULL);
   HIViewNewTrackingArea(subview, NULL, 0, NULL);

#elif defined(__WXMSW__)

   // Use a panel to host the plugins GUI
   wxPanel *w = new wxPanel(mParent, wxID_ANY);
   mHwnd = w->GetHWND();
   callDispatcher(effEditOpen, 0, 0, mHwnd, 0.0);

#else

   // Use a panel to host the plugins GUI
   wxPanel *w = new wxPanel(mParent, wxID_ANY);

   // Make sure the parent has a window
   if (!gtk_widget_get_realized(GTK_WIDGET(w->m_wxwindow)))
   {
      gtk_widget_realize(GTK_WIDGET(w->m_wxwindow));
   }

   GdkWindow *gwin = gtk_widget_get_window(GTK_WIDGET(w->m_wxwindow));
   mXdisp = GDK_WINDOW_XDISPLAY(gwin);
   mXwin = GDK_WINDOW_XID(gwin);

   callDispatcher(effEditOpen, 0, (intptr_t)mXdisp, (void *)mXwin, 0.0);

#endif

   // Get the final bounds of the effect GUI
   callDispatcher(effEditGetRect, 0, 0, &rect, 0.0);

   // Build our display now
   wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);
   wxBoxSizer *hs = new wxBoxSizer(wxHORIZONTAL);

#if defined(__WXMAC__)

   // Reserve space for the effect GUI
   mContainer = hs->Add(rect->right - rect->left, rect->bottom - rect->top);

#elif defined(__WXMSW__)

   // Add the effect host window to the layout
   mContainer = hs->Add(w, 1, wxCENTER | wxEXPAND);
   mContainer->SetMinSize(rect->right - rect->left, rect->bottom - rect->top);

#else

   // Add the effect host window to the layout
   mContainer = hs->Add(w, 1, wxCENTER | wxEXPAND);
   mContainer->SetMinSize(rect->right - rect->left, rect->bottom - rect->top);

#endif

   vs->Add(hs, 0, wxCENTER);

   mParent->SetSizerAndFit(vs);

#if defined(__WXMAC__)

   // Found out where the reserved space wound up
   wxPoint pos = mContainer->GetPosition();

   // Reposition the subview into the reserved space
   HIViewPlaceInSuperviewAt(subview, pos.x, pos.y);

   // Some VST effects do not work unless the default handler is removed since
   // it captures many of the events that the plugins need.  But, it must be
   // done last since proper window sizing will not occur otherwise.
   ::RemoveEventHandler((EventHandlerRef) mDialog->MacGetEventHandler());

#elif defined(__WXMSW__)
#else
#endif

   NeedEditIdle(true);
}

void VSTEffect::BuildPlain()
{
   wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
   wxScrolledWindow *scroller = new wxScrolledWindow(mParent,
                                                     wxID_ANY,
                                                     wxDefaultPosition,
                                                     wxDefaultSize,
                                                     wxVSCROLL | wxTAB_TRAVERSAL);

   // Try to give the window a sensible default/minimum size
   scroller->SetMinSize(wxSize(wxMax(600, mParent->GetSize().GetWidth() * 2 / 3),
                        mParent->GetSize().GetHeight() / 2));
   scroller->SetScrollRate(0, 20);

   // This fools NVDA into not saying "Panel" when the dialog gets focus
   scroller->SetName(wxT("\a"));
   scroller->SetLabel(wxT("\a"));

   mainSizer->Add(scroller, 1, wxEXPAND | wxALL, 5);
   mParent->SetSizer(mainSizer);

   mNames = new wxStaticText *[mAEffect->numParams];
   mSliders = new wxSlider *[mAEffect->numParams];
   mDisplays = new wxStaticText *[mAEffect->numParams];
   mLabels = new wxStaticText *[mAEffect->numParams];

   wxSizer *paramSizer = new wxStaticBoxSizer(wxVERTICAL, scroller, _("Effect Settings"));

   wxFlexGridSizer *gridSizer = new wxFlexGridSizer(4, 0, 0);
   gridSizer->AddGrowableCol(1);

   // Add the duration control for generators
   if (GetType() == EffectTypeGenerate)
   {
      wxControl *item = new wxStaticText(scroller, 0, _("Duration:"));
      gridSizer->Add(item, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxALL, 5);
      mDuration = new
         NumericTextCtrl(NumericConverter::TIME,
                         scroller,
                         ID_Duration,
                         mHost->GetDurationFormat(),
                         mHost->GetDuration(),
                         mSampleRate,
                         wxDefaultPosition,
                         wxDefaultSize,
                         true);
      mDuration->SetName(_("Duration"));
      mDuration->EnableMenu();
      gridSizer->Add(mDuration, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
      gridSizer->Add(1, 1, 0);
      gridSizer->Add(1, 1, 0);
   }

   // Find the longest parameter name.
   int namew = 0;
   int w;
   int h;
   for (int i = 0; i < mAEffect->numParams; i++)
   {
      wxString text = GetString(effGetParamName, i);

      if (text.Right(1) != wxT(':'))
      {
         text += wxT(':');
      }

      scroller->GetTextExtent(text, &w, &h);
      if (w > namew)
      {
         namew = w;
      }
   }

   scroller->GetTextExtent(wxT("HHHHHHHH"), &w, &h);

   for (int i = 0; i < mAEffect->numParams; i++)
   {
      mNames[i] = new wxStaticText(scroller,
                                   wxID_ANY,
                                   wxEmptyString,
                                   wxDefaultPosition,
                                   wxSize(namew, -1),
                                   wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
      gridSizer->Add(mNames[i], 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxALL, 5);

      mSliders[i] = new wxSlider(scroller,
                                 ID_Sliders + i,
                                 0,
                                 0,
                                 1000,
                                 wxDefaultPosition,
                                 wxSize(200, -1));
      gridSizer->Add(mSliders[i], 0, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 5);

      mDisplays[i] = new wxStaticText(scroller,
                                      wxID_ANY,
                                      wxEmptyString,
                                      wxDefaultPosition,
                                      wxSize(w, -1),
                                      wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
      gridSizer->Add(mDisplays[i], 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxALL, 5);

      mLabels[i] = new wxStaticText(scroller,
                                    wxID_ANY,
                                    wxEmptyString,
                                    wxDefaultPosition,
                                    wxSize(w, -1),
                                    wxALIGN_LEFT | wxST_NO_AUTORESIZE);
      gridSizer->Add(mLabels[i], 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxALL, 5);
   }

   paramSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 5);
   scroller->SetSizer(paramSizer);

   RefreshParameters();

   mSliders[0]->SetFocus();
}

void VSTEffect::RefreshParameters(int skip)
{
   if (mGui)
   {
      return;
   }

   for (int i = 0; i < mAEffect->numParams; i++)
   {
      wxString text = GetString(effGetParamName, i);

      text = text.Trim(true).Trim(false);

      wxString name = text;

      if (text.Right(1) != wxT(':'))
      {
         text += wxT(':');
      }
      mNames[i]->SetLabel(text);

      // For some parameters types like on/off, setting the slider value has
      // a side effect that causes it to only move when the parameter changes
      // from off to on.  However, this prevents changing the value using the
      // keyboard, so we skip the active slider if any.
      if (i != skip)
      {
         mSliders[i]->SetValue(callGetParameter(i) * 1000);
      }
      name = text;

      text = GetString(effGetParamDisplay, i);
      if (text.IsEmpty())
      {
         text.Printf(wxT("%.5g"),callGetParameter(i));
      }
      mDisplays[i]->SetLabel(wxString::Format(wxT("%8s"), text.c_str()));
      name += wxT(' ') + text;

      text = GetString(effGetParamDisplay, i);
      if (!text.IsEmpty())
      {
         text.Printf(wxT("%-8s"), GetString(effGetParamLabel, i).c_str());
         mLabels[i]->SetLabel(wxString::Format(wxT("%8s"), text.c_str()));
         name += wxT(' ') + text;
      }

      mSliders[i]->SetName(name);
   }
}

void VSTEffect::OnSizeWindow(wxCommandEvent & evt)
{
   if (!mContainer)
   {
      return;
   }

   // This really needs some work.  We shouldn't know anything about the parent...
   mContainer->SetMinSize(evt.GetInt(), (int) evt.GetExtraLong());
   mParent->SetMinSize(mContainer->GetMinSize());
   mDialog->Layout();
   mDialog->Fit();
}

void VSTEffect::OnSlider(wxCommandEvent & evt)
{
   wxSlider *s = (wxSlider *) evt.GetEventObject();
   int i = s->GetId() - ID_Sliders;

   callSetParameter(i, s->GetValue() / 1000.0);

   RefreshParameters(i);
}

bool VSTEffect::LoadFXB(const wxFileName & fn)
{
   bool ret = false;

   // Try to open the file...will be closed automatically when method returns
   wxFFile f(fn.GetFullPath(), wxT("rb"));
   if (!f.IsOpened())
   {
      return false;
   }

   // Allocate memory for the contents
   unsigned char *data = new unsigned char[f.Length()];
   if (!data)
   {
      wxMessageBox(_("Unable to allocate memory when loading presets file."),
                   _("Error Loading VST Presets"),
                   wxOK | wxCENTRE,
                   mParent);
      return false;
   }
   unsigned char *bptr = data;

   do
   {
      // Read in the whole file
      ssize_t len = f.Read((void *) bptr, f.Length());
      if (f.Error())
      {
         wxMessageBox(_("Unable to read presets file."),
                      _("Error Loading VST Presets"),
                      wxOK | wxCENTRE,
                      mParent);
         break;
      }

      // Most references to the data are via an "int" array
      int32_t *iptr = (int32_t *) bptr;

      // Verify that we have at least enough for the header
      if (len < 156)
      {
         break;
      }

      // Verify that we probably have an FX file
      if (wxINT32_SWAP_ON_LE(iptr[0]) != CCONST('C', 'c', 'n', 'K'))
      {
         break;
      }

      // Ignore the size...sometimes it's there, other times it's zero

      // Get the version and verify
      int version = wxINT32_SWAP_ON_LE(iptr[3]);
      if (version != 1 && version != 2)
      {
         break;
      }

      VstPatchChunkInfo info =
      {
         1,
         wxINT32_SWAP_ON_LE(iptr[4]),
         wxINT32_SWAP_ON_LE(iptr[5]),
         wxINT32_SWAP_ON_LE(iptr[6])
      };

      // Ensure this program looks to belong to the current plugin
      if ((info.pluginUniqueID != mAEffect->uniqueID) &&
          (info.pluginVersion != mAEffect->version) &&
          (info.numElements != mAEffect->numPrograms))
      {
         break;
      }

      // Get the number of programs
      int numProgs = info.numElements;

      // Get the current program index
      int curProg = 0;
      if (version >= 2)
      {
         curProg = wxINT32_SWAP_ON_LE(iptr[7]);
         if (curProg < 0 || curProg >= numProgs)
         {
            break;
         }
      }

      // Is it a bank of programs?
      if (wxINT32_SWAP_ON_LE(iptr[2]) == CCONST('F', 'x', 'B', 'k'))
      {
         // Drop the header
         bptr += 156;
         len -= 156;

         unsigned char *tempPtr = bptr;
         ssize_t tempLen = len;

         // Validate all of the programs
         for (int i = 0; i < numProgs; i++)
         {
            if (!LoadFXProgram(&tempPtr, tempLen, i, true))
            {
               break;
            }
         }

         // Ask the effect if this is an acceptable bank
         if (callDispatcher(effBeginLoadBank, 0, 0, &info, 0.0) == -1)
         {
            return false;
         }

         // Start loading the individual programs
         for (int i = 0; i < numProgs; i++)
         {
            ret = LoadFXProgram(&bptr, len, i, false);
         }
      }
      // Or maybe a bank chunk?
      else if (wxINT32_SWAP_ON_LE(iptr[2]) == CCONST('F', 'B', 'C', 'h'))
      {
         // Can't load programs chunks if the plugin doesn't support it
         if (!(mAEffect->flags & effFlagsProgramChunks))
         {
            break;
         }

         // Verify that we have enough to grab the chunk size
         if (len < 160)
         {
            break;
         }

         // Get the chunk size
         int size = wxINT32_SWAP_ON_LE(iptr[39]);

         // We finally know the full length of the program
         int proglen = 160 + size;

         // Verify that we have enough for the entire program
         if (len < proglen)
         {
            break;
         }

         // Set the entire bank in one shot
         callSetChunk(false, size, &iptr[40], &info);

         // Success
         ret = true;
      }
      // Unrecognizable type
      else
      {
         break;
      }

      // Set the active program
      if (ret && version >= 2)
      {
         callSetProgram(curProg);
      }
   } while (false);

   // Get rid of the data
   delete [] data;

   return ret;
}

bool VSTEffect::LoadFXP(const wxFileName & fn)
{
   bool ret = false;

   // Try to open the file...will be closed automatically when method returns
   wxFFile f(fn.GetFullPath(), wxT("rb"));
   if (!f.IsOpened())
   {
      return false;
   }

   // Allocate memory for the contents
   unsigned char *data = new unsigned char[f.Length()];
   if (!data)
   {
      wxMessageBox(_("Unable to allocate memory when loading presets file."),
                   _("Error Loading VST Presets"),
                   wxOK | wxCENTRE,
                   mParent);
      return false;
   }
   unsigned char *bptr = data;

   do
   {
      // Read in the whole file
      ssize_t len = f.Read((void *) bptr, f.Length());
      if (f.Error())
      {
         wxMessageBox(_("Unable to read presets file."),
                      _("Error Loading VST Presets"),
                      wxOK | wxCENTRE,
                      mParent);
         break;
      }

      // Get (or default) currently selected program
      int i = 0; //mProgram->GetCurrentSelection();
      if (i < 0)
      {
         i = 0;   // default to first program
      }

      // Go verify and set the program
      ret = LoadFXProgram(&bptr, len, i, false);
   } while (false);

   // Get rid of the data
   delete [] data;

   return ret;
}

bool VSTEffect::LoadFXProgram(unsigned char **bptr, ssize_t & len, int index, bool dryrun)
{
   // Most references to the data are via an "int" array
   int32_t *iptr = (int32_t *) *bptr;

   // Verify that we have at least enough for a program without parameters
   if (len < 28)
   {
      return false;
   }

   // Verify that we probably have an FX file
   if (wxINT32_SWAP_ON_LE(iptr[0]) != CCONST('C', 'c', 'n', 'K'))
   {
      return false;
   }

   // Ignore the size...sometimes it's there, other times it's zero

   // Get the version and verify
#if defined(IS_THIS_AN_FXP_ARTIFICAL_LIMITATION)
   int version = wxINT32_SWAP_ON_LE(iptr[3]);
   if (version != 1)
   {
      return false;
   }
#endif

   VstPatchChunkInfo info =
   {
      1,
      wxINT32_SWAP_ON_LE(iptr[4]),
      wxINT32_SWAP_ON_LE(iptr[5]),
      wxINT32_SWAP_ON_LE(iptr[6])
   };

   // Ensure this program looks to belong to the current plugin
   if ((info.pluginUniqueID != mAEffect->uniqueID) &&
         (info.pluginVersion != mAEffect->version) &&
         (info.numElements != mAEffect->numParams))
   {
      return false;
   }

   // Get the number of parameters
   int numParams = info.numElements;

   // At this point, we have to have enough to include the program name as well
   if (len < 56)
   {
      return false;
   }

   // Get the program name
   wxString progName(wxString::From8BitData((char *)&iptr[7]));

   // Might be a regular program
   if (wxINT32_SWAP_ON_LE(iptr[2]) == CCONST('F', 'x', 'C', 'k'))
   {
      // We finally know the full length of the program
      int proglen = 56 + (numParams * sizeof(float));

      // Verify that we have enough for all of the parameter values
      if (len < proglen)
      {
         return false;
      }

      // Validate all of the parameter values
      for (int i = 0; i < numParams; i++)
      {
         uint32_t ival = wxUINT32_SWAP_ON_LE(iptr[14 + i]);
         float val = *((float *) &ival);
         if (val < 0.0 || val > 1.0)
         {
            return false;
         }
      }
         
      // They look okay...time to start changing things
      if (!dryrun)
      {
         // Ask the effect if this is an acceptable program
         if (callDispatcher(effBeginLoadProgram, 0, 0, &info, 0.0) == -1)
         {
            return false;
         }

         // Load all of the parameters
         callDispatcher(effBeginSetProgram, 0, 0, NULL, 0.0);
         for (int i = 0; i < numParams; i++)
         {
            wxUint32 val = wxUINT32_SWAP_ON_LE(iptr[14 + i]);
            callSetParameter(i, *((float *) &val));
         }
         callDispatcher(effEndSetProgram, 0, 0, NULL, 0.0);
      }

      // Update in case we're loading an "FxBk" format bank file
      *bptr += proglen;
      len -= proglen;
   }
   // Maybe we have a program chunk
   else if (wxINT32_SWAP_ON_LE(iptr[2]) == CCONST('F', 'P', 'C', 'h'))
   {
      // Can't load programs chunks if the plugin doesn't support it
      if (!(mAEffect->flags & effFlagsProgramChunks))
      {
         return false;
      }

      // Verify that we have enough to grab the chunk size
      if (len < 60)
      {
         return false;
      }

      // Get the chunk size
      int size = wxINT32_SWAP_ON_LE(iptr[14]);

      // We finally know the full length of the program
      int proglen = 60 + size;

      // Verify that we have enough for the entire program
      if (len < proglen)
      {
         return false;
      }

      // Set the entire program in one shot
      if (!dryrun)
      {
         callSetChunk(true, size, &iptr[15], &info);
      }

      // Update in case we're loading an "FxBk" format bank file
      *bptr += proglen;
      len -= proglen;
   }
   else
   {
      // Unknown type
      return false;
   }
   
   if (!dryrun)
   {
      SetString(effSetProgramName, wxString(progName), index);
   }

   return true;
}

bool VSTEffect::LoadXML(const wxFileName & fn)
{
   mInChunk = false;
   mInSet = false;

   // default to read as XML file
   // Load the program
   XMLFileReader reader;
   bool ok = reader.Parse(this, fn.GetFullPath());

   // Something went wrong with the file, clean up
   if (mInSet)
   {
      callDispatcher(effEndSetProgram, 0, 0, NULL, 0.0);

      mInSet = false;
   }

   if (!ok)
   {
      // Inform user of load failure
      wxMessageBox(reader.GetErrorStr(),
                   _("Error Loading VST Presets"),
                   wxOK | wxCENTRE,
                   mParent);
      return false;
   }

   return true;
}

void VSTEffect::SaveFXB(const wxFileName & fn)
{
   // Create/Open the file
   wxFFile f(fn.GetFullPath(), wxT("wb"));
   if (!f.IsOpened())
   {
      wxMessageBox(wxString::Format(_("Could not open file: \"%s\""), fn.GetFullPath().c_str()),
                   _("Error Saving VST Presets"),
                   wxOK | wxCENTRE,
                   mParent);
      return;
   }

   wxMemoryBuffer buf;
   wxInt32 subType;
   void *chunkPtr;
   int chunkSize;
   int dataSize = 148;
   wxInt32 tab[8];
   int curProg = 0 ; //mProgram->GetCurrentSelection();

   if (mAEffect->flags & effFlagsProgramChunks)
   {
      subType = CCONST('F', 'B', 'C', 'h');

      chunkSize = callDispatcher(effGetChunk, 0, 0, &chunkPtr, 0.0);
      dataSize += 4 + chunkSize;
   }
   else
   {
      subType = CCONST('F', 'x', 'B', 'k');

      for (int i = 0; i < mAEffect->numPrograms; i++)
      {
         SaveFXProgram(buf, i);
      }

      dataSize += buf.GetDataLen();
   }

   tab[0] = wxINT32_SWAP_ON_LE(CCONST('C', 'c', 'n', 'K'));
   tab[1] = wxINT32_SWAP_ON_LE(dataSize);
   tab[2] = wxINT32_SWAP_ON_LE(subType);
   tab[3] = wxINT32_SWAP_ON_LE(curProg >= 0 ? 2 : 1);
   tab[4] = wxINT32_SWAP_ON_LE(mAEffect->uniqueID);
   tab[5] = wxINT32_SWAP_ON_LE(mAEffect->version);
   tab[6] = wxINT32_SWAP_ON_LE(mAEffect->numPrograms);
   tab[7] = wxINT32_SWAP_ON_LE(curProg >= 0 ? curProg : 0);

   f.Write(tab, sizeof(tab));
   if (!f.Error())
   {
      char padding[124];
      memset(padding, 0, sizeof(padding));
      f.Write(padding, sizeof(padding));

      if (!f.Error())
      {
         if (mAEffect->flags & effFlagsProgramChunks)
         {
            wxInt32 size = wxINT32_SWAP_ON_LE(chunkSize);
            f.Write(&size, sizeof(size));
            f.Write(chunkPtr, chunkSize);
         }
         else
         {
            f.Write(buf.GetData(), buf.GetDataLen());
         }
      }
   }

   if (f.Error())
   {
      wxMessageBox(wxString::Format(_("Error writing to file: \"%s\""), fn.GetFullPath().c_str()),
                   _("Error Saving VST Presets"),
                   wxOK | wxCENTRE,
                   mParent);
   }

   f.Close();

   return;
}

void VSTEffect::SaveFXP(const wxFileName & fn)
{
   // Create/Open the file
   wxFFile f(fn.GetFullPath(), wxT("wb"));
   if (!f.IsOpened())
   {
      wxMessageBox(wxString::Format(_("Could not open file: \"%s\""), fn.GetFullPath().c_str()),
                   _("Error Saving VST Presets"),
                   wxOK | wxCENTRE,
                   mParent);
      return;
   }

   wxMemoryBuffer buf;

   int ndx = callDispatcher(effGetProgram, 0, 0, NULL, 0.0);
   SaveFXProgram(buf, ndx);

   f.Write(buf.GetData(), buf.GetDataLen());
   if (f.Error())
   {
      wxMessageBox(wxString::Format(_("Error writing to file: \"%s\""), fn.GetFullPath().c_str()),
                   _("Error Saving VST Presets"),
                   wxOK | wxCENTRE,
                   mParent);
   }

   f.Close();

   return;
}

void VSTEffect::SaveFXProgram(wxMemoryBuffer & buf, int index)
{
   wxInt32 subType;
   void *chunkPtr;
   int chunkSize;
   int dataSize = 48;
   char progName[28];
   wxInt32 tab[7];

   callDispatcher(effGetProgramNameIndexed, index, 0, &progName, 0.0);
   progName[27] = '\0';
   chunkSize = strlen(progName);
   memset(&progName[chunkSize], 0, sizeof(progName) - chunkSize);

   if (mAEffect->flags & effFlagsProgramChunks)
   {
      subType = CCONST('F', 'P', 'C', 'h');

      chunkSize = callDispatcher(effGetChunk, 1, 0, &chunkPtr, 0.0);
      dataSize += 4 + chunkSize;
   }
   else
   {
      subType = CCONST('F', 'x', 'C', 'k');

      dataSize += (mAEffect->numParams << 2);
   }

   tab[0] = wxINT32_SWAP_ON_LE(CCONST('C', 'c', 'n', 'K'));
   tab[1] = wxINT32_SWAP_ON_LE(dataSize);
   tab[2] = wxINT32_SWAP_ON_LE(subType);
   tab[3] = wxINT32_SWAP_ON_LE(1);
   tab[4] = wxINT32_SWAP_ON_LE(mAEffect->uniqueID);
   tab[5] = wxINT32_SWAP_ON_LE(mAEffect->version);
   tab[6] = wxINT32_SWAP_ON_LE(mAEffect->numParams);

   buf.AppendData(tab, sizeof(tab));
   buf.AppendData(progName, sizeof(progName));

   if (mAEffect->flags & effFlagsProgramChunks)
   {
      wxInt32 size = wxINT32_SWAP_ON_LE(chunkSize);
      buf.AppendData(&size, sizeof(size));
      buf.AppendData(chunkPtr, chunkSize);
   }
   else
   {
      for (int i = 0; i < mAEffect->numParams; i++)
      {
         float val = callGetParameter(i);
         wxUint32 ival = wxUINT32_SWAP_ON_LE(*((wxUint32 *) &val));
         buf.AppendData(&ival, sizeof(ival));
      }
   }

   return;
}

void VSTEffect::SaveXML(const wxFileName & fn)
{
   XMLFileWriter xmlFile;

   // Create/Open the file
   xmlFile.Open(fn.GetFullPath(), wxT("wb"));

   xmlFile.StartTag(wxT("vstprogrampersistence"));
   xmlFile.WriteAttr(wxT("version"), wxT("2"));

   xmlFile.StartTag(wxT("effect"));
   xmlFile.WriteAttr(wxT("name"), GetName());
   xmlFile.WriteAttr(wxT("uniqueID"), mAEffect->uniqueID);
   xmlFile.WriteAttr(wxT("version"), mAEffect->version);
   xmlFile.WriteAttr(wxT("numParams"), mAEffect->numParams);

   xmlFile.StartTag(wxT("program"));
   xmlFile.WriteAttr(wxT("name"), wxEmptyString); //mProgram->GetValue());

   int clen = 0;
   if (mAEffect->flags & effFlagsProgramChunks)
   {
      void *chunk = NULL;

      clen = (int) callDispatcher(effGetChunk, 1, 0, &chunk, 0.0);
      if (clen != 0)
      {
         xmlFile.StartTag(wxT("chunk"));
         xmlFile.WriteSubTree(VSTEffect::b64encode(chunk, clen) + wxT('\n'));
         xmlFile.EndTag(wxT("chunk"));
      }
   }

   if (clen == 0)
   {
      for (int i = 0; i < mAEffect->numParams; i++)
      {
         xmlFile.StartTag(wxT("param"));

         xmlFile.WriteAttr(wxT("index"), i);
         xmlFile.WriteAttr(wxT("name"),
                           GetString(effGetParamName, i));
         xmlFile.WriteAttr(wxT("value"),
                           wxString::Format(wxT("%f"),
                           callGetParameter(i)));

         xmlFile.EndTag(wxT("param"));
      }
   }

   xmlFile.EndTag(wxT("program"));

   xmlFile.EndTag(wxT("effect"));

   xmlFile.EndTag(wxT("vstprogrampersistence"));

   // Close the file
   xmlFile.Close();

   return;
}

bool VSTEffect::HandleXMLTag(const wxChar *tag, const wxChar **attrs)
{
   if (wxStrcmp(tag, wxT("vstprogrampersistence")) == 0)
   {
      while (*attrs)
      {
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         if (!value)
         {
            break;
         }

         const wxString strValue = value;

         if (wxStrcmp(attr, wxT("version")) == 0)
         {
            if (!XMLValueChecker::IsGoodInt(strValue) || !strValue.ToLong(&mXMLVersion))
            {
               return false;
            }

            if (mXMLVersion < 1 || mXMLVersion > 2)
            {
               return false;
            }
         }
         else
         {
            return false;
         }
      }

      return true;
   }

   if (wxStrcmp(tag, wxT("effect")) == 0)
   {
      memset(&mXMLInfo, 0, sizeof(mXMLInfo));
      mXMLInfo.version = 1;
      mXMLInfo.pluginUniqueID = mAEffect->uniqueID;
      mXMLInfo.pluginVersion = mAEffect->version;
      mXMLInfo.numElements = mAEffect->numParams;

      while (*attrs)
      {
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         if (!value)
         {
            break;
         }

         const wxString strValue = value;

         if (wxStrcmp(attr, wxT("name")) == 0)
         {
            if (!XMLValueChecker::IsGoodString(strValue))
            {
               return false;
            }

            if (value != GetName())
            {
               wxString msg;
               msg.Printf(_("This parameter file was saved from %s.  Continue?"), value);
               int result = wxMessageBox(msg, wxT("Confirm"), wxYES_NO, mParent);
               if (result == wxNO)
               {
                  return false;
               }
            }
         }
         else if (wxStrcmp(attr, wxT("version")) == 0)
         {
            long version;
            if (!XMLValueChecker::IsGoodInt(strValue) || !strValue.ToLong(&version))
            {
               return false;
            }

            mXMLInfo.pluginVersion = (int) version;
         }
         else if (mXMLVersion > 1 && wxStrcmp(attr, wxT("uniqueID")) == 0)
         {
            long uniqueID;
            if (!XMLValueChecker::IsGoodInt(strValue) || !strValue.ToLong(&uniqueID))
            {
               return false;
            }

            mXMLInfo.pluginUniqueID = (int) uniqueID;
         }
         else if (mXMLVersion > 1 && wxStrcmp(attr, wxT("numParams")) == 0)
         {
            long numParams;
            if (!XMLValueChecker::IsGoodInt(strValue) || !strValue.ToLong(&numParams))
            {
               return false;
            }

            mXMLInfo.numElements = (int) numParams;
         }
         else
         {
            return false;
         }
      }

      return true;
   }

   if (wxStrcmp(tag, wxT("program")) == 0)
   {
      while (*attrs)
      {
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         if (!value)
         {
            break;
         }

         const wxString strValue = value;

         if (wxStrcmp(attr, wxT("name")) == 0)
         {
            if (!XMLValueChecker::IsGoodString(strValue))
            {
               return false;
            }

            if (strValue.length() > 24)
            {
               return false;
            }

            int ndx = 0; //mProgram->GetCurrentSelection();
            if (ndx == wxNOT_FOUND)
            {
               ndx = 0;
            }

            SetString(effSetProgramName, strValue, ndx);
         }
         else
         {
            return false;
         }
      }

      mInChunk = false;

      if (callDispatcher(effBeginLoadProgram, 0, 0, &mXMLInfo, 0.0) == -1)
      {
         return false;
      }

      callDispatcher(effBeginSetProgram, 0, 0, NULL, 0.0);

      mInSet = true;

      return true;
   }

   if (wxStrcmp(tag, wxT("param")) == 0)
   {
      long ndx = -1;
      double val = -1.0;
      while (*attrs)
      {
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         if (!value)
         {
            break;
         }

         const wxString strValue = value;

         if (wxStrcmp(attr, wxT("index")) == 0)
         {
            if (!XMLValueChecker::IsGoodInt(strValue) || !strValue.ToLong(&ndx))
            {
               return false;
            }

            if (ndx < 0 || ndx >= mAEffect->numParams)
            {
               // Could be a different version of the effect...probably should
               // tell the user
               return false;
            }
         }
         else if (wxStrcmp(attr, wxT("name")) == 0)
         {
            if (!XMLValueChecker::IsGoodString(strValue))
            {
               return false;
            }
            // Nothing to do with it for now
         }
         else if (wxStrcmp(attr, wxT("value")) == 0)
         {
            if (!XMLValueChecker::IsGoodInt(strValue) ||
               !Internat::CompatibleToDouble(strValue, &val))
            {
               return false;
            }

            if (val < 0.0 || val > 1.0)
            {
               return false;
            }
         }
      }

      if (ndx == -1 || val == -1.0)
      {
         return false;
      }

      callSetParameter(ndx, val);

      return true;
   }

   if (wxStrcmp(tag, wxT("chunk")) == 0)
   {
      mInChunk = true;
      return true;
   }

   return false;
}

void VSTEffect::HandleXMLEndTag(const wxChar *tag)
{
   if (wxStrcmp(tag, wxT("chunk")) == 0)
   {
      if (mChunk.length())
      {
         char *buf = new char[mChunk.length() / 4 * 3];

         int len = VSTEffect::b64decode(mChunk, buf);
         if (len)
         {
            callSetChunk(true, len, buf, &mXMLInfo);
         }

         delete [] buf;
         mChunk.clear();
      }
      mInChunk = false;
   }

   if (wxStrcmp(tag, wxT("program")) == 0)
   {
      if (mInSet)
      {
         callDispatcher(effEndSetProgram, 0, 0, NULL, 0.0);

         mInSet = false;
      }
   }
}

void VSTEffect::HandleXMLContent(const wxString & content)
{
   if (mInChunk)
   {
      mChunk += wxString(content).Trim(true).Trim(false);
   }
}

XMLTagHandler *VSTEffect::HandleXMLChild(const wxChar *tag)
{
   if (wxStrcmp(tag, wxT("vstprogrampersistence")) == 0)
   {
      return this;
   }

   if (wxStrcmp(tag, wxT("effect")) == 0)
   {
      return this;
   }

   if (wxStrcmp(tag, wxT("program")) == 0)
   {
      return this;
   }

   if (wxStrcmp(tag, wxT("param")) == 0)
   {
      return this;
   }

   if (wxStrcmp(tag, wxT("chunk")) == 0)
   {
      return this;
   }

   return NULL;
}

#endif // USE_VST
