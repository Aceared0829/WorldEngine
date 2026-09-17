#pragma once

#include <Core/Messages/EventMessageSender.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

struct WMsgAnimationReachedEnd;

//////////////////////////////////////////////////////////////////////////

using WFollowSplineComponentManager = WComponentManagerSimple<class WFollowSplineComponent, WComponentUpdateType::WhenSimulating>;

struct W_GAMEENGINE_DLL WFollowSplineMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    OnlyPosition,
    AlignUpZ,
    FullRotation,

    Default = OnlyPosition
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WFollowSplineMode)

/// This component makes the WGameObject, that it is attached to, move along a spline defined by an WSplineComponent.
///
/// Build a spline using an WSplineComponent and WSplineNodeComponents.
/// Then attach an WFollowSplineComponent to a free-standing WGameObject and reference the object with the WSplineComponent in it.
///
/// During simulation the WFollowSplineComponent will now move and rotate its owner object such that it moves along the spline.
///
/// The start location of the 'hook' (the object with the WFollowSplineComponent on it) may be anywhere. It will be teleported
/// onto the spline. For many objects this is not a problem, but physically simulated objects may be very sensitive about this.
///
/// One option is to align the 'hook' perfectly with the start location.
/// You can achieve this, using the "Keep Simulation Changes" feature of the editor (simulate with zero speed, press K, stop simulation).
/// Another option is to instead delay the spawning of the object below the hook, by using an WSpawnComponent next to the WFollowSplineComponent,
/// and thus have the payload spawn only after the hook has been placed properly.
class W_GAMEENGINE_DLL WFollowSplineComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WFollowSplineComponent, WComponent, WFollowSplineComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(WWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WFollowSplineComponent

public:
  WFollowSplineComponent();
  ~WFollowSplineComponent();

  /// Sets the reference to the game object on which an WSplineComponent should be attached.
  void SetSplineObject(const char* szReference);      // [ property ]

  WEnum<WPropertyAnimMode> m_Mode;                  ///< [ property ] How the spline should be traversed.
  WEnum<WFollowSplineMode> m_FollowMode;            ///< [ property ] How the transform of the follower should be affected by the spline.
  float m_fSpeed = 1.0f;                              ///< [ property ] How fast to move along the spline.
  float m_fLookAhead = 0.0f;                          ///< [ property ] How far along the spline to 'look ahead' to smooth the rotation. A small distance means rotations are very abrupt.
  float m_fSmoothing = 0.5f;                          ///< [ property ] How much to combine the current position with the new position. 0 to 1. At zero, the position follows the spline perfectly, but therefore also has very abrupt changes. With a lot of smoothing, the spline becomes very sluggish.
  float m_fTiltAmount = 5.0f;                         ///< [ property ] How much to tilt when turning.
  WAngle m_MaxTilt = WAngle::MakeFromDegree(30.0f); ///< [ property ] The max tilt angle of the object.

  /// Distance along the spline at which the WFollowSplineComponent should start off.
  void SetStartDistance(float fDistance);                     // [ property ]
  float GetStartDistance() const { return m_fStartDistance; } // [ property ]

  /// Sets the current distance along the spline at which the WFollowSplineComponent should be.
  void SetCurrentDistance(float fDistance);                       // [ scriptable ]
  float GetCurrentDistance() const { return m_fCurrentDistance; } // [ scriptable ]

  /// Whether the component should move along the spline 'forwards' or 'backwards'
  void SetDirectionForwards(bool bForwards); // [ scriptable ]

  /// Toggles the direction that it travels along the spline.
  void ToggleDirection(); // [ scriptable ]

  /// Whether the component currently moves 'forwards' along the spline.
  ///
  /// Note that if the 'speed' property is negative, moving 'forwards' along the spline still means that it effectively moves backwards.
  bool IsDirectionForwards() const { return m_bIsRunningForwards; } // [ scriptable ]

  /// Whether the component currently moves along the spline, at all.
  bool IsRunning() const { return m_bIsRunning; } // [ property ]

  /// Whether to move along the spline or not.
  void SetRunning(bool bRunning); // [ property ]

protected:
  void Update(bool bForce = false);

  WEventMessageSender<WMsgAnimationReachedEnd> m_ReachedEndEvent; // [ event ]
  WGameObjectHandle m_hSplineObject;                               // [ property ]

  float m_fStartDistance = 0.0f;                                    // [ property ]

  float m_fCurrentDistance = 0.0f;
  bool m_bLastStateValid = false;
  bool m_bIsRunning = true;
  bool m_bIsRunningForwards = true;
  WVec3 m_vLastPosition;
  WVec3 m_vLastForwardDir;
  WVec3 m_vLastUpDir;
  WAngle m_LastTiltAngle;

  const char* DummyGetter() const { return nullptr; }
};
