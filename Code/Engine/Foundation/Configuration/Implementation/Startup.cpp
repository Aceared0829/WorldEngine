#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>

W_ENUMERABLE_CLASS_IMPLEMENTATION(WSubSystem);

bool WStartup::s_bPrintAllSubSystems = true;
WStartupStage::Enum WStartup::s_CurrentState = WStartupStage::None;
WDynamicArray<const char*> WStartup::s_ApplicationTags;


void WStartup::AddApplicationTag(const char* szTag)
{
  s_ApplicationTags.PushBack(szTag);
}

bool WStartup::HasApplicationTag(const char* szTag)
{
  for (WUInt32 i = 0; i < s_ApplicationTags.GetCount(); ++i)
  {
    if (WStringUtils::IsEqual_NoCase(s_ApplicationTags[i], szTag))
      return true;
  }

  return false;
}

void WStartup::PrintAllSubsystems()
{
  W_LOG_BLOCK("Available Subsystems");

  WSubSystem* pSub = WSubSystem::GetFirstInstance();

  while (pSub)
  {
    WLog::Debug("Subsystem: '{0}::{1}'", pSub->GetGroupName(), pSub->GetSubSystemName());

    if (pSub->GetDependency(0) == nullptr)
      WLog::Debug("  <no dependencies>");
    else
    {
      for (WInt32 i = 0; pSub->GetDependency(i) != nullptr; ++i)
        WLog::Debug("  depends on '{0}'", pSub->GetDependency(i));
    }

    WLog::Debug("");

    pSub = pSub->GetNextInstance();
  }
}

void WStartup::AssignSubSystemPlugin(WStringView sPluginName)
{
  // iterates over all existing subsystems and finds those that have no plugin name yet
  // assigns the given name to them

  WSubSystem* pSub = WSubSystem::GetFirstInstance();

  while (pSub)
  {
    if (pSub->m_sPluginName.IsEmpty())
    {
      pSub->m_sPluginName = sPluginName;
    }

    pSub = pSub->GetNextInstance();
  }
}

void WStartup::PluginEventHandler(const WPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case WPluginEvent::BeforeLoading:
    {
      AssignSubSystemPlugin("Static");
    }
    break;

    case WPluginEvent::AfterLoadingBeforeInit:
    {
      AssignSubSystemPlugin(EventData.m_sPluginBinary);
    }
    break;

    case WPluginEvent::StartupShutdown:
    {
      WStartup::UnloadPluginSubSystems(EventData.m_sPluginBinary);
    }
    break;

    case WPluginEvent::AfterPluginChanges:
    {
      WStartup::ReinitToCurrentState();
    }
    break;

    default:
      break;
  }
}

static bool IsGroupName(WStringView sName)
{
  WSubSystem* pSub = WSubSystem::GetFirstInstance();

  bool bGroup = false;
  bool bSubSystem = false;

  while (pSub)
  {
    if (pSub->GetGroupName() == sName)
      bGroup = true;

    if (pSub->GetSubSystemName() == sName)
      bSubSystem = true;

    pSub = pSub->GetNextInstance();
  }

  W_ASSERT_ALWAYS(!bGroup || !bSubSystem, "There cannot be a SubSystem AND a Group called '{0}'.", sName);

  return bGroup;
}

static WStringView GetGroupSubSystems(WStringView sGroup, WInt32 iSubSystem)
{
  WSubSystem* pSub = WSubSystem::GetFirstInstance();

  while (pSub)
  {
    if (pSub->GetGroupName() == sGroup)
    {
      if (iSubSystem == 0)
        return pSub->GetSubSystemName();

      --iSubSystem;
    }

    pSub = pSub->GetNextInstance();
  }

  return nullptr;
}

void WStartup::ComputeOrder(WDeque<WSubSystem*>& Order)
{
  Order.Clear();
  WSet<WString> sSystemsInited;

  bool bCouldInitAny = true;

  while (bCouldInitAny)
  {
    bCouldInitAny = false;

    WSubSystem* pSub = WSubSystem::GetFirstInstance();

    while (pSub)
    {
      if (!sSystemsInited.Find(pSub->GetSubSystemName()).IsValid())
      {
        bool bAllDependsFulfilled = true;
        WInt32 iDep = 0;

        while (pSub->GetDependency(iDep) != nullptr)
        {
          if (IsGroupName(pSub->GetDependency(iDep)))
          {
            WInt32 iSubSystemIndex = 0;
            WStringView sNextSubSystem = GetGroupSubSystems(pSub->GetDependency(iDep), iSubSystemIndex);
            while (sNextSubSystem.IsValid())
            {
              if (!sSystemsInited.Find(sNextSubSystem).IsValid())
              {
                bAllDependsFulfilled = false;
                break;
              }

              ++iSubSystemIndex;
              sNextSubSystem = GetGroupSubSystems(pSub->GetDependency(iDep), iSubSystemIndex);
            }
          }
          else
          {
            if (!sSystemsInited.Find(pSub->GetDependency(iDep)).IsValid())
            {
              bAllDependsFulfilled = false;
              break;
            }
          }

          ++iDep;
        }

        if (bAllDependsFulfilled)
        {
          bCouldInitAny = true;
          Order.PushBack(pSub);
          sSystemsInited.Insert(pSub->GetSubSystemName());
        }
      }

      pSub = pSub->GetNextInstance();
    }
  }
}

