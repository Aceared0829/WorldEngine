#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class W_GUIFOUNDATION_DLL WDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(WStringView sMapping, WStringView sTargetMenu);
  static void MapToolbarActions(WStringView sMapping);
  static void MapToolsActions(WStringView sMapping);

  static WActionDescriptorHandle s_hSaveCategory;
  static WActionDescriptorHandle s_hSave;
  static WActionDescriptorHandle s_hSaveAs;
  static WActionDescriptorHandle s_hSaveAll;

  static WActionDescriptorHandle s_hClose;
  static WActionDescriptorHandle s_hCloseAll;
  static WActionDescriptorHandle s_hCloseAllButThis;

  static WActionDescriptorHandle s_hOpenContainingFolder;
  static WActionDescriptorHandle s_hCopyDocumentPath;

  static WActionDescriptorHandle s_hUpdatePrefabs;
};


/// Standard document actions.
class W_GUIFOUNDATION_DLL WDocumentAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WDocumentAction, WButtonAction);

public:
  enum class ButtonType
  {
    Save,
    SaveAs,
    SaveAll,
    Close,
    CloseAll,
    CloseAllButThis,
    OpenContainingFolder,
    CopyDocumentPath,
    UpdatePrefabs,
  };
  WDocumentAction(const WActionContext& context, const char* szName, ButtonType button);
  ~WDocumentAction();

  virtual void Execute(const WVariant& value) override;

private:
  void DocumentEventHandler(const WDocumentEvent& e);

  ButtonType m_ButtonType;
};
