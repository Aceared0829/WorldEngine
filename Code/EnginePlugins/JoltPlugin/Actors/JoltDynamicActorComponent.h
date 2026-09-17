#pragma once

#include <Foundation/Math/Float16.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>

struct WMsgPhysicsMakeTemporarilyDynamic;

//////////////////////////////////////////////////////////////////////////

class W_JOLTPLUGIN_DLL WJoltDynamicActorComponentManager : public WComponentManager<class WJoltDynamicActorComponent, WBlockStorageType::FreeList>
{
public:
  WJoltDynamicActorComponentManager(WWorld* pWorld);
  ~WJoltDynamicActorComponentManager();

private:
  friend class WJoltWorldModule;
  friend class WJoltDynamicActorComponent;

  void UpdateKinematicActors(WTime deltaTime);
  void UpdateDynamicActors();

  WDynamicArray<WJoltDynamicActorComponent*> m_KinematicActorComponents;
};

//////////////////////////////////////////////////////////////////////////

/// Turns an object into a fully physically simulated movable object.
///
/// By default dynamic objects push each other and collide with static obstacles.
/// They can be switched to be "kinematic" in which case they act like static objects, but can be moved around and will
/// still push aside other dynamic objects.
///
/// Dynamic actors can also be moved through forces and impulses.
///
/// Dynamic actors must be made up of convex collision meshes. They cannot use concave meshes.
class W_JOLTPLUGIN_DLL WJoltDynamicActorComponent : public WJoltActorComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltDynamicActorComponent, WJoltActorComponent, WJoltDynamicActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltDynamicActorComponent

