#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// The click gizmo displays a simple shape that can be clicked.
///
/// This can be used to provide the user with a way to select which part to edit further.
class W_EDITORFRAMEWORK_DLL WClickGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WClickGizmo, WGizmo);

public:
  WClickGizmo();

  void SetColor(const WColor& color);

protected:
  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;

  virtual void DoFocusLost(bool bCancel) override;
  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const WTransform& transform) override;

private:
  WEngineGizmoHandle m_hShape;
};
