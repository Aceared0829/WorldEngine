#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/Uuid.h>
#include <GuiFoundation/Action/BaseActions.h>

/// Creates LOD mesh assets from mesh assets. Hides itself unless the target is one or more mesh
/// assets that are not themselves LODs.
class WMeshLodActions
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
  static WActionDescriptorHandle s_hCreateLods;
  static WActionDescriptorHandle s_hCreateLodsDoc;
};

class WMeshLodAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WMeshLodAction, WButtonAction);

public:
  WMeshLodAction(const WActionContext& context, const char* szName);

  virtual void Execute(const WVariant& value) override;
  virtual void RefreshState() override;

private:
  /// The action's own document, or else every mesh asset in the asset browser selection. Empty when
  /// neither names a mesh asset.
  ///
  /// Non-mesh assets in the selection are dropped rather than disabling the action.
  void GetTargetAssets(WDynamicArray<WUuid>& out_assets) const;

  /// A LOD asset must not get LODs of its own, or the _data folders nest without end.
  static bool IsLodAsset(const WUuid& assetGuid);
};
