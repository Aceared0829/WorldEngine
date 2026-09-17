#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using WJoltConeConstraintComponentManager = WComponentManager<class WJoltConeConstraintComponent, WBlockStorageType::Compact>;

/// Implements a conical physics constraint.
///
/// The child actor can swing in a cone with a given angle around the anchor point on the parent actor.
class W_JOLTPLUGIN_DLL WJoltConeConstraintComponent : public WJoltConstraintComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltConeConstraintComponent, WJoltConstraintComponent, WJoltConeConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltConeConstraintComponent

public:
  WJoltConeConstraintComponent();
  ~WJoltConeConstraintComponent();

  void SetConeAngle(WAngle f);                        // [ property ]
  WAngle GetConeAngle() const { return m_ConeAngle; } // [ property ]

protected:
  WAngle m_ConeAngle;
};
