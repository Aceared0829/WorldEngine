#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class WTerrainBrush3DVisualizerAdapter : public WVisualizerAdapter
{
public:
  WTerrainBrush3DVisualizerAdapter();
  ~WTerrainBrush3DVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  WEngineGizmoHandle m_hLinesInner;
  WEngineGizmoHandle m_hLinesOuter;
};
