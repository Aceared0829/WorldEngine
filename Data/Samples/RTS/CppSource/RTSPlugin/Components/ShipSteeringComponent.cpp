#include <RTSPlugin/RTSPluginPCH.h>

#include <Foundation/Utilities/Stats.h>
#include <RTSPlugin/Components/ShipSteeringComponent.h>
#include <RTSPlugin/GameState/RTSGameState.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(RtsShipSteeringComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Speed", m_fMaxSpeed)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.1f, 100.0f)),
    W_MEMBER_PROPERTY("Acceleration", m_fMaxAcceleration)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.1f, 100.0f)),
    W_MEMBER_PROPERTY("Deceleration", m_fMaxDeceleration)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.1f, 100.0f)),
    W_MEMBER_PROPERTY("TurnSpeed", m_MaxTurnSpeed)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(90.0f))),
  }
  W_END_PROPERTIES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(RtsMsgNavigateTo, OnMsgNavigateTo),
    W_MESSAGE_HANDLER(RtsMsgStopNavigation, OnMsgStopNavigation),
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

RtsShipSteeringComponent::RtsShipSteeringComponent() = default;
RtsShipSteeringComponent::~RtsShipSteeringComponent() = default;

void RtsShipSteeringComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fMaxSpeed;
  s << m_fMaxAcceleration;
  s << m_MaxTurnSpeed;
  s << m_fMaxDeceleration;
}

void RtsShipSteeringComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fMaxSpeed;
  s >> m_fMaxAcceleration;
  s >> m_MaxTurnSpeed;
  s >> m_fMaxDeceleration;
}

void RtsShipSteeringComponent::OnMsgNavigateTo(RtsMsgNavigateTo& ref_msg)
{
  m_vTargetPosition = ref_msg.m_vTargetPosition;
  m_Mode = RtsShipSteeringComponent::Mode::Steering;
}

void RtsShipSteeringComponent::OnMsgStopNavigation(RtsMsgStopNavigation& ref_msg)
{
  if (m_Mode == RtsShipSteeringComponent::Mode::Steering)
  {
    m_Mode = RtsShipSteeringComponent::Mode::Stop;

    RtsMsgArrivedAtLocation msg2;
    GetOwner()->SendMessage(msg2);
  }
}

void RtsShipSteeringComponent::UpdateSteering()
{
  if (m_Mode == RtsShipSteeringComponent::Mode::None)
    return;

  const float tDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

  WTransform transform = GetOwner()->GetGlobalTransform();

  const WVec2 vOwnerPos = transform.m_vPosition.GetAsVec2();
  const WVec2 vOwnerDir = GetOwner()->GetGlobalDirForwards().GetAsVec2();
  const WVec2 vOwnerRight(-vOwnerDir.y, vOwnerDir.x);

  const float fArriveDistance = m_fCurrentSpeed * m_fCurrentSpeed / m_fMaxDeceleration;

  const WVec2 vDistToTarget = m_vTargetPosition - vOwnerPos;
  float fDistToTarget = vDistToTarget.GetLength();
  const WVec2 vDirToTarget = (fDistToTarget <= 0) ? vOwnerDir : (vDistToTarget / fDistToTarget);

  const bool bTargetIsInFront = vOwnerDir.Dot(vDirToTarget) > WMath::Cos(WAngle::MakeFromDegree(60));
  const bool bTargetIsRight = (vOwnerRight.Dot(m_vTargetPosition) - vOwnerRight.Dot(vOwnerPos)) > 0;

  if (m_Mode == RtsShipSteeringComponent::Mode::Stop)
  {
    fDistToTarget = 0;
  }

  if (fDistToTarget <= fArriveDistance)
  {
    m_fCurrentSpeed -= m_fMaxDeceleration * tDiff;
  }
  else
  {
    if (bTargetIsInFront)
    {
      m_fCurrentSpeed = WMath::Min(m_fCurrentSpeed + m_fMaxAcceleration * tDiff, m_fMaxSpeed);
    }
    else
    {
      m_fCurrentSpeed = WMath::Max(m_fCurrentSpeed - m_fMaxDeceleration * tDiff, 0.0f);
    }
  }

  if (fDistToTarget > 0.5f)
  {
    const WAngle angleToTurn = vDirToTarget.GetAngleBetween(vOwnerDir);

    if (angleToTurn != WAngle())
    {
      const WAngle toTurnNow = (bTargetIsRight ? 1.0f : -1.0f) * WMath::Min(angleToTurn, m_MaxTurnSpeed * tDiff);

      WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), toTurnNow);

      transform.m_qRotation = qRot * transform.m_qRotation;
    }
  }
  else
  {
    if (m_Mode == RtsShipSteeringComponent::Mode::Steering)
    {
      m_Mode = RtsShipSteeringComponent::Mode::Stop;

      RtsMsgArrivedAtLocation msg;
      GetOwner()->SendMessage(msg);
    }

    if (m_fCurrentSpeed <= 0)
    {
      m_fCurrentSpeed = 0;
      m_Mode = RtsShipSteeringComponent::Mode::None;
    }
  }

  {
    const WVec2 vMove = vOwnerDir * m_fCurrentSpeed * tDiff;
    transform.m_vPosition += vMove.GetAsVec3(0);
    transform.m_vPosition.z = 0;
  }

  GetOwner()->SetGlobalTransform(transform);
}

//////////////////////////////////////////////////////////////////////////

RtsShipSteeringComponentManager::RtsShipSteeringComponentManager(WWorld* pWorld)
  : WComponentManager<class RtsShipSteeringComponent, WBlockStorageType::Compact>(pWorld)
{
}

void RtsShipSteeringComponentManager::Initialize()
{
  // configure this system to update all components multi-threaded (async phase)

  auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(RtsShipSteeringComponentManager::SteeringUpdate, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = WWorldUpdatePhase::Async;
  desc.m_uiAsyncPhaseBatchSize = 8;

  RegisterUpdateFunction(desc);
}

void RtsShipSteeringComponentManager::SteeringUpdate(const WWorldModule::UpdateContext& context)
{
  if (RTSGameState::GetSingleton() == nullptr || RTSGameState::GetSingleton()->GetActiveGameMode() != RtsActiveGameMode::BattleMode)
    return;

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
    {
      it->UpdateSteering();
    }
  }
}
