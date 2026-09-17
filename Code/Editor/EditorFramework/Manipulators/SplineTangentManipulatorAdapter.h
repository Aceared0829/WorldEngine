#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Core/Graphics/Spline.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct WGizmoEvent;

/// Makes a spline tangent editable in the editor.
///
/// Enabled by attaching the WSplineTangentManipulatorAttribute.
class WSplineTangentManipulatorAdapter : public WManipulatorAdapter
{
public:
  WSplineTangentManipulatorAdapter();
  ~WSplineTangentManipulatorAdapter();

protected:
  virtual void Finalize() override;

  virtual void Update() override;
  void TangentGizmoEventHandler(const WGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  void BuildSpline();
  void ConfigureGizmos();

  bool CustomTangentsLinked() const;

  WSpline m_Spline;
  WUInt32 m_uiNodeIndex = WInvalidIndex;
  bool m_bIsTangentIn = false;

  WVec3 m_vLastTangent;

  WRotateGizmo m_RotateGizmo;
  WScaleGizmo m_ScaleGizmo;
};
