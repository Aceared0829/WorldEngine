#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>

WActionDescriptorHandle WVisualShaderActions::s_hVisualShaderCategory;
WActionDescriptorHandle WVisualShaderActions::s_hCleanGraph;

void WVisualShaderActions::RegisterActions()
{
  s_hVisualShaderCategory = W_REGISTER_CATEGORY("VisualShaderCategory");
  s_hCleanGraph = W_REGISTER_ACTION_0("VisualShader.CleanGraph", WActionScope::Document, "Visual Shader", "", WVisualShaderAction);
}

void WVisualShaderActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hVisualShaderCategory);
  WActionManager::UnregisterAction(s_hCleanGraph);
}

void WVisualShaderActions::MapActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  // Use a high sort key to place Visual Shader actions on the far right of the toolbar
  pMap->MapAction(s_hVisualShaderCategory, "", 1000.0f);
  pMap->MapAction(s_hCleanGraph, "VisualShaderCategory", 1.0f);
}


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualShaderAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WVisualShaderAction::WVisualShaderAction(const WActionContext& context, const char* szName)
  : WButtonAction(context, szName, false, "")
{
  SetIconPath(":/EditorPluginAssets/Cleanup.svg");

  // Register to listen for property changes on the document
  m_Context.m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WVisualShaderAction::PropertyEventHandler, this));

  // Initialize visibility based on current shader mode
  WMaterialAssetDocument* pMaterial = static_cast<WMaterialAssetDocument*>(m_Context.m_pDocument);
  const bool bCustom = pMaterial->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<WInt64>() == WMaterialShaderMode::Custom;
  SetVisible(bCustom, false);
}

WVisualShaderAction::~WVisualShaderAction()
{
  m_Context.m_pDocument->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WVisualShaderAction::PropertyEventHandler, this));
}

void WVisualShaderAction::Execute(const WVariant& value)
{
  WMaterialAssetDocument* pMaterial = static_cast<WMaterialAssetDocument*>(m_Context.m_pDocument);
  pMaterial->RemoveDisconnectedNodes();
}

void WVisualShaderAction::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  WMaterialAssetDocument* pMaterial = static_cast<WMaterialAssetDocument*>(m_Context.m_pDocument);

  // Only react to ShaderMode changes on the material property object
  if (e.m_pObject == pMaterial->GetPropertyObject() && e.m_sProperty == "ShaderMode")
  {
    const bool bCustom = e.m_pObject->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<WInt64>() == WMaterialShaderMode::Custom;
    SetVisible(bCustom);
  }
}
