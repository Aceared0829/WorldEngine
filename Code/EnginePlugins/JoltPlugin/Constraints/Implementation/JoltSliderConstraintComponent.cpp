#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <JoltPlugin/Constraints/JoltSliderConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltSliderConstraintComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("LimitMode", WJoltConstraintLimitMode, GetLimitMode, SetLimitMode),
    W_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitDistance, SetLowerLimitDistance)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitDistance, SetUpperLimitDistance)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ENUM_ACCESSOR_PROPERTY("DriveMode", WJoltConstraintDriveMode, GetDriveMode, SetDriveMode),
    W_ACCESSOR_PROPERTY("DriveTargetValue", GetDriveTargetValue, SetDriveTargetValue),
    W_ACCESSOR_PROPERTY("DriveStrength", GetDriveStrength, SetDriveStrength)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WMinValueTextAttribute("Maximum")),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 1.0f, WColor::Orange, nullptr, "UpperLimit"),
    new WDirectionVisualizerAttribute(WBasisAxis::NegativeX, 1.0f, WColor::Teal, nullptr, "LowerLimit"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltSliderConstraintComponent::WJoltSliderConstraintComponent() = default;
WJoltSliderConstraintComponent::~WJoltSliderConstraintComponent() = default;

void WJoltSliderConstraintComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fLowerLimitDistance;
  s << m_fUpperLimitDistance;
  s << m_fFriction;
  s << m_LimitMode;

  s << m_DriveMode;
  s << m_fDriveTargetValue;
  s << m_fDriveStrength;
}

void WJoltSliderConstraintComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fLowerLimitDistance;
  s >> m_fUpperLimitDistance;
  s >> m_fFriction;
  s >> m_LimitMode;

  s >> m_DriveMode;
  s >> m_fDriveTargetValue;
  s >> m_fDriveStrength;
}

void WJoltSliderConstraintComponent::SetLimitMode(WJoltConstraintLimitMode::Enum mode)
{
  m_LimitMode = mode;
  QueueApplySettings();
}

void WJoltSliderConstraintComponent::SetLowerLimitDistance(float f)
{
  m_fLowerLimitDistance = f;
  QueueApplySettings();
}

void WJoltSliderConstraintComponent::SetUpperLimitDistance(float f)
{
  m_fUpperLimitDistance = f;
  QueueApplySettings();
}

void WJoltSliderConstraintComponent::SetFriction(float f)
{
  m_fFriction = f;
  QueueApplySettings();
}

void WJoltSliderConstraintComponent::SetDriveMode(WJoltConstraintDriveMode::Enum mode)
{
  m_DriveMode = mode;
  QueueApplySettings();
}

void WJoltSliderConstraintComponent::SetDriveTargetValue(float f)
{
  m_fDriveTargetValue = f;
  QueueApplySettings();
}

void WJoltSliderConstraintComponent::SetDriveStrength(float f)
{
  m_fDriveStrength = WMath::Max(f, 0.0f);
  QueueApplySettings();
}


void WJoltSliderConstraintComponent::ApplySettings()
{
  WJoltConstraintComponent::ApplySettings();

  JPH::SliderConstraint* pConstraint = static_cast<JPH::SliderConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionForce(m_fFriction);

  if (m_LimitMode != WJoltConstraintLimitMode::NoLimit)
  {
    float low = m_fLowerLimitDistance;
    float high = m_fUpperLimitDistance;

    if (low == high) // both zero
    {
      high = low + 0.01f;
    }

    pConstraint->SetLimits(-low, high);
  }
  else
  {
    pConstraint->SetLimits(-FLT_MAX, FLT_MAX);
  }

  // drive
  {
    if (m_DriveMode == WJoltConstraintDriveMode::NoDrive)
    {
      pConstraint->SetMotorState(JPH::EMotorState::Off);
    }
    else
    {
      if (m_DriveMode == WJoltConstraintDriveMode::DriveVelocity)
      {
        pConstraint->SetMotorState(JPH::EMotorState::Velocity);
        pConstraint->SetTargetVelocity(m_fDriveTargetValue);
      }
      else
      {
        pConstraint->SetMotorState(JPH::EMotorState::Position);
        pConstraint->SetTargetPosition(m_fDriveTargetValue);
      }

      const float strength = (m_fDriveStrength == 0) ? FLT_MAX : m_fDriveStrength;

      pConstraint->GetMotorSettings().SetForceLimit(strength);
      pConstraint->GetMotorSettings().SetTorqueLimit(strength);
    }
  }

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

bool WJoltSliderConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::SliderConstraint*>(m_pConstraint))
  {
    if (m_fBreakForce > 0)
    {
      if (pConstraint->GetTotalLambdaPosition()[0] >= m_fBreakForce ||
          pConstraint->GetTotalLambdaPosition()[1] >= m_fBreakForce)
      {
        return true;
      }
    }

    if (m_fBreakTorque > 0)
    {
      if (pConstraint->GetTotalLambdaRotation().ReduceMax() >= m_fBreakTorque)
      {
        return true;
      }
    }
  }

  return false;
}

void WJoltSliderConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::SliderConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * WJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * WJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mSliderAxis1 = inv1.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * WVec3(1, 0, 0)));
  opt.mSliderAxis2 = inv2.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * WVec3(1, 0, 0)));
  opt.mNormalAxis1 = inv1.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * WVec3(0, 1, 0)));
  opt.mNormalAxis2 = inv2.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * WVec3(0, 1, 0)));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltSliderConstraintComponent);
