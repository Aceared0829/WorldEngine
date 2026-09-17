#pragma once

#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>

struct WMsgApplyRootMotion;

namespace JPH
{
  class CharacterContactListener;
}

using WJoltDefaultCharacterComponentManager = WComponentManager<class WJoltDefaultCharacterComponent, WBlockStorageType::FreeList>;

/// An example character controller (CC) implementation build upon WJoltCharacterControllerComponent
///
/// This component implements typical behavior for an FPS type of game.
/// It is mainly meant as an example, as most games would rather implement their own CC to control the exact details.
///
/// It is also possible to derive from this component and override some virtual functions to just tweak the behavior of this
/// sample implementation, in case you only need minor tweaks.
class W_JOLTPLUGIN_DLL WJoltDefaultCharacterComponent : public WJoltCharacterControllerComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltDefaultCharacterComponent, WJoltCharacterControllerComponent, WJoltDefaultCharacterComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltCharacterControllerComponent

public:
  WJoltDefaultCharacterComponent();
  ~WJoltDefaultCharacterComponent();

  enum class GroundState : WUInt8
  {
    OnGround, ///< Character is touching the ground
    Sliding,  ///< Character is touching a steep surface and therefore slides downwards
    InAir,    ///< Character isn't touching any ground surface (may still touch a wall or ceiling)
  };

  /// How many degrees per second the character turns
  WAngle m_RotateSpeed = WAngle::MakeFromDegree(90.0f); // [ property ]

  /// The radius of the capsule shape
  float m_fShapeRadius = 0.25f;

  /// The total cylinder height when the character crouches.
  float m_fCylinderHeightCrouch = 0.9f;

  /// The total cylinder height when the character stands.
  float m_fCylinderHeightStand = 1.7f;

  /// The radius of the feet area, where it is checked whether the CC properly stands on the ground.
  float m_fFootRadius = 0.15f;

  /// Meters per second movement speed when crouching.
  float m_fWalkSpeedCrouching = 0.5f;

  /// Meters per second movement speed when standing.
  float m_fWalkSpeedStanding = 1.5f;

  /// Meters per second movement speed when standing and running.
  float m_fWalkSpeedRunning = 3.5f;

  /// The maximum step height that the CC can step up in a single frame.
  float m_fMaxStepUp = 0.25f;

  /// The maximum step height that the CC can step down in a single frame (without 'falling').
  float m_fMaxStepDown = 0.25f;

  /// The physics impulse to use for jumping.
  float m_fJumpImpulse = 5.0f;

  /// The surface interaction to spawn regularly when walking.
  WHashedString m_sWalkSurfaceInteraction; // [ property ]

  /// The surface type to use for interactions, when no other surface type is available.
  WSurfaceResourceHandle m_hFallbackWalkSurface; // [ property ]

  /// How far the CC has to walk for spawning another surface interaction
  float m_fWalkInteractionDistance = 1.0f; // [ property ]

  /// How far the CC has to run for spawning another surface interaction.
  float m_fRunInteractionDistance = 3.0f;                                                          // [ property ]

  void SetWalkSurfaceInteraction(const char* szName) { m_sWalkSurfaceInteraction.Assign(szName); } // [ property ]
  const char* GetWalkSurfaceInteraction() const { return m_sWalkSurfaceInteraction.GetData(); }    // [ property ]

  void SetFallbackWalkSurfaceFile(WStringView sFile);                                             // [ property ]
  WStringView GetFallbackWalkSurfaceFile() const;                                                 // [ property ]

  /// How fast to move while falling. The higher, the more "air control" the player has.
  float m_fAirSpeed = 2.5f; // [ property ]

  /// How much lateral motion to lose while falling.
  float m_fAirFriction = 0.5f; // [ property ]

  /// Sets an object GUID for an object that is the 'head' (controls the camera).
  void SetHeadObjectReference(const char* szReference); // [ property ]

  /// This message is used to steer the CC.
  void SetInputState(WMsgMoveCharacterController& ref_msg); // [ msg handler ]

  /// Returns the current height of the entire capsule (crouching or standing).
  float GetCurrentCapsuleHeight() const;

  /// Returns the current height of the cylindrical part of the capsule (crouching or standing).
  float GetCurrentCylinderHeight() const;

  /// Returns the radius of the shape. This never changes at runtime.
  virtual float GetShapeRadius() const override;

  GroundState GetGroundState() const { return m_LastGroundState; }
  bool IsStandingOnGround() const { return m_LastGroundState == GroundState::OnGround; } // [ scriptable ]
  bool IsSlidingOnGround() const { return m_LastGroundState == GroundState::Sliding; }   // [ scriptable ]
  bool IsInAir() const { return m_LastGroundState == GroundState::InAir; }               // [ scriptable ]
  bool IsCrouching() const { return m_uiIsCrouchingBit; }                                // [ scriptable ]

  /// Makes the CC jump during the next update (if possible)
  void Jump(); // [ scriptable ]

  /// Sets the 'run' flag for the next update. Automatically reset by next frame.
  void Run(); // [ scriptable ]

  /// Makes the CC crouch during the next update (if possible)
  void Crouch(); // [ scriptable ]

  /// Sets the movement direction for the next update.
  ///
  /// Note: Values are typically in -1 to 1 range.
  /// The actual movement speed is configured elsewhere and depends on whether the CC crouches, jumps or runs.
  void Move(float fForward, float fRight); // [ scriptable ]

  /// Sets the rotation amount for the next update.
  ///
  /// Note: The actual rotation speed is another property.
  void RotateZ(float fAmount); // [ scriptable ]

  /// Instantly teleports the character to the target position. Doesn't change its rotation.
  void TeleportCharacter(const WVec3& vGlobalFootPosition);

  struct Config
  {
    bool m_bAllowJump = true;
    bool m_bAllowCrouch = true;
    bool m_bApplyGroundVelocity = true;
    WVec3 m_vVelocity = WVec3::MakeZero();
    float m_fPushDownForce = 0;
    WHashedString m_sGroundInteraction;
    float m_fGroundInteractionDistanceThreshold = 1.0f;
    float m_fMaxStepUp = 0;
    float m_fMaxStepDown = 0;
  };

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const;
  virtual void OnApplyRootMotion(WMsgApplyRootMotion& msg);

  virtual void DetermineConfig(Config& out_inputs);

  virtual void UpdateCharacter() override;
  virtual void ApplyRotationZ();

  /// Clears the input states to neutral values
  void ResetInputState();

  void ResetInternalState();

  /// Creates a new shape with the given height (and fixed radius)
  virtual JPH::Ref<JPH::Shape> MakeNextCharacterShape() override;

  void ApplyCrouchState();
  void InteractWithSurfaces(const ContactPoint& contact, const Config& cfg);

  void StoreLateralVelocity();
  void ClampLateralVelocity();
  void ClampUpVelocity();
  void MoveHeadObject();
  void DebugVisualizations();

  void CheckFeet();

  GroundState m_LastGroundState = GroundState::InAir;

  WUInt8 m_uiInputJumpBit : 1;
  WUInt8 m_uiInputCrouchBit : 1;
  WUInt8 m_uiInputRunBit : 1;
  WUInt8 m_uiIsCrouchingBit : 1;
  WAngle m_InputRotateZ;
  WVec2 m_vInputDirection = WVec2::MakeZero();
  float m_fVelocityUp = 0.0f;
  float m_fNextCylinderHeight = 0;
  float m_fAccumulatedWalkDistance = 0.0f;
  WVec2 m_vVelocityLateral = WVec2::MakeZero();
  WTransform m_PreviousTransform;
  bool m_bFeetOnSolidGround = false;

  float m_fCurrentCylinderHeight = 0;

  float m_fHeadHeightOffset = 0.0f;
  float m_fHeadTargetHeight = 0.0f;
  WGameObjectHandle m_hHeadObject;

  WVec3 m_vAbsoluteRootMotion = WVec3::MakeZero();

  WUInt32 m_uiUserDataIndex = WInvalidIndex;
  WUInt32 m_uiJoltBodyID = WInvalidIndex;

  WUniquePtr<JPH::CharacterContactListener> m_pContactListener;

private:
  const char* DummyGetter() const { return nullptr; }
};
