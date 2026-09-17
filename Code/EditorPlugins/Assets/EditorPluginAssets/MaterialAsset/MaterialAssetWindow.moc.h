#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WMaterialAssetDocument;
class WQtOrbitCamViewWidget;
class WQtVisualShaderScene;
class WQtVisualGraphView;
struct WSelectionManagerEvent;
class WDirectoryWatcher;
enum class WDirectoryWatcherAction;
enum class WDirectoryWatcherType;
class WQtDocumentPanel;
class QTextEdit;
struct WMaterialVisualShaderEvent;

class WQtMaterialAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtMaterialAssetDocumentWindow(WMaterialAssetDocument* pDocument);
  ~WQtMaterialAssetDocumentWindow();

  WMaterialAssetDocument* GetMaterialDocument();

protected:
  virtual void InternalRedraw() override;


  virtual void showEvent(QShowEvent* event) override;

private Q_SLOTS:
  void OnOpenShaderClicked(bool);

private:
  void UpdatePreview();
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void SelectionEventHandler(const WSelectionManagerEvent& e);
  void SendRedrawMsg();
  void RestoreResource();
  void UpdateNodeEditorVisibility();
  void OnVseConfigChanged(WStringView sFilename, WDirectoryWatcherAction action, WDirectoryWatcherType type);
  void VisualShaderEventHandler(const WMaterialVisualShaderEvent& e);
  void SetupDirectoryWatcher(bool needIt);

  WEngineViewConfig m_ViewConfig;
  WQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  WQtVisualShaderScene* m_pScene = nullptr;
  WQtVisualGraphView* m_pNodeView = nullptr;
  WQtDocumentPanel* m_pVsePanel = nullptr;
  QTextEdit* m_pOutputLine = nullptr;
  QPushButton* m_pOpenShaderButton = nullptr;
  bool m_bVisualShaderEnabled;

  static WInt32 s_iNodeConfigWatchers;

  /// One watcher for the editor's own VisualShader folder, plus one for each data directory of the
  /// open project that has such a folder (projects may ship their own nodes).
  static WHybridArray<WDirectoryWatcher*, 4> s_NodeConfigWatchers;
};

class WMaterialModelAction : public WEnumerationMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WMaterialModelAction, WEnumerationMenuAction);

public:
  WMaterialModelAction(const WActionContext& context, const char* szName, const char* szIconPath);
  virtual WInt64 GetValue() const override;
  virtual void Execute(const WVariant& value) override;
};

class WMaterialAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hMaterialModelAction;
};
