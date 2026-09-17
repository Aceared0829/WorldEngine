#pragma once

#include <EditorFramework/Gizmos/GizmoBase.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <QPoint>

class QWidget;
class WCamera;

class W_EDITORFRAMEWORK_DLL WOrthoGizmoContext : public WEditorInputContext
{
  W_ADD_DYNAMIC_REFLECTION(WOrthoGizmoContext, WEditorInputContext);

public:
  WOrthoGizmoContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView, const WCamera* pCamera);

  void SetWindowConfig(const WVec2I32& vViewport) { m_vViewport = vViewport; }

  virtual void FocusLost(bool bCancel);

  WEvent<const WGizmoEvent&> m_GizmoEvents;

  const WVec3& GetTranslationResult() const { return m_vTranslationResult; }
  const WVec3& GetTranslationDiff() const { return m_vTranslationDiff; }
  const WQuat& GetRotationResult() const { return m_qRotationResult; }
  float GetScalingResult() const { return m_fScalingResult; }

protected:
  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override {}

private:
  bool IsViewInOrthoMode() const;

  WVec2I32 m_vLastMousePos;
  WVec3 m_vUnsnappedTranslationResult;
  WVec3 m_vTranslationResult;
  WVec3 m_vTranslationDiff;
  WAngle m_UnsnappedRotationResult;
  WQuat m_qRotationResult;
  float m_fScaleMouseMove;
  float m_fScalingResult;
  float m_fUnsnappedScalingResult;
  bool m_bCanInteract;
  const WCamera* m_pCamera;
  WVec2I32 m_vViewport;
};
