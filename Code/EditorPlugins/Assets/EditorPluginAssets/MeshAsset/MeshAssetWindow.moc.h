#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshEditorContext.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>

class WQtOrbitCamViewWidget;

class WQtMeshAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtMeshAssetDocumentWindow(WMeshAssetDocument* pDocument);
  ~WQtMeshAssetDocumentWindow();

  WMeshAssetDocument* GetMeshDocument();

  virtual int GetCameraMode() const override { return m_iCameraMode; }
  virtual void SetCameraMode(int iMode) override;

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg) override;

protected Q_SLOTS:
  void HighlightTimer();

private:
  void SendRedrawMsg();
  void QueryObjectBBox(WInt32 iPurpose = 0);
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  bool UpdatePreview();

  WEngineViewConfig m_ViewConfig;
  WQtOrbitCamViewWidget* m_pViewWidget;
  WUniquePtr<WMeshEditorInputContext> m_pMeshEditorInputContext;
  WUniquePtr<WCameraMoveContext> m_pCameraFlyContext;
  int m_iCameraMode = 0;
  WUInt32 m_uiHighlightSlots = 0;
  QPointer<QTimer> m_pHighlightTimer;
};