void WStartup::Startup(WStartupStage::Enum stage)
{
  if (stage == WStartupStage::BaseSystems)
  {
    WFoundation::Initialize();
  }

  const char* szStartup[] = {"Startup Base", "Startup Core", "Startup Engine"};

  if (stage == WStartupStage::CoreSystems)
  {
    Startup(WStartupStage::BaseSystems);

    WGlobalEvent::Broadcast(W_GLOBALEVENT_STARTUP_CORESYSTEMS_BEGIN);

    if (s_bPrintAllSubSystems)
    {
      s_bPrintAllSubSystems = false;
      PrintAllSubsystems();
    }
  }

  if (stage == WStartupStage::HighLevelSystems)
  {
    Startup(WStartupStage::CoreSystems);

    WGlobalEvent::Broadcast(W_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_BEGIN);
  }

  W_LOG_BLOCK(szStartup[stage]);

  WDeque<WSubSystem*> Order;
  ComputeOrder(Order);

  for (WUInt32 i = 0; i < Order.GetCount(); ++i)
  {
    if (!Order[i]->m_bStartupDone[stage])
    {
      Order[i]->m_bStartupDone[stage] = true;

      switch (stage)
      {
        case WStartupStage::BaseSystems:
          WLog::Debug("Executing 'Base' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnBaseSystemsStartup();
          break;
        case WStartupStage::CoreSystems:
          WLog::Debug("Executing 'Core' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnCoreSystemsStartup();
          break;
        case WStartupStage::HighLevelSystems:
          WLog::Debug("Executing 'Engine' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnHighLevelSystemsStartup();
          break;

        default:
          break;
      }
    }
  }

  // now everything should be started
  {
    W_LOG_BLOCK("Failed SubSystems");

    WSet<WString> sSystemsFound;

    WSubSystem* pSub = WSubSystem::GetFirstInstance();

    while (pSub)
    {
      sSystemsFound.Insert(pSub->GetSubSystemName());
      pSub = pSub->GetNextInstance();
    }

    pSub = WSubSystem::GetFirstInstance();

    while (pSub)
    {
      if (!pSub->m_bStartupDone[stage])
      {
        WInt32 iDep = 0;

        while (pSub->GetDependency(iDep) != nullptr)
        {
          if (!sSystemsFound.Find(pSub->GetDependency(iDep)).IsValid())
          {
            WLog::Error("SubSystem '{0}::{1}' could not be started because dependency '{2}' is unknown.", pSub->GetGroupName(),
              pSub->GetSubSystemName(), pSub->GetDependency(iDep));
          }
          else
          {
            WLog::Error("SubSystem '{0}::{1}' could not be started because dependency '{2}' has not been initialized.", pSub->GetGroupName(),
              pSub->GetSubSystemName(), pSub->GetDependency(iDep));
          }

          ++iDep;
        }
      }

      pSub = pSub->GetNextInstance();
    }
  }

  switch (stage)
  {
    case WStartupStage::BaseSystems:
      break;
    case WStartupStage::CoreSystems:
      WGlobalEvent::Broadcast(W_GLOBALEVENT_STARTUP_CORESYSTEMS_END);
      break;
    case WStartupStage::HighLevelSystems:
      WGlobalEvent::Broadcast(W_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_END);
      break;

    default:
      break;
  }

  if (s_CurrentState == WStartupStage::None)
  {
    WPlugin::Events().AddEventHandler(PluginEventHandler);
  }

  s_CurrentState = stage;
}

void WStartup::Shutdown(WStartupStage::Enum stage)
{
  // without that we cannot function, so make sure it is up and running
  WFoundation::Initialize();

  {
    const char* szStartup[] = {"Shutdown Base", "Shutdown Core", "Shutdown Engine"};

    if (stage == WStartupStage::BaseSystems)
    {
      Shutdown(WStartupStage::CoreSystems);
    }

    if (stage == WStartupStage::CoreSystems)
    {
      Shutdown(WStartupStage::HighLevelSystems);
      s_bPrintAllSubSystems = true;

      WGlobalEvent::Broadcast(W_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_BEGIN);
    }

    if (stage == WStartupStage::HighLevelSystems)
    {
      WGlobalEvent::Broadcast(W_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_BEGIN);
    }

    W_LOG_BLOCK(szStartup[stage]);

    WDeque<WSubSystem*> Order;
    ComputeOrder(Order);

    for (WInt32 i = (WInt32)Order.GetCount() - 1; i >= 0; --i)
    {
      if (Order[i]->m_bStartupDone[stage])
      {
        switch (stage)
        {
          case WStartupStage::CoreSystems:
            WLog::Debug("Executing 'Core' shutdown of sub-system '{0}::{1}'", Order[i]->GetGroupName(), Order[i]->GetSubSystemName());
            Order[i]->OnCoreSystemsShutdown();
            break;

          case WStartupStage::HighLevelSystems:
            WLog::Debug("Executing 'Engine' shutdown of sub-system '{0}::{1}'", Order[i]->GetGroupName(), Order[i]->GetSubSystemName());
            Order[i]->OnHighLevelSystemsShutdown();
            break;

          default:
            break;
        }

        Order[i]->m_bStartupDone[stage] = false;
      }
    }
  }

  switch (stage)
  {
    case WStartupStage::CoreSystems:
      WGlobalEvent::Broadcast(W_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_END);
      break;

    case WStartupStage::HighLevelSystems:
      WGlobalEvent::Broadcast(W_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_END);
      break;

    default:
      break;
  }

  if (s_CurrentState != WStartupStage::None)
  {
    s_CurrentState = (WStartupStage::Enum)(((WInt32)stage) - 1);

    if (s_CurrentState == WStartupStage::None)
    {
      WPlugin::Events().RemoveEventHandler(PluginEventHandler);
    }
  }
}

bool WStartup::HasDependencyOnPlugin(WSubSystem* pSubSystem, WStringView sModule)
{
  if (pSubSystem->m_sPluginName == sModule)
    return true;

  for (WUInt32 i = 0; pSubSystem->GetDependency(i) != nullptr; ++i)
  {
    WSubSystem* pSub = WSubSystem::GetFirstInstance();
    while (pSub)
    {
      if (pSub->GetSubSystemName() == pSubSystem->GetDependency(i))
      {
        if (HasDependencyOnPlugin(pSub, sModule))
          return true;

        break;
      }

      pSub = pSub->GetNextInstance();
    }
  }

  return false;
}

void WStartup::UnloadPluginSubSystems(WStringView sPluginName)
{
  W_LOG_BLOCK("Unloading Plugin SubSystems", sPluginName);
  WLog::Dev("Plugin to unload: '{0}'", sPluginName);

  WGlobalEvent::Broadcast(W_GLOBALEVENT_UNLOAD_PLUGIN_BEGIN, WVariant(sPluginName));

  WDeque<WSubSystem*> Order;
  ComputeOrder(Order);

  for (WInt32 i = (WInt32)Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[WStartupStage::HighLevelSystems] && HasDependencyOnPlugin(Order[i], sPluginName))
    {
      WLog::Info("Engine shutdown of SubSystem '{0}::{1}', because it depends on Plugin '{2}'.", Order[i]->GetGroupName(), Order[i]->GetSubSystemName(), sPluginName);
      Order[i]->OnHighLevelSystemsShutdown();
      Order[i]->m_bStartupDone[WStartupStage::HighLevelSystems] = false;
    }
  }

  for (WInt32 i = (WInt32)Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[WStartupStage::CoreSystems] && HasDependencyOnPlugin(Order[i], sPluginName))
    {
      WLog::Info("Core shutdown of SubSystem '{0}::{1}', because it depends on Plugin '{2}'.", Order[i]->GetGroupName(), Order[i]->GetSubSystemName(), sPluginName);
      Order[i]->OnCoreSystemsShutdown();
      Order[i]->m_bStartupDone[WStartupStage::CoreSystems] = false;
    }
  }


  WGlobalEvent::Broadcast(W_GLOBALEVENT_UNLOAD_PLUGIN_END, WVariant(sPluginName));
}

void WStartup::ReinitToCurrentState()
{
  if (s_CurrentState != WStartupStage::None)
    Startup(s_CurrentState);
}
