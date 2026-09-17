#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WBoxGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WBoxGizmo, WGizmo);

public:
  WBoxGizmo();

  void SetSize(const WVec3& vSize);

  const WVec3& GetSize() const { return m_vSize; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const WTransform& transform) override;


private:
  WTime m_LastInteraction;

  WVec2I32 m_vLastMousePos;

  WEngineGizmoHandle m_hCorners;
  WEngineGizmoHandle m_Edges[3];
  WEngineGizmoHandle m_Faces[3];

  enum class ManipulateMode
  {
    None,
    Uniform,
    AxisX,
    AxisY,
    AxisZ,
    PlaneXY,
    PlaneXZ,
    PlaneYZ,
  };

  ManipulateMode m_ManipulateMode;

  WVec3 m_vSize;
};
