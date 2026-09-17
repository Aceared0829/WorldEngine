#pragma once

#include <Core/World/World.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>

struct WMsgMoveCharacterController;
struct WMsgUpdateLocalBounds;

namespace JPH
{
  class CharacterVirtual;
  class TempAllocator;
} // namespace JPH

W_DECLARE_FLAGS(WUInt32, WJoltCharacterDebugFlags, PrintState, VisShape, VisContacts, VisCasts, VisGroundContact, VisFootCheck);
W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltCharacterDebugFlags);

/// Base class for character controllers (CC).
///
/// This class provides general functionality for building a character controller.
/// It tries not to implement things that are game specific.
/// It is assumed that most games implement their own character controller to be able to build very specific behavior.
/// The WJoltDefaultCharacterComponent is an example implementation that shows how this can be achieved on top of this class.
class W_JOLTPLUGIN_DLL WJoltCharacterControllerComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WJoltCharacterControllerComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltCharacterControllerComponent

public:
  WJoltCharacterControllerComponent();
  ~WJoltCharacterControllerComponent();

  /// Describes a point where the CC collided with other geometry.
  struct ContactPoint
  {
    float m_fCastFraction = 0.0f;
    float m_fPenetrationDepth = 0.0f;
    WVec3 m_vPosition;
    WVec3 m_vSurfaceNormal;
    WVec3 m_vContactNormal;
    JPH::BodyID m_BodyID;
    JPH::SubShapeID m_SubShapeID;
  };

  /// The CC will move through the given physics body.
  ///
  /// Currently only one such object can be set. This is mainly used to ignore an object that the player is currently carrying,
  /// so that there are no unintended collisions.
  ///
  /// Call ClearObjectToIgnore() to re-enable collisions.
  void SetObjectToIgnore(WUInt32 uiObjectFilterID);

  /// \see SetObjectToIgnore()
  void ClearObjectToIgnore();

public:
  /// The collision layer determines with which other actors this actor collides. \see WJoltActorComponent
  WUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// In case a 'presence shape' is used, this defines which geometry the presence bodies collides with.
  WUInt8 m_uiPresenceCollisionLayer = 0; // [ property ]

  /// What aspects of the CC to visualize.
  WBitflags<WJoltCharacterDebugFlags> m_DebugFlags; // [ property ]

  /// The maximum slope that the character can walk up.
  void SetMaxClimbingSlope(WAngle slope);                           // [ property ]
  WAngle GetMaxClimbingSlope() const { return m_MaxClimbingSlope; } // [ property ]

  WUInt8 m_uiWeightCategory = 0;                                    // [ property ]
  float m_fWeightMass = 50.0f;                                       // [ property ]
  float m_fWeightScale = 1.0f;                                       // [ property ]

  /// The strength with which the character will push against objects that it is running into.
  void SetStrength(float fStrength);                        // [ property ]
  float GetStrength() const { return m_fStrength; }         // [ property ]

  float GetMass() const { return m_fMass; }

private:
  WAngle m_MaxClimbingSlope = WAngle::MakeFromDegree(45); // [ property ]
  float m_fStrength = 500.0f;                               // [ property ]

  float GetWeight_Mass() const { return m_fWeightMass; }
  float GetWeight_Scale() const { return m_fWeightScale; }
  void SetWeight_Mass(float fValue) { m_fWeightMass = fValue; }
  void SetWeight_Scale(float fValue) { m_fWeightScale = fValue; }

