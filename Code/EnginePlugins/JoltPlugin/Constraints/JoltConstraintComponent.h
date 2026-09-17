#pragma once

#include <Core/World/ComponentManager.h>
#include <JoltPlugin/Declarations.h>

class WJoltDynamicActorComponent;

namespace JPH
{
  class Body;
}

namespace JPH
{
  class Constraint;
}

/// Configures how a physics constraint's limit acts.
struct WJoltConstraintLimitMode
{
  using StorageType = WInt8;

  enum Enum
  {
    NoLimit,   ///< The constraint has no limit.
    HardLimit, ///< The constraint has a hard limit, no soft spring is used to prevent further movement.
    // SoftLimit,

    Default = NoLimit
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltConstraintLimitMode);

/// Configures how a drive on a constraint works.
struct WJoltConstraintDriveMode
{
  using StorageType = WInt8;

  enum Enum
  {
    NoDrive,       ///< The constraint has no drive.
    DriveVelocity, ///< The drive attempts to reach a target velocity (of rotation or motion).
    DrivePosition, ///< The drive attempts to reach a target position (or angle).

    Default = NoDrive
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltConstraintDriveMode);

//////////////////////////////////////////////////////////////////////////

/// Base class for all Jolt physics joints (constraints).
///
/// A constraint always limits the movement of a dynamic actor.
/// The actor may be joined to another actor. If the other actor is static or kinematic,
/// the dynamic actor is restricted to follow those actors.
/// If the other actor is also dynamic, both actors' freedom of movement is restricted.
///
/// A constraint can also only affect a single dynamic actor, in which case it is joined "to the world",
/// which is an imaginary static actor.
///
/// Derived types implement different constraints.
///
/// All constraints can be breakable, meaning that they get disabled when a sufficiently large force or torque
/// acts on the constraint.
/// By default two joined actors still collide with each other, but it is often convenient to disable
/// collisions between only those two actors. For example a door that is joined to a door frame may not work
/// right, if the edge of the door still collides with the door frame. It is easier to make the door work smoothly
/// if it doesn't collide with the frame and its movement is mainly limited by the constraint.
class W_JOLTPLUGIN_DLL WJoltConstraintComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WJoltConstraintComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltConstraintComponent

public:
  WJoltConstraintComponent();
  ~WJoltConstraintComponent();

  /// Removes the connection between the joined bodies. This cannot be reversed.
  void BreakConstraint();

  /// If set to larger than zero, the constraint will break when a linear force larger than this acts onto the constraint.
  void SetBreakForce(float value);                      // [ property ]
  float GetBreakForce() const { return m_fBreakForce; } // [ property ]

  /// If set to larger than zero, the constraint will break when a rotational force larger than this acts onto the constraint.
  void SetBreakTorque(float value);                       // [ property ]
  float GetBreakTorque() const { return m_fBreakTorque; } // [ property ]

  /// If disabled, the two joined actors pass through each other, rather than colliding.
  void SetPairCollision(bool value);                          // [ property ]
  bool GetPairCollision() const { return m_bPairCollision; }  // [ property ]

  void SetParentActorReference(const char* szReference);      // [ property ]
  void SetChildActorReference(const char* szReference);       // [ property ]
  void SetChildActorAnchorReference(const char* szReference); // [ property ]

  /// Sets which actor to attach the constraint to.
  void SetParentActor(WGameObjectHandle hActor);
  /// Sets which actor to attach to the constraint.
  void SetChildActor(WGameObjectHandle hActor);
  /// Sets an actor as a reference frame so that the constraint can start in a non-default configuration.
  void SetChildActorAnchor(WGameObjectHandle hActor);

  /// For manually providing actors and local frames to configure the start state. This is for advanced uses.
  void SetActors(WGameObjectHandle hActorA, const WTransform& localFrameA, WGameObjectHandle hActorB, const WTransform& localFrameB);

  /// Forwards to BreakConstraint().
  void OnJoltMsgDisconnectConstraints(WJoltMsgDisconnectConstraints& ref_msg); // [ msg handler ]

protected:
  friend class WJoltWorldModule;

  virtual bool ExceededBreakingPoint() = 0;
  virtual void ApplySettings() = 0;

  WResult FindParentBody(WUInt32& out_uiJoltBodyID, WJoltDynamicActorComponent*& pRbComp);
  WResult FindChildBody(WUInt32& out_uiJoltBodyID, WJoltDynamicActorComponent*& pRbComp);

  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) = 0;

  WTransform ComputeParentBodyGlobalFrame() const;
  WTransform ComputeChildBodyGlobalFrame() const;

  void QueueApplySettings();

  WGameObjectHandle m_hActorA;
  WGameObjectHandle m_hActorB;
  WGameObjectHandle m_hActorBAnchor;

  // UserFlag0 specifies whether m_localFrameA is already set
  WTransform m_LocalFrameA;
  // UserFlag1 specifies whether m_localFrameB is already set
  WTransform m_LocalFrameB;

  JPH::Constraint* m_pConstraint = nullptr;

  float m_fBreakForce = 0.0f;
  float m_fBreakTorque = 0.0f;
  bool m_bPairCollision = true;

private:
  const char* DummyGetter() const { return nullptr; }
};
