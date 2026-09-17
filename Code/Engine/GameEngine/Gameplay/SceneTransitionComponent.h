#pragma once

#include <Core/World/EventMessageHandlerComponent.h>
#include <GameEngine/GameEngineDLL.h>

struct WMsgTriggerTriggered;

using WSceneTransitionComponentManager = WComponentManager<class WSceneTransitionComponent, WBlockStorageType::Compact>;

/// What WSceneTransitionComponent should do when it gets triggered through a WMsgTriggerTriggered
struct WSceneLoadMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    None,          ///< Do nothing, ignore trigger messages.
    LoadAndSwitch, ///< Immediately switch to the target scene, show a loading screen, if necessary.
    Preload,       ///< Start preloading the target scene.
    CancelPreload, ///< Cancel any previously preloaded scene loading.

    Default = LoadAndSwitch
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WSceneLoadMode);

/// Provides functionality to transition from one scene to another (level loading).
///
/// The component references a target level and spawn point (optional).
/// When triggered, either manually or through a trigger message (e.g. from a physics trigger
/// attached to the same object), the component instructs the active game state to (pre-) load a scene.
///
/// It may also automatically forward the relative position of the player from this object into the
/// target scene, such that the level transition appears more seamless.
class W_GAMEENGINE_DLL WSceneTransitionComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WSceneTransitionComponent, WComponent, WSceneTransitionComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WSceneTransitionComponent

public:
  WSceneTransitionComponent();
  ~WSceneTransitionComponent();

  /// GUID or path to the scene that shall be loaded.
  WHashedString m_sTargetScene; // [ property ]

  /// Optional name of the spawn point (see WPlayerStartPointComponent).
  /// If no spawn point with this name exists, the first one in the scene is used.
  WHashedString m_sSpawnPoint; // [ property ]

  /// If true, the relative player position in this scene is forwarded to the target scene spawn point.
  /// Thus if the two levels looks the same at the transition point, the transition appears more seamless.
  bool m_bRelativeSpawnPosition = true; // [ property ]

  /// Optional collection file to use for preloading.
  /// Necessary for proper loading progress calculation.
  WHashedString m_sPreloadCollectionFile; // [ property ]

  /// If not set to 'None' the component reacts to trigger messages with the desired operation.
  /// You can attach a trigger component to the same object (or child) to automatically
  /// switch levels. If this is set to 'None', though, operations have to be triggered
  /// manually through script code or by sending WMsgTriggerTriggered directly.
  WEnum<WSceneLoadMode> m_Mode; // [ property ]

  /// Makes the game immediately switch to the target level.
  ///
  /// If necessary, the loading screen is shown first.
  /// If offsets are given, the player spawns relative to the target spawn point.
  /// This
  void StartTransition(const WVec3& vPositionOffset = WVec3::MakeZero(), const WQuat& qRotationOffset = WQuat::MakeIdentity()); // [ scriptable ]

  /// Same as StartTransition() but computes the relative offset to this object from the given global transform.
  void StartTransitionWithOffsetTo(const WVec3& vGlobalPosition, const WQuat& qGlobalRotation); // [ scriptable ]

  /// Starts preloading the target scene.
  ///
  /// Does not switch to it automatically. Call StartTransition() at any time to do so.
  /// If the preload is finished by then, the transition will be as seamless as possible,
  /// if not, a loading screen may show up.
  void StartPreload(); // [ scriptable ]

  /// Cancels any pending preload and frees up memory that memory.
  ///
  /// Should be used, if the player can enter a preload area and then leave it again, without transitioning the level.
  void CancelPreload(); // [ scriptable ]

protected:
  void OnMsgTriggerTriggered(WMsgTriggerTriggered& ref_msg);
};
