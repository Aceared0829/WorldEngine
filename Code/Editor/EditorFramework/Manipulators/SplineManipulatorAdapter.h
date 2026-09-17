#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Core/Graphics/Spline.h>
#include <EditorFramework/Gizmos/ClickGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct WGizmoEvent;

/// Makes spline nodes editable in the editor by providing small helper gizmos to easily add new nodes.
///
/// Enabled by attaching the WSplineManipulatorAttribute.
class WSplineManipulatorAdapter : public WManipulatorAdapter
{
public:
  WSplineManipulatorAdapter();
  ~WSplineManipulatorAdapter();

  static WResult BuildSpline(const WDocumentObject* pSplineComponent, WStringView sClosedPropertyName, WSpline& out_spline, WStringView sNodeName = WStringView(), WUInt32* out_pNodeIndex = nullptr);
  static WResult FillControlPointFromNodeComponent(const WDocumentObject* pNodeComponent, WSpline::ControlPoint& out_cp);

protected:
  virtual void Finalize() override;

  virtual void Update() override;
  void ClickGizmoEventHandler(const WGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  void BuildSpline();
  void ConfigureGizmos();
  void MakeUniqueName(WInt32 iIndex, WStringBuilder& ref_sName);

  WSpline m_Spline;

  WDeque<WClickGizmo> m_Gizmos;
};
