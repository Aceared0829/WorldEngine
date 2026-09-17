#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/AI/AiUtilitySystem.h>

RtsAiUtilitySystem::RtsAiUtilitySystem() = default;
RtsAiUtilitySystem::~RtsAiUtilitySystem() = default;

void RtsAiUtilitySystem::AddUtility(WUniquePtr<RtsAiUtility>&& pUtility)
{
  m_Utilities.PushBack(std::move(pUtility));
}

void RtsAiUtilitySystem::Reevaluate(WGameObject* pOwnerObject, WComponent* pOwnerComponent, WTime now, WTime frequency)
{
  if (now - m_LastUpdate < frequency)
    return;

  m_LastUpdate = now;

  double fBestPrio = 0;
  RtsAiUtility* pBestUtility = nullptr;

  for (const auto& pUtility : m_Utilities)
  {
    double prio = pUtility->ComputePriority(pOwnerObject, pOwnerComponent);

    if (prio > fBestPrio)
    {
      fBestPrio = prio;
      pBestUtility = pUtility.Borrow();
    }
  }

  if (pBestUtility == m_pActiveUtility)
    return;

  if (m_pActiveUtility)
    m_pActiveUtility->Deactivate(pOwnerObject, pOwnerComponent);

  m_pActiveUtility = pBestUtility;

  if (m_pActiveUtility)
    m_pActiveUtility->Activate(pOwnerObject, pOwnerComponent);
}

bool RtsAiUtilitySystem::Execute(WGameObject* pOwnerObject, WComponent* pOwnerComponent, WTime now)
{
  if (m_pActiveUtility == nullptr)
    return false;

  m_pActiveUtility->Execute(pOwnerObject, pOwnerComponent, now);
  return true;
}

//////////////////////////////////////////////////////////////////////////

RtsAiUtility::RtsAiUtility() = default;
RtsAiUtility::~RtsAiUtility() = default;
