#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

///
class W_GUIFOUNDATION_DLL WCommandHistoryActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping, WStringView sTargetMenu = "G.Edit");

  static WActionDescriptorHandle s_hCommandHistoryCategory;
  static WActionDescriptorHandle s_hUndo;
  static WActionDescriptorHandle s_hRedo;
};


///
class W_GUIFOUNDATION_DLL WCommandHistoryAction : public WDynamicActionAndMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WCommandHistoryAction, WDynamicActionAndMenuAction);

public:
  enum class ButtonType
  {
    Undo,
    Redo,
  };

  WCommandHistoryAction(const WActionContext& context, const char* szName, ButtonType button);
  ~WCommandHistoryAction();

  virtual void Execute(const WVariant& value) override;
  virtual void GetEntries(WDynamicArray<Item>& out_entries) override;

private:
  void UpdateState();
  void CommandHistoryEventHandler(const WCommandHistoryEvent& e);

  ButtonType m_ButtonType;
};
