#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct WGizmoEvent;

class WCylinderVisualizerAdapter : public WVisualizerAdapter
{
public:
  WCylinderVisualizerAdapter();
  ~WCylinderVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fRadius;
  float m_fHeight;
  WVec3 m_vPositionOffset;
  WBitflags<WVisualizerAnchor> m_Anchor;
  WBasisAxis::Enum m_Axis;

  WEngineGizmoHandle m_hCylinder;
};
