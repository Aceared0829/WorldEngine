
#pragma once

#include <Core/Interfaces/PhysicsQuery.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

class WPhysicsWorldModuleInterface;

class WRaycastComponentManager : public WComponentManager<class WRaycastComponent, WBlockStorageType::Compact>
{
  using SUPER = WComponentManager<class WRaycastComponent, WBlockStorageType::Compact>;

public:
  WRaycastComponentManager(WWorld* pWorld);

  virtual void Initialize() override;

  void Update(const WWorldModule::UpdateContext& context);
};

/// A component which does a ray cast and positions a target object there.
///
/// This component does a ray cast along the forward axis of the game object it is attached to.
/// If this produces a hit the target object is placed there.
/// If no hit is found the target object is either placed at the maximum distance or deactivated depending on the component configuration.
///
/// This component can also trigger messages when objects enter the ray. E.g. when a player trips a laser detection beam.
/// To enable this set the trigger collision layer to another layer than the main ray cast and set a trigger message.
///
/// Sample setup:
///   m_uiCollisionLayerEndPoint = Default
///   m_uiCollisionLayerTrigger = Player
///   m_sTriggerMessage = "APlayerEnteredTheBeam"
///
/// This will lead to trigger messages being sent when a physics actor on the 'Player' layer comes between
/// the original hit on the default layer and the ray cast origin.
///
class W_GAMECOMPONENTS_DLL WRaycastComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WRaycastComponent, WComponent, WRaycastComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  void Deinitialize() override;
  void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WRaycastComponent

public:
  WRaycastComponent();
  ~WRaycastComponent();

  float GetCurrentDistance() const { return m_fCurrentDistance; }     // [ scriptable ]
  WVec3 GetCurrentEndPosition() const;                               // [ scriptable ]
  bool HasHit() const { return m_fCurrentDistance < m_fMaxDistance; } // [ scriptable ]

  void SetRaycastEndObject(const char* szReference);                  // [ property ]

  WGameObjectHandle m_hRaycastEndObject;                             // [ property ]
  float m_fMaxDistance = 100.0f;                                      // [ property ]
  bool m_bForceTargetParentless = false;                              // [ property ]
  bool m_bDisableTargetObjectOnNoHit = false;                         // [ property ]
  WUInt8 m_uiCollisionLayerEndPoint = 0;                             // [ property ]
  WBitflags<WPhysicsShapeType> m_ShapeTypesToHit;                   // [ property ]
  WHashedString m_sChangeNotificationMsg;                            // [ property ]

private:
  void Update();

  const char* DummyGetter() const { return nullptr; }

  float m_fCurrentDistance = 0.0f;
  WPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;
};
