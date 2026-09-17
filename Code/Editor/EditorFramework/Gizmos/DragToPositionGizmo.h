#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Math/Quat.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WDragToPositionGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WDragToPositionGizmo, WGizmo);

public:
  WDragToPositionGizmo();

  const WVec3 GetTranslationResult() const { return GetTransformation().m_vPosition - m_vStartPosition; }
  const WQuat GetRotationResult() const { return GetTransformation().m_qRotation; }

  virtual bool IsPickingSelectedAllowed() const override { return false; }

  /// Returns true if any of the 'align with' handles is selected, and thus the rotation of the dragged object should be modified as well
  bool ModifiesRotation() const { return m_bModifiesRotation; }

  virtual void UpdateStatusBarText(WQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const WTransform& transform) override;

  WEngineGizmoHandle m_hBobble;
  WEngineGizmoHandle m_hAlignPX;
  WEngineGizmoHandle m_hAlignNX;
  WEngineGizmoHandle m_hAlignPY;
  WEngineGizmoHandle m_hAlignNY;
  WEngineGizmoHandle m_hAlignPZ;
  WEngineGizmoHandle m_hAlignNZ;

  bool m_bModifiesRotation;
  WTime m_LastInteraction;
  WVec3 m_vStartPosition;
  WQuat m_qStartOrientation;
};
