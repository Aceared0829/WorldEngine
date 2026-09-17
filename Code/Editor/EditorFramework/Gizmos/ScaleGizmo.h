#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WScaleGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WScaleGizmo, WGizmo);

public:
  WScaleGizmo();

  const WVec3& GetScalingResult() const { return m_vScalingResult; }

  virtual void UpdateStatusBarText(WQtEngineDocumentWindow* pWindow) override;

  void EnableAxis(bool x, bool y, bool z, bool bXyz);

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const WTransform& transform) override;

protected:
  bool m_bEnableAxisX = true;
  bool m_bEnableAxisY = true;
  bool m_bEnableAxisZ = true;
  bool m_bEnableAxisXYZ = true;

  WEngineGizmoHandle m_hAxisX;
  WEngineGizmoHandle m_hAxisY;
  WEngineGizmoHandle m_hAxisZ;
  WEngineGizmoHandle m_hAxisXYZ;

private:
  WVec3 m_vScalingResult;
  WVec3 m_vScaleMouseMove;

  WVec2I32 m_vLastMousePos;

  WTime m_LastInteraction;
  WVec3 m_vMoveAxis;
  WMat4 m_mInvViewProj;
};

/// Scale gizmo version that only uses boxes that can be composited with
/// rotate and translate gizmos without major overlap.
/// Used by the WTransformManipulatorAdapter.
class W_EDITORFRAMEWORK_DLL WManipulatorScaleGizmo : public WScaleGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WManipulatorScaleGizmo, WScaleGizmo);

public:
  WManipulatorScaleGizmo();

protected:
  virtual void OnTransformationChanged(const WTransform& transform) override;
};
