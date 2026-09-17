#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WSphereGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WSphereGizmo, WGizmo);

public:
  WSphereGizmo();

  void SetInnerSphere(bool bEnabled, float fRadius = 0.0f);
  void SetOuterSphere(float fRadius);

  float GetInnerRadius() const { return m_fRadiusInner; }
  float GetOuterRadius() const { return m_fRadiusOuter; }

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

  WEngineGizmoHandle m_hInnerSphere;
  WEngineGizmoHandle m_hOuterSphere;

  enum class ManipulateMode
  {
    None,
    InnerSphere,
    OuterSphere
  };

  ManipulateMode m_ManipulateMode;
  bool m_bInnerEnabled;

  float m_fRadiusInner;
  float m_fRadiusOuter;
};
