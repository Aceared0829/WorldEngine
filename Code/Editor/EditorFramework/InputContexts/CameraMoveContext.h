#pragma once

#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <Foundation/Time/Time.h>
#include <QPoint>

class WCamera;

class W_EDITORFRAMEWORK_DLL WCameraMoveContext : public WEditorInputContext
{
public:
  WCameraMoveContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView);

  void LoadState();

  void SetCamera(WCamera* pCamera);

  WVec3 GetOrbitPoint() const;
  void SetOrbitDistance(float fDistance);

  static float ConvertCameraSpeed(WUInt32 uiSpeedIdx);

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual WEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual WEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;
  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual WEditorInput DoWheelEvent(QWheelEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override {}


  void OnActivated() override;

private:
  virtual void UpdateContext() override;

  void SetMoveSpeed(WInt32 iSpeed);
  void ResetCursor();
  void SetCurrentMouseMode();
  void DeactivateIfLast();

  float m_fOrbitPointDistance = 1.0f;

  WVec2I32 m_vLastMousePos = WVec2I32::MakeZero();
  WVec2I32 m_vMouseClickPos = WVec2I32::MakeZero();

  bool m_bRotateCamera = false;
  bool m_bMoveCamera = false;
  bool m_bMoveCameraInPlane = false;
  bool m_bOrbitCamera = false;
  bool m_bSlideForwards = false;
  bool m_bPanOrbitPoint = false;
  bool m_bPanCamera = false;
  bool m_bOpenMenuOnMouseUp = false;

  WCamera* m_pCamera = nullptr;

  bool m_bRun = false;
  bool m_bMoveForwards = false;
  bool m_bMoveBackwards = false;
  bool m_bMoveRight = false;
  bool m_bMoveLeft = false;
  bool m_bMoveUp = false;
  bool m_bMoveDown = false;
  bool m_bMoveForwardsInPlane = false;
  bool m_bMoveBackwardsInPlane = false;
  WInt32 m_iDidMoveMouse[3] = {0, 0, 0}; // Left Click, Right Click, Middle Click

  bool m_bRotateLeft = false;
  bool m_bRotateRight = false;
  bool m_bRotateUp = false;
  bool m_bRotateDown = false;

  WTime m_LastUpdate;
};
