#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Effects/Wind/SimpleWindComponent.h>
#include <GameEngine/Effects/Wind/SimpleWindWorldModule.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSimpleWindComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("MinWindStrength", WWindStrength, m_MinWindStrength),
    W_ENUM_MEMBER_PROPERTY("MaxWindStrength", WWindStrength, m_MaxWindStrength),
    W_MEMBER_PROPERTY("MaxDeviation", m_Deviation)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(180))),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects/Wind"),
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 0.5f, WColor::DodgerBlue),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSimpleWindComponent::WSimpleWindComponent() = default;
WSimpleWindComponent::~WSimpleWindComponent() = default;

void WSimpleWindComponent::Update()
{
  WSimpleWindWorldModule* pWindModule = GetWorld()->GetModule<WSimpleWindWorldModule>();

  if (pWindModule == nullptr)
    return;

  const WTime tCur = GetWorld()->GetClock().GetAccumulatedTime();
  const float fLerp = static_cast<float>((tCur - m_LastChange).GetSeconds() / (m_NextChange - m_LastChange).GetSeconds());

  WVec3 vCurWind;

  if (fLerp >= 1.0f)
  {
    ComputeNextState();

    vCurWind = m_vLastDirection * m_fLastStrength;
  }
  else
  {
    const float fCurStrength = WMath::Lerp(m_fLastStrength, m_fNextStrength, fLerp);
    const WVec3 vCurDir = WMath::Lerp(m_vLastDirection, m_vNextDirection, fLerp);

    vCurWind = vCurDir * fCurStrength;
  }

  pWindModule->SetFallbackWind(vCurWind);
}

void WSimpleWindComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_MinWindStrength;
  s << m_MaxWindStrength;
  s << m_Deviation;
}

void WSimpleWindComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  if (uiVersion == 1)
  {
    float m_fWindStrengthMin, m_fWindStrengthMax;
    s >> m_fWindStrengthMin;
    s >> m_fWindStrengthMax;
  }
  else
  {
    s >> m_MinWindStrength;
    s >> m_MaxWindStrength;
  }

  s >> m_Deviation;
}

void WSimpleWindComponent::OnActivated()
{
  SUPER::OnActivated();

  m_fNextStrength = WWindStrength::GetInMetersPerSecond(m_MinWindStrength);
  m_vNextDirection = GetOwner()->GetGlobalDirForwards();
  m_NextChange = GetWorld()->GetClock().GetAccumulatedTime();
  m_LastChange = m_NextChange - WTime::MakeFromSeconds(1);

  ComputeNextState();
}

void WSimpleWindComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  WSimpleWindWorldModule* pWindModule = GetWorld()->GetModule<WSimpleWindWorldModule>();

  if (pWindModule == nullptr)
    return;

  pWindModule->SetFallbackWind(WVec3::MakeZero());
}

void WSimpleWindComponent::ComputeNextState()
{
  m_fLastStrength = m_fNextStrength;
  m_vLastDirection = m_vNextDirection;
  m_LastChange = GetWorld()->GetClock().GetAccumulatedTime();

  auto& rng = GetWorld()->GetRandomNumberGenerator();

  const WEnum<WWindStrength> minWind = WMath::Min(m_MinWindStrength, m_MaxWindStrength);
  const WEnum<WWindStrength> maxWind = WMath::Max(m_MinWindStrength, m_MaxWindStrength);

  const float fMinStrength = WWindStrength::GetInMetersPerSecond(minWind);
  const float fMaxStrength = WWindStrength::GetInMetersPerSecond(maxWind);

  float fStrengthDiff = fMaxStrength - fMinStrength;
  float fStrengthChange = fStrengthDiff * 0.2f;

  if (minWind == WWindStrength::None && maxWind == WWindStrength::None)
  {
    m_NextChange = m_LastChange + WTime::MakeFromSeconds(0.2f);
    m_fNextStrength *= 0.5f;
  }
  else
  {
    m_NextChange = m_LastChange + WTime::MakeFromSeconds(rng.DoubleMinMax(2.0f, 5.0f));
    m_fNextStrength = WMath::Clamp<float>(m_fLastStrength + (float)rng.DoubleMinMax(-fStrengthChange, +fStrengthChange), fMinStrength, fMaxStrength);
  }

  const WVec3 vMainDir = GetOwner()->GetGlobalDirForwards();

  if (m_Deviation < WAngle::MakeFromDegree(1))
    m_vNextDirection = vMainDir;
  else
    m_vNextDirection = WVec3::MakeRandomDeviation(rng, m_Deviation, vMainDir);

  WCoordinateSystem cs;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), cs);
  const float fRemoveUp = m_vNextDirection.Dot(cs.m_vUpDir);

  m_vNextDirection -= cs.m_vUpDir * fRemoveUp;
  m_vNextDirection.NormalizeIfNotZero(WVec3::MakeZero()).IgnoreResult();
}

void WSimpleWindComponent::Initialize()
{
  SUPER::Initialize();

  // make sure to query the wind interface before any simulation starts
  /*WWindWorldModuleInterface* pWindInterface =*/GetWorld()->GetOrCreateModule<WSimpleWindWorldModule>();
}



W_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_SimpleWindComponent);
