#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class WCameraVisualizerAdapter : public WVisualizerAdapter
{
public:
  WCameraVisualizerAdapter();
  ~WCameraVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  WTransform m_LocalTransformFrustum;
  WTransform m_LocalTransformNearPlane;
  WTransform m_LocalTransformFarPlane;
  WEngineGizmoHandle m_hBoxGizmo;
  WEngineGizmoHandle m_hFrustumGizmo;
  WEngineGizmoHandle m_hNearPlaneGizmo;
  WEngineGizmoHandle m_hFarPlaneGizmo;
};
