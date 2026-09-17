#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct WGizmoEvent;

class WCapsuleManipulatorAdapter : public WManipulatorAdapter
{
public:
  WCapsuleManipulatorAdapter();
  ~WCapsuleManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const WGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  WCapsuleGizmo m_Gizmo;
};
