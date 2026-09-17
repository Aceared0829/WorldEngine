#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WSkeletonAssetDocument;
struct WSkeletonAssetEvent;

class WSkeletonActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping);

  static WActionDescriptorHandle s_hCategory;
  static WActionDescriptorHandle s_hRenderBones;
  static WActionDescriptorHandle s_hRenderColliders;
  static WActionDescriptorHandle s_hRenderJoints;
  static WActionDescriptorHandle s_hRenderSwingLimits;
  static WActionDescriptorHandle s_hRenderTwistLimits;
  static WActionDescriptorHandle s_hRenderPreviewMesh;
};

class WSkeletonAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WSkeletonAction, WButtonAction);

public:
  enum class ActionType
  {
    RenderBones,
    RenderColliders,
    RenderJoints,
    RenderSwingLimits,
    RenderTwistLimits,
    RenderPreviewMesh,
  };

  WSkeletonAction(const WActionContext& context, const char* szName, ActionType type);
  ~WSkeletonAction();

  virtual void Execute(const WVariant& value) override;

private:
  void AssetEventHandler(const WSkeletonAssetEvent& e);
  void UpdateState();

  WSkeletonAssetDocument* m_pSkeletonpDocument = nullptr;
  ActionType m_Type;
};
