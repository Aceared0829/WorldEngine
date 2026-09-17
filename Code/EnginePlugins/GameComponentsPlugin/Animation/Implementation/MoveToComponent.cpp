#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Animation/MoveToComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WMoveToComponent, 3, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning),
    W_MEMBER_PROPERTY("TranslationSpeed", m_fMaxTranslationSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("TranslationAcceleration", m_fTranslationAcceleration),
    W_MEMBER_PROPERTY("TranslationDeceleration", m_fTranslationDeceleration),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetTargetPosition, In, "position"),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE;
// clang-format on

WMoveToComponent::WMoveToComponent() = default;
WMoveToComponent::~WMoveToComponent() = default;

void WMoveToComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_vTargetPosition = GetOwner()->GetGlobalPosition();
}

void WMoveToComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_Flags.GetValue();
  s << m_fCurTranslationSpeed;
  s << m_fMaxTranslationSpeed;
  s << m_fTranslationAcceleration;
  s << m_fTranslationDeceleration;
  s << m_vTargetPosition;
}


void WMoveToComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  auto& s = inout_stream.GetStream();

  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_Flags;
  s >> m_fCurTranslationSpeed;
  s >> m_fMaxTranslationSpeed;
  s >> m_fTranslationAcceleration;
  s >> m_fTranslationDeceleration;
  s >> m_vTargetPosition;
}

void WMoveToComponent::SetRunning(bool bRunning)
{
  m_Flags.AddOrRemove(WMoveToComponentFlags::Running, bRunning);
}

bool WMoveToComponent::IsRunning() const
{
  return m_Flags.IsSet(WMoveToComponentFlags::Running);
}

void WMoveToComponent::SetTargetPosition(const WVec3& vPos)
{
  if (!m_vTargetPosition.IsEqual(vPos, 0.001f))
  {
    m_vTargetPosition = vPos;
    SetRunning(true);
  }
}

static float CalculateNewSpeed(float fRemainingDistance, float fCurSpeed, float fMaxSpeed, float fAcceleration, float fDeceleration, float fTimeStep)
{
  float fMaxAllowedSpeed = fMaxSpeed;

  if (fDeceleration > 0)
  {
    const float fMaxSpeedForDistance = WMath::Sqrt(2.0f * fDeceleration * fRemainingDistance);
    fMaxAllowedSpeed = WMath::Min(fMaxSpeed, fMaxSpeedForDistance);
  }

  float fMaxNewSpeed = fMaxSpeed;

  if (fAcceleration > 0)
  {
    fMaxNewSpeed = fCurSpeed + fTimeStep * fAcceleration;
  }

  return WMath::Clamp(fMaxNewSpeed, 0.0f, fMaxAllowedSpeed);
}

void WMoveToComponent::Update()
{
  if (!m_Flags.IsAnySet(WMoveToComponentFlags::Running))
    return;

  WGameObject* pOwner = GetOwner();

  const WVec3 vCurPos = pOwner->GetGlobalPosition();

  WVec3 vDiff = m_vTargetPosition - vCurPos;
  const float fRemainingLength = vDiff.GetLength();

  if (WMath::IsZero(fRemainingLength, 0.002f))
  {
    SetRunning(false);
    pOwner->SetGlobalPosition(m_vTargetPosition);

    WMsgAnimationReachedEnd msg;
    m_ReachedEndMsgSender.SendEventMessage(msg, this, GetOwner());

    return;
  }

  const WVec3 vDir = vDiff / fRemainingLength;

  m_fCurTranslationSpeed = CalculateNewSpeed(fRemainingLength, m_fCurTranslationSpeed, m_fMaxTranslationSpeed, m_fTranslationAcceleration,
    m_fTranslationDeceleration, GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds());

  const float fTravelDist = WMath::Min<float>(fRemainingLength, m_fCurTranslationSpeed * GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds());

  pOwner->SetGlobalPosition(vCurPos + vDir * fTravelDist);
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Animation_Implementation_MoveToComponent);
