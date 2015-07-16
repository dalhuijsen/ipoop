/**********************************************************************

  Audacity: A Digital Audio Editor

  PluginManager.h

  Leland Lucius

**********************************************************************/

#ifndef __AUDACITY_PLUGINMANAGER_H__
#define __AUDACITY_PLUGINMANAGER_H__

#include <wx/defs.h>
#include <wx/dynarray.h>
#include <wx/fileconf.h>
#include <wx/string.h>

#include <map>

#include "audacity/EffectInterface.h"
#include "audacity/ImporterInterface.h"
#include "audacity/ModuleInterface.h"
#include "audacity/PluginInterface.h"

///////////////////////////////////////////////////////////////////////////////
//
// PluginDescriptor
//
///////////////////////////////////////////////////////////////////////////////

typedef enum
{
   PluginTypeNone = -1,          // 2.1.0 placeholder entries...not used by 2.1.1 or greater
   PluginTypeStub,               // Used for plugins that have not yet been registered
   PluginTypeEffect,
   PluginTypeExporter,
   PluginTypeImporter,
   PluginTypeModule,
} PluginType;

// TODO:  Convert this to multiple derived classes
class PluginDescriptor
{
public:
   PluginDescriptor();
   virtual ~PluginDescriptor();

   bool IsInstantiated() const;
   IdentInterface *GetInstance();
   void SetInstance(IdentInterface *instance);

   PluginType GetPluginType() const;
   void SetPluginType(PluginType type);

   // All plugins

   // These return untranslated strings
   const wxString & GetID() const;
   const wxString & GetProviderID() const;
   const wxString & GetPath() const;
   const wxString & GetSymbol() const;

   // These return translated strings (if available and if requested)
   wxString GetName(bool translate = true) const;
   wxString GetVersion(bool translate = true) const;
   wxString GetVendor(bool translate = true) const;
   wxString GetDescription(bool translate = true) const;
   bool IsEnabled() const;
   bool IsValid() const;

   // These should be passed an untranslated value
   void SetID(const PluginID & ID);
   void SetProviderID(const PluginID & providerID);
   void SetPath(const wxString & path);
   void SetSymbol(const wxString & symbol);

   // These should be passed an untranslated value wrapped in XO() so
   // the value will still be extracted for translation
   void SetName(const wxString & name);
   void SetVersion(const wxString & version);
   void SetVendor(const wxString & vendor);
   void SetDescription(const wxString & description);

   void SetEnabled(bool enable);
   void SetValid(bool valid);

   // Effect plugins only

   // Will return an untranslated string
   wxString GetEffectFamily(bool translate = true) const;
   EffectType GetEffectType() const;
   bool IsEffectDefault() const;
   bool IsEffectInteractive() const;
   bool IsEffectLegacy() const;
   bool IsEffectRealtime() const;
   bool IsEffectAutomatable() const;

   // "family" should be an untranslated string wrapped in wxT()
   void SetEffectFamily(const wxString & family);
   void SetEffectType(EffectType type);
   void SetEffectDefault(bool dflt);
   void SetEffectInteractive(bool interactive);
   void SetEffectLegacy(bool legacy);
   void SetEffectRealtime(bool realtime);
   void SetEffectAutomatable(bool automatable);

   // Importer plugins only

   const wxString & GetImporterIdentifier() const;
   const wxString & GetImporterFilterDescription() const;
   const wxArrayString & GetImporterExtensions() const;

   void SetImporterIdentifier(const wxString & identifier);
   void SetImporterFilterDescription(const wxString & filterDesc);
   void SetImporterExtensions(const wxArrayString & extensions);

private:

   // Common

   IdentInterface *mInstance;

   PluginType mPluginType;

   wxString mID;
   wxString mPath;
   wxString mSymbol;
   wxString mName;
   wxString mVersion;
   wxString mVendor;
   wxString mDescription;
   wxString mProviderID;
   bool mEnabled;
   bool mValid;

   // Effects

   wxString mEffectFamily;
   EffectType mEffectType;
   bool mEffectInteractive;
   bool mEffectDefault;
   bool mEffectLegacy;
   bool mEffectRealtime;
   bool mEffectAutomatable;

   // Importers

   wxString mImporterIdentifier;
   wxString mImporterFilterDesc;
   wxArrayString mImporterExtensions;
};

///////////////////////////////////////////////////////////////////////////////
//
// PluginManager
//
///////////////////////////////////////////////////////////////////////////////

typedef std::map<PluginID, PluginDescriptor> PluginMap;

typedef wxArrayString PluginIDList;

class ProviderMap;
class PluginRegistrationDialog;

class PluginManager : public PluginManagerInterface
{
public:
   PluginManager();
   virtual ~PluginManager();

   // PluginManagerInterface implementation

   virtual bool IsPluginRegistered(const wxString & path);

   virtual const PluginID & RegisterPlugin(ModuleInterface *module);
   virtual const PluginID & RegisterPlugin(ModuleInterface *provider, EffectIdentInterface *effect);
   virtual const PluginID & RegisterPlugin(ModuleInterface *provider, ImporterInterface *importer);

   virtual void FindFilesInPathList(const wxString & pattern,
                                    const wxArrayString & pathList,
                                    wxArrayString & files,
                                    bool directories = false);

   virtual bool HasSharedConfigGroup(const PluginID & ID, const wxString & group);
   virtual bool GetSharedConfigSubgroups(const PluginID & ID, const wxString & group, wxArrayString & subgroups);

