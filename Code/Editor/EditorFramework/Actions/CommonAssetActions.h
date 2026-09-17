#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WAssetDocument;

class W_EDITORFRAMEWORK_DLL WCommonAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(WStringView sMapping, WUInt32 uiStateMask);

  static WActionDescriptorHandle s_hCategory;
  static WActionDescriptorHandle s_hPause;
  static WActionDescriptorHandle s_hRestart;
  static WActionDescriptorHandle s_hLoop;
  static WActionDescriptorHandle s_hSimulationSpeedMenu;
  static WActionDescriptorHandle s_hSimulationSpeed[10];
  static WActionDescriptorHandle s_hGrid;
  static WActionDescriptorHandle s_hVisualizers;
};

class W_EDITORFRAMEWORK_DLL WCommonAssetAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WCommonAssetAction, WButtonAction);

public:
  enum class ActionType
  {
    Pause,
    Restart,
    Loop,
    SimulationSpeed,
    Grid,
    Visualizers,
  };

  WCommonAssetAction(const WActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~WCommonAssetAction();

  virtual void Execute(const WVariant& value) override;

private:
  void CommonUiEventHandler(const WCommonAssetUiState& e);
  void UpdateState();

  WAssetDocument* m_pAssetDocument = nullptr;
  ActionType m_Type;
  float m_fSimSpeed;
};
