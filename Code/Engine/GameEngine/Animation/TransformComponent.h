#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

/// Internal flags for the current state of a transform component
struct WTransformComponentFlags
{
  using StorageType = WUInt16;

  enum Enum
  {
    None = 0,
    Running = W_BIT(0),           ///< Start state for the CurrentlyRunning flag
    AutoReturnStart = W_BIT(1),   ///< When reaching the start point, the transform should automatically turn around
    AutoReturnEnd = W_BIT(2),     ///< When reaching the end point, the transform should automatically turn around
    CurrentlyRunning = W_BIT(3),  ///< The component is currently modifying the transform
    AnimationReversed = W_BIT(5), ///< The animation playback is currently in reverse
    Default = Running | AutoReturnStart | AutoReturnEnd
  };

  struct Bits
  {
    StorageType Running : 1;
    StorageType AutoReturnStart : 1;
    StorageType AutoReturnEnd : 1;
    StorageType CurrentlyRunning : 1;
    StorageType Unused2 : 1;
    StorageType AnimationReversed : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WTransformComponentFlags);

/// Base class for some components that modify an object's transform.
class W_GAMEENGINE_DLL WTransformComponent : public WComponent
{
  W_ADD_DYNAMIC_REFLECTION(WTransformComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WTransformComponent

public:
  WTransformComponent();
  ~WTransformComponent();

  /// Sets the animation to be played forwards or backwards.
  ///
  /// \note Does not start the animation, if it is currently not running.
  void SetDirectionForwards(bool bForwards); // [ scriptable ]

  /// Toggles the directon of the animation.
  ///
  /// \note Does not start the animation, if it is currently not running.
  void ToggleDirection(); // [ scriptable ]

  /// Returns whether the animation is currently being played forwards or backwards.
  bool IsDirectionForwards() const; // [ scriptable ]

  /// Returns whether the animation is currently being played back or paused.
  bool IsRunning(void) const; // [ property ]

  /// Starts or stops animation playback.
  void SetRunning(bool bRunning); // [ property ]

  /// Returns whether the animation would turn around automatically when reaching the start point.
  bool GetReverseAtStart(void) const; // [ property ]
  void SetReverseAtStart(bool b);     // [ property ]

  /// Returns whether the animation would turn around automatically when reaching the end point.
  bool GetReverseAtEnd(void) const; // [ property ]
  void SetReverseAtEnd(bool b);     // [ property ]

  /// The speed at which the animation should be played back.
  float m_fAnimationSpeed = 1.0f; // [ property ]

protected:
  WBitflags<WTransformComponentFlags> m_Flags;
  WTime m_AnimationTime;
};
