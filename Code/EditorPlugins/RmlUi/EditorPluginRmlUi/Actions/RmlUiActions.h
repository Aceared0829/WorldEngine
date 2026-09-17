#pragma once

#include <EditorPluginRmlUi/EditorPluginRmlUiDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WRmlUiAssetDocument;
struct WRmlUiAssetEvent;

class WRmlUiActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActionsMenu(WStringView sMapping);
  static void MapActionsToolbar(WStringView sMapping);

  static WActionDescriptorHandle s_hCategory;
  static WActionDescriptorHandle s_hOpenInVSC;
};

class WRmlUiAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WRmlUiAction, WButtonAction);

public:
  enum class ActionType
  {
    OpenInVSC,
  };

  WRmlUiAction(const WActionContext& context, const char* szName, ActionType type);

  virtual void Execute(const WVariant& value) override;

private:
  WRmlUiAssetDocument* m_pDocument = nullptr;
  ActionType m_Type;
};
