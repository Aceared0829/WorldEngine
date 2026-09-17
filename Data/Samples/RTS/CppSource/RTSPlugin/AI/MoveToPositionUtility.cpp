#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/AI/MoveToPositionUtility.h>
#include <RTSPlugin/Components/UnitComponent.h>
#include <RTSPlugin/GameState/RTSGameState.h>

void RtsMoveToPositionAiUtility::Activate(WGameObject* pOwnerObject, WComponent* pOwnerComponent) {}

void RtsMoveToPositionAiUtility::Deactivate(WGameObject* pOwnerObject, WComponent* pOwnerComponent)
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  RtsMsgStopNavigation msg;
  pOwnerObject->SendMessage(msg);
}

void RtsMoveToPositionAiUtility::Execute(WGameObject* pOwnerObject, WComponent* pOwnerComponent, WTime now)
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  RtsMsgNavigateTo msg;
  msg.m_vTargetPosition = pUnit->m_vAssignedPosition;

  pOwnerObject->SendMessage(msg);

  // shoot at close by enemies
  pUnit->AttackClosestEnemey(7.0f, 10.0f);
}

double RtsMoveToPositionAiUtility::ComputePriority(WGameObject* pOwnerObject, WComponent* pOwnerComponent) const
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  if (pUnit->m_UnitMode == RtsUnitMode::MoveToPosition)
  {
    // always follow the move command with high priority
    return 1000;
  }

  if (pUnit->m_UnitMode == RtsUnitMode::GuardLocation)
  {
    // return to assigned position when strayed too far from it
    float fDistSqr = (pOwnerObject->GetGlobalPosition().GetAsVec2() - pUnit->m_vAssignedPosition).GetLengthSquared();

    // don't participate at all, when very close to the target
    // otherwise this often gets activated and executes the shoot at action
    if (fDistSqr > WMath::Square(3.0f))
      return fDistSqr;
  }

  return 0;
}
