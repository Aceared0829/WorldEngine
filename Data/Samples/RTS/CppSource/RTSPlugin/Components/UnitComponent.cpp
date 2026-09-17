#include <RTSPlugin/RTSPluginPCH.h>

#include <Foundation/Utilities/Stats.h>
#include <RTSPlugin/AI/AttackUnitAiUtility.h>
#include <RTSPlugin/AI/GuardLocationUtility.h>
#include <RTSPlugin/AI/MoveToPositionUtility.h>
#include <RTSPlugin/Components/UnitComponent.h>
#include <RTSPlugin/GameState/RTSGameState.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(RtsUnitComponent, 2, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MaxHealth", m_uiMaxHealth)->AddAttributes(new WDefaultValueAttribute(100)),
    W_MEMBER_PROPERTY("CurHealth", m_uiCurHealth),
    W_RESOURCE_MEMBER_PROPERTY("OnDestroyedPrefab", m_hOnDestroyedPrefab)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab", WDependencyFlags::Package)),
  }
  W_END_PROPERTIES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(RtsMsgSetTarget, OnMsgSetTarget),
    W_MESSAGE_HANDLER(RtsMsgAssignPosition, OnMsgAssignPosition),
    W_MESSAGE_HANDLER(RtsMsgApplyDamage, OnMsgApplyDamage),
    W_MESSAGE_HANDLER(RtsMsgGatherUnitStats, OnMsgGatherUnitStats),
    W_MESSAGE_HANDLER(RtsMsgArrivedAtLocation, OnMsgArrivedAtLocation),
  }
  W_END_MESSAGEHANDLERS;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("RTS Sample"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

RtsUnitComponent::RtsUnitComponent() = default;
RtsUnitComponent::~RtsUnitComponent() = default;

void RtsUnitComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiMaxHealth;
  s << m_uiCurHealth;
  s << m_hOnDestroyedPrefab;
}

void RtsUnitComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_uiMaxHealth;
  s >> m_uiCurHealth;

  if (uiVersion >= 2)
  {
    s >> m_hOnDestroyedPrefab;
  }
}

void RtsUnitComponent::OnMsgAssignPosition(RtsMsgAssignPosition& msg)
{
  m_UnitMode = RtsUnitMode::MoveToPosition;
  m_vAssignedPosition = msg.m_vTargetPosition;
  m_bModeChanged = true;
}

void RtsUnitComponent::OnMsgSetTarget(RtsMsgSetTarget& msg)
{
  m_hAssignedUnitToAttack = msg.m_hObject;
  m_UnitMode = RtsUnitMode::AttackUnit;
  m_bModeChanged = true;
}

void RtsUnitComponent::OnMsgApplyDamage(RtsMsgApplyDamage& msg)
{
  WInt32 lastHealth = m_uiCurHealth;

  if (msg.m_iDamage >= m_uiCurHealth)
  {
    m_uiCurHealth = 0;
  }
  else
  {
    m_uiCurHealth -= (WInt16)msg.m_iDamage;
  }

  // in case damage was negative
  m_uiCurHealth = WMath::Min(m_uiCurHealth, m_uiMaxHealth);

  RtsMsgUnitHealthStatus msg2;
  msg2.m_uiCurHealth = m_uiCurHealth;
  msg2.m_uiMaxHealth = m_uiMaxHealth;
  msg2.m_iDifference = static_cast<WInt16>(m_uiCurHealth - lastHealth);

  // theoretically the sub-systems could give us a health boost (or additional damage) here
  GetOwner()->SendMessageRecursive(msg2);

  if (m_uiCurHealth == 0)
  {
    OnUnitDestroyed();
  }
}

void RtsUnitComponent::OnMsgGatherUnitStats(RtsMsgGatherUnitStats& msg)
{
  msg.m_uiCurHealth = m_uiCurHealth;
  msg.m_uiMaxHealth = m_uiMaxHealth;
}


void RtsUnitComponent::OnMsgArrivedAtLocation(RtsMsgArrivedAtLocation& msg)
{
  if (m_UnitMode == RtsUnitMode::MoveToPosition)
  {
    m_UnitMode = RtsUnitMode::GuardLocation;
    m_vAssignedPosition = GetOwner()->GetGlobalPosition().GetAsVec2();
  }
}

void RtsUnitComponent::OnUnitDestroyed()
{
  if (m_hOnDestroyedPrefab.IsValid())
  {
    WResourceLock<WPrefabResource> pPrefab(m_hOnDestroyedPrefab, WResourceAcquireMode::AllowLoadingFallback);

    WPrefabInstantiationOptions options;
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), options);
  }

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}

void RtsUnitComponent::UpdateUnit()
{
  const WTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  // Unit state machine for defining what the unit is 'supposed to do'
  // affects what the unit decides to actually do through the utility system
  switch (m_UnitMode)
  {
    case RtsUnitMode::GuardLocation:
      break;

    case RtsUnitMode::MoveToPosition:
      break;

    case RtsUnitMode::AttackUnit:
    {
      // if the target unit is dead, revert to guarding the current location

      WGameObject* pTarget;
      if (!GetWorld()->TryGetObject(m_hAssignedUnitToAttack, pTarget) || pTarget == GetOwner())
      {
        m_UnitMode = RtsUnitMode::GuardLocation;
        m_vAssignedPosition = GetOwner()->GetGlobalPosition().GetAsVec2();

        // could add an offset in the current travel direction for smoother stops
      }

      break;
    }
  }

  m_pAiSystem->Reevaluate(GetOwner(), this, tNow, m_bModeChanged ? WTime() : WTime::MakeFromSeconds(0.5));
  m_pAiSystem->Execute(GetOwner(), this, tNow);

  m_bModeChanged = false;
}

void RtsUnitComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // 0 means 'whatever max Health is set to'
  if (m_uiCurHealth == 0)
    m_uiCurHealth = m_uiMaxHealth;

  m_uiCurHealth = WMath::Min(m_uiCurHealth, m_uiMaxHealth);
  m_vAssignedPosition = GetOwner()->GetGlobalPosition().GetAsVec2();

  // Setup the AI system
  {
    m_pAiSystem = W_DEFAULT_NEW(RtsAiUtilitySystem);

    {
      WUniquePtr<RtsGuardLocationAiUtility> pUtility = W_DEFAULT_NEW(RtsGuardLocationAiUtility);
      m_pAiSystem->AddUtility(std::move(pUtility));
    }

    {
      WUniquePtr<RtsAttackUnitAiUtility> pUtility = W_DEFAULT_NEW(RtsAttackUnitAiUtility);
      m_pAiSystem->AddUtility(std::move(pUtility));
    }

    {
      WUniquePtr<RtsMoveToPositionAiUtility> pUtility = W_DEFAULT_NEW(RtsMoveToPositionAiUtility);
      m_pAiSystem->AddUtility(std::move(pUtility));
    }
  }
}

//////////////////////////////////////////////////////////////////////////

RtsUnitComponentManager::RtsUnitComponentManager(WWorld* pWorld)
  : WComponentManager<class RtsUnitComponent, WBlockStorageType::FreeList>(pWorld)
{
}

void RtsUnitComponentManager::Initialize()
{
  // configure this system to update all components multi-threaded (async phase)

  auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(RtsUnitComponentManager::UnitUpdate, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = WWorldUpdatePhase::PostAsync;
  // desc.m_uiGranularity = 8;

  RegisterUpdateFunction(desc);
}

void RtsUnitComponentManager::UnitUpdate(const WWorldModule::UpdateContext& context)
{
  if (RTSGameState::GetSingleton() == nullptr || RTSGameState::GetSingleton()->GetActiveGameMode() != RtsActiveGameMode::BattleMode)
    return;

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
    {
      it->UpdateUnit();
    }
  }
}

WGameObject* RtsUnitComponent::FindClosestEnemy(float fMaxRadius) const
{
  struct Payload
  {
    WGameObject* pBestObject = nullptr;
    float fBestDistSQR;
    WVec3 m_vOwnPosition;
    WUInt16 m_uiOwnTeamID;
  };

  Payload pl;
  pl.fBestDistSQR = WMath::Square(fMaxRadius);
  pl.m_vOwnPosition = GetOwner()->GetGlobalPosition();
  pl.m_uiOwnTeamID = GetOwner()->GetTeamID();

  WSpatialSystem::QueryCallback cb = [&pl](WGameObject* pObject) -> WVisitorExecution::Enum
  {
    if (pObject->GetTeamID() == pl.m_uiOwnTeamID)
      return WVisitorExecution::Skip;

    RtsUnitComponent* pUnit = nullptr;
    if (pObject->TryGetComponentOfBaseType(pUnit))
    {
      const float dist = (pObject->GetGlobalPosition() - pl.m_vOwnPosition).GetLengthSquared();

      if (dist < pl.fBestDistSQR)
      {
        pl.fBestDistSQR = dist;
        pl.pBestObject = pObject;
      }
    }

    return WVisitorExecution::Continue;
  };

  RTSGameState::GetSingleton()->InspectObjectsInArea(pl.m_vOwnPosition.GetAsVec2(), fMaxRadius, cb);

  return pl.pBestObject;
}

void RtsUnitComponent::FireAt(WGameObjectHandle hUnit)
{
  const WTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (tNow - m_TimeLastShot >= WTime::MakeFromSeconds(0.75))
  {
    m_TimeLastShot = tNow;

    RtsMsgSetTarget msg;
    msg.m_hObject = hUnit;

    WGameObject* pSpawned = RTSGameState::GetSingleton()->SpawnNamedObjectAt(GetOwner()->GetGlobalTransform(), "ProtonTorpedo1", GetOwner()->GetTeamID());

    pSpawned->PostMessage(msg, WTime::MakeZero(), WObjectMsgQueueType::AfterInitialized);
  }
}

WGameObject* RtsUnitComponent::AttackClosestEnemey(float fSearchRadius, float fIgnoreRadius)
{
  if (m_hCurrentUnitToAttack.IsInvalidated())
  {
    // TODO: don't check all the time

    if (WGameObject* pEnemy = FindClosestEnemy(fSearchRadius))
    {
      m_hCurrentUnitToAttack = pEnemy->GetHandle();
    }
  }

  WGameObject* pEnemy = nullptr;
  if (m_hCurrentUnitToAttack.IsInvalidated())
    return nullptr;

  if (!GetWorld()->TryGetObject(m_hCurrentUnitToAttack, pEnemy))
  {
    m_hCurrentUnitToAttack.Invalidate();
    return nullptr;
  }

  if ((pEnemy->GetGlobalPosition() - GetOwner()->GetGlobalPosition()).GetLengthSquared() > WMath::Square(fIgnoreRadius))
  {
    m_hCurrentUnitToAttack.Invalidate();
    return nullptr;
  }

  FireAt(m_hCurrentUnitToAttack);
  return pEnemy;
}
