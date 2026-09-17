#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WNonUniformBoxGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WNonUniformBoxGizmo, WGizmo);

public:
  WNonUniformBoxGizmo();

  void SetSize(const WVec3& vNegSize, const WVec3& vPosSize, bool bLinkAxis = false);

  const WVec3& GetNegSize() const { return m_vNegSize; }
  const WVec3& GetPosSize() const { return m_vPosSize; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const WTransform& transform) override;

private:
  WResult GetPointOnAxis(WInt32 iScreenPosX, WInt32 iScreenPosY, WVec3& out_Result) const;

  WTime m_LastInteraction;
  WMat4 m_mInvViewProj;

  WVec2I32 m_vLastMousePos;

  WEngineGizmoHandle m_hOutline;
  WEngineGizmoHandle m_Nobs[6];
  WVec3 m_vMainAxis[6];

  enum ManipulateMode
  {
    None = -1,
    DragNegX,
    DragPosX,
    DragNegY,
    DragPosY,
    DragNegZ,
    DragPosZ,
  };

  ManipulateMode m_ManipulateMode = ManipulateMode::None;

  WVec3 m_vNegSize;
  WVec3 m_vPosSize;
  WVec3 m_vStartNegSize;
  WVec3 m_vStartPosSize;
  WVec3 m_vMoveAxis;
  WVec3 m_vStartPosition;
  WVec3 m_vInteractionPivot;
  float m_fStartScale = 1.0f;
  bool m_bLinkAxis = false;
};
