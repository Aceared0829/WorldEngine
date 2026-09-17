#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

class WCamera;

/// A simple orbit camera. Use LMB to rotate, wheel to zoom, Alt to slow down.
class W_EDITORFRAMEWORK_DLL WOrbitCameraContext : public WEditorInputContext
{
public:
  WOrbitCameraContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView);

  void SetCamera(WCamera* pCamera);
  WCamera* GetCamera() const;

  void SetDefaultCameraRelative(const WVec3& vDirection, float fDistanceScale);
  void SetDefaultCameraFixed(const WVec3& vPosition);

  void MoveCameraToDefaultPosition();

  /// Defines the box in which the user may move the camera around
  void SetOrbitVolume(const WVec3& vCenterPos, const WVec3& vHalfBoxSize);

  /// The center point around which the camera can be moved and rotated.
  WVec3 GetVolumeCenter() const { return m_Volume.GetCenter(); }

  /// The half-size of the volume in which the camera may move around
  WVec3 GetVolumeHalfSize() const { return m_Volume.GetHalfExtents(); }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual WEditorInput DoWheelEvent(QWheelEvent* e) override;
  virtual WEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual WEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override {}

private:
  virtual void UpdateContext() override;

  float GetCameraSpeed() const;

  void ResetCursor();
  void SetCurrentMouseMode();

  WVec2I32 m_vLastMousePos;

  enum class Mode
  {
    Off,
    Orbit,
    Free,
    Pan,
  };

  Mode m_Mode = Mode::Off;
  WCamera* m_pCamera;

  WBoundingBox m_Volume;

  bool m_bFixedDefaultCamera = true;
  WVec3 m_vDefaultCamera = WVec3(1, 0, 0);

  bool m_bRun = false;
  bool m_bMoveForwards = false;
  bool m_bMoveBackwards = false;
  bool m_bMoveRight = false;
  bool m_bMoveLeft = false;
  bool m_bMoveUp = false;
  bool m_bMoveDown = false;

  WTime m_LastUpdate;
};
