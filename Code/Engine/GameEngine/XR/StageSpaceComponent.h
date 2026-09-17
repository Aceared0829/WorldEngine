#pragma once

#include <Core/World/SettingsComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRInterface.h>

//////////////////////////////////////////////////////////////////////////

using WStageSpaceComponentManager = WSettingsComponentManager<class WStageSpaceComponent>;

/// Singleton to set the type of stage space and its global transform in the world.
///
/// The global transform of the owner and the set stage space are read out by the XR
/// implementation every frame.
class W_GAMEENGINE_DLL WStageSpaceComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WStageSpaceComponent, WComponent, WStageSpaceComponentManager);

public:
  WStageSpaceComponent();
  ~WStageSpaceComponent();

  //
  // WDeviceTrackingComponent Interface
  //

  /// Sets the stage space used by the XR experience.
  void SetStageSpace(WEnum<WXRStageSpace> space);
  WEnum<WXRStageSpace> GetStageSpace() const;

protected:
  //
  // WComponent Interface
  //
  virtual void SerializeComponent(WWorldWriter& stream) const override;
  virtual void DeserializeComponent(WWorldReader& stream) override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

private:
  WEnum<WXRStageSpace> m_Space;
};
