#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>

struct WMsgComponentInternalTrigger;

struct WSpawnComponentFlags
{
  using StorageType = WUInt16;

  enum Enum
  {
    None = 0,
    SpawnAtStart = W_BIT(0),      ///< The component will schedule a spawn once at creation time
    SpawnContinuously = W_BIT(1), ///< Every time a scheduled spawn was done, a new one is scheduled
    AttachAsChild = W_BIT(2),     ///< All objects spawned will be attached as children to this node
    SpawnInFlight = W_BIT(3),     ///< [internal] A spawn trigger message has been posted.

    Default = None
  };

  struct Bits
  {
    StorageType SpawnAtStart : 1;
    StorageType SpawnContinuously : 1;
    StorageType AttachAsChild : 1;
    StorageType SpawnInFlight : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WSpawnComponentFlags);

using WSpawnComponentManager = WComponentManager<class WSpawnComponent, WBlockStorageType::Compact>;

/// Spawns instances of prefabs dynamically at runtime.
///
/// The component may spawn prefabs automatically and also continuously, or it may only spawn objects on-demand
/// when triggered from code.
///
/// It keeps track of when it spawned an object and can ignore spawn requests that come in too early. Thus it can
/// also be used to take care of the logic that certain actions are only allowed every once in a while.
class W_GAMEENGINE_DLL WSpawnComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WSpawnComponent, WComponent, WSpawnComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WSpawnComponent

public:
  WSpawnComponent();
  ~WSpawnComponent();

  /// Checks whether the last spawn time was long enough ago that a call to TriggerManualSpawn() would succeed.
  bool CanTriggerManualSpawn() const; // [ scriptable ]

  /// Spawns a new object, unless the minimum spawn delay has not been reached between calls to this function.
  ///
  /// Manual spawns and continuous (scheduled) spawns are independent from each other regarding minimum spawn delays.
  /// If this function is called in too short intervals, it is ignored and false is returned.
  /// Returns true, if an object was spawned.
  bool TriggerManualSpawn(bool bIgnoreSpawnDelay = false, const WVec3& vLocalOffset = WVec3::MakeZero()); // [ scriptable ]

  /// Unless a spawn is already scheduled, this will schedule one within the configured time frame.
  ///
  /// If continuous spawning is enabled, this will kick off the first spawn and then continue indefinitely.
  /// To stop continuously spawning, remove the continuous spawn flag.
  void ScheduleSpawn(); // [ scriptable ]

  /// Enables that the component spawns right at creation time. Otherwise it needs to be triggered manually.
  void SetSpawnAtStart(bool b); // [ property ]
  bool GetSpawnAtStart() const; // [ property ]

  /// Enables that once an object was spawned, another spawn action will be scheduled right away.
  void SetSpawnContinuously(bool b); // [ property ]
  bool GetSpawnContinuously() const; // [ property ]

  /// Sets that spawned objects will be attached as child objects to this game object.
  void SetAttachAsChild(bool b);    // [ property ]
  bool GetAttachAsChild() const;    // [ property ]

  WPrefabResourceHandle m_hPrefab; // [ property ]

  /// The minimum delay between spawning objects. This is also enforced for manually spawning things.
  WTime m_MinDelay; // [ property ]

  /// For scheduled spawns (continuous / at start) this is an additional random range on top of the minimum spawn delay.
  WTime m_DelayRange; // [ property ]

  /// The spawned object's orientation may deviate by this amount around the X axis. 180° is completely random orientation.
  WAngle m_MaxDeviation;                                           // [ property ]

  const WRangeView<const char*, WUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const WVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, WVariant& out_value) const; // [ property ] (exposed parameter)

  /// Key/value pairs of parameters to pass to the prefab instantiation.
  WArrayMap<WHashedString, WVariant> m_Parameters;

protected:
  WBitflags<WSpawnComponentFlags> m_SpawnFlags;

  virtual void DoSpawn(const WTransform& tLocalSpawn);
  bool SpawnOnce(const WVec3& vLocalOffset);
  void OnTriggered(WMsgComponentInternalTrigger& msg);

  WTime m_LastManualSpawn;
};
