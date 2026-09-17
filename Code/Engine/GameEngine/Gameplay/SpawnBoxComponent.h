#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

struct WMsgComponentInternalTrigger;

struct WSpawnBoxComponentFlags
{
  using StorageType = WUInt16;

  enum Enum
  {
    None = 0,
    SpawnAtStart = W_BIT(0),      ///< The component will schedule a spawn once at creation time
    SpawnContinuously = W_BIT(1), ///< Every time a spawn duration has finished, a new one is started

    Default = None
  };

  struct Bits
  {
    StorageType SpawnAtStart : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WSpawnBoxComponentFlags);

using WSpawnBoxComponentManager = WComponentManager<class WSpawnBoxComponent, WBlockStorageType::Compact>;

/// This component spawns prefabs inside a box.
///
/// The prefabs are spawned over a fixed duration.
/// The number of prefabs to spawn over the time duration is randomly chosen.
/// Each prefab may get rotated around the Z axis and tilted away from the Z axis.
/// If desired, the component can start spawning automatically, or it can be (re-)started from code.
/// If 'spawn continuously' is enabled, the component restarts itself after the spawn duration is over,
/// thus for every spawn duration the number of prefabs to spawn gets reevaluated.
class WSpawnBoxComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WSpawnBoxComponent, WComponent, WSpawnBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WSpawnBoxComponent

public:
  /// When called, the component starts spawning the chosen number of prefabs over the set duration.
  ///
  /// If this is called while the component is already active, the internal state is reset and it starts over.
  void StartSpawning();                                           // [ scriptable ]

  void SetHalfExtents(const WVec3& value);                       // [ property ]
  const WVec3& GetHalfExtents() const { return m_vHalfExtents; } // [ property ]

  bool GetSpawnAtStart() const;                                   // [ property ]
  void SetSpawnAtStart(bool b);                                   // [ property ]

  bool GetSpawnContinuously() const;                              // [ property ]
  void SetSpawnContinuously(bool b);                              // [ property ]

  WTime m_SpawnDuration;                                         // [ property ]
  WUInt16 m_uiMinSpawnCount = 5;                                 // [ property ]
  WUInt16 m_uiSpawnCountRange = 5;                               // [ property ]
  WPrefabResourceHandle m_hPrefab;                               // [ property ]

  /// The spawned object's forward direction may deviate this amount from the spawn box's forward rotation. This is accomplished by rotating around the Z axis.
  WAngle m_MaxRotationZ; // [ property ]

  /// The spawned object's Z (up) axis may deviate by this amount from the spawn box's Z axis.
  WAngle m_MaxTiltZ; // [ property ]


private:
  void OnTriggered(WMsgComponentInternalTrigger& msg);
  void Spawn(WUInt32 uiCount);
  void InternalStartSpawning(bool bFirstTime);

  WUInt16 m_uiSpawned = 0;
  WUInt16 m_uiTotalToSpawn = 0;
  WTime m_StartTime;
  WBitflags<WSpawnBoxComponentFlags> m_Flags;
  WVec3 m_vHalfExtents = WVec3(0.5f);
};
