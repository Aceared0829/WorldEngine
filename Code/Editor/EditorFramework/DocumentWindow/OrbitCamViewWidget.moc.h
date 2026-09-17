#pragma once

#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Basics.h>
#include <Foundation/Types/UniquePtr.h>

class WOrbitCameraContext;
class WSelectionContext;

class W_EDITORFRAMEWORK_DLL WQtOrbitCamViewWidget : public WQtEngineViewWidget
{
  Q_OBJECT
public:
  WQtOrbitCamViewWidget(WQtEngineDocumentWindow* pOwnerWindow, WEngineViewConfig* pViewConfig, bool bPicking = false);
  ~WQtOrbitCamViewWidget();

  void ConfigureFixed(const WVec3& vCenterPos, const WVec3& vHalfBoxSize, const WVec3& vCamPosition);
  void ConfigureRelative(const WVec3& vCenterPos, const WVec3& vHalfBoxSize, const WVec3& vCamDirection, float fCamDistanceScale);

  void SetOrbitVolume(const WVec3& vCenterPos, const WVec3& vHalfBoxSize);

  WOrbitCameraContext* GetOrbitCamera();

  virtual void SyncToEngine() override;

private:
  bool m_bSetDefaultCamPos = true;

  WUniquePtr<WOrbitCameraContext> m_pOrbitCameraContext;
  WUniquePtr<WSelectionContext> m_pSelectionContext;
};
