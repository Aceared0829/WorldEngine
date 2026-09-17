#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct WGizmoEvent;

class WNonUniformBoxManipulatorAdapter : public WManipulatorAdapter
{
public:
  WNonUniformBoxManipulatorAdapter();
  ~WNonUniformBoxManipulatorAdapter();

  virtual void QueryGridSettings(WGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const WGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  WNonUniformBoxGizmo m_Gizmo;
};
