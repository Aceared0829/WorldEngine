#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using WJoltSwingTwistConstraintComponentManager = WComponentManager<class WJoltSwingTwistConstraintComponent, WBlockStorageType::Compact>;

/// Implements a swing-twist physics constraint.
///
/// This is similar to a cone constraint but with more control.
/// The swing angle can be limited along Y and Z, so it can have a squashed shape, rather than perfectly round.
/// Additionally it can be limited how far the child actor may twist around the main axis.
class W_JOLTPLUGIN_DLL WJoltSwingTwistConstraintComponent : public WJoltConstraintComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltSwingTwistConstraintComponent, WJoltConstraintComponent, WJoltSwingTwistConstraintComponentManager);

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
  // WJoltSwingTwistConstraintComponent

public:
  WJoltSwingTwistConstraintComponent();
  ~WJoltSwingTwistConstraintComponent();

  /// Sets how far the child actor may swing along the Y axis.
  void SetSwingLimitY(WAngle f);                          // [ property ]
  WAngle GetSwingLimitY() const { return m_SwingLimitY; } // [ property ]

  /// Sets how far the child actor may swing along the Z axis.
  void SetSwingLimitZ(WAngle f);                          // [ property ]
  WAngle GetSwingLimitZ() const { return m_SwingLimitZ; } // [ property ]

  /// Sets how difficult it is to rotate the constraint.
  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  /// Sets how far the child actor can rotate around the twist axis in one direction.
  void SetLowerTwistLimit(WAngle f);                              // [ property ]
  WAngle GetLowerTwistLimit() const { return m_LowerTwistLimit; } // [ property ]

  /// Sets how far the child actor can rotate around the twist axis in the other direction.
  void SetUpperTwistLimit(WAngle f);                              // [ property ]
  WAngle GetUpperTwistLimit() const { return m_UpperTwistLimit; } // [ property ]

  // void SetTwistDriveMode(WJoltConstraintDriveMode::Enum mode);                          // [ property ]
  // WJoltConstraintDriveMode::Enum GetTwistDriveMode() const { return m_TwistDriveMode; } // [ property ]

  // void SetTwistDriveTargetValue(WAngle f);                                    // [ property ]
  // WAngle GetTwistDriveTargetValue() const { return m_TwistDriveTargetValue; } // [ property ]

  // void SetTwistDriveStrength(float f);                                  // [ property ]
  // float GetTwistDriveStrength() const { return m_fTwistDriveStrength; } // [ property ]

protected:
  WAngle m_SwingLimitY;
  WAngle m_SwingLimitZ;

  float m_fFriction = 0.0f;

  WAngle m_LowerTwistLimit = WAngle::MakeFromDegree(90);
  WAngle m_UpperTwistLimit = WAngle::MakeFromDegree(90);

  // not sure whether these are useful
  // maybe just expose an 'untwist' feature, with strength/frequency and drive to position 0 ?
  // driving to velocity makes no sense, since the constraint always has a lower/upper twist limit
  // probably would need a 6DOF joint for more advanced use cases
  // WEnum<WJoltConstraintDriveMode> m_TwistDriveMode;
  // WAngle m_TwistDriveTargetValue;
  // float m_fTwistDriveStrength = 0; // 0 means maximum strength
};
