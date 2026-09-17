#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Types/VariantTypeRegistry.h>

W_IMPLEMENT_SINGLETON(WVariantTypeRegistry);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, VariantTypeRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WVariantTypeRegistry);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WVariantTypeRegistry * pDummy = WVariantTypeRegistry::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WVariantTypeRegistry::WVariantTypeRegistry()
  : m_SingletonRegistrar(this)
{
  WPlugin::Events().AddEventHandler(WMakeDelegate(&WVariantTypeRegistry::PluginEventHandler, this));

  UpdateTypes();
}

WVariantTypeRegistry::~WVariantTypeRegistry()
{
  WPlugin::Events().RemoveEventHandler(WMakeDelegate(&WVariantTypeRegistry::PluginEventHandler, this));
}

const WVariantTypeInfo* WVariantTypeRegistry::FindVariantTypeInfo(const WRTTI* pType) const
{
  const WVariantTypeInfo* pTypeInfo = nullptr;
  m_TypeInfos.TryGetValue(pType, pTypeInfo);
  return pTypeInfo;
}

void WVariantTypeRegistry::PluginEventHandler(const WPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case WPluginEvent::AfterLoadingBeforeInit:
    case WPluginEvent::AfterUnloading:
      UpdateTypes();
      break;
    default:
      break;
  }
}

void WVariantTypeRegistry::UpdateTypes()
{
  m_TypeInfos.Clear();
  WVariantTypeInfo* pInstance = WVariantTypeInfo::GetFirstInstance();

  while (pInstance)
  {
    W_ASSERT_DEV(pInstance->GetType()->GetAllocator()->CanAllocate(), "Custom type '{0}' needs to be allocatable.", pInstance->GetType()->GetTypeName());

    m_TypeInfos.Insert(pInstance->GetType(), pInstance);
    pInstance = pInstance->GetNextInstance();
  }
}

//////////////////////////////////////////////////////////////////////////

W_ENUMERABLE_CLASS_IMPLEMENTATION(WVariantTypeInfo);

WVariantTypeInfo::WVariantTypeInfo() = default;


W_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_VariantTypeRegistry);
