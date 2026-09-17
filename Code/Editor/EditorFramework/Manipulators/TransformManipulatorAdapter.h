#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct WGizmoEvent;

class WTransformManipulatorAdapter : public WManipulatorAdapter
{
public:
  WTransformManipulatorAdapter();
  ~WTransformManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const WGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  WVec3 GetTranslation();
  WQuat GetRotation();
  WVec3 GetScale();

  virtual WTransform GetOffsetTransform() const override;

  WTranslateGizmo m_TranslateGizmo;
  WRotateGizmo m_RotateGizmo;
  WManipulatorScaleGizmo m_ScaleGizmo;
  WVec3 m_vOldScale;

  bool m_bHideTranslate = true;
  bool m_bHideRotate = true;
  bool m_bHideScale = true;
};
