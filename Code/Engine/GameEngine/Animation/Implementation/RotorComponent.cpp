#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Animation/RotorComponent.h>

float CalculateAcceleratedMovement(
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, WTime& ref_timeSinceStartInSec);

// clang-format off
W_BEGIN_COMPONENT_TYPE(WRotorComponent, 3, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Axis", WBasisAxis, m_Axis),
    W_MEMBER_PROPERTY("AxisDeviation", m_AxisDeviation)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(-180), WAngle::MakeFromDegree(180))),
    W_MEMBER_PROPERTY("DegreesToRotate", m_iDegreeToRotate),
    W_MEMBER_PROPERTY("Acceleration", m_fAcceleration),
    W_MEMBER_PROPERTY("Deceleration", m_fDeceleration),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WRotorComponent::WRotorComponent() = default;
WRotorComponent::~WRotorComponent() = default;

void WRotorComponent::Update()
{
  if (m_Flags.IsAnySet(WTransformComponentFlags::CurrentlyRunning) && m_fAnimationSpeed > 0.0f)
  {
    if (m_Flags.IsAnySet(WTransformComponentFlags::AnimationReversed))
      m_AnimationTime -= GetWorld()->GetClock().GetTimeDiff();
    else
      m_AnimationTime += GetWorld()->GetClock().GetTimeDiff();

    if (m_iDegreeToRotate > 0)
    {
      const float fNewDistance =
        CalculateAcceleratedMovement((float)m_iDegreeToRotate, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

      WQuat qRotation = WQuat::MakeFromAxisAndAngle(m_vRotationAxis, WAngle::MakeFromDegree(fNewDistance));

      GetOwner()->SetLocalRotation(GetOwner()->GetLocalRotation() * m_qLastRotation.GetInverse() * qRotation);

      m_qLastRotation = qRotation;

      if (!m_Flags.IsAnySet(WTransformComponentFlags::AnimationReversed))
      {
        if (fNewDistance >= m_iDegreeToRotate)
        {
          if (!m_Flags.IsSet(WTransformComponentFlags::AutoReturnEnd))
          {
            m_Flags.Remove(WTransformComponentFlags::CurrentlyRunning);
          }

          m_Flags.Add(WTransformComponentFlags::AnimationReversed);

          /// \todo Scripting integration
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

          /// \todo Scripting integration
          // if (PrepareEvent("ANIMATOR_OnReachStart"))
          // RaiseEvent();
        }
      }
    }
    else
    {
      /// \todo This will probably give precision issues pretty quickly

      WQuat qRotation = WQuat::MakeFromAxisAndAngle(m_vRotationAxis, WAngle::MakeFromDegree(m_fAnimationSpeed * GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds()));

      GetOwner()->SetLocalRotation(GetOwner()->GetLocalRotation() * qRotation);
    }
  }
}

void WRotorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_iDegreeToRotate;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_Axis.GetValue();
  s << m_qLastRotation;
  s << m_AxisDeviation;
}


void WRotorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_iDegreeToRotate;
  s >> m_fAcceleration;
  s >> m_fDeceleration;
  s >> m_Axis;
  s >> m_qLastRotation;

  if (uiVersion >= 3)
  {
    s >> m_AxisDeviation;
  }
}

void WRotorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // reset to start state
  m_qLastRotation = WQuat::MakeIdentity();

  switch (m_Axis)
  {
    case WBasisAxis::PositiveX:
      m_vRotationAxis.Set(1, 0, 0);
      break;
    case WBasisAxis::PositiveY:
      m_vRotationAxis.Set(0, 1, 0);
      break;
    case WBasisAxis::PositiveZ:
      m_vRotationAxis.Set(0, 0, 1);
      break;
    case WBasisAxis::NegativeX:
      m_vRotationAxis.Set(-1, 0, 0);
      break;
    case WBasisAxis::NegativeY:
      m_vRotationAxis.Set(0, -1, 0);
      break;
    case WBasisAxis::NegativeZ:
      m_vRotationAxis.Set(0, 0, -1);
      break;
  }

  if (m_AxisDeviation.GetRadian() != 0.0f)
  {
    if (m_AxisDeviation > WAngle::MakeFromDegree(179))
    {
      m_vRotationAxis = WVec3::MakeRandomDirection(GetWorld()->GetRandomNumberGenerator());
    }
    else
    {
      m_vRotationAxis = WVec3::MakeRandomDeviation(GetWorld()->GetRandomNumberGenerator(), m_AxisDeviation, m_vRotationAxis);

      if (m_AxisDeviation.GetRadian() > 0 && GetWorld()->GetRandomNumberGenerator().Bool())
        m_vRotationAxis = -m_vRotationAxis;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WRotorComponentPatch_1_2 : public WGraphPatch
{
public:
  WRotorComponentPatch_1_2()
    : WGraphPatch("WRotorComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // Base class
    ref_context.PatchBaseClass("WTransformComponent", 2, true);

    // this class
    pNode->RenameProperty("Degrees to Rotate", "DegreesToRotate");
  }
};

WRotorComponentPatch_1_2 g_WRotorComponentPatch_1_2;


W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_RotorComponent);
