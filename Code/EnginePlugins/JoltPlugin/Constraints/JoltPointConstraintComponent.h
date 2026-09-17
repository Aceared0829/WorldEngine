#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using WJoltPointConstraintComponentManager = WComponentManager<class WJoltPointConstraintComponent, WBlockStorageType::Compact>;

/// Implements a physics constraint that allows rotation around one point.
///
/// The joined actors can freely rotate around the constraint position.
class W_JOLTPLUGIN_DLL WJoltPointConstraintComponent : public WJoltConstraintComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltPointConstraintComponent, WJoltConstraintComponent, WJoltPointConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // WJoltConstraintComponent

protected:
  virtual void ApplySettings() override;
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual bool ExceededBreakingPoint() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltPointConstraintComponent

public:
  WJoltPointConstraintComponent();
  ~WJoltPointConstraintComponent();
};
