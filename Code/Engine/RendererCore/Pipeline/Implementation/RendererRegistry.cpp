#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RendererRegistry.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RendererRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WRendererRegistry::UpdateRendererTypes();

    WPlugin::Events().AddEventHandler(WRendererRegistry::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WPlugin::Events().RemoveEventHandler(WRendererRegistry::PluginEventHandler);

    WRendererRegistry::ClearRendererInstances();
  }

W_END_SUBSYSTEM_DECLARATION;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderer, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WHybridArray<const WRTTI*, 16> WRendererRegistry::s_RendererTypes;
WDynamicArray<WUniquePtr<WRenderer>> WRendererRegistry::s_RendererInstances;
WHashTable<const WRTTI*, WUInt32> WRendererRegistry::s_RenderDataTypeToRendererIndex;
bool WRendererRegistry::s_bRendererInstancesDirty = false;

// static
void WRendererRegistry::PluginEventHandler(const WPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case WPluginEvent::AfterPluginChanges:
      UpdateRendererTypes();
      break;

    default:
      break;
  }
}

// static
void WRendererRegistry::UpdateRendererTypes()
{
  s_RendererTypes.Clear();

  WRTTI::ForEachDerivedType<WRenderer>([](const WRTTI* pRtti)
    { s_RendererTypes.PushBack(pRtti); },
    WRTTI::ForEachOptions::ExcludeNonAllocatable);

  s_bRendererInstancesDirty = true;
}

// static
void WRendererRegistry::CreateRendererInstances()
{
  if (!s_bRendererInstancesDirty)
    return;

  ClearRendererInstances();

  for (auto pRendererType : s_RendererTypes)
  {
    W_ASSERT_DEV(pRendererType->IsDerivedFrom(WGetStaticRTTI<WRenderer>()), "Renderer type '{}' must be derived from WRenderer", pRendererType->GetTypeName());

    auto pRenderer = pRendererType->GetAllocator()->Allocate<WRenderer>();

    WUInt32 uiIndex = s_RendererInstances.GetCount();
    s_RendererInstances.PushBack(pRenderer);

    WTempHybridArray<const WRTTI*, 8> supportedTypes;
    pRenderer->GetSupportedRenderDataTypes(supportedTypes);

    for (auto pType : supportedTypes)
    {
      s_RenderDataTypeToRendererIndex.Insert(pType, uiIndex);
    }
  }

  s_bRendererInstancesDirty = false;
}

// static
void WRendererRegistry::ClearRendererInstances()
{
  s_RendererInstances.Clear();
  s_RenderDataTypeToRendererIndex.Clear();
}


W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RendererRegistry);
