#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/PhantomRtti.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

WCopyOnBroadcastEvent<const WPhantomRttiManagerEvent&> WPhantomRttiManager::s_Events;

WHashTable<WStringView, WPhantomRTTI*> WPhantomRttiManager::s_NameToPhantom;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WPhantomRttiManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WPhantomRttiManager::Shutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// WPhantomRttiManager public functions
////////////////////////////////////////////////////////////////////////

const WRTTI* WPhantomRttiManager::RegisterType(WReflectedTypeDescriptor& ref_desc)
{
  W_PROFILE_SCOPE("RegisterType");
  const WRTTI* pType = WRTTI::FindTypeByName(ref_desc.m_sTypeName);
  WPhantomRTTI* pPhantom = nullptr;
  s_NameToPhantom.TryGetValue(ref_desc.m_sTypeName, pPhantom);

  // concrete type !
  if (pPhantom == nullptr && pType != nullptr)
  {
    return pType;
  }

  if (pPhantom != nullptr && pPhantom->IsEqualToDescriptor(ref_desc))
    return pPhantom;

  if (pPhantom == nullptr)
  {
    pPhantom = W_DEFAULT_NEW(WPhantomRTTI, ref_desc.m_sTypeName.GetData(), WRTTI::FindTypeByName(ref_desc.m_sParentTypeName), 0,
      ref_desc.m_uiTypeVersion, WVariantType::Invalid, ref_desc.m_Flags, ref_desc.m_sPluginName.GetData());

    pPhantom->SetProperties(ref_desc.m_Properties);
    pPhantom->SetAttributes(ref_desc.m_Attributes);
    pPhantom->SetFunctions(ref_desc.m_Functions);
    pPhantom->SetupParentHierarchy();

    s_NameToPhantom[pPhantom->GetTypeName()] = pPhantom;

    WPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = WPhantomRttiManagerEvent::Type::TypeAdded;
    s_Events.Broadcast(msg, 1); /// \todo Had to increase the recursion depth to allow registering phantom types that are based on actual
                                /// types coming from the engine process
  }
  else
  {
    pPhantom->UpdateType(ref_desc);

    WPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = WPhantomRttiManagerEvent::Type::TypeChanged;
    s_Events.Broadcast(msg, 1);
  }

  return pPhantom;
}

bool WPhantomRttiManager::UnregisterType(const WRTTI* pRtti)
{
  WPhantomRTTI* pPhantom = nullptr;
  s_NameToPhantom.TryGetValue(pRtti->GetTypeName(), pPhantom);

  if (pPhantom == nullptr)
    return false;

  {
    WPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = WPhantomRttiManagerEvent::Type::TypeRemoved;
    s_Events.Broadcast(msg);
  }

  s_NameToPhantom.Remove(pPhantom->GetTypeName());

  W_DEFAULT_DELETE(pPhantom);
  return true;
}

////////////////////////////////////////////////////////////////////////
// WPhantomRttiManager private functions
////////////////////////////////////////////////////////////////////////

void WPhantomRttiManager::PluginEventHandler(const WPluginEvent& e)
{
  if (e.m_EventType == WPluginEvent::Type::BeforeUnloading)
  {
    while (!s_NameToPhantom.IsEmpty())
    {
      UnregisterType(s_NameToPhantom.GetIterator().Value());
    }

    W_ASSERT_DEV(s_NameToPhantom.IsEmpty(), "WPhantomRttiManager::Shutdown: Removal of types failed!");
  }
}

void WPhantomRttiManager::Startup()
{
  WPlugin::Events().AddEventHandler(&WPhantomRttiManager::PluginEventHandler);
}


void WPhantomRttiManager::Shutdown()
{
  WPlugin::Events().RemoveEventHandler(&WPhantomRttiManager::PluginEventHandler);

  while (!s_NameToPhantom.IsEmpty())
  {
    UnregisterType(s_NameToPhantom.GetIterator().Value());
  }

  W_ASSERT_DEV(s_NameToPhantom.IsEmpty(), "WPhantomRttiManager::Shutdown: Removal of types failed!");
}
