#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CameraModeSwitchActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorPluginJolt/Actions/JoltActions.h>
#include <EditorPluginJolt/Actions/MeshColliderActions.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>
#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

void UpdateCollisionLayerDynamicEnumValues();
void UpdateWeightCategoryDynamicEnumValues();
void UpdateImpulseTypeDynamicEnumValues();

static void ToolsProjectEventHandler(const WToolsProjectEvent& e);

void WDynamicActorComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WRagdollComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WCharacterControllerComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WRopeComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WClothSheetComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

void OnLoadPlugin()
{
  WToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Collision Mesh
  {
    WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);

    // Menu Bar
    {
      WActionMapManager::RegisterActionMap("JoltCollisionMeshAssetMenuBar", "AssetMenuBar");
    }

    // Tool Bar
    {
      WActionMapManager::RegisterActionMap("JoltCollisionMeshAssetToolBar", "AssetToolbar");
      WCommonAssetActions::MapToolbarActions("JoltCollisionMeshAssetToolBar", WCommonAssetUiState::Grid);
      WCameraModeSwitchActions::MapToolbarActions("JoltCollisionMeshAssetToolBar");
    }

    // View Tool Bar
    {
      WActionMapManager::RegisterActionMap("JoltCollisionMeshAssetViewToolBar", "SimpleAssetViewToolbar");
    }
  }

  // Creating collision meshes from mesh assets
  {
    WMeshColliderActions::RegisterActions();

    WMeshColliderActions::MapActions("AssetBrowserContextMenu", WAssetBrowserContextMenu::s_sAssetMenu).IgnoreResult();
    WMeshColliderActions::MapActions("MeshAssetMenuBar", "G.Asset", true).IgnoreResult();
    WMeshColliderActions::MapActions("AnimatedMeshAssetMenuBar", "G.Asset", true).IgnoreResult();
  }

  // Scene
  {
    // Menu Bar
    {
      WJoltActions::RegisterActions();
      WJoltActions::MapMenuActions();
    }

    // Tool Bar
    {
    }
  }

  // component property meta states
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WDynamicActorComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WRagdollComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WCharacterControllerComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WRopeComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WClothSheetComponent_PropertyMetaStateEventHandler);
}

void OnUnloadPlugin()
{
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WDynamicActorComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WRagdollComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WCharacterControllerComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WRopeComponent_PropertyMetaStateEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WClothSheetComponent_PropertyMetaStateEventHandler);

  WJoltActions::UnregisterActions();
  WMeshColliderActions::UnregisterActions();
  WToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);
}

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void UpdateCollisionLayerDynamicEnumValues()
{
  auto& cfe = WDynamicEnum::GetDynamicEnum("PhysicsCollisionLayer");
  cfe.Clear();
  cfe.SetEditCommand("Jolt.Settings.Project", "CollisionLayers");

  WCollisionFilterConfig cfg;
  if (cfg.Load().Failed())
  {
    return;
  }

  // add all names and values that are valid (non-empty)
  for (WInt32 i = 0; i < 32; ++i)
  {
    if (!cfg.GetGroupName(i).IsEmpty())
    {
      cfe.SetValueAndName(i, cfg.GetGroupName(i));
    }
  }
}

void UpdateWeightCategoryDynamicEnumValues()
{
  auto& cfe = WDynamicEnum::GetDynamicEnum("PhysicsWeightCategoryWithDensity");
  auto& cfeNC = WDynamicEnum::GetDynamicEnum("PhysicsWeightCategory");

  cfe.Clear();
  cfe.SetEditCommand("Jolt.Settings.Project", "WeightCategories");

  cfeNC.Clear();
  cfeNC.SetEditCommand("Jolt.Settings.Project", "WeightCategories");

  WWeightCategoryConfig cfg;
  if (cfg.Load().Succeeded())
  {
    for (const auto it : cfg.m_Categories)
    {
      cfe.SetValueAndName(it.key, it.value.m_sName.GetView());
      cfeNC.SetValueAndName(it.key, it.value.m_sName.GetView());
    }
  }

  cfeNC.SetValueAndName(WWeightCategoryConfig::DefaultValueKey, "<Default>");
  cfeNC.SetValueAndName(WWeightCategoryConfig::CustomMassKey, "<Custom Mass>");

  cfe.SetValueAndName(WWeightCategoryConfig::DefaultValueKey, "<Default>");
  cfe.SetValueAndName(WWeightCategoryConfig::CustomMassKey, "<Custom Mass>");
  cfe.SetValueAndName(WWeightCategoryConfig::CustomDensityKey, "<Custom Density>");
}

