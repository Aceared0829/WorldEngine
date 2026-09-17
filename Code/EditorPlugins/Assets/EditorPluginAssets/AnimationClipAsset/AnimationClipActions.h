#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WQtAnimationClipAssetDocumentWindow;

class WAnimationClipActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sActionMap, WStringView sSubPath);

  static WActionDescriptorHandle s_hCategory;
  static WActionDescriptorHandle s_hRootMotionFromFeet;
};

class WAnimationClipAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationClipAction, WButtonAction);

public:
  enum class ActionType
  {
    RootMotionFromFeet,
  };

  WAnimationClipAction(const WActionContext& context, const char* szName, ActionType type);
  ~WAnimationClipAction();

  virtual void Execute(const WVariant& value) override;

private:
  WQtAnimationClipAssetDocumentWindow* m_pAssetWindow = nullptr;
  ActionType m_Type;
};
