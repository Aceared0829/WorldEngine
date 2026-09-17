#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/ClickGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

struct WGizmoEvent;

/// Makes an array of WExposedBone properties editable in the viewport
///
/// Enabled by attaching the WBoneManipulatorAttribute.
class WBoneManipulatorAdapter : public WManipulatorAdapter
{
public:
  WBoneManipulatorAdapter();
  ~WBoneManipulatorAdapter();

protected:
  virtual void Finalize() override;

  void MigrateSelection();

  virtual void Update() override;
  void RotateGizmoEventHandler(const WGizmoEvent& e);
  void ClickGizmoEventHandler(const WGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  struct ElementGizmo
  {
    WMat4 m_Offset;
    WMat4 m_InverseOffset;
    WRotateGizmo m_RotateGizmo;
    WClickGizmo m_ClickGizmo;
  };

  WVariantArray m_Keys;
  WDynamicArray<WExposedBone> m_Bones;
  WDeque<ElementGizmo> m_Gizmos;
  WTransform m_RootTransform = WTransform::MakeIdentity();

  void RetrieveBones();
  void ConfigureGizmos();
  void SetTransform(WUInt32 uiBone, const WTransform& value);
  WMat4 ComputeFullTransform(WUInt32 uiBone) const;
  WMat4 ComputeParentTransform(WUInt32 uiBone) const;

  static WString s_sLastSelectedBone;
};
