#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using WJoltSliderConstraintComponentManager = WComponentManager<class WJoltSliderConstraintComponent, WBlockStorageType::Compact>;

/// Implements a sliding physics constraint.
///
/// The child actor may move along the parent actor along the positive X axis of the constraint.
/// Usually lower and upper limits are used to prevent infinite movement.
class W_JOLTPLUGIN_DLL WJoltSliderConstraintComponent : public WJoltConstraintComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltSliderConstraintComponent, WJoltConstraintComponent, WJoltSliderConstraintComponentManager);

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
  // WJoltSliderConstraintComponent

public:
  WJoltSliderConstraintComponent();
  ~WJoltSliderConstraintComponent();

  /// Enables a translational limit on the slider.
  void SetLimitMode(WJoltConstraintLimitMode::Enum mode);                     // [ property ]
  WJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  /// Sets how far child actor may move in one direction.
  void SetLowerLimitDistance(float f);                                  // [ property ]
  float GetLowerLimitDistance() const { return m_fLowerLimitDistance; } // [ property ]

  /// Sets how far child actor may move in the other direction.
  void SetUpperLimitDistance(float f);                                  // [ property ]
  float GetUpperLimitDistance() const { return m_fUpperLimitDistance; } // [ property ]

  /// Sets how difficult it is to move the child actor.
  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  /// Enables a drive for the slider to either constantly move or attempt to reach a certain position.
  void SetDriveMode(WJoltConstraintDriveMode::Enum mode);                     // [ property ]
  WJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  /// Sets the drive target position or velocity.
  void SetDriveTargetValue(float f);                                // [ property ]
  float GetDriveTargetValue() const { return m_fDriveTargetValue; } // [ property ]

  /// Sets how much force the drive may use to reach its target.
  void SetDriveStrength(float f);                             // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

protected:
  WEnum<WJoltConstraintLimitMode> m_LimitMode;
  float m_fLowerLimitDistance = 0;
  float m_fUpperLimitDistance = 0;
  float m_fFriction = 0;

  WEnum<WJoltConstraintDriveMode> m_DriveMode;
  float m_fDriveTargetValue;
  float m_fDriveStrength = 0; // 0 means maximum strength
};
