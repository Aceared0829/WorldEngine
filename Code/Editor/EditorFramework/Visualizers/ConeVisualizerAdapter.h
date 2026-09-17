#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct WGizmoEvent;

class WConeVisualizerAdapter : public WVisualizerAdapter
{
public:
  WConeVisualizerAdapter();
  ~WConeVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fFinalScale;
  float m_fAngleScale;
  WEngineGizmoHandle m_hGizmo;
};
