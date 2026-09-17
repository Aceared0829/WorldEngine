#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WTranslateGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WTranslateGizmo, WGizmo);

public:
  WTranslateGizmo();

  const WVec3 GetStartPosition() const { return m_vStartPosition; }
  const WVec3 GetTranslationResult() const { return GetTransformation().m_vPosition - m_vStartPosition; }
  const WVec3 GetTranslationDiff() const { return m_vLastMoveDiff; }

  enum class MovementMode
  {
    ScreenProjection,
    MouseDiff
  };

  enum class HandleInteraction
  {
    None,
    AxisX,
    AxisY,
    AxisZ,
    PlaneX,
    PlaneY,
    PlaneZ,
  };

  enum class TranslateMode
  {
    None,
    Axis,
    Plane
  };

  void SetMovementMode(MovementMode mode);
  HandleInteraction GetLastHandleInteraction() const { return m_LastHandleInteraction; }
  TranslateMode GetTranslateMode() const { return m_Mode; }

  /// Used when CTRL+drag moves the object AND the camera
  void SetCameraSpeed(float fSpeed);

  virtual void UpdateStatusBarText(WQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const WTransform& transform) override;

  WResult GetPointOnPlane(const WVec2I32& vScreenPos, WVec3& out_Result) const;

private:
  WVec2I32 m_vLastMousePos;
  WVec2 m_vTotalMouseDiff;

  WVec3 m_vLastMoveDiff;

  MovementMode m_MovementMode;
  WEngineGizmoHandle m_hAxisX;
  WEngineGizmoHandle m_hAxisY;
  WEngineGizmoHandle m_hAxisZ;

  WEngineGizmoHandle m_hPlaneXY;
  WEngineGizmoHandle m_hPlaneXZ;
  WEngineGizmoHandle m_hPlaneYZ;

  TranslateMode m_Mode;
  HandleInteraction m_LastHandleInteraction;

  float m_fStartScale;
  float m_fCameraSpeed;

  WTime m_LastInteraction;
  WVec3 m_vMoveAxis;
  WVec3 m_vPlaneAxis[2];
  WVec3 m_vStartPosition;
  WMat4 m_mInvViewProj;
};
