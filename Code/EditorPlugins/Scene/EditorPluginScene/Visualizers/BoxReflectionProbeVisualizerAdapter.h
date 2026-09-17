#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class WBoxReflectionProbeVisualizerAdapter : public WVisualizerAdapter
{
public:
  WBoxReflectionProbeVisualizerAdapter();
  ~WBoxReflectionProbeVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  WVec3 m_vScale;
  WVec3 m_vPositionOffset;
  WQuat m_qRotation;

  WEngineGizmoHandle m_hGizmo;
};
