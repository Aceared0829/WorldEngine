#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/ConeAngleGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct WGizmoEvent;

class WConeAngleManipulatorAdapter : public WManipulatorAdapter
{
public:
  WConeAngleManipulatorAdapter();
  ~WConeAngleManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const WGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  WConeAngleGizmo m_Gizmo;
};