   virtual bool GetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, wxString & value, const wxString & defval = _T(""));
   virtual bool GetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, int & value, int defval = 0);
   virtual bool GetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, bool & value, bool defval = false);
   virtual bool GetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, float & value, float defval = 0.0);
   virtual bool GetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, double & value, double defval = 0.0);
   virtual bool GetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, sampleCount & value, sampleCount defval = 0);

   virtual bool SetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, const wxString & value);
   virtual bool SetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, const int & value);
   virtual bool SetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, const bool & value);
   virtual bool SetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, const float & value);
   virtual bool SetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, const double & value);
   virtual bool SetSharedConfig(const PluginID & ID, const wxString & group, const wxString & key, const sampleCount & value);

   virtual bool RemoveSharedConfigSubgroup(const PluginID & ID, const wxString & group);
   virtual bool RemoveSharedConfig(const PluginID & ID, const wxString & group, const wxString & key);

   virtual bool HasPrivateConfigGroup(const PluginID & ID, const wxString & group);
   virtual bool GetPrivateConfigSubgroups(const PluginID & ID, const wxString & group, wxArrayString & subgroups);

   virtual bool GetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, wxString & value, const wxString & defval = _T(""));
   virtual bool GetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, int & value, int defval = 0);
   virtual bool GetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, bool & value, bool defval = false);
   virtual bool GetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, float & value, float defval = 0.0);
   virtual bool GetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, double & value, double defval = 0.0);
   virtual bool GetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, sampleCount & value, sampleCount defval = 0);

   virtual bool SetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, const wxString & value);
   virtual bool SetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, const int & value);
   virtual bool SetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, const bool & value);
   virtual bool SetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, const float & value);
   virtual bool SetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, const double & value);
   virtual bool SetPrivateConfig(const PluginID & ID, const wxString & group, const wxString & key, const sampleCount & value);

   virtual bool RemovePrivateConfigSubgroup(const PluginID & ID, const wxString & group);
   virtual bool RemovePrivateConfig(const PluginID & ID, const wxString & group, const wxString & key);

   // PluginManager implementation

   void Initialize();
   void Terminate();

   static PluginManager & Get();
   static void Destroy();

   static PluginID GetID(ModuleInterface *module);
   static PluginID GetID(EffectIdentInterface *effect);
   static PluginID GetID(ImporterInterface *importer);

   static wxString GetPluginTypeString(PluginType type);

   int GetPluginCount(PluginType type);
   const PluginDescriptor *GetPlugin(const PluginID & ID);

   const PluginDescriptor *GetFirstPlugin(PluginType type);
   const PluginDescriptor *GetNextPlugin(PluginType type);

   const PluginDescriptor *GetFirstPluginForEffectType(EffectType type);
   const PluginDescriptor *GetNextPluginForEffectType(EffectType type);

   bool IsPluginEnabled(const PluginID & ID);
   void EnablePlugin(const PluginID & ID, bool enable);

   // Returns untranslated string
   const wxString & GetSymbol(const PluginID & ID);
   // Returns translated string
   wxString GetName(const PluginID & ID);
   IdentInterface *GetInstance(const PluginID & ID);

   void CheckForUpdates();

   bool ShowManager(wxWindow *parent, EffectType type = EffectTypeNone);

   // Here solely for the purpose of Nyquist Workbench until
   // a better solution is devised.
   const PluginID & RegisterPlugin(EffectIdentInterface *effect);
   void UnregisterPlugin(const PluginID & ID);

private:
   void Load();
   void LoadGroup(PluginType type);
   void Save();
   void SaveGroup(PluginType type);

   PluginDescriptor & CreatePlugin(const PluginID & id, IdentInterface *ident, PluginType type);

   wxFileConfig *GetSettings();

   bool HasGroup(const wxString & group);
   bool GetSubgroups(const wxString & group, wxArrayString & subgroups);

   bool GetConfig(const wxString & key, wxString & value, const wxString & defval = L"");
   bool GetConfig(const wxString & key, int & value, int defval = 0);
   bool GetConfig(const wxString & key, bool & value, bool defval = false);
   bool GetConfig(const wxString & key, float & value, float defval = 0.0);
   bool GetConfig(const wxString & key, double & value, double defval = 0.0);
   bool GetConfig(const wxString & key, sampleCount & value, sampleCount defval = 0);

   bool SetConfig(const wxString & key, const wxString & value);
   bool SetConfig(const wxString & key, const int & value);
   bool SetConfig(const wxString & key, const bool & value);
   bool SetConfig(const wxString & key, const float & value);
   bool SetConfig(const wxString & key, const double & value);
   bool SetConfig(const wxString & key, const sampleCount & value);

   wxString SettingsPath(const PluginID & ID, bool shared);
   wxString SharedGroup(const PluginID & ID, const wxString & group);
   wxString SharedKey(const PluginID & ID, const wxString & group, const wxString & key);
   wxString PrivateGroup(const PluginID & ID, const wxString & group);
   wxString PrivateKey(const PluginID & ID, const wxString & group, const wxString & key);

   // The PluginID must be kept unique.  Since the wxFileConfig class does not preserve
   // case, we use base64 encoding.
   wxString ConvertID(const PluginID & ID);
   wxString b64encode(const void *in, int len);
   int b64decode(wxString in, void *out);

private:
   static PluginManager *mInstance;

   bool IsDirty();
   void SetDirty(bool dirty = true);
   wxFileConfig *mRegistry;
   wxFileConfig *mSettings;

   bool mDirty;
   int mCurrentIndex;

   PluginMap mPlugins;
   PluginMap::iterator mPluginsIter;

   friend class PluginRegistrationDialog;
};

#endif /* __AUDACITY_PLUGINMANAGER_H__ */
