#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct WMsgUpdateLocalBounds;
struct WMsgComponentInternalTrigger;
struct WMsgDeleteGameObject;

/// Base class for components that define wind volumes.
///
/// These components define the shape in which to apply wind to objects that support this functionality.
class W_GAMEENGINE_DLL WWindVolumeComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WWindVolumeComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WWindVolumeComponent

public:
  WWindVolumeComponent();
  ~WWindVolumeComponent();

  /// The spatial category to use to find all wind volume components through the spatial system.
  static WSpatialData::Category SpatialDataCategory;

  /// If non-zero, the wind will only last for a limited amount of time.
  WTime m_BurstDuration; // [ property ]

  /// How strong the wind shall blow at the strongest point of the volume.
  WEnum<WWindStrength> m_Strength; // [ property ]

  /// Factor to scale the wind strength. Negative values can be used to reverse the wind direction.
  float m_fStrengthFactor = 1.0f;

  /// Computes the wind force at a global position.
  ///
  /// Only the x,y,z components are used, they are a wind direction vector scaled to the wind speed.
  WSimdVec4f ComputeForceAtGlobalPosition(const WSimdVec4f& vGlobalPos) const;

  virtual WSimdVec4f ComputeForceAtLocalPosition(const WSimdVec4f& vLocalPos) const = 0;

  /// What happens after the wind burst is over.
  WEnum<WOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

protected:
  void OnTriggered(WMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(WMsgDeleteGameObject& msg);

  float GetWindInMetersPerSecond() const;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using WWindVolumeSphereComponentManager = WComponentManager<class WWindVolumeSphereComponent, WBlockStorageType::Compact>;

/// A spherical shape in which wind shall be applied to objects.
///
/// The wind blows outwards from the center of the sphere. If the wind direction is reversed, it pulls objects inwards.
class W_GAMEENGINE_DLL WWindVolumeSphereComponent : public WWindVolumeComponent
{
  W_DECLARE_COMPONENT_TYPE(WWindVolumeSphereComponent, WWindVolumeComponent, WWindVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WWindVolumeSphereComponent

public:
  WWindVolumeSphereComponent();
  ~WWindVolumeSphereComponent();

  virtual WSimdVec4f ComputeForceAtLocalPosition(const WSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float fVal);                   // [ property ]

private:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);

  float m_fRadius = 1.0f;
  WSimdFloat m_fOneDivRadius;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// How the wind direction shall be calculated in a cylindrical wind volume.
struct WWindVolumeCylinderMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Directional, ///< The wind direction is outwards from the cylinder.
    Vortex,      ///< The wind direction is tangential, moving in a circular fashion around the cylinder like in a tornado.

    Default = Directional
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WWindVolumeCylinderMode);

using WWindVolumeCylinderComponentManager = WComponentManager<class WWindVolumeCylinderComponent, WBlockStorageType::Compact>;

/// A cylindrical volume in which wind shall be applied.
///
/// The wind direction may be either outwards from the cylinder center, or tangential (a vortex).
class W_GAMEENGINE_DLL WWindVolumeCylinderComponent : public WWindVolumeComponent
{
  W_DECLARE_COMPONENT_TYPE(WWindVolumeCylinderComponent, WWindVolumeComponent, WWindVolumeCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WWindVolumeCylinderComponent

public:
  WWindVolumeCylinderComponent();
  ~WWindVolumeCylinderComponent();

  virtual WSimdVec4f ComputeForceAtLocalPosition(const WSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; }                   // [ property ]
  void SetRadius(float fVal);                                     // [ property ]

  float GetRadiusFalloff() const { return m_fRadiusFalloff; }     // [ property ]
  void SetRadiusFalloff(float fVal);                              // [ property ]

  float GetLength() const { return m_fLength; }                   // [ property ]
  void SetLength(float fVal);                                     // [ property ]

  float GetPositiveFalloff() const { return m_fPositiveFalloff; } // [ property ]
  void SetPositiveFalloff(float fVal);                            // [ property ]

  float GetNegativeFalloff() const { return m_fNegativeFalloff; } // [ property ]
  void SetNegativeFalloff(float fVal);                            // [ property ]

  WEnum<WWindVolumeCylinderMode> m_Mode;                        // [ property ]

private:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);

  void ComputeScaleBiasValues();

  float m_fRadius = 1.0f;
  float m_fRadiusFalloff = 0.0f;
  float m_fLength = 5.0f;
  float m_fPositiveFalloff = 0.0f;
  float m_fNegativeFalloff = 0.0f;

  WSimdVec4f m_vScaleValues;
  WSimdVec4f m_vBiasValues;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using WWindVolumeConeComponentManager = WComponentManager<class WWindVolumeConeComponent, WBlockStorageType::Compact>;

/// A conical shape in which wind shall be applied to objects.
///
/// The wind is applied from the tip of the cone along the cone axis.
/// Strength falloff is only by distance along the cone main axis.
class W_GAMEENGINE_DLL WWindVolumeConeComponent : public WWindVolumeComponent
{
  W_DECLARE_COMPONENT_TYPE(WWindVolumeConeComponent, WWindVolumeComponent, WWindVolumeConeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WWindVolumeCylinderComponent

public:
  WWindVolumeConeComponent();
  ~WWindVolumeConeComponent();

  virtual WSimdVec4f ComputeForceAtLocalPosition(const WSimdVec4f& vLocalPos) const override;

  float GetLength() const { return m_fLength; } // [ property ]
  void SetLength(float fVal);                   // [ property ]

  WAngle GetAngle() const { return m_Angle; }  // [ property ]
  void SetAngle(WAngle val);                   // [ property ]

private:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);

  float m_fLength = 1.0f;
  WAngle m_Angle = WAngle::MakeFromDegree(45);
};
