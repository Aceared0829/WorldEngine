#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/Components/ComponentMessages.h>
#include <RTSPlugin/Components/TorpedoComponent.h>
#include <RTSPlugin/GameState/RTSGameState.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(RtsTorpedoComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new WDefaultValueAttribute(10.0f)),
    W_MEMBER_PROPERTY("Damage", m_iDamage)->AddAttributes(new WDefaultValueAttribute(10)),
  }
  W_END_PROPERTIES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(RtsMsgSetTarget, OnMsgSetTarget),
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

RtsTorpedoComponent::RtsTorpedoComponent()
{
  m_vTargetPosition.SetZero();
}

RtsTorpedoComponent::~RtsTorpedoComponent() = default;

void RtsTorpedoComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fSpeed;
  s << m_iDamage;
}

void RtsTorpedoComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fSpeed;
  s >> m_iDamage;
}

void RtsTorpedoComponent::OnMsgSetTarget(RtsMsgSetTarget& ref_msg)
{
  m_vTargetPosition.SetZero();
  m_hTargetObject = ref_msg.m_hObject;
}

void RtsTorpedoComponent::Update()
{
  // TODO: do this check in the component manager
  if (RTSGameState::GetSingleton() == nullptr || RTSGameState::GetSingleton()->GetActiveGameMode() != RtsActiveGameMode::BattleMode)
    return;

  WGameObject* pObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pObject))
  {
    m_vTargetPosition = pObject->GetGlobalTransform().m_vPosition.GetAsVec2();
  }

  const WTransform transform = GetOwner()->GetGlobalTransform();
  const WVec2 vCurPos = transform.m_vPosition.GetAsVec2();
  const WVec2 vDir = m_vTargetPosition - vCurPos;
  const float fDist = vDir.GetLength();

  const float tDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

  bool bExplode = false;

  if (fDist > 0)
  {
    const WVec2 vDirNorm = vDir / fDist;
    float fTravelDist = m_fSpeed * tDiff;

    if (fTravelDist >= fDist)
    {
      fTravelDist = fDist;
      bExplode = true;
    }

    const WVec3 vNewPos = transform.m_vPosition + vDirNorm.GetAsVec3(0) * fTravelDist;

    GetOwner()->SetGlobalPosition(vNewPos);
  }
  else
  {
    bExplode = true;
  }

  if (bExplode)
  {
    RtsMsgApplyDamage msg;
    msg.m_iDamage = m_iDamage;

    if (!m_hTargetObject.IsInvalidated())
    {
      GetWorld()->SendMessage(m_hTargetObject, msg);
    }

    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
  }
}
