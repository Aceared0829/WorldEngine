#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CameraModeSwitchActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorPluginAssets/Actions/MeshLodActions.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipActions.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/Dialogs/ShaderTemplateDlg.moc.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonActions.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ConfigureAnimationGraphAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("AnimationGraphAssetMenuBar", "AssetMenuBar");
    WEditActions::MapActions("AnimationGraphAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("AnimationGraphAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureTexture2DAsset()
{
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WTextureAssetProperties::PropertyMetaStateEventHandler);

  WTextureAssetActions::RegisterActions();

  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("TextureAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("TextureAssetToolBar", "AssetToolbar");
    WTextureAssetActions::MapToolbarActions("TextureAssetToolBar");
  }
}

static void ConfigureTextureCubeAsset()
{
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WTextureCubeAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("TextureCubeAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("TextureCubeAssetToolBar", "AssetToolbar");
    WTextureAssetActions::MapToolbarActions("TextureCubeAssetToolBar");
  }
}

static void ConfigureLUTAsset()
{
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WLUTAssetProperties::PropertyMetaStateEventHandler);

  WLUTAssetActions::RegisterActions();

  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("LUTAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("LUTAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureMaterialAsset()
{
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WMaterialAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("MaterialAssetMenuBar", "AssetMenuBar");
    WDocumentActions::MapToolsActions("MaterialAssetMenuBar");
    WEditActions::MapActions("MaterialAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("MaterialAssetToolBar", "AssetToolbar");

    WMaterialAssetActions::RegisterActions();
    WMaterialAssetActions::MapToolbarActions("MaterialAssetToolBar");

    WVisualShaderActions::RegisterActions();
    WVisualShaderActions::MapActions("MaterialAssetToolBar");
  }

  // View Tool Bar
  {
    WActionMapManager::RegisterActionMap("MaterialAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureRenderPipelineAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("RenderPipelineAssetMenuBar", "AssetMenuBar");
    WEditActions::MapActions("RenderPipelineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("RenderPipelineAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureMeshAsset()
{
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("MeshAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("MeshAssetToolBar", "AssetToolbar");
    WCommonAssetActions::MapToolbarActions("MeshAssetToolBar", WCommonAssetUiState::Grid);
    WCameraModeSwitchActions::MapToolbarActions("MeshAssetToolBar");
  }

  // View Tool Bar
  {
    WActionMapManager::RegisterActionMap("MeshAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureSurfaceAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("SurfaceAssetMenuBar", "AssetMenuBar");
    WDocumentActions::MapToolsActions("SurfaceAssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("SurfaceAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureCollectionAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("CollectionAssetMenuBar", "AssetMenuBar");
    WDocumentActions::MapToolsActions("CollectionAssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("CollectionAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureColorGradientAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("ColorGradientAssetMenuBar", "AssetMenuBar");
    WDocumentActions::MapToolsActions("ColorGradientAssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("ColorGradientAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureCurve1DAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("Curve1DAssetMenuBar", "AssetMenuBar");
    WDocumentActions::MapToolsActions("Curve1DAssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("Curve1DAssetToolBar", "AssetToolbar");
  }
}

static void ConfigurePropertyAnimAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("PropertyAnimAssetMenuBar", "AssetMenuBar");
    WStandardMenus::MapActions("PropertyAnimAssetMenuBar", WStandardMenuTypes::Scene | WStandardMenuTypes::View);
    WDocumentActions::MapToolsActions("PropertyAnimAssetMenuBar");
    WGameObjectSelectionActions::MapActions("PropertyAnimAssetMenuBar");
    WGameObjectDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar");
    WGameObjectDocumentActions::MapMenuSimulationSpeed("PropertyAnimAssetMenuBar");
    WTransformGizmoActions::MapMenuActions("PropertyAnimAssetMenuBar");
    WTranslateGizmoAction::MapActions("PropertyAnimAssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("PropertyAnimAssetToolBar", "AssetToolbar");
    WGameObjectContextActions::MapToolbarActions("PropertyAnimAssetToolBar");
    WGameObjectDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar");
    WTransformGizmoActions::MapToolbarActions("PropertyAnimAssetToolBar");
  }

  // View Tool Bar
  {
    WActionMapManager::RegisterActionMap("PropertyAnimAssetViewToolBar", "AssetViewToolbar");
    WViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar", WViewActions::PerspectiveMode | WViewActions::RenderMode /*| WViewActions::ActivateRemoteProcess*/);
    WQuadViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar");
  }

  // SceneGraph Context Menu
  {
    WActionMapManager::RegisterActionMap("PropertyAnimAsset_ScenegraphContextMenu");
    WGameObjectSelectionActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
    WGameObjectContextActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
  }
}

static void ConfigureDecalAsset()
{
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WDecalAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("DecalAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("DecalAssetToolBar", "AssetToolbar");
  }

  // View Tool Bar
  {
    WActionMapManager::RegisterActionMap("DecalAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureAnimationClipAsset()
{
  WAnimationClipActions::RegisterActions();

  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WAnimationClipAssetProperties::PropertyMetaStateEventHandler);
  WDynamicStringEnum::s_RefreshValuesEvent.AddEventHandler(WAnimationClipAssetDocument::OnRefreshDynamicStringEnum);

  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("AnimationClipAssetMenuBar", "AssetMenuBar");

    WAnimationClipActions::MapActions("AnimationClipAssetMenuBar", "G.Asset");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("AnimationClipAssetToolBar", "AssetToolbar");
    WAnimationClipActions::MapActions("AnimationClipAssetToolBar", "");
    WCommonAssetActions::MapToolbarActions("AnimationClipAssetToolBar", WCommonAssetUiState::Loop | WCommonAssetUiState::Pause | WCommonAssetUiState::Restart | WCommonAssetUiState::SimulationSpeed | WCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    WActionMapManager::RegisterActionMap("AnimationClipAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureSkeletonAsset()
{
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WSkeletonAssetDocument::PropertyMetaStateEventHandler);

  WSkeletonActions::RegisterActions();

  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("SkeletonAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("SkeletonAssetToolBar", "AssetToolbar");
    WCommonAssetActions::MapToolbarActions("SkeletonAssetToolBar", WCommonAssetUiState::Grid);
    WSkeletonActions::MapActions("SkeletonAssetToolBar");
  }

  // View Tool Bar
  {
    WActionMapManager::RegisterActionMap("SkeletonAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureAnimatedMeshAsset()
{
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WAnimatedMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("AnimatedMeshAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("AnimatedMeshAssetToolBar", "AssetToolbar");
    WCommonAssetActions::MapToolbarActions("AnimatedMeshAssetToolBar", WCommonAssetUiState::Grid);
    WCameraModeSwitchActions::MapToolbarActions("AnimatedMeshAssetToolBar");
  }

  // View Tool Bar
  {
    WActionMapManager::RegisterActionMap("AnimatedMeshAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureImageDataAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("ImageDataAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("ImageDataAssetToolBar", "AssetToolbar");
  }

  // View Tool Bar
  {
    WActionMapManager::RegisterActionMap("ImageDataAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureStateMachineAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("StateMachineAssetMenuBar", "AssetMenuBar");
    WEditActions::MapActions("StateMachineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("StateMachineAssetToolBar", "AssetToolbar");
  }
}
static void ConfigureBlackboardTemplateAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("BlackboardTemplateAssetMenuBar", "AssetMenuBar");
    WEditActions::MapActions("BlackboardTemplateAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("BlackboardTemplateAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureCustomDataAsset()
{
  // Menu Bar
  {
    WActionMapManager::RegisterActionMap("CustomDataAssetMenuBar", "AssetMenuBar");
    WDocumentActions::MapToolsActions("CustomDataAssetMenuBar");
  }

  // Tool Bar
  {
    WActionMapManager::RegisterActionMap("CustomDataAssetToolBar", "AssetToolbar");
  }
}

WVariant CustomAction_CreateShaderFromTemplate(const WDocument* pDoc)
{
  WQtShaderTemplateDlg dlg(nullptr, pDoc);

  if (dlg.exec() == QDialog::Accepted)
  {
    WStringBuilder abs;
    if (WFileSystem::ResolvePath(dlg.m_sResult, &abs, nullptr).Succeeded())
    {
      if (WQtUiServices::GetSingleton()->OpenFileInDefaultProgram(abs).Failed())
      {
        WQtUiServices::GetSingleton()->MessageBoxInformation(WFmt("There is no default program set to open shader files:\n\n{}", abs));
      }
    }

    return dlg.m_sResult;
  }

  return {};
}

void OnLoadPlugin()
{
  ConfigureAnimationGraphAsset();
  ConfigureTexture2DAsset();
  ConfigureTextureCubeAsset();
  ConfigureLUTAsset();
  ConfigureMaterialAsset();
  ConfigureRenderPipelineAsset();
  ConfigureMeshAsset();
  ConfigureSurfaceAsset();
  ConfigureCollectionAsset();
  ConfigureColorGradientAsset();
  ConfigureCurve1DAsset();
  ConfigurePropertyAnimAsset();
  ConfigureDecalAsset();
  ConfigureAnimationClipAsset();
  ConfigureSkeletonAsset();
  ConfigureAnimatedMeshAsset();
  ConfigureImageDataAsset();
  ConfigureStateMachineAsset();
  ConfigureBlackboardTemplateAsset();
  ConfigureCustomDataAsset();

  // Creating LOD meshes from mesh assets.
  {
    WMeshLodActions::RegisterActions();

    WMeshLodActions::MapActions("AssetBrowserContextMenu", WAssetBrowserContextMenu::s_sAssetMenu).IgnoreResult();
    WMeshLodActions::MapActions("MeshAssetMenuBar", "G.Asset", true).IgnoreResult();
    WMeshLodActions::MapActions("AnimatedMeshAssetMenuBar", "G.Asset", true).IgnoreResult();
    WMeshLodActions::MapActions("MeshAssetToolBar", "", true).IgnoreResult();
    WMeshLodActions::MapActions("AnimatedMeshAssetToolBar", "", true).IgnoreResult();
  }

  WDocumentManager::s_CustomActions["CustomAction_CreateShaderFromTemplate"] = CustomAction_CreateShaderFromTemplate;
}

void OnUnloadPlugin()
{
  WMeshLodActions::UnregisterActions();
  WTextureAssetActions::UnregisterActions();
  WLUTAssetActions::UnregisterActions();
  WVisualShaderActions::UnregisterActions();
  WMaterialAssetActions::UnregisterActions();
  WSkeletonActions::UnregisterActions();
  WAnimationClipActions::UnregisterActions();

  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WAnimatedMeshAssetProperties::PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WMeshAssetProperties::PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WTextureAssetProperties::PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WDecalAssetProperties::PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WTextureCubeAssetProperties::PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WMaterialAssetProperties::PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WSkeletonAssetDocument::PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WAnimationClipAssetProperties::PropertyMetaStateEventHandler);
  WDynamicStringEnum::s_RefreshValuesEvent.RemoveEventHandler(WAnimationClipAssetDocument::OnRefreshDynamicStringEnum);
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
