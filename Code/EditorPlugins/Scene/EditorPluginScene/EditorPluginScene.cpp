#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Actions/MeshPrefabActions.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/McpTools/MeshLodTool.h>
#include <EditorPluginScene/McpTools/MeshPrefabTool.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/Scene2DocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Visualizers/BoxReflectionProbeVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/DirectionalLightVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <GameEngine/Physics/ImpulseType.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <Mcp/McpToolRegistry.h>
#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Utils/CoreRenderProfile.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

static void ToolsProjectEventHandler(const WToolsProjectEvent& e);

void OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WScene2Document>())
      {
        new WQtScene2DocumentWindow(static_cast<WScene2Document*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
      else if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WSceneDocument>())
      {
        new WQtSceneDocumentWindow(static_cast<WSceneDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ToolsProjectEventHandler(const WToolsProjectEvent& e)
{
  if (e.m_Type == WToolsProjectEvent::Type::ProjectFirstSetup)
  {
    auto project = WToolsProject::GetSingleton();

    project->CreateSubFolder("Scenes");
    project->CreateSubFolder("Prefabs");

    for (auto& dm : WAssetDocumentManager::GetAllDocumentManagers())
    {
      if (dm->IsInstanceOf<WSceneDocumentManager>())
      {
        WDocument* doc;

        WStringBuilder path(project->GetProjectDirectory(), "/Scenes/Main.WScene");
        dm->CreateDocument("Scene", path, doc).IgnoreResult();
      }
    }
  }
}

void AssetCuratorEventHandler(const WAssetCuratorEvent& e)
{
  if (e.m_Type == WAssetCuratorEvent::Type::ActivePlatformChanged)
  {
    WSet<WString> allCamPipes;

    auto& dynEnum = WDynamicStringEnum::CreateDynamicEnum("CameraPipelines");

    for (WUInt32 profileIdx = 0; profileIdx < WAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++profileIdx)
    {
      const WPlatformProfile* pProfile = WAssetCurator::GetSingleton()->GetAssetProfile(profileIdx);

      const WRenderPipelineProfileConfig* pConfig = pProfile->GetTypeConfig<WRenderPipelineProfileConfig>();

      for (auto it = pConfig->m_CameraPipelines.GetIterator(); it.IsValid(); ++it)
      {
        dynEnum.AddValidValue(it.Key(), true);
      }
    }
  }
}

void WCameraComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WSkyLightComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WGreyBoxComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WLightComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WSceneDocument_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WAreaDamageComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WProjectileSurfaceInteraction_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WOccluderComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WSplineNodeComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WLensFlareComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

QImage SliderImageGenerator_LightTemperature(WUInt32 uiWidth, WUInt32 uiHeight, double fMinValue, double fMaxValue)
{
  // can use a 1D image, height doesn't need to be all used
  QImage image = QImage(uiWidth, 1, QImage::Format::Format_RGB32);

  for (WUInt32 x = 0; x < uiWidth; ++x)
  {
    const double pos = (double)x / (uiWidth - 1.0);
    WColor c = WColor::MakeFromKelvin(static_cast<WUInt32>((pos * (fMaxValue - fMinValue)) + fMinValue));

    WColorGammaUB cg = c;
    image.setPixel(x, 0, qRgb(cg.r, cg.g, cg.b));
  }

  return image;
}

void OnLoadPlugin()
{
  WToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WSceneDocument_PropertyMetaStateEventHandler);

  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(OnDocumentManagerEvent));

  WAssetCurator::GetSingleton()->m_Events.AddEventHandler(AssetCuratorEventHandler);

  // Add built in tags
  {
    WToolsTagRegistry::AddTag(WToolsTag("Default", "Exclude From Export", true));
    WToolsTagRegistry::AddTag(WToolsTag("Default", "CastShadow", true));
    WToolsTagRegistry::AddTag(WToolsTag("Default", "SkyLight", true));
  }

  WSelectionActions::RegisterActions();
  WSceneGizmoActions::RegisterActions();
  WSceneActions::RegisterActions();
  WLayerActions::RegisterActions();
  WMeshPrefabActions::RegisterActions();

  // Lives here rather than with the mesh asset, because what it knows about is prefabs, not meshes.
  WMeshPrefabActions::MapActions("AssetBrowserContextMenu", WAssetBrowserContextMenu::s_sAssetMenu).IgnoreResult();
  WMeshPrefabActions::MapActions("MeshAssetMenuBar", "G.Asset", true).IgnoreResult();
  WMeshPrefabActions::MapActions("AnimatedMeshAssetMenuBar", "G.Asset", true).IgnoreResult();

  // misc
  WQtImageSliderWidget::s_ImageGenerators["LightTemperature"] = SliderImageGenerator_LightTemperature;

  // Menu Bar
  const char* MenuBars[] = {"EditorPluginScene_DocumentMenuBar", "EditorPluginScene_Scene2MenuBar"};
  for (const char* szMenuBar : MenuBars)
  {
    WActionMapManager::RegisterActionMap(szMenuBar, "AssetMenuBar");
    WStandardMenus::MapActions(szMenuBar, WStandardMenuTypes::Scene | WStandardMenuTypes::View);
    WDocumentActions::MapToolsActions(szMenuBar);
    WTransformGizmoActions::MapMenuActions(szMenuBar);
    WSceneGizmoActions::MapMenuActions(szMenuBar);
    WGameObjectSelectionActions::MapActions(szMenuBar);
    WSelectionActions::MapActions(szMenuBar);
    WEditActions::MapActions(szMenuBar, true, true);
    WTranslateGizmoAction::MapActions(szMenuBar);
    WGameObjectDocumentActions::MapMenuActions(szMenuBar);
    WGameObjectDocumentActions::MapMenuSimulationSpeed(szMenuBar);
    WSceneActions::MapMenuActions(szMenuBar);
  }
  // Scene2 Menu bar adjustments
  {
    WActionMap* pMap = WActionMapManager::GetActionMap(MenuBars[1]);
    pMap->HideAction(WDocumentActions::s_hSave, "G.File.Common");
    pMap->MapAction(WLayerActions::s_hSaveActiveLayer, "G.File.Common", 6.5f);
  }


  // Tool Bar
  const char* ToolBars[] = {"EditorPluginScene_DocumentToolBar", "EditorPluginScene_Scene2ToolBar"};
  for (const char* szToolBar : ToolBars)
  {
    WActionMapManager::RegisterActionMap(szToolBar, "AssetToolbar");

    WTransformGizmoActions::MapToolbarActions(szToolBar);
    WSceneGizmoActions::MapToolbarActions(szToolBar);
    WGameObjectDocumentActions::MapToolbarActions(szToolBar);
    WSceneActions::MapToolbarActions(szToolBar);
  }
  // Scene2 Tool bar adjustments
  {
    WActionMap* pMap = WActionMapManager::GetActionMap(ToolBars[1]);
    pMap->HideAction(WDocumentActions::s_hSave, "SaveCategory");
    pMap->MapAction(WLayerActions::s_hSaveActiveLayer, "SaveCategory", 1.0f);
  }

  // View Tool Bar
  WActionMapManager::RegisterActionMap("EditorPluginScene_ViewToolBar", "AssetViewToolbar");
  WViewActions::MapToolbarActions("EditorPluginScene_ViewToolBar", WViewActions::PerspectiveMode | WViewActions::RenderMode /*| WViewActions::ActivateRemoteProcess*/);
  WQuadViewActions::MapToolbarActions("EditorPluginScene_ViewToolBar");

  // Visualizers
  WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WPointLightVisualizerAttribute>(), [](const WRTTI* pRtti) -> WVisualizerAdapter*
    { return W_DEFAULT_NEW(WPointLightVisualizerAdapter); });
  WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WDirectionalLightVisualizerAttribute>(), [](const WRTTI* pRtti) -> WVisualizerAdapter*
    { return W_DEFAULT_NEW(WDirectionalLightVisualizerAdapter); });
  WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WSpotLightVisualizerAttribute>(), [](const WRTTI* pRtti) -> WVisualizerAdapter*
    { return W_DEFAULT_NEW(WSpotLightVisualizerAdapter); });
  WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WBoxReflectionProbeVisualizerAttribute>(), [](const WRTTI* pRtti) -> WVisualizerAdapter*
    { return W_DEFAULT_NEW(WBoxReflectionProbeVisualizerAdapter); });

  // SceneGraph Context Menu
  WActionMapManager::RegisterActionMap("EditorPluginScene_ScenegraphContextMenu");
  WGameObjectSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");
  WSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");
  WEditActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");

  // Layer Context Menu
  WActionMapManager::RegisterActionMap("EditorPluginScene_LayerContextMenu");
  WLayerActions::MapContextMenuActions("EditorPluginScene_LayerContextMenu");

  WActionMapManager::RegisterActionMap("EditorPluginScene_LayerToolbar");
  WLayerActions::MapToolbarActions("EditorPluginScene_LayerToolbar");

  // component property meta states
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WCameraComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WSkyLightComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WGreyBoxComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WLightComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WAreaDamageComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WProjectileSurfaceInteraction_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WOccluderComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WSplineNodeComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WLensFlareComponent_PropertyMetaStateEventHandler);
}

