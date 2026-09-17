#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAsset.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtOrbitCamViewWidget;

class WQtJoltCollisionMeshAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtJoltCollisionMeshAssetDocumentWindow(WAssetDocument* pDocument);

  virtual int GetCameraMode() const override { return m_iCameraMode; }
  virtual void SetCameraMode(int iMode) override;

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(WInt32 iPurpose = 0);

  WEngineViewConfig m_ViewConfig;
  WQtOrbitCamViewWidget* m_pViewWidget;
  WUniquePtr<WCameraMoveContext> m_pCameraFlyContext;
  int m_iCameraMode = 0;
  WAssetDocument* m_pAssetDoc;
};
