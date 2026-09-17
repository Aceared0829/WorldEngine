#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

#include <Plugin_Platform.inl>

WResult UnloadPluginModule(WPluginModule& ref_pModule, WStringView sPluginFile);
WResult LoadPluginModule(WStringView sFileToLoad, WPluginModule& ref_pModule, WStringView sPluginFile);

WDynamicArray<WString>& GetStaticPlugins()
{
  static WDynamicArray<WString> s_StaticPlugins;
  return s_StaticPlugins;
}

struct ModuleData
{
  WPluginModule m_hModule = 0;
  WUInt8 m_uiFileNumber = 0;
  bool m_bCalledOnLoad = false;
  WHybridArray<WPluginInitCallback, 2> m_OnLoadCB;
  WHybridArray<WPluginInitCallback, 2> m_OnUnloadCB;
  WHybridArray<WString, 2> m_sPluginDependencies;
  WBitflags<WPluginLoadFlags> m_LoadFlags;

  void Initialize();
  void Uninitialize();
};

static ModuleData g_StaticModule;
static ModuleData* g_pCurrentlyLoadingModule = nullptr;
static WMap<WString, ModuleData> g_LoadedModules;
static WDynamicArray<WString> s_PluginLoadOrder;
static WUInt32 s_uiMaxParallelInstances = 32;
static WInt32 s_iPluginChangeRecursionCounter = 0;

WCopyOnBroadcastEvent<const WPluginEvent&> s_PluginEvents;

void WPlugin::SetMaxParallelInstances(WUInt32 uiMaxParallelInstances)
{
  s_uiMaxParallelInstances = WMath::Max(1u, uiMaxParallelInstances);
}

void WPlugin::InitializeStaticallyLinkedPlugins()
{
  if (!g_StaticModule.m_bCalledOnLoad)
  {
    // We need to trigger the WPlugin events to make sure the sub-systems are initialized at least once.
    WPlugin::BeginPluginChanges();
    W_SCOPE_EXIT(WPlugin::EndPluginChanges());
    g_StaticModule.Initialize();

#if W_DISABLED(W_COMPILE_ENGINE_AS_DLL)
    W_LOG_BLOCK("Initialize Statically Linked Plugins");
    // Merely add dummy entries so plugins can be enumerated etc.
    for (WStringView sPlugin : GetStaticPlugins())
    {
      g_LoadedModules.FindOrAdd(sPlugin);
      WLog::Debug("Plugin '{0}' statically linked.", sPlugin);
    }
#endif
  }
}

void WPlugin::GetAllPluginInfos(WDynamicArray<PluginInfo>& ref_infos)
{
  ref_infos.Clear();

  ref_infos.Reserve(g_LoadedModules.GetCount());

  for (auto mod : g_LoadedModules)
  {
    auto& pi = ref_infos.ExpandAndGetRef();
    pi.m_sName = mod.Key();
    pi.m_sDependencies = mod.Value().m_sPluginDependencies;
    pi.m_LoadFlags = mod.Value().m_LoadFlags;
  }
}

void ModuleData::Initialize()
{
  if (m_bCalledOnLoad)
    return;

  m_bCalledOnLoad = true;

  for (const auto& dep : m_sPluginDependencies)
  {
    // TODO: ignore ??
    WPlugin::LoadPlugin(dep).IgnoreResult();
  }

  for (auto cb : m_OnLoadCB)
  {
    cb();
  }
}

void ModuleData::Uninitialize()
{
  if (!m_bCalledOnLoad)
    return;

  for (WUInt32 i = m_OnUnloadCB.GetCount(); i > 0; --i)
  {
    m_OnUnloadCB[i - 1]();
  }

  m_bCalledOnLoad = false;
}

void WPlugin::BeginPluginChanges()
{
  if (s_iPluginChangeRecursionCounter == 0)
  {
    WPluginEvent e;
    e.m_EventType = WPluginEvent::BeforePluginChanges;
    s_PluginEvents.Broadcast(e);
  }

  ++s_iPluginChangeRecursionCounter;
}

void WPlugin::EndPluginChanges()
{
  --s_iPluginChangeRecursionCounter;

  if (s_iPluginChangeRecursionCounter == 0)
  {
    WPluginEvent e;
    e.m_EventType = WPluginEvent::AfterPluginChanges;
    s_PluginEvents.Broadcast(e);
  }
}

