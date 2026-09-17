#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct WGizmoEvent;

class WBoxManipulatorAdapter : public WManipulatorAdapter
{
public:
  WBoxManipulatorAdapter();
  ~WBoxManipulatorAdapter();

  virtual void QueryGridSettings(WGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const WGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  WVec3 m_vPositionOffset;
  WQuat m_qRotation;
  WNonUniformBoxGizmo m_Gizmo;

  WVec3 m_vOldSize;
};
