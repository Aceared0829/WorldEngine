#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using WJoltHingeConstraintComponentManager = WComponentManager<class WJoltHingeConstraintComponent, WBlockStorageType::Compact>;

/// Implements a rotational physics constraint.
///
/// Hinge constraints are typically used for doors and wheels. They may either rotate freely
/// or be limited between an upper and lower angle.
/// It is possible to enable a drive to make the hinge rotate at a certain speed, or return to a desired angle.
class W_JOLTPLUGIN_DLL WJoltHingeConstraintComponent : public WJoltConstraintComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltHingeConstraintComponent, WJoltConstraintComponent, WJoltHingeConstraintComponentManager);

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
  // WJoltHingeConstraintComponent

public:
  WJoltHingeConstraintComponent();
  ~WJoltHingeConstraintComponent();

  /// Enables a rotational limit on the hinge.
  void SetLimitMode(WJoltConstraintLimitMode::Enum mode);                     // [ property ]
  WJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  /// Sets how far the hinge may rotate in one direction.
  void SetLowerLimitAngle(WAngle f);                         // [ property ]
  WAngle GetLowerLimitAngle() const { return m_LowerLimit; } // [ property ]

  /// Sets how far the hinge may rotate in the other direction.
  void SetUpperLimitAngle(WAngle f);                         // [ property ]
  WAngle GetUpperLimitAngle() const { return m_UpperLimit; } // [ property ]

  /// Sets how difficult it is to rotate the hinge.
  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  /// Enables a drive for the hinge to either constantly rotate or attempt to reach a certain rotation angle.
  void SetDriveMode(WJoltConstraintDriveMode::Enum mode);                     // [ property ]
  WJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  /// Sets the drive target angle or velocity.
  void SetDriveTargetValue(WAngle f);                               // [ property ]
  WAngle GetDriveTargetValue() const { return m_DriveTargetValue; } // [ property ]

  /// Sets how much force the drive may use to reach its target.
  void SetDriveStrength(float f);                             // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

protected:
  WEnum<WJoltConstraintLimitMode> m_LimitMode;
  WAngle m_LowerLimit;
  WAngle m_UpperLimit;
  float m_fFriction = 0;
  WEnum<WJoltConstraintDriveMode> m_DriveMode;
  WAngle m_DriveTargetValue;
  float m_fDriveStrength = 0; // 0 means maximum strength
};