public:
  WJoltDynamicActorComponent();
  ~WJoltDynamicActorComponent();

  /// Turns the actor into a 'kinematic' actor.
  ///
  /// If an actor is kinematic, it isn't fully simulated anymore, meaning forces do not affect it further.
  /// Thus it does not fall down, it can't be pushed by other actors, it is mostly like a static actor.
  ///
  /// However, it can be moved programmatically. If other code changes the position of this object,
  /// it will move to that location and while doing so, it pushes other dynamic actors out of the way.
  ///
  /// Kinematic actors are typically used for moving platforms, doors and other objects that need to move and push aside
  /// dynamic actors.
  void SetKinematic(bool b);                         // [ property ]
  bool GetKinematic() const { return m_bKinematic; } // [ property ]

  /// Adjusts how strongly gravity affects this actor. Has no effect for kinematic actors.
  ///
  /// Set this to zero to fully disable gravity.
  void SetGravityFactor(float fFactor);                       // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  void SetSurfaceFile(WStringView sFile);                    // [ property ]
  WStringView GetSurfaceFile() const;                        // [ property ]

  /// If enabled, a more precise simulation method is used, preventing fast moving actors from tunneling through walls.
  /// This comes at an extra performance cost.
  bool m_bCCD = false; // [ property ]

  /// If true, the actor will not be simulated right away, but only after another actor starts interacting with it.
  /// This is a performance optimization to prevent performance spikes after loading a level.
  bool m_bStartAsleep = false; // [ property ]

  /// Whether this actor is allowed to go to sleep. Disabling sleeping will come with a performance impact and
  /// should only be done in very rare cases.
  bool m_bAllowSleeping = true;        // [ property ]

  WUInt8 m_uiWeightCategory = 0;      // [ property ]
  WFloat16 m_fWeightMass = 10.0f;     // [ property ]
  WFloat16 m_fWeightDensity = 100.0f; // [ property ]
  WFloat16 m_fWeightScale = 1.0f;     // [ property ]

  /// How much buoyancy to apply when the actor is submerged in a fluid. 1.0 means neutral buoyancy, <1.0 sinks, >1.0 floats.
  WFloat16 m_fBuoyancyFactor = 1.1f; // [ property ]

  /// How much to dampen linear motion. The higher the value, the quicker a moving object comes to rest.
  float m_fLinearDamping = 0.1f; // [ property ]

  /// How much to dampen angular motion. The higher the value, the quicker a rotating object comes to rest.
  float m_fAngularDamping = 0.05f; // [ property ]

  /// Which surface to use for all shapes. The surface defines various physical properties and interactions.
  WSurfaceResourceHandle m_hSurface; // [ property ]

  /// What reactions should be triggered when an actor gets into contact with another.
  WBitflags<WOnJoltContact> m_OnContact; // [ property ]

  /// A local offset for the center of mass. \see SetUseCustomCoM()
  WVec3 m_vCenterOfMass = WVec3::MakeZero(); // [ property ]

  /// Whether a custom center-of-mass shall be used.
  void SetUseCustomCoM(bool b) { SetUserFlag(0, b); }     // [ property ]
  bool GetUseCustomCoM() const { return GetUserFlag(0); } // [ property ]

  /// Adds a physics impulse to this body at the given location.
  ///
  /// An impulse is a force that is applied only once, e.g. a sudden push.
  void AddLinearImpulseAtPos(WMsgPhysicsAddImpulse& ref_msg); // [ message ]

  /// Adds a linear impulse to the center-of-mass of this actor. Unless there are other constraints, this would push the object, but not introduce any rotation.
  ///
  /// For uiImpulseType see WImpulseTypeConfig. Use 0 to use vImpulse without modification.
  void AddLinearImpulse(const WVec3& vImpulse, WUInt8 uiImpulseType = 0); // [ scriptable ]

  /// Adds an angular impulse to the center-of-mass of this actor. Unless there are other constraints, this would make the object rotate, but not move away.
  ///
  /// For uiImpulseType see WImpulseTypeConfig. Use 0 to use vImpulse without modification.
  void AddAngularImpulse(const WVec3& vImpulse, WUInt8 uiImpulseType = 0); // [ scriptable ]

  /// Should be called by components that add Jolt constraints to this body.
  ///
  /// All registered components receive WJoltMsgDisconnectConstraints in case the body is deleted.
  /// It is necessary to react to that by removing the Jolt constraint, otherwise Jolt will crash during the next update.
  void AddConstraint(WComponentHandle hComponent);

  /// Should be called when a constraint is removed (though not strictly required) to prevent unnecessary message sending.
  void RemoveConstraint(WComponentHandle hComponent);

  /// Returns the actual mass of this actor which is either the user defined mass or has been calculated from density.
  /// For kinematic actors this function will return 0.
  float GetMass() const;

  /// Adds a force that will be applied to this actor every frame for the given duration.
  ///
  /// Returns a force ID. Pass in an invalid ID (such as 0) to create a new force.
  /// If an existing force shall be updated, pass in the ID that was returned last time.
  ///
  /// Once the duration has elapsed and no update was done to reset the duration, the force is automatically removed.
  /// If a force ID of such a force is used again, it will be ignored and a new force is created instead,
  /// so just always store the returned ID, to replaced force IDs that became invalid.
  ///
  /// If this is called multiple times with invalid force IDs, multiple forces are created to act on the body.
  WUInt32 AddOrUpdateForce(WUInt32 uiForceID, WTime duration, const WVec3& vForce);

  /// Removes the force with the given ID. Does nothing, if the ID is invalid.
  void ClearForce(WUInt32 uiForceID);

protected:
  void OnMsgPhysicsMakeTemporarilyDynamic(WMsgPhysicsMakeTemporarilyDynamic& msg);

  const WJoltMaterial* GetJoltMaterial() const;

  float GetWeight_Scale() const { return m_fWeightScale; }
  float GetWeight_Mass() const { return m_fWeightMass; }
  float GetWeight_Density() const { return m_fWeightDensity; }
  float GetBuoyancyFactor() const { return m_fBuoyancyFactor; }

  void SetWeight_Scale(float fValue) { m_fWeightScale = fValue; }
  void SetWeight_Mass(float fValue) { m_fWeightMass = fValue; }
  void SetWeight_Density(float fValue) { m_fWeightDensity = fValue; }
  void SetBuoyancyFactor(float fValue) { m_fBuoyancyFactor = fValue; }

  bool m_bKinematic = false;
  float m_fGravityFactor = 1.0f; // [ property ]

  WSmallArray<WComponentHandle, 1> m_Constraints;
};
