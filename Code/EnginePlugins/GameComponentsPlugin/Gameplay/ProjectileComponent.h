#pragma once

#include <Core/Interfaces/PhysicsQuery.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

struct WMsgComponentInternalTrigger;

using WProjectileComponentManager = WComponentManagerSimple<class WProjectileComponent, WComponentUpdateType::WhenSimulating>;

/// Defines what a projectile will do when it hits a surface
struct W_GAMECOMPONENTS_DLL WProjectileReaction
{
  using StorageType = WInt8;

  enum Enum : StorageType
  {
    Absorb,      ///< The projectile simply stops and is deleted
    Reflect,     ///< Bounces away along the reflected direction. Maintains momentum.
    Bounce,      ///< Bounces away along the reflected direction. Loses momentum.
    Attach,      ///< Stops at the hit point, does not continue further and attaches itself as a child to the hit object
    PassThrough, ///< Continues flying through the geometry (but may spawn prefabs at the intersection points)

    Default = Absorb
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMECOMPONENTS_DLL, WProjectileReaction);

/// Defines how the projectile owner orientation is updated on reflection / bounce.
struct W_GAMECOMPONENTS_DLL WProjectileBounceOrientation
{
  using StorageType = WInt8;

  enum Enum : StorageType
  {
    Spinning = 0,
    Reflection = 1,

    Default = Reflection
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMECOMPONENTS_DLL, WProjectileBounceOrientation);

/// Holds the information about how a projectile interacts with a specific surface type
struct W_GAMECOMPONENTS_DLL WProjectileSurfaceInteraction
{
  /// The surface type (and derived ones) for which this interaction is used
  WSurfaceResourceHandle m_hSurface;

  /// How the projectile itself will react when hitting the surface type
  WProjectileReaction::Enum m_Reaction;

  /// Which interaction should be triggered. See WSurfaceResource.
  WString m_sInteraction;

  /// Which impulse type to use.
  WUInt8 m_uiImpulseType = 0;

  /// The force (or rather impulse) that is applied on the object
  float m_fImpulse = 0.0f;

  /// How much damage to do on this type of surface. Send via WMsgDamage
  float m_fDamage = 0.0f;

  /// How much the rotation (spinning) of the owner object is affected by reflections/bounces about the surface
  /// Positive values correspond to the rotation due to object "getting stuck in the surface"
  /// Negative values correspond to the rotation due to object "sliding over the surface"
  /// Larger by magnitude values correspond to smaller rotations.
  /// If zero, than the rotation of the owner is not changed during reflection/bounces
  /// Generally, it is an analogue of mass but for rotational movement
  float m_fInertiaRatio = 5.0f;
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMECOMPONENTS_DLL, WProjectileSurfaceInteraction);

/// Shoots a game object in a straight line and uses physics raycasts to detect hits.
///
/// When a raycast detects a hit, the surface information is used to determine how the projectile should proceed
/// and which prefab it should spawn as an effect.
class W_GAMECOMPONENTS_DLL WProjectileComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WProjectileComponent, WComponent, WProjectileComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // WProjectileComponent

public:
  WProjectileComponent();
  ~WProjectileComponent();

  /// The speed at which the projectile flies.
  float m_fMetersPerSecond; // [ property ]

  /// If 0, the projectile is not affected by gravity.
  float m_fGravityMultiplier; // [ property ]

  // If true the death prefab will be spawned when the velocity goes under the threshold to be considered static
  bool m_bSpawnPrefabOnStatic; // [ property ]

  /// Defines which other physics objects the projectile will collide with.
  WUInt8 m_uiCollisionLayer; // [ property ]

  /// If greater than zero, a sphere shape query is used for collision detection
  /// otherwise raycasting is used and projectile is treated like a "dot"
  float m_fRadius; // [ property ]

  /// Defines how reflections / bounces rotate the owner object.
  WProjectileBounceOrientation::Enum m_BounceOrientation = WProjectileBounceOrientation::Reflection; // [ property ]

  /// Velocity ratio below which a bounced projectile is considered static.
  float m_fStaticVelocityRatio = 0.05f; // [ property ]

  /// A broad filter to ignore certain types of colliders.
  WBitflags<WPhysicsShapeType> m_ShapeTypesToHit; // [ property ]

  /// After this time the projectile is removed, if it didn't hit anything yet.
  WTime m_MaxLifetime; // [ property ]

  /// If the projectile hits something that has no valid surface, this surface is used instead.
  WSurfaceResourceHandle m_hFallbackSurface; // [ property ]

  /// Specifies how the projectile interacts with different surface types.
  WHybridArray<WProjectileSurfaceInteraction, 12> m_SurfaceInteractions; // [ property ]

  /// If the projectile reaches its maximum lifetime it can spawn this prefab.
  WPrefabResourceHandle m_hDeathPrefab;           // [ property ]

  void SetFallbackSurfaceFile(WStringView sFile); // [ property ]
  WStringView GetFallbackSurfaceFile() const;     // [ property ]

private:
  void Update();
  void OnTriggered(WMsgComponentInternalTrigger& msg); // [ msg handler ]

  void SpawnDeathPrefab();
  void ApplyReflectionRotation(const WVec3& vCurDirection, const WVec3& vSurfaceNormal);
  void ApplySpinningRotation(const WProjectileSurfaceInteraction& interaction, const WPhysicsCastResult& castResult, const WVec3& vPositionOnReflection, const WVec3& vCurDirection, const WVec3& vNewVelocity);
  bool QueryCollision(const WPhysicsWorldModuleInterface& physicsInterface, WPhysicsCastResult& out_result, const WVec3& vStart, const WVec3& vDirection, float fDistance, const WPhysicsQueryParameters& queryParams) const;
  bool ShouldStopProjectile(const WPhysicsWorldModuleInterface& physicsInterface, const WPhysicsCastResult& castResult, const WVec3& vVelocity);


  /// If an unknown surface type is hit, the projectile will just delete itself without further interaction
  WInt32 FindSurfaceInteraction(const WSurfaceResourceHandle& hSurface) const;

  void TriggerSurfaceInteraction(const WSurfaceResourceHandle& hSurface, WGameObjectHandle hObject, const WVec3& vPos, const WVec3& vNormal, const WVec3& vDirection, const char* szInteraction);

  WVec3 m_vVelocity;
};
