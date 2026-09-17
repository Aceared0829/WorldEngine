#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonActions.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSkeletonAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WSkeletonActions::s_hCategory;
WActionDescriptorHandle WSkeletonActions::s_hRenderBones;
WActionDescriptorHandle WSkeletonActions::s_hRenderColliders;
WActionDescriptorHandle WSkeletonActions::s_hRenderJoints;
WActionDescriptorHandle WSkeletonActions::s_hRenderSwingLimits;
WActionDescriptorHandle WSkeletonActions::s_hRenderTwistLimits;
WActionDescriptorHandle WSkeletonActions::s_hRenderPreviewMesh;

void WSkeletonActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("SkeletonCategory");
  s_hRenderBones = W_REGISTER_ACTION_1("Skeleton.RenderBones", WActionScope::Document, "Skeletons", "", WSkeletonAction, WSkeletonAction::ActionType::RenderBones);
  s_hRenderColliders = W_REGISTER_ACTION_1("Skeleton.RenderColliders", WActionScope::Document, "Skeletons", "", WSkeletonAction, WSkeletonAction::ActionType::RenderColliders);
  s_hRenderJoints = W_REGISTER_ACTION_1("Skeleton.RenderJoints", WActionScope::Document, "Skeletons", "", WSkeletonAction, WSkeletonAction::ActionType::RenderJoints);
  s_hRenderSwingLimits = W_REGISTER_ACTION_1("Skeleton.RenderSwingLimits", WActionScope::Document, "Skeletons", "", WSkeletonAction, WSkeletonAction::ActionType::RenderSwingLimits);
  s_hRenderTwistLimits = W_REGISTER_ACTION_1("Skeleton.RenderTwistLimits", WActionScope::Document, "Skeletons", "", WSkeletonAction, WSkeletonAction::ActionType::RenderTwistLimits);
  s_hRenderPreviewMesh = W_REGISTER_ACTION_1("Skeleton.RenderPreviewMesh", WActionScope::Document, "Skeletons", "", WSkeletonAction, WSkeletonAction::ActionType::RenderPreviewMesh);
}

void WSkeletonActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hRenderBones);
  WActionManager::UnregisterAction(s_hRenderColliders);
  WActionManager::UnregisterAction(s_hRenderJoints);
  WActionManager::UnregisterAction(s_hRenderSwingLimits);
  WActionManager::UnregisterAction(s_hRenderTwistLimits);
  WActionManager::UnregisterAction(s_hRenderPreviewMesh);
}

void WSkeletonActions::MapActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "SkeletonCategory";

  pMap->MapAction(s_hRenderBones, szSubPath, 1.0f);
  pMap->MapAction(s_hRenderColliders, szSubPath, 2.0f);
  // pMap->MapAction(s_hRenderJoints, szSubPath, 3.0f);
  pMap->MapAction(s_hRenderSwingLimits, szSubPath, 4.0f);
  pMap->MapAction(s_hRenderTwistLimits, szSubPath, 5.0f);
  pMap->MapAction(s_hRenderPreviewMesh, szSubPath, 6.0f);
}

WSkeletonAction::WSkeletonAction(const WActionContext& context, const char* szName, WSkeletonAction::ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pSkeletonpDocument = const_cast<WSkeletonAssetDocument*>(static_cast<const WSkeletonAssetDocument*>(context.m_pDocument));
  m_pSkeletonpDocument->Events().AddEventHandler(WMakeDelegate(&WSkeletonAction::AssetEventHandler, this));

  switch (m_Type)
  {
    case ActionType::RenderBones:
      SetIconPath(":/EditorPluginAssets/SkeletonBones.svg");
      break;

    case ActionType::RenderColliders:
      SetIconPath(":/EditorPluginAssets/SkeletonColliders.svg");
      break;

    case ActionType::RenderJoints:
      SetIconPath(":/EditorPluginAssets/SkeletonJoints.svg");
      break;

    case ActionType::RenderSwingLimits:
      SetIconPath(":/EditorPluginAssets/JointSwingLimits.svg");
      break;

    case ActionType::RenderTwistLimits:
      SetIconPath(":/EditorPluginAssets/JointTwistLimits.svg");
      break;

    case ActionType::RenderPreviewMesh:
      SetIconPath(":/EditorPluginAssets/PreviewMesh.svg");
      break;
  }

  UpdateState();
}

WSkeletonAction::~WSkeletonAction()
{
  m_pSkeletonpDocument->Events().RemoveEventHandler(WMakeDelegate(&WSkeletonAction::AssetEventHandler, this));
}

void WSkeletonAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::RenderBones:
      m_pSkeletonpDocument->SetRenderBones(!m_pSkeletonpDocument->GetRenderBones());
      return;

    case ActionType::RenderColliders:
      m_pSkeletonpDocument->SetRenderColliders(!m_pSkeletonpDocument->GetRenderColliders());
      return;

    case ActionType::RenderJoints:
      m_pSkeletonpDocument->SetRenderJoints(!m_pSkeletonpDocument->GetRenderJoints());
      return;

    case ActionType::RenderSwingLimits:
      m_pSkeletonpDocument->SetRenderSwingLimits(!m_pSkeletonpDocument->GetRenderSwingLimits());
      return;

    case ActionType::RenderTwistLimits:
      m_pSkeletonpDocument->SetRenderTwistLimits(!m_pSkeletonpDocument->GetRenderTwistLimits());
      return;

    case ActionType::RenderPreviewMesh:
      m_pSkeletonpDocument->SetRenderPreviewMesh(!m_pSkeletonpDocument->GetRenderPreviewMesh());
      return;
  }
}

void WSkeletonAction::AssetEventHandler(const WSkeletonAssetEvent& e)
{
  switch (e.m_Type)
  {
    case WSkeletonAssetEvent::RenderStateChanged:
      UpdateState();
      break;
    default:
      break;
  }
}

void WSkeletonAction::UpdateState()
{
  if (m_Type == ActionType::RenderBones)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderBones());
  }

  if (m_Type == ActionType::RenderColliders)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderColliders());
  }

  if (m_Type == ActionType::RenderJoints)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderJoints());
  }

  if (m_Type == ActionType::RenderSwingLimits)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderSwingLimits());
  }

  if (m_Type == ActionType::RenderTwistLimits)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderTwistLimits());
  }

  if (m_Type == ActionType::RenderPreviewMesh)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderPreviewMesh());
  }
}
