#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

/// Visualizer adapter for WPointLightComponent.
///
/// Always shows a sphere gizmo at the light's range. If either Length or Radius is non-zero, also
/// shows a wireframe capsule so that tube (capsule) area lights are distinguishable from plain point lights.
class WPointLightVisualizerAdapter : public WVisualizerAdapter
{
public:
  WPointLightVisualizerAdapter();
  ~WPointLightVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fDisplayRange = 1.0f;
  float m_fLength = 0.0f;
  float m_fRadius = 0.0f;
  bool m_bIsTube = false;

  WEngineGizmoHandle m_hRangeGizmo;
  WEngineGizmoHandle m_hCapsuleL;
  WEngineGizmoHandle m_hCapsuleM;
  WEngineGizmoHandle m_hCapsuleR;
};
