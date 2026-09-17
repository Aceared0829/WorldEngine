#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WRotateGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WRotateGizmo, WGizmo);

public:
  WRotateGizmo();

  const WQuat& GetRotationResult() const { return m_qCurrentRotation; }

  virtual void UpdateStatusBarText(WQtEngineDocumentWindow* pWindow) override;

  void EnableAxis(bool x, bool y, bool z);

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const WTransform& transform) override;

private:
  bool m_bEnableAxisX = true;
  bool m_bEnableAxisY = true;
  bool m_bEnableAxisZ = true;

  WEngineGizmoHandle m_hAxisX;
  WEngineGizmoHandle m_hAxisY;
  WEngineGizmoHandle m_hAxisZ;

  WQuat m_qStartRotation;
  WQuat m_qCurrentRotation;
  WAngle m_Rotation;

  WVec2I32 m_vLastMousePos;

  WTime m_LastInteraction;
  WVec3 m_vRotationAxis;
  WMat4 m_mInvViewProj;
  WVec2 m_vScreenTangent;
};
