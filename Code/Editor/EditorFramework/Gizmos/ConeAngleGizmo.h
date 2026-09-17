#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WConeAngleGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WConeAngleGizmo, WGizmo);

public:
  WConeAngleGizmo();

  void SetAngle(WAngle angle);
  WAngle GetAngle() const { return m_Angle; }

  void SetRadius(float fRadius) { m_fRadius = fRadius; }

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

  WEngineGizmoHandle m_hConeAngle;

  enum class ManipulateMode
  {
    None,
    Angle,
  };

  ManipulateMode m_ManipulateMode;

  WAngle m_Angle;
  float m_fRadius;
  float m_fAngleScale;
};
