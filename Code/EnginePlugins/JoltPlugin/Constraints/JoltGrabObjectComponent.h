#pragma once

#include <Foundation/Communication/Message.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>

namespace JPH
{
  class SixDOFConstraint;
}

using WJoltGrabObjectComponentManager = WComponentManagerSimple<class WJoltGrabObjectComponent, WComponentUpdateType::WhenSimulating, WBlockStorageType::Compact>;

/// Used to 'grab' physical objects and attach them to an object. For player objects to pick up objects.
///
/// The component does a raycast along its X axis to detect nearby physics objects. If it finds a non-kinematic WJoltDynamicActor
/// it connects a dedicated object with the picked object through a 6DOF joint, which is set up to drag the picked object towards its
/// position and rotation.
/// The grabbed object can be dropped or thrown away.
///
/// If the picked object has a WGrabbableItemComponent, the custom grab points are used to determine how to grab the object.
class W_JOLTPLUGIN_DLL WJoltGrabObjectComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltGrabObjectComponent, WComponent, WJoltGrabObjectComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltGrabObjectComponent

public:
  WJoltGrabObjectComponent();
  ~WJoltGrabObjectComponent();

  /// Checks whether there is an object nearby. Note that this function reports static and dynamic objects that are within reach.
  /// Whether these objects are interact able or not is up to the caller.
  bool FindNearbyObject(WGameObject*& out_pObject, WTransform& out_localGrabPoint, bool bIgnoreGrabbedActor = true) const;

  /// Grabs the given object at the given grab point if possible.
  bool GrabObject(WGameObject* pObjectToGrab, const WTransform& localGrabPoint);

  /// Tries to find an object to pick up and do so.
  bool GrabNearbyObject(); // [ scriptable ]

  /// Returns whether an object is currently being held.
  bool HasObjectGrabbed() const; // [ scriptable ]

  /// Returns the grabbed object's actor component.
  WComponentHandle GetGrabbedActor() const { return m_hGrabbedActor; }

  /// Returns the grabbed object's mass.
  float GetGrabbedActorMass() const { return m_fGrabbedActorInverseMass > 0.0f ? 1.0f / m_fGrabbedActorInverseMass : 0.0f; }

  /// The grabbed object is dropped in place.
  ///
  /// If an impulse type is given (see WImpulseTypeConfig) the dropped object is allowed to retain as much linear velocity
  /// as a push with such a force would give it.
  /// E.g. if you pass in the same impulse type as in ThrowGrabbedObject(), releasing a grabbed object while
  /// rotating, would allow to throw it as far as if you had actually "thrown" the object.
  /// If any invalid impulse type is passed in (e.g. 0 or 1), the object drops in place.
  /// However, momentum from the character (this objects owner) is always preserved.
  void DropGrabbedObject(WUInt8 uiImpulseType = 0); // [ scriptable ]

  /// Throws the held object away.
  ///
  /// See WImpulseTypeConfig for impulse types.
  /// If a non-zero impulse type is given, vRelativeDir is scaled by the impulse type,
  /// such that heavy and light objects may get a different impulse.
  void ThrowGrabbedObject(const WVec3& vRelativeDir, WUInt8 uiImpulseType = 0); // [ scriptable ]

  /// Similar to DropGrabbedObject() but additionally posts the event message WMsgPhysicsJointBroke.
  ///
  /// This can be used to inform other code that the object was ripped from the hands of the player.
  void BreakObjectGrab(); // [ scriptable ]

  /// If the held actor is pushed out of the hands farther than this, the 'joint' breaks. Set to zero to disable this feature.
  float m_fBreakDistance = 0.5f; // [ property ]

  /// The stiffness of the joint to pull the object towards the player's hands.
  /// Careful, too large values mean the held object can push objects that the player itself cannot push.
  float m_fSpringStiffness = 50.0f; // [ property ]

  /// The damping of the joint, to prevent oscillation when moving around.
  float m_fSpringDamping = 10.0f; // [ property ]

  /// How far grab points are allowed to be away to pick them
  float m_fMaxGrabPointDistance = 2.0f; // [ property ]

  /// The radius of the sphere cast that is used instead of a raycast if the radius is greater than zero.
  /// This can make it easier to pick up very small objects.
  float m_fCastRadius = 0.0f; // [ property ]

  /// The collision layer to use for the raycast.
  WUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// If non-zero, the player can pick up objects that have no WGrabbableItemComponent, if their bounding box extents are below this value.
  float m_fAllowGrabAnyObjectWithSize = 0.75f;        // [ property ]

  void SetAttachToReference(const char* szReference); // [ property ]

  /// Which other game object to attach the grabbed object to.
  /// It is expected to hold a kinematic WJoltDynamicActorComponent that an WJoltJointComponent can be attached to.
  WGameObjectHandle m_hAttachTo;

protected:
  void Update();
  void ReleaseGrabbedObject(float fMaxAllowedImpulse);

  WJoltDynamicActorComponent* GetAttachToActor();
  WResult DetermineGrabPoint(const WComponent* pActor, WTransform& out_LocalGrabPoint) const;
  void CreateJoint(WJoltDynamicActorComponent* pParent, WJoltDynamicActorComponent* pChild);
  void DetectDistanceViolation(WJoltDynamicActorComponent* pGrabbedActor);
  bool IsCharacterStandingOnObject(WGameObjectHandle hActorToGrab) const;

  void OnMsgReleaseObjectGrab(WMsgReleaseObjectGrab& msg); // [ message handler ]

  WComponentHandle m_hGrabbedActor;
  float m_fGrabbedActorGravity = 1.0f;
  float m_fGrabbedActorInverseMass = 0.0f;

  WTime m_LastValidTime;
  WTransform m_ChildAnchorLocal;
  WComponentHandle m_hCharacterControllerComponent;
  JPH::SixDOFConstraint* m_pConstraint = nullptr;

private:
  const char* DummyGetter() const { return nullptr; }
};
