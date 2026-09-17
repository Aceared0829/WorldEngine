#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/SphereGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct WGizmoEvent;

class WSphereManipulatorAdapter : public WManipulatorAdapter
{
public:
  WSphereManipulatorAdapter();
  ~WSphereManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const WGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  WSphereGizmo m_Gizmo;
};
