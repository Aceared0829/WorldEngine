#pragma once

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/Component.h>
#include <Core/World/SpatialData.h>
#include <Core/World/World.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

using WParticleAttractorComponentManager = WComponentManager<class WParticleAttractorComponent, WBlockStorageType::Compact>;

/// Defines an attractor (or repulsor) volume for nearby particles.
///
/// Particle systems with the "Attractors" behavior mode find these components via the spatial
/// system and apply their force per particle. All attractor components share the same spatial
/// category ("ParticleAttractor") so they can all be found with a single query.
///
/// Negative \a Strength values turn the attractor into a repulsor.
class W_PARTICLEPLUGIN_DLL WParticleAttractorComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WParticleAttractorComponent, WComponent, WParticleAttractorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WParticleAttractorComponent

public:
  WParticleAttractorComponent();
  ~WParticleAttractorComponent();

  /// Influence radius. Particles further away than this are not affected.
  float m_fRadius = 5.0f; // [ property ]

  /// Attraction force applied per second at full strength. Negative values repel.
  float m_fStrength = 5.0f; // [ property ]

  /// Minimum distance from the attractor center. Prevents singularity effects at the center.
  float m_fMinDistance = 0.1f; // [ property ]

  /// Particles that come closer than this distance are destroyed. Zero disables the kill zone.
  float m_fKillDistance = 0.0f; // [ property ]

  /// The spatial category shared by all particle attractor components.
  static WSpatialData::Category GetSpatialCategory();

protected:
  void OnMsgUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const; // [ msg handler ]
};
