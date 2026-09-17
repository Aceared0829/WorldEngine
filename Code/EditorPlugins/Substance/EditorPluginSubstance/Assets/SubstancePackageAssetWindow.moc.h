#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtOrbitCamViewWidget;
class WSubstancePackageAssetDocument;

class WQtSubstancePackageAssetWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtSubstancePackageAssetWindow(WSubstancePackageAssetDocument* pDocument);

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  WEngineViewConfig m_ViewConfig;
  WQtOrbitCamViewWidget* m_pViewWidget;
};

//////////////////////////////////////////////////////////////////////////

class WSubstanceSelectOutputAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WSubstanceSelectOutputAction, WDynamicMenuAction);

public:
  WSubstanceSelectOutputAction(const WActionContext& context, const char* szName, const char* szIconPath);

  virtual void GetEntries(WDynamicArray<Item>& out_entries) override;
  virtual void Execute(const WVariant& value) override;
};

//////////////////////////////////////////////////////////////////////////

class WSubstancePackageAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hSelectedOutput;
  static WActionDescriptorHandle s_hTextureChannelMode;
  static WActionDescriptorHandle s_hLodSlider;
};