protected:
  /// Returns the time delta to use for updating the character. This may differ from the world delta.
  W_ALWAYS_INLINE float GetUpdateTimeDelta() const { return m_fUpdateTimeDelta; }

  /// Returns the inverse of update time delta.
  W_ALWAYS_INLINE float GetInverseUpdateTimeDelta() const { return m_fInverseUpdateTimeDelta; }

  /// Returns the shape that the character is supposed to use next.
  ///
  /// The desired target state (radius, height, etc) has to be stored somewhere else (e.g. as members in derived classes).
  /// The shape can be cached.
  /// The shape may not get applied to the character, in case this is used by things like TryResize and the next shape is
  /// determined to not fit.
  virtual JPH::Ref<JPH::Shape> MakeNextCharacterShape() = 0;

  /// Returns the radius of the shape. This never changes at runtime.
  virtual float GetShapeRadius() const = 0;

  /// Called up to once per frame, but potentially less often, if physics updates were skipped due to high framerates.
  ///
  /// All shape modifications and moves should only be executed during this step.
  /// The given deltaTime should be used, rather than the world's time diff.
  virtual void UpdateCharacter() = 0;

  /// Gives access to the internally used JPH::CharacterVirtual.
  JPH::CharacterVirtual* GetJoltCharacter() { return m_pCharacter; }
  const JPH::CharacterVirtual* GetJoltCharacter() const { return m_pCharacter; }

  /// Attempts to change the character shape to the new one. Fails if the new shape overlaps with surrounding geometry.
  WResult TryChangeShape(JPH::Shape* pNewShape);

  /// Moves the character using the given velocity and timestep, making it collide with and slide along obstacles.
  void RawMoveWithVelocity(const WVec3& vVelocity, float fMaxStairStepUp, float fMaxStepDown);

  /// Variant of RawMoveWithVelocity() that takes a direction vector instead.
  void RawMoveIntoDirection(const WVec3& vDirection);

  /// Variant of RawMoveWithVelocity() that takes a target position instead.
  void RawMoveToPosition(const WVec3& vTargetPosition);

  /// Teleports the character to the destination position, even if it would get stuck there.
  void TeleportToPosition(const WVec3& vGlobalFootPos);

  /// If the CC is slightly above the ground, this will move it down so that it touches the ground.
  ///
  /// If within the max distance no ground contact is found, the function does nothing and returns false.
  bool StickToGround(float fMaxDist);

  /// Gathers all contact points that are found by sweeping the shape along a direction
  void CollectCastContacts(WDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const WVec3& vQueryPosition, const WQuat& qQueryRotation, const WVec3& vSweepDir) const;

  /// Gathers all contact points of the shape at the target position.
  ///
  /// Use fMaxSeparationDistance > 0 (e.g. 0.02f) to find contacts with walls/ground that the shape is touching but not penetrating.
  void CollectContacts(WDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const WVec3& vQueryPosition, const WQuat& qQueryRotation, float fMaxSeparationDistance) const;

  /// Detects the velocity at the contact point. If it is a dynamic body, a force pushing it away is applied.
  ///
  /// This is mainly used to get the velocity of the kinematic object that a character is standing on.
  /// It can then be incorporated into the movement, such that the character rides along.
  /// If the body at the contact point is dynamic, optionally a force can be applied, simulating that the character's
  /// weight pushes down on it.
  WVec3 GetContactVelocityAndPushAway(const ContactPoint& contact, float fPushForce);

  /// Spawns a surface interaction prefab at the given contact point.
  ///
  /// hFallbackSurface is used, if no other surface could be determined from the contact point.
  void SpawnContactInteraction(const ContactPoint& contact, const WHashedString& sSurfaceInteraction, WSurfaceResourceHandle hFallbackSurface, const WVec3& vInteractionNormal = WVec3(0, 0, 1));

  /// Debug draws the contact point.
  void VisualizeContact(const ContactPoint& contact, const WColor& color) const;

  /// Debug draws all the contact points.
  void VisualizeContacts(const WDynamicArray<ContactPoint>& contacts, const WColor& color) const;

private:
  friend class WJoltWorldModule;

  void Update(WTime deltaTime);

  float m_fMass = 50.0f;
  float m_fUpdateTimeDelta = 0.1f;
  float m_fInverseUpdateTimeDelta = 1.0f;
  JPH::CharacterVirtual* m_pCharacter = nullptr;

  void CreatePresenceBody();
  void RemovePresenceBody();
  void MovePresenceBody(WTime deltaTime);

  WUInt32 m_uiPresenceBodyID = WInvalidIndex;

  WJoltBodyFilter m_BodyFilter;
  WUInt32 m_uiUserDataIndex = WInvalidIndex;
};
