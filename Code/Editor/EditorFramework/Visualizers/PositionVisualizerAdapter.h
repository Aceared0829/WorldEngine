#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class W_EDITORFRAMEWORK_DLL WPositionVisualizerAdapter : public WVisualizerAdapter
{
public:
  WPositionVisualizerAdapter();
  ~WPositionVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  virtual void UpdateGizmoTransform() override;

  float m_fScale = 0.1f;
  WVec3 m_vPosition;
  WEngineGizmoHandle m_hGizmo;
};
