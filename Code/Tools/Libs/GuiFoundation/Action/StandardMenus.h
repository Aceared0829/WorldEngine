#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

struct WStandardMenuTypes
{
  using StorageType = WUInt32;

  enum Enum
  {
    Project = W_BIT(0),
    File = W_BIT(1),
    Edit = W_BIT(2),
    Panels = W_BIT(3),
    Scene = W_BIT(4),
    Asset = W_BIT(5),
    View = W_BIT(6),
    Tools = W_BIT(7),
    Help = W_BIT(8),

    Default = Project | File | Panels | Tools | Help
  };

  struct Bits
  {
    StorageType Project : 1;
    StorageType File : 1;
    StorageType Edit : 1;
    StorageType Panels : 1;
    StorageType Scene : 1;
    StorageType Asset : 1;
    StorageType View : 1;
    StorageType Tools : 1;
    StorageType Help : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WStandardMenuTypes);

///
class W_GUIFOUNDATION_DLL WStandardMenus
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping, const WBitflags<WStandardMenuTypes>& menus);

  static WActionDescriptorHandle s_hMenuProject;
  static WActionDescriptorHandle s_hMenuFile;
  static WActionDescriptorHandle s_hMenuEdit;
  static WActionDescriptorHandle s_hMenuPanels;
  static WActionDescriptorHandle s_hMenuPanelsAll;
  static WActionDescriptorHandle s_hMenuScene;
  static WActionDescriptorHandle s_hMenuAsset;
  static WActionDescriptorHandle s_hMenuView;
  static WActionDescriptorHandle s_hMenuTools;
  static WActionDescriptorHandle s_hMenuHelp;
  static WActionDescriptorHandle s_hCheckForUpdates;
  static WActionDescriptorHandle s_hReportProblem;
  static WActionDescriptorHandle s_hAskQuestion;
};

///
class W_GUIFOUNDATION_DLL WApplicationPanelsMenuAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WApplicationPanelsMenuAction, WDynamicMenuAction);

public:
  WApplicationPanelsMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
    : WDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(WDynamicArray<Item>& out_entries) override;
  virtual void Execute(const WVariant& value) override;
};

//////////////////////////////////////////////////////////////////////////

class W_GUIFOUNDATION_DLL WHelpActions : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WHelpActions, WButtonAction);

public:
  enum class ButtonType
  {
    CheckForUpdates,
    ReportProblem,
    AskQuestion,
  };

  WHelpActions(const WActionContext& context, const char* szName, ButtonType button);
  ~WHelpActions();

  virtual void Execute(const WVariant& value) override;

private:
  ButtonType m_ButtonType;
};