static WResult UnloadPluginInternal(WStringView sPluginFile)
{
  auto thisMod = g_LoadedModules.Find(sPluginFile);

  if (!thisMod.IsValid())
    return W_SUCCESS;

  WLog::Debug("Plugin to unload: \"{0}\"", sPluginFile);

  WPlugin::BeginPluginChanges();
  W_SCOPE_EXIT(WPlugin::EndPluginChanges());

  // Broadcast event: Before unloading plugin
  {
    WPluginEvent e;
    e.m_EventType = WPluginEvent::BeforeUnloading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: Startup Shutdown
  {
    WPluginEvent e;
    e.m_EventType = WPluginEvent::StartupShutdown;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: After Startup Shutdown
  {
    WPluginEvent e;
    e.m_EventType = WPluginEvent::AfterStartupShutdown;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  thisMod.Value().Uninitialize();

  // unload the plugin module
  if (UnloadPluginModule(thisMod.Value().m_hModule, sPluginFile) == W_FAILURE)
  {
    WLog::Error("Unloading plugin module '{}' failed.", sPluginFile);
    return W_FAILURE;
  }

  // delete the plugin copy that we had loaded
  if (WPlugin::PlatformNeedsPluginCopy())
  {
    WStringBuilder sOriginalFile, sCopiedFile;
    WPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, g_LoadedModules[sPluginFile].m_uiFileNumber);

    WOSFile::DeleteFile(sCopiedFile).IgnoreResult();
  }

  // Broadcast event: After unloading plugin
  {
    WPluginEvent e;
    e.m_EventType = WPluginEvent::AfterUnloading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  WLog::Success("Plugin '{0}' is unloaded.", sPluginFile);
  g_LoadedModules.Remove(thisMod);

  return W_SUCCESS;
}

#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)

static WResult LoadPluginInternal(WStringView sPluginFile, WBitflags<WPluginLoadFlags> flags)
{
  WUInt8 uiFileNumber = 0;

  WStringBuilder sOriginalFile, sCopiedFile;
  WPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, uiFileNumber);

  if (!WOSFile::ExistsFile(sOriginalFile))
  {
    WLog::Error("The plugin '{0}' does not exist.", sPluginFile);
    return W_FAILURE;
  }

  if (WPlugin::PlatformNeedsPluginCopy() && flags.IsSet(WPluginLoadFlags::LoadCopy))
  {
    // create a copy of the original plugin file
    const WUInt8 uiMaxParallelInstances = static_cast<WUInt8>(s_uiMaxParallelInstances);
    for (uiFileNumber = 0; uiFileNumber < uiMaxParallelInstances; ++uiFileNumber)
    {
      WPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, uiFileNumber);
      if (WOSFile::CopyFile(sOriginalFile, sCopiedFile) == W_SUCCESS)
        goto success;
    }

    WLog::Error("Could not copy the plugin file '{0}' to '{1}' (and all previous file numbers). Plugin MaxParallelInstances is set to {2}.", sOriginalFile, sCopiedFile, s_uiMaxParallelInstances);

    g_LoadedModules.Remove(sCopiedFile);
    return W_FAILURE;
  }
  else
  {
    sCopiedFile = sOriginalFile;
  }

success:

  auto& thisMod = g_LoadedModules[sPluginFile];
  thisMod.m_uiFileNumber = uiFileNumber;
  thisMod.m_LoadFlags = flags;

  WPlugin::BeginPluginChanges();
  W_SCOPE_EXIT(WPlugin::EndPluginChanges());

  // Broadcast Event: Before loading plugin
  {
    WPluginEvent e;
    e.m_EventType = WPluginEvent::BeforeLoading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  g_pCurrentlyLoadingModule = &thisMod;

  if (LoadPluginModule(sCopiedFile, g_pCurrentlyLoadingModule->m_hModule, sPluginFile) == W_FAILURE)
  {
    // loaded, but failed
    g_pCurrentlyLoadingModule = nullptr;
    thisMod.m_hModule = 0;

    return W_FAILURE;
  }

  g_pCurrentlyLoadingModule = nullptr;

  {
    // Broadcast Event: After loading plugin, before init
    {
      WPluginEvent e;
      e.m_EventType = WPluginEvent::AfterLoadingBeforeInit;
      e.m_sPluginBinary = sPluginFile;
      s_PluginEvents.Broadcast(e);
    }

    thisMod.Initialize();

    // Broadcast Event: After loading plugin
    {
      WPluginEvent e;
      e.m_EventType = WPluginEvent::AfterLoading;
      e.m_sPluginBinary = sPluginFile;
      s_PluginEvents.Broadcast(e);
    }
  }

  WLog::Success("Plugin '{0}' is loaded.", sPluginFile);
  return W_SUCCESS;
}

#endif

bool WPlugin::ExistsPluginFile(WStringView sPluginFile)
{
  WStringBuilder sOriginalFile, sCopiedFile;
  GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, 0);

  return WOSFile::ExistsFile(sOriginalFile);
}

WResult WPlugin::LoadPlugin(WStringView sPluginFile, WBitflags<WPluginLoadFlags> flags /*= WPluginLoadFlags::Default*/)
{
  W_LOG_BLOCK("Loading Plugin", sPluginFile);

  // make sure this is done first
  InitializeStaticallyLinkedPlugins();

  if (g_LoadedModules.Find(sPluginFile).IsValid())
  {
    WLog::Debug("Plugin '{0}' already loaded.", sPluginFile);
    return W_SUCCESS;
  }

#if W_DISABLED(W_COMPILE_ENGINE_AS_DLL)
  // #TODO W_COMPILE_ENGINE_AS_DLL and being able to load plugins are not necessarily the same thing.
  W_IGNORE_UNUSED(flags);
  return W_FAILURE;
#else

  if (flags.IsSet(WPluginLoadFlags::PluginIsOptional))
  {
    // early out without logging an error
    if (!ExistsPluginFile(sPluginFile))
      return W_FAILURE;
  }

  WLog::Debug("Plugin to load: \"{0}\"", sPluginFile);

  // make sure to use a static string pointer from now on, that stays where it is
  sPluginFile = g_LoadedModules.FindOrAdd(sPluginFile).Key();

  WResult res = LoadPluginInternal(sPluginFile, flags);

  if (res.Succeeded())
  {
    s_PluginLoadOrder.PushBack(sPluginFile);
  }
  else
  {
    // If we failed to load the plugin, it shouldn't be in the loaded modules list
    g_LoadedModules.Remove(sPluginFile);
  }

  return res;
#endif
}

void WPlugin::UnloadAllPlugins()
{
  BeginPluginChanges();
  W_SCOPE_EXIT(EndPluginChanges());

  for (WUInt32 i = s_PluginLoadOrder.GetCount(); i > 0; --i)
  {
    if (UnloadPluginInternal(s_PluginLoadOrder[i - 1]).Failed())
    {
      // not sure what to do
    }
  }

  W_ASSERT_DEBUG(g_LoadedModules.IsEmpty(), "Not all plugins were unloaded somehow.");

  for (auto mod : g_LoadedModules)
  {
    mod.Value().Uninitialize();
  }

  // also shut down all plugin objects that are statically linked
  g_StaticModule.Uninitialize();

  s_PluginLoadOrder.Clear();
  g_LoadedModules.Clear();
}

const WCopyOnBroadcastEvent<const WPluginEvent&>& WPlugin::Events()
{
  return s_PluginEvents;
}

WPlugin::Init::Init(WPluginInitCallback onLoadOrUnloadCB, bool bOnLoad)
{
  ModuleData* pMD = g_pCurrentlyLoadingModule ? g_pCurrentlyLoadingModule : &g_StaticModule;

  if (bOnLoad)
    pMD->m_OnLoadCB.PushBack(onLoadOrUnloadCB);
  else
    pMD->m_OnUnloadCB.PushBack(onLoadOrUnloadCB);
}

WPlugin::Init::Init(const char* szAddPluginDependency)
{
  ModuleData* pMD = g_pCurrentlyLoadingModule ? g_pCurrentlyLoadingModule : &g_StaticModule;

  pMD->m_sPluginDependencies.PushBack(szAddPluginDependency);
}

#if W_DISABLED(W_COMPILE_ENGINE_AS_DLL)
WPluginRegister::WPluginRegister(const char* szAddPlugin)
{
  if (g_pCurrentlyLoadingModule == nullptr)
  {
    GetStaticPlugins().PushBack(szAddPlugin);
  }
}
#endif
