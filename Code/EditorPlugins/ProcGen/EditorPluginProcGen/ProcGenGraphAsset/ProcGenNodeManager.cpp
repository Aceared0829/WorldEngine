#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodeManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <Foundation/Configuration/Startup.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGenPin, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginProcGen, ProcGen)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    const WRTTI* pBaseType = WGetStaticRTTI<WProcGenNodeBase>();

    WQtVisualGraphScene::GetPinFactory().RegisterCreator(WGetStaticRTTI<WProcGenPin>(), [](const WRTTI* pRtti)->WQtVisualGraphPin* { return new WQtProcGenPin(); });
    WQtVisualGraphScene::GetNodeFactory().RegisterCreator(pBaseType, [](const WRTTI* pRtti)->WQtVisualGraphNode* { return new WQtProcGenNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const WRTTI* pBaseType = WGetStaticRTTI<WProcGenNodeBase>();

    WQtVisualGraphScene::GetPinFactory().UnregisterCreator(WGetStaticRTTI<WProcGenPin>());
    WQtVisualGraphScene::GetNodeFactory().UnregisterCreator(pBaseType);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

bool WProcGenNodeManager::InternalIsNode(const WDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(WGetStaticRTTI<WProcGenNodeBase>());
}

void WProcGenNodeManager::InternalCreatePins(const WDocumentObject* pObject, NodeInternal& ref_node)
{
  const WRTTI* pNodeBaseType = WGetStaticRTTI<WProcGenNodeBase>();

  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom(pNodeBaseType))
    return;

  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != WPropertyCategory::Member)
      continue;

    const WRTTI* pPropType = pProp->GetSpecificType();
    if (!pPropType->IsDerivedFrom<WProcGenNodePin>())
      continue;

    WColor pinColor = WColorScheme::DarkUI(WColorScheme::Gray);
    if (const WColorAttribute* pAttr = pProp->GetAttributeByType<WColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }

    if (pPropType->IsDerivedFrom<WProcGenNodeInputPin>())
    {
      auto pPin = W_DEFAULT_NEW(WProcGenPin, WVisualGraphPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pPropType->IsDerivedFrom<WProcGenNodeOutputPin>())
    {
      auto pPin = W_DEFAULT_NEW(WProcGenPin, WVisualGraphPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPin);
    }
  }
}

void WProcGenNodeManager::GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  WRTTI::ForEachDerivedType<WProcGenNodeBase>(
    [&](const WRTTI* pRtti)
    { out_types.PushBack(pRtti); },
    WRTTI::ForEachOptions::ExcludeAbstract);
}

WStatus WProcGenNodeManager::InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNto1;
  return WStatus(W_SUCCESS);
}
