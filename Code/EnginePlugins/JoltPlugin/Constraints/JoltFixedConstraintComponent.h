#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using WJoltFixedConstraintComponentManager = WComponentManager<class WJoltFixedConstraintComponent, WBlockStorageType::Compact>;

/// Implements a fixed physics constraint.
///
/// Actors constrained this way may not move apart, at all.
/// This is mainly useful for adding constraints dynamically, for example to attach a dynamic object to another one once it hits it,
/// or to make it breakable, such that it gets removed when too much force acts on it.
class W_JOLTPLUGIN_DLL WJoltFixedConstraintComponent : public WJoltConstraintComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltFixedConstraintComponent, WJoltConstraintComponent, WJoltFixedConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WJoltFixedConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltFixedConstraintComponent

public:
  WJoltFixedConstraintComponent();
  ~WJoltFixedConstraintComponent();
};
