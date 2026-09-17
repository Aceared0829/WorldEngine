#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/AI/GuardLocationUtility.h>
#include <RTSPlugin/Components/UnitComponent.h>

void RtsGuardLocationAiUtility::Activate(WGameObject* pOwnerObject, WComponent* pOwnerComponent) {}

void RtsGuardLocationAiUtility::Deactivate(WGameObject* pOwnerObject, WComponent* pOwnerComponent) {}

void RtsGuardLocationAiUtility::Execute(WGameObject* pOwnerObject, WComponent* pOwnerComponent, WTime now)
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  // shoot at close by enemies
  WGameObject* pEnemy = pUnit->AttackClosestEnemey(7.0f, 10.0f);

  if (pEnemy)
  {
    WVec3 vDiff = pEnemy->GetGlobalPosition() - pOwnerObject->GetGlobalPosition();

    if (vDiff.GetLengthSquared() > WMath::Square(5.0f))
    {
      vDiff.Normalize();

      // harass the enemy when it moves away
      RtsMsgNavigateTo msg;
      msg.m_vTargetPosition = pOwnerObject->GetGlobalPosition().GetAsVec2() + vDiff.GetAsVec2();

      pOwnerObject->SendMessage(msg);
    }
  }
}

double RtsGuardLocationAiUtility::ComputePriority(WGameObject* pOwnerObject, WComponent* pOwnerComponent) const
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  if (pUnit->m_UnitMode == RtsUnitMode::GuardLocation)
  {
    return 10.0f;
  }

  return 0;
}
