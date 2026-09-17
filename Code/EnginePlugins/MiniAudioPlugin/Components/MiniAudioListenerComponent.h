#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <MiniAudioPlugin/MiniAudioPluginDLL.h>

class WMiniAudioListenerComponentManager : public WComponentManager<class WMiniAudioListenerComponent, WBlockStorageType::Compact>
{
public:
  WMiniAudioListenerComponentManager(WWorld* pWorld);

  virtual void Initialize() override;

private:
  void UpdateListeners(const WWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

/// Represents the position of the sound listener
class W_MINIAUDIOPLUGIN_DLL WMiniAudioListenerComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WMiniAudioListenerComponent, WComponent, WMiniAudioListenerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WMiniAudioListenerComponent

public:
  WMiniAudioListenerComponent();
  ~WMiniAudioListenerComponent();

protected:
  void Update();
};
