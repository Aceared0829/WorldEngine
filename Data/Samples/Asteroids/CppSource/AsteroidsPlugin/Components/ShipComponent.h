#pragma once

#include <Core/World/World.h>

class ShipComponent;
using ShipComponentManager = WComponentManagerSimple<ShipComponent, WComponentUpdateType::WhenSimulating>;

class ShipComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(ShipComponent, WComponent, ShipComponentManager);

public:
  ShipComponent();

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override {}
  virtual void DeserializeComponent(WWorldReader& inout_stream) override {}

  void Update();

  void AddExternalForce(const WVec3& vVel);

  void SetIsShooting(bool b);

  bool IsAlive() const { return m_fHealth > 0.0f; }

  float m_fHealth = 0;
  WInt32 m_iPlayerIndex = -1;

private:
  void Explode();

  WVec3 m_vExternalForce = WVec3::MakeZero();
  WVec3 m_vVelocity = WVec3::MakeZero();
  bool m_bIsShooting = false;
  WTime m_CurShootCooldown;
  float m_fAmmunition;
};
