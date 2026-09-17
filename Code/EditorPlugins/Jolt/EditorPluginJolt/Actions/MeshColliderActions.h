#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <EditorPluginJolt/Utils/MeshColliderCreator.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/Uuid.h>
#include <GuiFoundation/Action/BaseActions.h>

/// Creates Jolt collision mesh assets from mesh assets. Hides itself unless the target is one or
/// more mesh assets.
class WMeshColliderActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  /// Pass bDocumentScope for a map belonging to a document window, so that the action is given that
  /// document; leave it off for the asset browser, which has none and uses WAssetBrowserSelection.
  ///
  /// Fails if the action map does not exist, i.e. the plugin that owns it is not loaded.
  static WResult MapActions(WStringView sActionMap, WStringView sSubPath, bool bDocumentScope = false);

  static WActionDescriptorHandle s_hCategory;
  static WActionDescriptorHandle s_hCreateCollider;
  static WActionDescriptorHandle s_hCreateColliderDoc;
};

class WMeshColliderAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WMeshColliderAction, WButtonAction);

public:
  WMeshColliderAction(const WActionContext& context, const char* szName);

  virtual void Execute(const WVariant& value) override;
  virtual void RefreshState() override;

private:
  /// The action's own document, or else every mesh asset in the asset browser selection. Empty when
  /// neither names a mesh asset.
  ///
  /// Non-mesh assets in the selection are dropped rather than disabling the action.
  void GetTargetAssets(WDynamicArray<WUuid>& out_assets) const;
};
