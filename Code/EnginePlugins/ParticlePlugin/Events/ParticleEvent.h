#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/ArrayPtr.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

/// Represents an event triggered by a particle system.
///
/// Particle events are raised during a particle's lifetime (e.g., on death, collision)
/// and can trigger reactions like spawning effects or prefabs.
struct W_PARTICLEPLUGIN_DLL WParticleEvent
{
  W_DECLARE_POD_TYPE();

  WTempHashedString m_EventType; ///< The type identifier for this event (e.g., "death", "collision")
  WVec3 m_vPosition;             ///< World-space position where the event occurred
  WVec3 m_vDirection;            ///< Direction vector associated with the event (e.g., movement direction)
  WVec3 m_vNormal;               ///< Surface normal at the event location (relevant for collision events)
};

/// Queue of particle events to be processed.
using WParticleEventQueue = WArrayPtr<WParticleEvent>;
