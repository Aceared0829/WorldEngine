#include <Core/CorePCH.h>

#include <Core/World/World.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WWorldModule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WWorldModule::WWorldModule(WWorld* pWorld)
  : m_pWorld(pWorld)
{
}

WWorldModule::~WWorldModule() = default;

WUInt32 WWorldModule::GetWorldIndex() const
{
  return GetWorld()->GetIndex();
}

// protected methods

void WWorldModule::RegisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->RegisterUpdateFunction(desc);
}

void WWorldModule::DeregisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->DeregisterUpdateFunction(desc);
}

WAllocator* WWorldModule::GetAllocator()
{
  return m_pWorld->GetAllocator();
}

WInternal::WorldLargeBlockAllocator* WWorldModule::GetBlockAllocator()
{
  return m_pWorld->GetBlockAllocator();
}

bool WWorldModule::GetWorldSimulationEnabled() const
{
  return m_pWorld->GetWorldSimulationEnabled();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Core, WorldModuleFactory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WPlugin::Events().AddEventHandler(WWorldModuleFactory::PluginEventHandler);
    WWorldModuleFactory::GetInstance()->FillBaseTypeIds();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WPlugin::Events().RemoveEventHandler(WWorldModuleFactory::PluginEventHandler);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

static WWorldModuleTypeId s_uiNextTypeId = 0;
static WDynamicArray<WWorldModuleTypeId> s_freeTypeIds;
static constexpr WWorldModuleTypeId s_InvalidWorldModuleTypeId = WWorldModuleTypeId(-1);

WWorldModuleFactory::WWorldModuleFactory() = default;

// static
WWorldModuleFactory* WWorldModuleFactory::GetInstance()
{
  static WWorldModuleFactory* pInstance = new WWorldModuleFactory();
  return pInstance;
}

WWorldModuleTypeId WWorldModuleFactory::GetTypeId(const WRTTI* pRtti)
{
  WWorldModuleTypeId uiTypeId = s_InvalidWorldModuleTypeId;
  m_TypeToId.TryGetValue(pRtti, uiTypeId);
  return uiTypeId;
}

WWorldModule* WWorldModuleFactory::CreateWorldModule(WWorldModuleTypeId typeId, WWorld* pWorld)
{
  if (typeId < m_CreatorFuncs.GetCount())
  {
    CreatorFunc func = m_CreatorFuncs[typeId].m_Func;
    return (*func)(pWorld->GetAllocator(), pWorld);
  }

  return nullptr;
}

void WWorldModuleFactory::RegisterInterfaceImplementation(WStringView sInterfaceName, WStringView sImplementationName)
{
  m_InterfaceImplementations.Insert(sInterfaceName, sImplementationName);

  WStringBuilder sTemp = sInterfaceName;
  const WRTTI* pInterfaceRtti = WRTTI::FindTypeByName(sTemp);

  sTemp = sImplementationName;
  const WRTTI* pImplementationRtti = WRTTI::FindTypeByName(sTemp);

  if (pInterfaceRtti != nullptr && pImplementationRtti != nullptr)
  {
    m_TypeToId[pInterfaceRtti] = m_TypeToId[pImplementationRtti];
    return;
  }

  // Clear existing mapping if it maps to the wrong type
  WUInt16 uiTypeId;
  if (pInterfaceRtti != nullptr && m_TypeToId.TryGetValue(pInterfaceRtti, uiTypeId))
  {
    if (m_CreatorFuncs[uiTypeId].m_pRtti->GetTypeName() != sImplementationName)
    {
      W_ASSERT_DEV(pImplementationRtti == nullptr, "Implementation error");
      m_TypeToId.Remove(pInterfaceRtti);
    }
  }
}
WWorldModuleTypeId WWorldModuleFactory::RegisterWorldModule(const WRTTI* pRtti, CreatorFunc creatorFunc)
{
  W_ASSERT_DEV(pRtti != WGetStaticRTTI<WWorldModule>(), "Trying to register a world module that is not reflected!");
  W_ASSERT_DEV(
    m_TypeToId.GetCount() < WWorld::GetMaxNumWorldModules(), "Max number of world modules reached: {}", WWorld::GetMaxNumWorldModules());

  WWorldModuleTypeId uiTypeId = s_InvalidWorldModuleTypeId;
  if (m_TypeToId.TryGetValue(pRtti, uiTypeId))
  {
    return uiTypeId;
  }

  if (s_freeTypeIds.IsEmpty())
  {
    W_ASSERT_DEV(s_uiNextTypeId < W_MAX_WORLD_MODULE_TYPES - 1, "World module id overflow!");

    uiTypeId = s_uiNextTypeId++;
  }
  else
  {
    uiTypeId = s_freeTypeIds.PeekBack();
    s_freeTypeIds.PopBack();
  }

  m_TypeToId.Insert(pRtti, uiTypeId);

  m_CreatorFuncs.EnsureCount(uiTypeId + 1);

  auto& creatorFuncContext = m_CreatorFuncs[uiTypeId];
  creatorFuncContext.m_Func = creatorFunc;
  creatorFuncContext.m_pRtti = pRtti;

  return uiTypeId;
}

// static
void WWorldModuleFactory::PluginEventHandler(const WPluginEvent& EventData)
{
  if (EventData.m_EventType == WPluginEvent::AfterLoadingBeforeInit)
  {
    WWorldModuleFactory::GetInstance()->FillBaseTypeIds();
  }

  if (EventData.m_EventType == WPluginEvent::AfterUnloading)
  {
    WWorldModuleFactory::GetInstance()->ClearUnloadedTypeToIDs();
  }
}

namespace
{
  struct NewEntry
  {
    W_DECLARE_POD_TYPE();

    const WRTTI* m_pRtti;
    WWorldModuleTypeId m_uiTypeId;
  };
} // namespace

void WWorldModuleFactory::AdjustBaseTypeId(const WRTTI* pParentRtti, const WRTTI* pRtti, WUInt16 uiParentTypeId)
{
  WDynamicArray<WPlugin::PluginInfo> infos;
  WPlugin::GetAllPluginInfos(infos);

  auto HasManualDependency = [&](WStringView sPluginName) -> bool
  {
    for (const auto& p : infos)
    {
      if (p.m_sName == sPluginName)
      {
        return !p.m_LoadFlags.IsSet(WPluginLoadFlags::CustomDependency);
      }
    }

    return false;
  };

  WStringView szPlugin1 = m_CreatorFuncs[uiParentTypeId].m_pRtti->GetPluginName();
  WStringView szPlugin2 = pRtti->GetPluginName();

  const bool bPrio1 = HasManualDependency(szPlugin1);
  const bool bPrio2 = HasManualDependency(szPlugin2);

  if (bPrio1 && !bPrio2)
  {
    // keep the previous one
    return;
  }

  if (!bPrio1 && bPrio2)
  {
    // take the new one
    m_TypeToId[pParentRtti] = m_TypeToId[pRtti];
    return;
  }

  WLog::Error("Interface '{}' is already implemented by '{}'. Specify which implementation should be used via RegisterInterfaceImplementation() or WorldModules.ddl config file.", pParentRtti->GetTypeName(), m_CreatorFuncs[uiParentTypeId].m_pRtti->GetTypeName());
}

void WWorldModuleFactory::FillBaseTypeIds()
{
  // m_TypeToId contains RTTI types for WWorldModules and WComponents
  // m_TypeToId[WComponent] maps to TypeID for its respective WComponentManager
  // m_TypeToId[WWorldModule] maps to TypeID for itself OR in case of an interface to the derived type that implements the interface
  // after types are registered we only have a mapping for m_TypeToId[WWorldModule(impl)] and now we want to add
  // the mapping for m_TypeToId[WWorldModule(interface)], such that querying the TypeID for the interface works as well
  // and yields the implementation

  WTempHybridArray<NewEntry, 64> newEntries;
  const WRTTI* pModuleRtti = WGetStaticRTTI<WWorldModule>(); // base type where we want to stop iterating upwards

  // explicit mappings
  for (auto it = m_InterfaceImplementations.GetIterator(); it.IsValid(); ++it)
  {
    const WRTTI* pInterfaceRtti = WRTTI::FindTypeByName(it.Key());
    const WRTTI* pImplementationRtti = WRTTI::FindTypeByName(it.Value());

    if (pInterfaceRtti != nullptr && pImplementationRtti != nullptr)
    {
      m_TypeToId[pInterfaceRtti] = m_TypeToId[pImplementationRtti];
    }
  }

  // automatic mappings
  for (auto it = m_TypeToId.GetIterator(); it.IsValid(); ++it)
  {
    const WRTTI* pRtti = it.Key();

    // ignore components, we only want to fill out mappings for the base types of world modules
    if (!pRtti->IsDerivedFrom<WWorldModule>())
      continue;

    const WWorldModuleTypeId uiTypeId = it.Value();

    for (const WRTTI* pParentRtti = pRtti->GetParentType(); pParentRtti != pModuleRtti; pParentRtti = pParentRtti->GetParentType())
    {
      // we are only interested in parent types that are pure interfaces
      if (!pParentRtti->GetTypeFlags().IsSet(WTypeFlags::Abstract))
        continue;

      // skip if we have an explicit mapping for this interface, they are already handled above
      if (m_InterfaceImplementations.GetValue(pParentRtti->GetTypeName()) != nullptr)
        continue;


      if (WUInt16* pParentTypeId = m_TypeToId.GetValue(pParentRtti))
      {
        if (*pParentTypeId != uiTypeId)
        {
          AdjustBaseTypeId(pParentRtti, pRtti, *pParentTypeId);
        }
      }
      else
      {
        auto& newEntry = newEntries.ExpandAndGetRef();
        newEntry.m_pRtti = pParentRtti;
        newEntry.m_uiTypeId = uiTypeId;
      }
    }
  }

  // delayed insertion to not interfere with the iteration above
  for (auto& newEntry : newEntries)
  {
    m_TypeToId.Insert(newEntry.m_pRtti, newEntry.m_uiTypeId);
  }
}

void WWorldModuleFactory::ClearUnloadedTypeToIDs()
{
  WSet<const WRTTI*> allRttis;
  WRTTI::ForEachType([&](const WRTTI* pRtti)
    { allRttis.Insert(pRtti); });

  WSet<WWorldModuleTypeId> mappedIdsToRemove;

  for (auto it = m_TypeToId.GetIterator(); it.IsValid();)
  {
    const WRTTI* pRtti = it.Key();
    const WWorldModuleTypeId uiTypeId = it.Value();

    if (!allRttis.Contains(pRtti))
    {
      // type got removed, clear it from the map
      it = m_TypeToId.Remove(it);

      // and record that all other types that map to the same typeId also must be removed
      mappedIdsToRemove.Insert(uiTypeId);
    }
    else
    {
      ++it;
    }
  }

  // now remove all mappings that map to an invalid typeId
  // this can be more than one, since we can map multiple (interface) types to the same implementation
  for (auto it = m_TypeToId.GetIterator(); it.IsValid();)
  {
    const WWorldModuleTypeId uiTypeId = it.Value();

    if (mappedIdsToRemove.Contains(uiTypeId))
    {
      it = m_TypeToId.Remove(it);
    }
    else
    {
      ++it;
    }
  }

  // Finally, adding all invalid typeIds to the free list for reusing later
  for (WWorldModuleTypeId removedId : mappedIdsToRemove)
  {
    s_freeTypeIds.PushBack(removedId);
  }
}

W_STATICLINK_FILE(Core, Core_World_Implementation_WorldModule);
