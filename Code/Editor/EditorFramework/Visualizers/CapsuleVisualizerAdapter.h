#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct WGizmoEvent;

class WCapsuleVisualizerAdapter : public WVisualizerAdapter
{
public:
  WCapsuleVisualizerAdapter();
  ~WCapsuleVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fRadius = 0.0f;
  float m_fHeight = 0.0f;
  WBitflags<WVisualizerAnchor> m_Anchor;

  WEngineGizmoHandle m_hSphereTop;
  WEngineGizmoHandle m_hSphereBottom;
  WEngineGizmoHandle m_hCylinder;
};
