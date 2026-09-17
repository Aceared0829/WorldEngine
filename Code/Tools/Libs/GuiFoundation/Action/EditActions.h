#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
///
class W_GUIFOUNDATION_DLL WEditActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping, bool bDeleteAction, bool bAdvancedPasteActions);
  static void MapContextMenuActions(WStringView sMapping);
  static void MapViewContextMenuActions(WStringView sMapping);

  static WActionDescriptorHandle s_hEditCategory;
  static WActionDescriptorHandle s_hCopy;
  static WActionDescriptorHandle s_hPaste;
  static WActionDescriptorHandle s_hPasteAsChild;
  static WActionDescriptorHandle s_hPasteAtOriginalLocation;
  static WActionDescriptorHandle s_hDelete;
};


///
class W_GUIFOUNDATION_DLL WEditAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WEditAction, WButtonAction);

public:
  enum class ButtonType
  {
    Copy,
    Paste,
    PasteAsChild,
    PasteAtOriginalLocation,
    Delete,
  };
  WEditAction(const WActionContext& context, const char* szName, ButtonType button);
  ~WEditAction();

  virtual void Execute(const WVariant& value) override;

private:
  void SelectionEventHandler(const WSelectionManagerEvent& e);

  ButtonType m_ButtonType;
};
