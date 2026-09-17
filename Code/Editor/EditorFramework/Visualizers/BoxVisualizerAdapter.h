#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct WGizmoEvent;

class WBoxVisualizerAdapter : public WVisualizerAdapter
{
public:
  WBoxVisualizerAdapter();
  ~WBoxVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  WVec3 m_vScale;
  WVec3 m_vPositionOffset;
  WQuat m_qRotation;
  WBitflags<WVisualizerAnchor> m_Anchor;
  WEngineGizmoHandle m_hGizmo;
};
