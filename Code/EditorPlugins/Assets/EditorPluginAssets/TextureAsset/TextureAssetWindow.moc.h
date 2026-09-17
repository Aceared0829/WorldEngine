#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtOrbitCamViewWidget;
class WTextureAssetDocument;

class WQtTextureAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtTextureAssetDocumentWindow(WTextureAssetDocument* pDocument);

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  WEngineViewConfig m_ViewConfig;
  WQtOrbitCamViewWidget* m_pViewWidget;
};

class W_EDITORPLUGINASSETS_DLL WTextureChannelModeAction : public WEnumerationMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WTextureChannelModeAction, WEnumerationMenuAction);

public:
  WTextureChannelModeAction(const WActionContext& context, const char* szName, const char* szIconPath);
  virtual WInt64 GetValue() const override;
  virtual void Execute(const WVariant& value) override;

private:
  const WAbstractMemberProperty* m_pValueProperty = nullptr;
};

class W_EDITORPLUGINASSETS_DLL WTextureLodSliderAction : public WSliderAction
{
  W_ADD_DYNAMIC_REFLECTION(WTextureLodSliderAction, WSliderAction);

public:
  WTextureLodSliderAction(const WActionContext& context, const char* szName);

  virtual void Execute(const WVariant& value) override;

private:
  const WAbstractMemberProperty* m_pValueProperty = nullptr;
};

class WTextureAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hTextureChannelMode;
  static WActionDescriptorHandle s_hLodSlider;
};
