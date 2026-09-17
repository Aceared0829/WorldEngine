#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class W_EDITORFRAMEWORK_DLL WAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(WStringView sMapping);
  static void MapToolBarActions(WStringView sMapping, bool bDocument);

  static WActionDescriptorHandle s_hAssetCategory;
  static WActionDescriptorHandle s_hTransformAsset;
  static WActionDescriptorHandle s_hAssetHelp;
  static WActionDescriptorHandle s_hTransformAllAssets;
  static WActionDescriptorHandle s_hCheckFileSystem;
  static WActionDescriptorHandle s_hWriteDependencyDGML;
  static WActionDescriptorHandle s_hCopyAssetGuid;
  static WActionDescriptorHandle s_hSelectInAssetBrowser;
};

///
class W_EDITORFRAMEWORK_DLL WAssetAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WAssetAction, WButtonAction);

public:
  enum class ButtonType
  {
    TransformAsset,
    AssetHelp,
    TransformAllAssets,
    CheckFileSystem,
    WriteDependencyDGML,
    CopyAssetGuid,
    SelectInAssetBrowser,
  };

  WAssetAction(const WActionContext& context, const char* szName, ButtonType button);
  ~WAssetAction();

  virtual void Execute(const WVariant& value) override;

private:
  ButtonType m_ButtonType;
};
