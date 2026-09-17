#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/ConeLengthGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct WGizmoEvent;

class WConeLengthManipulatorAdapter : public WManipulatorAdapter
{
public:
  WConeLengthManipulatorAdapter();
  ~WConeLengthManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const WGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  WConeLengthGizmo m_Gizmo;
};
