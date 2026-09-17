#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct WGizmoEvent;

class WDirectionVisualizerAdapter : public WVisualizerAdapter
{
public:
  WDirectionVisualizerAdapter();
  ~WDirectionVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  WEngineGizmoHandle m_hGizmo;
};
