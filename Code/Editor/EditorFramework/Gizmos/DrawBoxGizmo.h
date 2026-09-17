#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_EDITORFRAMEWORK_DLL WDrawBoxGizmo : public WGizmo
{
  W_ADD_DYNAMIC_REFLECTION(WDrawBoxGizmo, WGizmo);

public:
  enum class ManipulateMode
  {
    None,
    DrawBase,
    DrawHeight,
  };

  WDrawBoxGizmo();
  ~WDrawBoxGizmo();

  void GetResult(WVec3& out_vOrigin, float& out_fSizeNegX, float& out_fSizePosX, float& out_fSizeNegY, float& out_fSizePosY, float& out_fSizeNegZ,
    float& out_fSizePosZ) const;

  ManipulateMode GetCurrentMode() const { return m_ManipulateMode; }
  const WVec3& GetStartPosition() const { return m_vFirstCorner; }

  virtual void UpdateStatusBarText(WQtEngineDocumentWindow* pWindow) override;

  bool GetDisplayGrid() const { return m_bDisplayGrid; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual WEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual WEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const WTransform& transform) override;

private:
  void SwitchMode(bool bCancel);
  void UpdateBox();
  void DisableGrid(bool bControlPressed);
  void UpdateGrid(QMouseEvent* e);
  bool PickPosition(QMouseEvent* e);

  ManipulateMode m_ManipulateMode;
  WEngineGizmoHandle m_hBox;

  WInt32 m_iHeightChange = 0;
  WVec2I32 m_vLastMousePos;
  WVec3 m_vCurrentPosition;
  WVec3 m_vFirstCorner;
  WVec3 m_vSecondCorner;
  WVec3 m_vUpAxis;
  WVec3 m_vLastStartPoint;
  float m_fBoxHeight = 0.5f;
  float m_fOriginalBoxHeight = 0.5f;
  bool m_bDisplayGrid = false;
};
