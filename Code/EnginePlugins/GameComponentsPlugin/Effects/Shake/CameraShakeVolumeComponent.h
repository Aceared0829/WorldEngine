#pragma once

#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

struct WMsgUpdateLocalBounds;
struct WMsgComponentInternalTrigger;
struct WMsgDeleteGameObject;

/// Base class for components that define volumes in which a camera shake effect shall be applied.
///
/// Derived classes implement different shape types and how the shake strength is calculated.
class W_GAMECOMPONENTS_DLL WCameraShakeVolumeComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WCameraShakeVolumeComponent, WComponent);

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
  // WCameraShakeVolumeComponent

public:
  WCameraShakeVolumeComponent();
  ~WCameraShakeVolumeComponent();

  /// The spatial category used to find camera shake volume components through the spatial system.
  static WSpatialData::Category SpatialDataCategory;

  /// How long a shake burst should last. Zero for constant shaking.
  WTime m_BurstDuration; // [ property ]

  /// How strong the shake should be at the strongest point. Typically a value between one and zero.
  float m_fStrength; // [ property ]

  /// Calculates the shake strength at the given global position.
  float ComputeForceAtGlobalPosition(const WSimdVec4f& vGlobalPos) const;

  /// Calculates the shake strength in local space of the component.
  virtual float ComputeForceAtLocalPosition(const WSimdVec4f& vLocalPos) const = 0;

  /// In case of a burst shake, defines whether the component should delete itself afterwards.
  WEnum<WOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

protected:
  void OnTriggered(WMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(WMsgDeleteGameObject& msg);
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using WCameraShakeVolumeSphereComponentManager = WComponentManager<class WCameraShakeVolumeSphereComponent, WBlockStorageType::Compact>;

/// A spherical volume in which a camera shake will be applied.
///
/// The shake strength is strongest at the center of the sphere and gradually weaker towards the sphere radius.
///
/// \see WCameraShakeVolumeComponent
/// \see WCameraShakeComponent
class W_GAMECOMPONENTS_DLL WCameraShakeVolumeSphereComponent : public WCameraShakeVolumeComponent
{
  W_DECLARE_COMPONENT_TYPE(WCameraShakeVolumeSphereComponent, WCameraShakeVolumeComponent, WCameraShakeVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WCameraShakeVolumeSphereComponent

public:
  WCameraShakeVolumeSphereComponent();
  ~WCameraShakeVolumeSphereComponent();

  virtual float ComputeForceAtLocalPosition(const WSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float fVal);                   // [ property ]

private:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);

  float m_fRadius = 1.0f;
  WSimdFloat m_fOneDivRadius;
};
