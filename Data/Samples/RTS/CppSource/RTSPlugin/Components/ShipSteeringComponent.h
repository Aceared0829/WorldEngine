#pragma once

#include <RTSPlugin/Components/ComponentMessages.h>
#include <RTSPlugin/RTSPluginDLL.h>

class RtsShipSteeringComponentManager : public WComponentManager<class RtsShipSteeringComponent, WBlockStorageType::Compact>
{
public:
  RtsShipSteeringComponentManager(WWorld* pWorld);

  virtual void Initialize() override;

  void SteeringUpdate(const WWorldModule::UpdateContext& context);
};


class W_RTSPLUGIN_DLL RtsShipSteeringComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(RtsShipSteeringComponent, WComponent, RtsShipSteeringComponentManager);

public:
  RtsShipSteeringComponent();
  ~RtsShipSteeringComponent();

  //////////////////////////////////////////////////////////////////////////
  // WComponent interface

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  float m_fMaxSpeed = 5.0f;
  float m_fMaxAcceleration = 5.0f;
  float m_fMaxDeceleration = 10.0f;
  WAngle m_MaxTurnSpeed;

  //////////////////////////////////////////////////////////////////////////
  // Message Handlers
public:
  void OnMsgNavigateTo(RtsMsgNavigateTo& ref_msg);
  void OnMsgStopNavigation(RtsMsgStopNavigation& ref_msg);

  //////////////////////////////////////////////////////////////////////////
  //

protected:
  void UpdateSteering();

  enum class Mode
  {
    None,
    Steering,
    Stop
  };

  Mode m_Mode = Mode::None;
  WVec2 m_vTargetPosition;
  float m_fCurrentSpeed = 0;
};
