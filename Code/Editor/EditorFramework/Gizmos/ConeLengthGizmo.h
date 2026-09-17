#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WConeLengthGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WConeLengthGizmo, WGizmo);

public:
  WConeLengthGizmo();

  void SetRadius(float fRadius);
  float GetRadius() const { return m_fRadius; }

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

  WEngineGizmoHandle m_hConeRadius;

  enum class ManipulateMode
  {
    None,
    Radius
  };

  ManipulateMode m_ManipulateMode;

  float m_fRadius;
  float m_fRadiusScale;
};
