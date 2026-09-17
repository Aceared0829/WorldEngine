#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

struct WMsgComponentInternalTrigger;
using WTimedDeathComponentManager = WComponentManager<class WTimedDeathComponent, WBlockStorageType::Compact>;
using WPrefabResourceHandle = WTypedResourceHandle<class WPrefabResource>;

/// This component deletes the object it is attached to after a timeout.
///
/// \note The timeout must be set immediately after component creation. Once the component
/// has been initialized (start of the next frame), changing the value has no effect.
/// The only way around this, is to delete the entire component and create a new one.
class W_GAMEENGINE_DLL WTimedDeathComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WTimedDeathComponent, WComponent, WTimedDeathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  /// Once this function has been executed, the timeout for deletion is fixed and cannot be reset.
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // WTimedDeathComponent

public:
  WTimedDeathComponent();
  ~WTimedDeathComponent();

  WTime m_MinDelay = WTime::MakeFromSeconds(1.0);   // [ property ]
  WTime m_DelayRange = WTime::MakeFromSeconds(0.0); // [ property ]

  WPrefabResourceHandle m_hTimeoutPrefab;            ///< [ property ] Spawned when the component is killed due to the timeout

protected:
  void OnTriggered(WMsgComponentInternalTrigger& msg);
};
