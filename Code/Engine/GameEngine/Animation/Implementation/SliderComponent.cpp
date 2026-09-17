#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/SliderComponent.h>

float CalculateAcceleratedMovement(
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, WTime& ref_timeSinceStartInSec);

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSliderComponent, 3, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Axis", WBasisAxis, m_Axis)->AddAttributes(new WDefaultValueAttribute((int)WBasisAxis::PositiveZ)),
    W_MEMBER_PROPERTY("Distance", m_fDistanceToTravel)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("Acceleration", m_fAcceleration)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Deceleration", m_fDeceleration)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("RandomStart", m_RandomStart)->AddAttributes(new WClampValueAttribute(WTime::MakeZero(), WVariant())),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WDirectionVisualizerAttribute("Axis", 1.0, WColor::MediumPurple, nullptr, "Distance")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSliderComponent::WSliderComponent() = default;
WSliderComponent::~WSliderComponent() = default;

void WSliderComponent::Update()
{
  if (m_Flags.IsAnySet(WTransformComponentFlags::CurrentlyRunning))
  {
    WVec3 vAxis;

    switch (m_Axis)
    {
      case WBasisAxis::PositiveX:
        vAxis.Set(1, 0, 0);
        break;
      case WBasisAxis::PositiveY:
        vAxis.Set(0, 1, 0);
        break;
      case WBasisAxis::PositiveZ:
        vAxis.Set(0, 0, 1);
        break;
      case WBasisAxis::NegativeX:
        vAxis.Set(-1, 0, 0);
        break;
      case WBasisAxis::NegativeY:
        vAxis.Set(0, -1, 0);
        break;
      case WBasisAxis::NegativeZ:
        vAxis.Set(0, 0, -1);
        break;
    }

    if (m_Flags.IsAnySet(WTransformComponentFlags::AnimationReversed))
      m_AnimationTime -= GetWorld()->GetClock().GetTimeDiff();
    else
      m_AnimationTime += GetWorld()->GetClock().GetTimeDiff();

    const float fNewDistance = CalculateAcceleratedMovement(m_fDistanceToTravel, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

    const float fDistanceDiff = fNewDistance - m_fLastDistance;

    GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + GetOwner()->GetLocalRotation() * vAxis * fDistanceDiff);

    m_fLastDistance = fNewDistance;

    if (!m_Flags.IsAnySet(WTransformComponentFlags::AnimationReversed))
    {
      if (fNewDistance >= m_fDistanceToTravel)
      {
        if (!m_Flags.IsSet(WTransformComponentFlags::AutoReturnEnd))
        {
          m_Flags.Remove(WTransformComponentFlags::CurrentlyRunning);
        }

        m_Flags.Add(WTransformComponentFlags::AnimationReversed);

        // if (PrepareEvent("ANIMATOR_OnReachEnd"))
        // RaiseEvent();
      }
    }
    else
    {
      if (fNewDistance <= 0.0f)
      {
        if (!m_Flags.IsSet(WTransformComponentFlags::AutoReturnStart))
        {
          m_Flags.Remove(WTransformComponentFlags::CurrentlyRunning);
        }

        m_Flags.Remove(WTransformComponentFlags::AnimationReversed);

        // if (PrepareEvent("ANIMATOR_OnReachStart"))
        // RaiseEvent();
      }
    }
  }
}

void WSliderComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // reset to start state
  m_fLastDistance = 0.0f;

  if (m_RandomStart.IsPositive())
  {
    m_AnimationTime = WTime::MakeFromSeconds(GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, m_RandomStart.GetSeconds()));
  }
}

void WSliderComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fDistanceToTravel;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_Axis.GetValue();
  s << m_fLastDistance;
  s << m_RandomStart;
}


void WSliderComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fDistanceToTravel;
  s >> m_fAcceleration;
  s >> m_fDeceleration;
  s >> m_Axis;
  s >> m_fLastDistance;

  if (uiVersion >= 3)
  {
    s >> m_RandomStart;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WSliderComponentPatch_1_2 : public WGraphPatch
{
public:
  WSliderComponentPatch_1_2()
    : WGraphPatch("WSliderComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // Base class
    ref_context.PatchBaseClass("WTransformComponent", 2, true);
  }
};

WSliderComponentPatch_1_2 g_WSliderComponentPatch_1_2;


W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_SliderComponent);