void UpdateImpulseTypeDynamicEnumValues()
{
  auto& cfe = WDynamicEnum::GetDynamicEnum("PhysicsImpulseType");

  cfe.Clear();
  cfe.SetEditCommand("Jolt.Settings.Project", "ImpulseTypes");

  WImpulseTypeConfig cfg;
  if (cfg.Load().Succeeded())
  {
    for (const auto it : cfg.m_Types)
    {
      cfe.SetValueAndName(it.key, it.value.m_sName.GetView());
    }
  }

  cfe.SetValueAndName(WImpulseTypeConfig::CustomValueKey, "<Custom Value>");
  cfe.SetValueAndName(WImpulseTypeConfig::NoValueKey, "<None>");
}

static void ToolsProjectEventHandler(const WToolsProjectEvent& e)
{
  if (e.m_Type == WToolsProjectEvent::Type::ProjectSaveState)
  {
    WQtJoltProjectSettingsDlg::EnsureConfigFileExists();
  }

  if (e.m_Type == WToolsProjectEvent::Type::ProjectOpened)
  {
    WQtJoltProjectSettingsDlg::EnsureConfigFileExists();
    UpdateCollisionLayerDynamicEnumValues();
    UpdateWeightCategoryDynamicEnumValues();
    UpdateImpulseTypeDynamicEnumValues();
  }
}


//////////////////////////////////////////////////////////////////////////

void WJoltWeightComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  auto& props = *e.m_pPropertyStates;

  const WInt32 iCategory = e.m_pObject->GetTypeAccessor().GetValue("WeightCategory").ConvertTo<WInt32>();

  if (iCategory == WWeightCategoryConfig::DefaultValueKey)
  {
    props["WeightScale"].m_Visibility = WPropertyUiState::Invisible;
    props["Mass"].m_Visibility = WPropertyUiState::Invisible;
    props["Density"].m_Visibility = WPropertyUiState::Invisible;
  }
  else if (iCategory == WWeightCategoryConfig::CustomMassKey)
  {
    props["WeightScale"].m_Visibility = WPropertyUiState::Invisible;
    props["Density"].m_Visibility = WPropertyUiState::Invisible;
  }
  else if (iCategory == WWeightCategoryConfig::CustomDensityKey)
  {
    props["WeightScale"].m_Visibility = WPropertyUiState::Invisible;
    props["Mass"].m_Visibility = WPropertyUiState::Invisible;
  }
  else
  {
    props["Density"].m_Visibility = WPropertyUiState::Invisible;
    props["Mass"].m_Visibility = WPropertyUiState::Invisible;
  }
}

void WDynamicActorComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WJoltDynamicActorComponent");
  W_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  WJoltWeightComponent_PropertyMetaStateEventHandler(e);
}

void WRagdollComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WJoltRagdollComponent");
  W_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  WJoltWeightComponent_PropertyMetaStateEventHandler(e);
}

void WCharacterControllerComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WJoltCharacterControllerComponent");
  W_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  WJoltWeightComponent_PropertyMetaStateEventHandler(e);
}

void WRopeComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WJoltRopeComponent");
  W_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  WJoltWeightComponent_PropertyMetaStateEventHandler(e);
}

void WClothSheetComponent_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WJoltClothSheetComponent");
  W_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  WJoltWeightComponent_PropertyMetaStateEventHandler(e);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WJoltRopeComponentPatch_1_2 : public WGraphPatch
{
public:
  WJoltRopeComponentPatch_1_2()
    : WGraphPatch("WJoltRopeComponent", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Anchor", "Anchor2");
    pNode->RenameProperty("AttachToOrigin", "AttachToAnchor1");
    pNode->RenameProperty("AttachToAnchor", "AttachToAnchor2");
  }
};

WJoltRopeComponentPatch_1_2 g_WJoltRopeComponentPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class WJoltHitboxComponentPatch_1_2 : public WGraphPatch
{
public:
  WJoltHitboxComponentPatch_1_2()
    : WGraphPatch("WJoltBoneColliderComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("WJoltHitboxComponent");
  }
};

WJoltHitboxComponentPatch_1_2 g_WJoltHitboxComponentPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class WJoltDynamicActorComponentPatch_5_6 : public WGraphPatch
{
public:
  WJoltDynamicActorComponentPatch_5_6()
    : WGraphPatch("WJoltDynamicActorComponent", 6)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto pPropMass = pNode->FindProperty("Mass");

    float fMass = 0.0f;

    if (pPropMass && pPropMass->m_Value.IsNumber())
    {
      fMass = pPropMass->m_Value.ConvertTo<float>();
    }

    if (fMass != 0.0f)
    {
      pNode->AddProperty("WeightCategory", static_cast<WUInt8>(WWeightCategoryConfig::CustomMassKey));
    }
    else
    {
      pNode->AddProperty("WeightCategory", static_cast<WUInt8>(WWeightCategoryConfig::CustomDensityKey));
    }
  }
};

WJoltDynamicActorComponentPatch_5_6 g_WJoltDynamicActorComponentPatch_5_6;
