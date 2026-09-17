#pragma once

#include <RTSPlugin/RTSPluginDLL.h>

#include <Core/World/ComponentManager.h>

struct RtsMsgSetTarget;

using RtsTorpedoComponentManager = WComponentManagerSimple<class RtsTorpedoComponent, WComponentUpdateType::WhenSimulating>;

class W_RTSPLUGIN_DLL RtsTorpedoComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(RtsTorpedoComponent, WComponent, RtsTorpedoComponentManager);

public:
  RtsTorpedoComponent();
  ~RtsTorpedoComponent();

  //////////////////////////////////////////////////////////////////////////
  // WComponent interface

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  float m_fSpeed = 10.0f;
  WInt16 m_iDamage = 10;

  //////////////////////////////////////////////////////////////////////////
  //
public:
  void OnMsgSetTarget(RtsMsgSetTarget& ref_msg);

protected:
  void Update();

  WGameObjectHandle m_hTargetObject;
  WVec2 m_vTargetPosition;
};
