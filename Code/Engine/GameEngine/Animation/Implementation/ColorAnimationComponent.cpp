#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/ColorAnimationComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WColorAnimationComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Gradient", GetColorGradient, SetColorGradient)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Gradient"), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("Duration", m_Duration),
    W_ENUM_MEMBER_PROPERTY("SetColorMode", WSetColorMode, m_SetColorMode),
    W_ENUM_MEMBER_PROPERTY("AnimationMode", WPropertyAnimMode, m_AnimationMode),
    W_ACCESSOR_PROPERTY("RandomStartOffset", GetRandomStartOffset, SetRandomStartOffset)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("ApplyToChildren", GetApplyRecursive, SetApplyRecursive),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WColorAnimationComponent::WColorAnimationComponent() = default;

void WColorAnimationComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hGradient;
  s << m_Duration;

  // version 2
  s << m_SetColorMode;
  s << m_AnimationMode;
  s << GetRandomStartOffset();
  s << GetApplyRecursive();
}

void WColorAnimationComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hGradient;
  s >> m_Duration;

  if (uiVersion >= 2)
  {
    s >> m_SetColorMode;
    s >> m_AnimationMode;
    bool b;
    s >> b;
    SetRandomStartOffset(b);
    s >> b;
    SetApplyRecursive(b);
  }
}

void WColorAnimationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetRandomStartOffset())
  {
    m_CurAnimTime = WTime::MakeFromSeconds(GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, m_Duration.GetSeconds()));
  }
}

void WColorAnimationComponent::SetColorGradient(const WColorGradientResourceHandle& hResource)
{
  m_hGradient = hResource;
}

bool WColorAnimationComponent::GetApplyRecursive() const
{
  return GetUserFlag(0);
}

void WColorAnimationComponent::SetApplyRecursive(bool value)
{
  SetUserFlag(0, value);
}

bool WColorAnimationComponent::GetRandomStartOffset() const
{
  return GetUserFlag(1);
}

void WColorAnimationComponent::SetRandomStartOffset(bool value)
{
  SetUserFlag(1, value);
}

void WColorAnimationComponent::Update()
{
  if (!m_hGradient.IsValid() || m_Duration <= WTime::MakeZero())
    return;

  WTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  const bool bReverse = GetUserFlag(0);

  if (bReverse)
    m_CurAnimTime -= tDiff;
  else
    m_CurAnimTime += tDiff;

  switch (m_AnimationMode)
  {
    case WPropertyAnimMode::Once:
    {
      m_CurAnimTime = WMath::Min(m_CurAnimTime, m_Duration);
      break;
    }

    case WPropertyAnimMode::Loop:
    {
      if (m_CurAnimTime >= m_Duration)
        m_CurAnimTime -= m_Duration;

      break;
    }

    case WPropertyAnimMode::BackAndForth:
    {
      if (m_CurAnimTime > m_Duration)
      {
        SetUserFlag(0, !bReverse);

        const WTime tOver = m_Duration - m_CurAnimTime;

        m_CurAnimTime = m_Duration - tOver;
      }
      else if (m_CurAnimTime < WTime::MakeZero())
      {
        SetUserFlag(0, !bReverse);

        m_CurAnimTime = -m_CurAnimTime;
      }

      break;
    }
  }

  WResourceLock<WColorGradientResource> pGradient(m_hGradient, WResourceAcquireMode::AllowLoadingFallback);

  if (pGradient.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  WMsgSetColor msg;
  msg.m_Color = pGradient->Evaluate(m_CurAnimTime.GetSeconds() / m_Duration.GetSeconds());
  msg.m_Mode = m_SetColorMode;

  if (GetApplyRecursive())
    GetOwner()->SendMessageRecursive(msg);
  else
    GetOwner()->SendMessage(msg);
}

W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_ColorAnimationComponent);
