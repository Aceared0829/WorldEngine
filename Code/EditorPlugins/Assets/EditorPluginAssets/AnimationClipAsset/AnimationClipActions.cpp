#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationClipAsset/AnimationClipActions.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationClipAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WAnimationClipActions::s_hCategory;
WActionDescriptorHandle WAnimationClipActions::s_hRootMotionFromFeet;

void WAnimationClipActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("AnimationClipAssetCategory");
  s_hRootMotionFromFeet = W_REGISTER_ACTION_1("AnimationClip.RootMotionFromFeet", WActionScope::Document, "Animation Clip", "", WAnimationClipAction, WAnimationClipAction::ActionType::RootMotionFromFeet);
}

void WAnimationClipActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hRootMotionFromFeet);
}

void WAnimationClipActions::MapActions(WStringView sActionMapName, WStringView sSubPath)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sActionMapName);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sActionMapName);

  pMap->MapAction(s_hCategory, sSubPath, 10.0f);

  pMap->MapAction(s_hRootMotionFromFeet, "AnimationClipAssetCategory", 1.0f);
}

WAnimationClipAction::WAnimationClipAction(const WActionContext& context, const char* szName, WAnimationClipAction::ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pAssetWindow = static_cast<WQtAnimationClipAssetDocumentWindow*>(WQtDocumentWindow::FindWindowByDocument(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::RootMotionFromFeet:
      SetIconPath(":/EditorPluginAssets/Foot.svg");
      break;

    default:
      break;
  }
}

WAnimationClipAction::~WAnimationClipAction() = default;

void WAnimationClipAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::RootMotionFromFeet:
      m_pAssetWindow->ExtractRootMotionFromFeet();
      return;
  }
}
