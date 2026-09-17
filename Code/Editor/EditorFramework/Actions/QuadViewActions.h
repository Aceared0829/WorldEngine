#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class W_EDITORFRAMEWORK_DLL WQuadViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hToggleViews;
  static WActionDescriptorHandle s_hSpawnView;
};

///
class W_EDITORFRAMEWORK_DLL WQuadViewAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WQuadViewAction, WButtonAction);

public:
  enum class ButtonType
  {
    ToggleViews,
    SpawnView,
  };

  WQuadViewAction(const WActionContext& context, const char* szName, ButtonType button);
  ~WQuadViewAction();

  virtual void Execute(const WVariant& value) override;

private:
  ButtonType m_ButtonType;
};
