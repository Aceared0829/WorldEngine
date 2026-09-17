#if 0

#  pragma once

#  include <JoltPlugin/Constraints/JoltConstraintComponent.h>

struct W_JOLTPLUGIN_DLL WJoltAxis
{
  using StorageType = WUInt8;

  enum Enum
  {
    None = 0,
    X = W_BIT(0),
    Y = W_BIT(1),
    Z = W_BIT(2),
    All = X | Y | Z,
    Default = All
  };

  struct Bits
  {
    StorageType X : 1;
    StorageType Y : 1;
    StorageType Z : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WJoltAxis);
W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltAxis);

using WJolt6DOFConstraintComponentManager = WComponentManager<class WJolt6DOFConstraintComponent, WBlockStorageType::Compact>;

class W_JOLTPLUGIN_DLL WJolt6DOFConstraintComponent : public WJoltConstraintComponent
{
  W_DECLARE_COMPONENT_TYPE(WJolt6DOFConstraintComponent, WJoltConstraintComponent, WJolt6DOFConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& stream) const override;
  virtual void DeserializeComponent(WWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // WJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;


  //////////////////////////////////////////////////////////////////////////
  // WJolt6DOFConstraintComponent

public:
  WJolt6DOFConstraintComponent();
  ~WJolt6DOFConstraintComponent();

  virtual void ApplySettings() final override;

  void SetFreeLinearAxis(WBitflags<WJoltAxis> flags);                         // [ property ]
  WBitflags<WJoltAxis> GetFreeLinearAxis() const { return m_FreeLinearAxis; } // [ property ]

  void SetFreeAngularAxis(WBitflags<WJoltAxis> flags);                          // [ property ]
  WBitflags<WJoltAxis> GetFreeAngularAxis() const { return m_FreeAngularAxis; } // [ property ]

  void SetLinearLimitMode(WJoltConstraintLimitMode::Enum mode);                           // [ property ]
  WJoltConstraintLimitMode::Enum GetLinearLimitMode() const { return m_LinearLimitMode; } // [ property ]

  void SetLinearRangeX(const WVec2& value);                        // [ property ]
  const WVec2& GetLinearRangeX() const { return m_vLinearRangeX; } // [ property ]
  void SetLinearRangeY(const WVec2& value);                        // [ property ]
  const WVec2& GetLinearRangeY() const { return m_vLinearRangeY; } // [ property ]
  void SetLinearRangeZ(const WVec2& value);                        // [ property ]
  const WVec2& GetLinearRangeZ() const { return m_vLinearRangeZ; } // [ property ]

  void SetLinearStiffness(float f);                               // [ property ]
  float GetLinearStiffness() const { return m_fLinearStiffness; } // [ property ]

  void SetLinearDamping(float f);                             // [ property ]
  float GetLinearDamping() const { return m_fLinearDamping; } // [ property ]

  void SetSwingLimitMode(WJoltConstraintLimitMode::Enum mode);                          // [ property ]
  WJoltConstraintLimitMode::Enum GetSwingLimitMode() const { return m_SwingLimitMode; } // [ property ]

  void SetSwingLimit(WAngle f);                         // [ property ]
  WAngle GetSwingLimit() const { return m_SwingLimit; } // [ property ]

  void SetSwingStiffness(float f);                              // [ property ]
  float GetSwingStiffness() const { return m_fSwingStiffness; } // [ property ]

  void SetSwingDamping(float f);                            // [ property ]
  float GetSwingDamping() const { return m_fSwingDamping; } // [ property ]

  void SetTwistLimitMode(WJoltConstraintLimitMode::Enum mode);                          // [ property ]
  WJoltConstraintLimitMode::Enum GetTwistLimitMode() const { return m_TwistLimitMode; } // [ property ]

  void SetLowerTwistLimit(WAngle f);                              // [ property ]
  WAngle GetLowerTwistLimit() const { return m_LowerTwistLimit; } // [ property ]

  void SetUpperTwistLimit(WAngle f);                              // [ property ]
  WAngle GetUpperTwistLimit() const { return m_UpperTwistLimit; } // [ property ]

  void SetTwistStiffness(float f);                              // [ property ]
  float GetTwistStiffness() const { return m_fTwistStiffness; } // [ property ]

  void SetTwistDamping(float f);                            // [ property ]
  float GetTwistDamping() const { return m_fTwistDamping; } // [ property ]

protected:
  WBitflags<WJoltAxis> m_FreeLinearAxis;

  WEnum<WJoltConstraintLimitMode> m_LinearLimitMode;

  float m_fLinearStiffness = 0.0f;
  float m_fLinearDamping = 0.0f;

  WVec2 m_vLinearRangeX = WVec2::MakeZero();
  WVec2 m_vLinearRangeY = WVec2::MakeZero();
  WVec2 m_vLinearRangeZ = WVec2::MakeZero();

  WBitflags<WJoltAxis> m_FreeAngularAxis;

  WEnum<WJoltConstraintLimitMode> m_SwingLimitMode;
  WAngle m_SwingLimit;

  float m_fSwingStiffness = 0.0f; // [ property ]
  float m_fSwingDamping = 0.0f;   // [ property ]

  WEnum<WJoltConstraintLimitMode> m_TwistLimitMode;
  WAngle m_LowerTwistLimit;
  WAngle m_UpperTwistLimit;

  float m_fTwistStiffness = 0.0f; // [ property ]
  float m_fTwistDamping = 0.0f;   // [ property ]
};

#endif
