#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

class WPhysicsWorldModuleInterface;
struct WPhysicsOverlapResult;

class W_GAMECOMPONENTS_DLL WAreaDamageComponentManager : public WComponentManager<class WAreaDamageComponent, WBlockStorageType::FreeList>
{
  using SUPER = WComponentManager<WAreaDamageComponent, WBlockStorageType::FreeList>;

public:
  WAreaDamageComponentManager(WWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class WAreaDamageComponent;
  WPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
};

/// Used to apply damage to objects in the vicinity and push physical objects away.
///
/// The component queries for dynamic physics shapes within a given radius.
/// For all objects found it sends the messages WMsgPhysicsAddImpulse and WMsgDamage.
/// The former is used to apply a physical impulse, to push the objects away from the center of the explosion.
/// The second message is used to apply damage to the objects. This only has an effect, if those objects
/// handle that message type.///
///
/// This component is mainly meant as an example how to make gameplay functionality, such as explosions.
/// If its functionality is insufficient for your use-case, write your own and take its code as inspiration.
class W_GAMECOMPONENTS_DLL WAreaDamageComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WAreaDamageComponent, WComponent, WAreaDamageComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // WAreaDamageComponent

public:
  WAreaDamageComponent();
  ~WAreaDamageComponent();

  void ApplyAreaDamage();           // [ scriptable ]

  bool m_bTriggerOnCreation = true; // [ property ]
  WUInt8 m_uiCollisionLayer = 0;   // [ property ]
  float m_fRadius = 5.0f;           // [ property ]
  float m_fDamage = 10.0f;          // [ property ]
  WUInt8 m_uiImpulseType = 0;      // [ property ]
  float m_fImpulse = 100.0f;        // [ property ]
};
