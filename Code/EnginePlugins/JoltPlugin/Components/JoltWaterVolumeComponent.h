#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

#include <Foundation/SimdMath/SimdNoise.h>

namespace JPH
{
  class PhysicsSystem;
} // namespace JPH

class W_JOLTPLUGIN_DLL WJoltWaterVolumeComponentManager : public WComponentManager<class WJoltWaterVolumeComponent, WBlockStorageType::FreeList>
{
public:
  WJoltWaterVolumeComponentManager(WWorld* pWorld);
  ~WJoltWaterVolumeComponentManager();

  void UpdateWaterVolumes(WTime deltaTime);

  WSimdPerlinNoise m_Noise;
};

//////////////////////////////////////////////////////////////////////////

/// Creates a water volume that applies a buoyancy force to submerged dynamic actors and triggers surface interactions.
///
/// This component needs a trigger component besides it to detect when actors enter or leave the water volume.
class W_JOLTPLUGIN_DLL WJoltWaterVolumeComponent : public WJoltShapeComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltWaterVolumeComponent, WJoltShapeComponent, WJoltWaterVolumeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltWaterVolumeComponent

public:
  WJoltWaterVolumeComponent();
  ~WJoltWaterVolumeComponent();

  WVec3 m_vExtents = WVec3(10.0f); // [ property ]

  /// Direction and speed of the water flow in local space.
  WVec3 m_vFlow = WVec3::MakeZero(); // [ property ]

  /// Strength of the noise that is added to vary the water surface height.
  float m_fNoiseStrength = 0.0f; // [ property ]

  /// The surface resource that defines the water surface interaction. No other properties of the surface are used.
  WSurfaceResourceHandle m_hSurface; // [ property ]

  /// Which interaction should be triggered when an actor enters the water volume. See WSurfaceResource.
  WHashedString m_sInteraction;                          // [ property ]

private:
  void OnMsgTriggerTriggered(WMsgTriggerTriggered& msg); // [ msg handler ]

  virtual void CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial) override;

  void Update(JPH::PhysicsSystem& joltSystem, WTime deltaTime);
  void UpdateWaterPlane(const WVec3& vGravity);

  WPlane m_SurfacePlane = WPlane::MakeFromNormalAndPoint(WVec3::MakeAxisZ(), WVec3::MakeZero());
  WVec3 m_vGravity = WVec3::MakeZero();
  float m_fNoiseTime = 0.0f;

  WHashSet<WComponentHandle> m_SubmergedActors;
};
