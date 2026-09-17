#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/AI/AttackUnitAiUtility.h>
#include <RTSPlugin/Components/UnitComponent.h>
#include <RTSPlugin/GameState/RTSGameState.h>

double RtsAttackUnitAiUtility::ComputePriority(WGameObject* pOwnerObject, WComponent* pOwnerComponent) const
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  if (pUnit->m_UnitMode != RtsUnitMode::AttackUnit)
    return 0;

  return 100;
}

void RtsAttackUnitAiUtility::Activate(WGameObject* pOwnerObject, WComponent* pOwnerComponent) {}

void RtsAttackUnitAiUtility::Deactivate(WGameObject* pOwnerObject, WComponent* pOwnerComponent) {}

void RtsAttackUnitAiUtility::Execute(WGameObject* pOwnerObject, WComponent* pOwnerComponent, WTime now)
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  WGameObject* pTarget = nullptr;
  if (!pOwnerObject->GetWorld()->TryGetObject(pUnit->m_hAssignedUnitToAttack, pTarget))
    return;

  const WVec2 vTargetPos = pTarget->GetGlobalPosition().GetAsVec2();

  if ((pOwnerObject->GetGlobalPosition().GetAsVec2() - vTargetPos).GetLengthSquared() > 100)
  {
    RtsMsgNavigateTo msg;
    msg.m_vTargetPosition = vTargetPos;

    pOwnerObject->SendMessage(msg);
    return;
  }

  pUnit->FireAt(pUnit->m_hAssignedUnitToAttack);
}
