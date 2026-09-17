#pragma once

#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

class WPhysicsWorldModuleInterface;

struct W_GAMEENGINE_DLL WMsgSensorDetectedObjectsChanged : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgSensorDetectedObjectsChanged, WMessage);

  WArrayPtr<WGameObjectHandle> m_DetectedObjects;
};

//////////////////////////////////////////////////////////////////////////

/// Base class for sensor components that can be used for AI perception like vision or hearing.
///
/// Derived component classes implemented different shapes like sphere cylinder or cone.
/// All sensors do a query with the specified spatial category in the world's spatial system first, therefore it is necessary to have objects
/// with matching spatial category for the sensors to detect them. This can be achieved with components like e.g. WMarkerComponent.
/// Visibility tests via raycasts are done afterwards by default but can be disabled.
/// The components store an array of all their currently detected objects and send an WMsgSensorDetectedObjectsChanged message if this array changes.
class W_GAMEENGINE_DLL WSensorComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WSensorComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WSensorComponent

public:
  WSensorComponent();
  ~WSensorComponent();

  virtual void GetObjectsInSensorVolume(WDynamicArray<WGameObject*>& out_objects) const = 0;
  virtual void DebugDrawSensorShape() const = 0;

  void SetSpatialCategory(const char* szCategory);            // [ property ]
  const char* GetSpatialCategory() const;                     // [ property ]

  bool m_bTestVisibility = true;                              // [ property ]
  WUInt8 m_uiCollisionLayer = 0;                             // [ property ]

  void SetUpdateRate(const WEnum<WUpdateRate>& updateRate); // [ property ]
  const WEnum<WUpdateRate>& GetUpdateRate() const;          // [ property ]

  void SetShowDebugInfo(bool bShow);                          // [ property ]
  bool GetShowDebugInfo() const;                              // [ property ]

  void SetColor(WColorGammaUB color);                        // [ property ]
  WColorGammaUB GetColor() const;                            // [ property ]

  WTagSet m_IncludeTags;                                     // [ property ]
  WTagSet m_ExcludeTags;                                     // [ property ]

  /// Returns the list of objects that this sensor has detected during its last update
  WArrayPtr<const WGameObjectHandle> GetLastDetectedObjects() const { return m_LastDetectedObjects; }

  /// Updates the sensor state right now.
  ///
  /// If the update rate isn't set to 'Never', this is periodically done automatically.
  /// Otherwise, it has to be called manually to update the state on demand.
  ///
  /// Afterwards out_objectsInSensorVolume will contain all objects that were found inside the volume.
  /// ref_detectedObjects needs to be provided as a temp array, but will not contain a usable result afterwards,
  /// call GetLastDetectedObjects() instead.
  ///
  /// If bPostChangeMsg is true, WMsgSensorDetectedObjectsChanged is posted in case there is a change.
  /// Physical visibility checks are skipped in case pPhysicsWorldModule is null.
  ///
  /// Returns true, if there was a change in detected objects, false if the same objects were detected as last time.
  bool RunSensorCheck(WPhysicsWorldModuleInterface* pPhysicsWorldModule, WDynamicArray<WGameObject*>& out_objectsInSensorVolume, WDynamicArray<WGameObjectHandle>& ref_detectedObjects, bool bPostChangeMsg) const;

  /// How many objects were detected last.
  WUInt32 GetDetectedObjectsCount() const { return m_LastDetectedObjects.GetCount(); } // [ scriptable ]

  /// Returns a handle to the n-th detected object.
  WGameObjectHandle GetDetectedObject(WUInt32 uiIndex) const { return m_LastDetectedObjects[uiIndex]; } // [ scriptable ]

protected:
  void UpdateSpatialCategory();
  void UpdateScheduling();
  void UpdateDebugInfo();

  WEnum<WUpdateRate> m_UpdateRate;
  bool m_bShowDebugInfo = false;
  mutable bool m_bHadUpdate = false;
  WColorGammaUB m_Color = WColorScheme::LightUI(WColorScheme::Orange);

  WHashedString m_sSpatialCategory;
  WSpatialData::Category m_SpatialCategory = WInvalidSpatialDataCategory;

  friend class WSensorWorldModule;
  mutable WDynamicArray<WGameObjectHandle> m_LastDetectedObjects;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  mutable WDynamicArray<WVec3> m_LastOccludedObjectPositions;
#endif
};

//////////////////////////////////////////////////////////////////////////

using WSensorSphereComponentManager = WComponentManager<class WSensorSphereComponent, WBlockStorageType::Compact>;

class W_GAMEENGINE_DLL WSensorSphereComponent : public WSensorComponent
{
  W_DECLARE_COMPONENT_TYPE(WSensorSphereComponent, WSensorComponent, WSensorSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WSensorComponent

  virtual void GetObjectsInSensorVolume(WDynamicArray<WGameObject*>& out_objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // WSensorSphereComponent

public:
  WSensorSphereComponent();
  ~WSensorSphereComponent();

  float m_fRadius = 10.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using WSensorCylinderComponentManager = WComponentManager<class WSensorCylinderComponent, WBlockStorageType::Compact>;

class W_GAMEENGINE_DLL WSensorCylinderComponent : public WSensorComponent
{
  W_DECLARE_COMPONENT_TYPE(WSensorCylinderComponent, WSensorComponent, WSensorCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WSensorComponent

  virtual void GetObjectsInSensorVolume(WDynamicArray<WGameObject*>& out_objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // WSensorCylinderComponent

public:
  WSensorCylinderComponent();
  ~WSensorCylinderComponent();

  float m_fRadius = 10.0f; // [ property ]
  float m_fHeight = 10.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using WSensorConeComponentManager = WComponentManager<class WSensorConeComponent, WBlockStorageType::Compact>;

class W_GAMEENGINE_DLL WSensorConeComponent : public WSensorComponent
{
  W_DECLARE_COMPONENT_TYPE(WSensorConeComponent, WSensorComponent, WSensorConeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WSensorComponent

  virtual void GetObjectsInSensorVolume(WDynamicArray<WGameObject*>& out_objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // WSensorConeComponent

public:
  WSensorConeComponent();
  ~WSensorConeComponent();

  float m_fNearDistance = 0.0f;                     // [ property ]
  float m_fFarDistance = 10.0f;                     // [ property ]
  WAngle m_Angle = WAngle::MakeFromDegree(90.0f); // [ property ]
};

//////////////////////////////////////////////////////////////////////////

class WSensorWorldModule : public WWorldModule
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WSensorWorldModule, WWorldModule);

public:
  WSensorWorldModule(WWorld* pWorld);

  virtual void Initialize() override;

  void AddComponentToSchedule(WSensorComponent* pComponent, WUpdateRate::Enum updateRate);
  void RemoveComponentToSchedule(WSensorComponent* pComponent);

  void AddComponentForDebugRendering(WSensorComponent* pComponent);
  void RemoveComponentForDebugRendering(WSensorComponent* pComponent);

private:
  void UpdateSensors(const WWorldModule::UpdateContext& context);
  void DebugDrawSensors(const WWorldModule::UpdateContext& context);

  WIntervalScheduler<WComponentHandle> m_Scheduler;
  WPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;

  WDynamicArray<WGameObject*> m_ObjectsInSensorVolume;
  WDynamicArray<WGameObjectHandle> m_DetectedObjects;

  WDynamicArray<WComponentHandle> m_DebugComponents;
};
