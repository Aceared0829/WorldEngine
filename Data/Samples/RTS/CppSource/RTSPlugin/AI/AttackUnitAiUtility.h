#pragma once

#include <RTSPlugin/AI/AiUtilitySystem.h>

class RtsAttackUnitAiUtility : public RtsUnitComponentUtility
{
public:
  virtual void Activate(WGameObject* pOwnerObject, WComponent* pOwnerComponent) override;
  virtual void Deactivate(WGameObject* pOwnerObject, WComponent* pOwnerComponent) override;
  virtual void Execute(WGameObject* pOwnerObject, WComponent* pOwnerComponent, WTime now) override;
  virtual double ComputePriority(WGameObject* pOwnerObject, WComponent* pOwnerComponent) const override;
};