void OnUnloadPlugin()
{
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WSceneDocument_PropertyMetaStateEventHandler);

  WToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(OnDocumentManagerEvent));
  WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(AssetCuratorEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WGreyBoxComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WSkyLightComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WCameraComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WLightComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WAreaDamageComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WProjectileSurfaceInteraction_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WOccluderComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WSplineNodeComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WLensFlareComponent_PropertyMetaStateEventHandler);


  WSelectionActions::UnregisterActions();
  WSceneGizmoActions::UnregisterActions();
  WLayerActions::UnregisterActions();
  WSceneActions::UnregisterActions();
  WMeshPrefabActions::UnregisterActions();
  WMcpToolRegistry::RemoveProvider(WGetStaticRTTI<WMcpMeshPrefabTool>());
  WMcpToolRegistry::RemoveProvider(WGetStaticRTTI<WMcpMeshLodTool>());
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void WCameraComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WCameraComponent");
  W_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const WInt64 usage = e.m_pObject->GetTypeAccessor().GetValue("UsageHint").ConvertTo<WInt64>();
  const bool isRenderTarget = (usage == 3); // WCameraUsageHint::RenderTarget

  auto& props = *e.m_pPropertyStates;

  props["RenderTarget"].m_Visibility = isRenderTarget ? WPropertyUiState::Default : WPropertyUiState::Disabled;
  props["RenderTargetOffset"].m_Visibility = isRenderTarget ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  props["RenderTargetSize"].m_Visibility = isRenderTarget ? WPropertyUiState::Default : WPropertyUiState::Invisible;
}

void WSkyLightComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WSkyLightComponent");
  W_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const WInt64 iReflectionProbeMode = e.m_pObject->GetTypeAccessor().GetValue("ReflectionProbeMode").ConvertTo<WInt64>();
  const bool bIsStatic = (iReflectionProbeMode == 0); // WReflectionProbeMode::Static

  auto& props = *e.m_pPropertyStates;

  props["CubeMap"].m_Visibility = bIsStatic ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  // props["RenderTargetOffset"].m_Visibility = isRenderTarget ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  // props["RenderTargetSize"].m_Visibility = isRenderTarget ? WPropertyUiState::Default : WPropertyUiState::Invisible;
}

void WGreyBoxComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WGreyBoxComponent");
  W_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  auto& props = *e.m_pPropertyStates;

  const WInt64 iShapeType = e.m_pObject->GetTypeAccessor().GetValue("Shape").ConvertTo<WInt64>();

  props["Detail"].m_Visibility = WPropertyUiState::Invisible;
  props["Detail"].m_sNewLabelText = "Detail";
  props["Curvature"].m_Visibility = WPropertyUiState::Invisible;
  props["Thickness"].m_Visibility = WPropertyUiState::Invisible;
  props["SlopedTop"].m_Visibility = WPropertyUiState::Invisible;
  props["SlopedBottom"].m_Visibility = WPropertyUiState::Invisible;

  switch (iShapeType)
  {
    case WGreyBoxShape::Box:
      break;
    case WGreyBoxShape::RampPosX:
    case WGreyBoxShape::RampNegX:
    case WGreyBoxShape::RampPosY:
    case WGreyBoxShape::RampNegY:
      break;
    case WGreyBoxShape::Column:
      props["Detail"].m_Visibility = WPropertyUiState::Default;
      break;
    case WGreyBoxShape::StairsPosX:
    case WGreyBoxShape::StairsNegX:
    case WGreyBoxShape::StairsPosY:
    case WGreyBoxShape::StairsNegY:
      props["Detail"].m_Visibility = WPropertyUiState::Default;
      props["Curvature"].m_Visibility = WPropertyUiState::Default;
      props["SlopedTop"].m_Visibility = WPropertyUiState::Default;
      props["Detail"].m_sNewLabelText = "Steps";
      break;
    case WGreyBoxShape::ArchX:
    case WGreyBoxShape::ArchY:
      props["Detail"].m_Visibility = WPropertyUiState::Default;
      props["Curvature"].m_Visibility = WPropertyUiState::Default;
      props["Thickness"].m_Visibility = WPropertyUiState::Default;
      break;
    case WGreyBoxShape::SpiralStairs:
      props["Detail"].m_Visibility = WPropertyUiState::Default;
      props["Curvature"].m_Visibility = WPropertyUiState::Default;
      props["Thickness"].m_Visibility = WPropertyUiState::Default;
      props["SlopedTop"].m_Visibility = WPropertyUiState::Default;
      props["SlopedBottom"].m_Visibility = WPropertyUiState::Default;
      props["Detail"].m_sNewLabelText = "Steps";
      break;
  }
}

void WLightComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pLightComponentRtti = WRTTI::FindTypeByName("WLightComponent");
  static const WRTTI* pFillLightComponentRtti = WRTTI::FindTypeByName("WFillLightComponent");
  W_ASSERT_DEBUG(pLightComponentRtti != nullptr && pFillLightComponentRtti != nullptr, "Did the typename change?");

  auto& props = *e.m_pPropertyStates;

  const WRTTI* pObjectType = e.m_pObject->GetTypeAccessor().GetType();
  const bool bIsLight = pObjectType->IsDerivedFrom(pLightComponentRtti);
  const bool bIsFillLight = pObjectType->IsDerivedFrom(pFillLightComponentRtti);

  if (bIsLight || bIsFillLight)
  {
    const bool bUseColorTemperature = e.m_pObject->GetTypeAccessor().GetValue("UseColorTemperature").ConvertTo<bool>();
    props["Temperature"].m_Visibility = bUseColorTemperature ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["LightColor"].m_Visibility = bUseColorTemperature ? WPropertyUiState::Invisible : WPropertyUiState::Default;
  }

  if (bIsLight)
  {
    const bool bCastShadows = e.m_pObject->GetTypeAccessor().GetValue("CastShadows").ConvertTo<bool>();
    props["TransparentShadows"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["PenumbraSize"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SlopeBias"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["ConstantBias"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;

    // Point/Spot light
    props["ShadowFadeOutRange"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;

    // Directional light
    props["NumCascades"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MinShadowRange"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["FadeOutStart"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SplitModeWeight"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["NearPlaneOffset"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["ScreenSpaceShadows"].m_Visibility = bCastShadows ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
}

void WAreaDamageComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WAreaDamageComponent");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  auto& props = *e.m_pPropertyStates;

  const WInt32 iImpulseType = e.m_pObject->GetTypeAccessor().GetValue("ImpulseType").ConvertTo<WInt32>();

  if (iImpulseType != WImpulseTypeConfig::CustomValueKey)
  {
    props["Impulse"].m_Visibility = WPropertyUiState::Invisible;
  }
}

void WProjectileSurfaceInteraction_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WProjectileSurfaceInteraction");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  auto& props = *e.m_pPropertyStates;

  const WInt32 iImpulseType = e.m_pObject->GetTypeAccessor().GetValue("ImpulseType").ConvertTo<WInt32>();

  if (iImpulseType != WImpulseTypeConfig::CustomValueKey)
  {
    props["Impulse"].m_Visibility = WPropertyUiState::Invisible;
  }
}

void WOccluderComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WOccluderComponent");
  W_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const WInt64 type = e.m_pObject->GetTypeAccessor().GetValue("Type").ConvertTo<WInt64>();
  const bool isMesh = (type == 2); // WOccluderType::Mesh

  auto& props = *e.m_pPropertyStates;

  props["Extents"].m_Visibility = isMesh ? WPropertyUiState::Invisible : WPropertyUiState::Default;
  props["Mesh"].m_Visibility = isMesh ? WPropertyUiState::Default : WPropertyUiState::Invisible;
}

void WSplineNodeComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WSplineNodeComponent");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const WInt32 iTangentModeIn = e.m_pObject->GetTypeAccessor().GetValue("TangentModeIn").ConvertTo<WInt32>();
  const WInt32 iTangentModeOut = e.m_pObject->GetTypeAccessor().GetValue("TangentModeOut").ConvertTo<WInt32>();

  auto& props = *e.m_pPropertyStates;
  props["CustomTangentIn"].m_Visibility = iTangentModeIn == WSplineTangentMode::Custom ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  props["CustomTangentOut"].m_Visibility = iTangentModeOut == WSplineTangentMode::Custom ? WPropertyUiState::Default : WPropertyUiState::Invisible;
}

void WLensFlareComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WLensFlareComponent");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const bool bLinkToLightShape = e.m_pObject->GetTypeAccessor().GetValue("LinkToLightShape").ConvertTo<bool>();

  auto& props = *e.m_pPropertyStates;
  props["LightColor"].m_Visibility = bLinkToLightShape ? WPropertyUiState::Invisible : WPropertyUiState::Default;
}
