#pragma once

#include <FmodPlugin/Components/FmodComponent.h>

class WFmodListenerComponentManager : public WComponentManager<class WFmodListenerComponent, WBlockStorageType::Compact>
{
public:
  WFmodListenerComponentManager(WWorld* pWorld);

  virtual void Initialize() override;

private:
  void UpdateListeners(const WWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

/// Represents the position of the sound listener
class W_FMODPLUGIN_DLL WFmodListenerComponent : public WFmodComponent
{
  W_DECLARE_COMPONENT_TYPE(WFmodListenerComponent, WFmodComponent, WFmodListenerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WFmodComponent

private:
  virtual void WFmodComponentIsAbstract() override {}

  //////////////////////////////////////////////////////////////////////////
  // WFmodListenerComponent

public:
  WFmodListenerComponent();
  ~WFmodListenerComponent();

  WUInt8 m_uiListenerIndex = 0; // [ property ]

protected:
  void Update();
};
