#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct WGizmoEvent;

class WSphereVisualizerAdapter : public WVisualizerAdapter
{
public:
  WSphereVisualizerAdapter();
  ~WSphereVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fScale;
  WVec3 m_vPositionOffset;
  WEngineGizmoHandle m_hGizmo;
  WBitflags<WVisualizerAnchor> m_Anchor;
};
