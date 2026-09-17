#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Constraints/JoltHingeConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltHingeConstraintComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("LimitMode", WJoltConstraintLimitMode, GetLimitMode, SetLimitMode),
    W_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitAngle, SetLowerLimitAngle)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(180))),
    W_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitAngle, SetUpperLimitAngle)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(180))),
    W_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ENUM_ACCESSOR_PROPERTY("DriveMode", WJoltConstraintDriveMode, GetDriveMode, SetDriveMode),
    W_ACCESSOR_PROPERTY("DriveTargetValue", GetDriveTargetValue, SetDriveTargetValue),
    W_ACCESSOR_PROPERTY("DriveStrength", GetDriveStrength, SetDriveStrength)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WMinValueTextAttribute("Maximum")),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 0.2f, WColor::BurlyWood)
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltHingeConstraintComponent::WJoltHingeConstraintComponent() = default;
WJoltHingeConstraintComponent::~WJoltHingeConstraintComponent() = default;

void WJoltHingeConstraintComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_LimitMode;
  s << m_LowerLimit;
  s << m_UpperLimit;

  s << m_DriveMode;
  s << m_DriveTargetValue;
  s << m_fDriveStrength;

  s << m_fFriction;
}

void WJoltHingeConstraintComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_LimitMode;
  s >> m_LowerLimit;
  s >> m_UpperLimit;

  s >> m_DriveMode;
  s >> m_DriveTargetValue;
  s >> m_fDriveStrength;

  s >> m_fFriction;
}

void WJoltHingeConstraintComponent::SetLimitMode(WJoltConstraintLimitMode::Enum mode)
{
  m_LimitMode = mode;
  QueueApplySettings();
}

void WJoltHingeConstraintComponent::SetLowerLimitAngle(WAngle f)
{
  m_LowerLimit = WMath::Clamp(f, WAngle(), WAngle::MakeFromDegree(180));
  QueueApplySettings();
}

void WJoltHingeConstraintComponent::SetUpperLimitAngle(WAngle f)
{
  m_UpperLimit = WMath::Clamp(f, WAngle(), WAngle::MakeFromDegree(180));
  QueueApplySettings();
}

void WJoltHingeConstraintComponent::SetFriction(float f)
{
  m_fFriction = WMath::Max(f, 0.0f);
  QueueApplySettings();
}

void WJoltHingeConstraintComponent::SetDriveMode(WJoltConstraintDriveMode::Enum mode)
{
  m_DriveMode = mode;
  QueueApplySettings();
}

void WJoltHingeConstraintComponent::SetDriveTargetValue(WAngle f)
{
  m_DriveTargetValue = f;
  QueueApplySettings();
}

void WJoltHingeConstraintComponent::SetDriveStrength(float f)
{
  m_fDriveStrength = WMath::Max(f, 0.0f);
  QueueApplySettings();
}

void WJoltHingeConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::HingeConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * WJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * WJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mHingeAxis1 = inv1.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * WVec3(1, 0, 0)));
  opt.mHingeAxis2 = inv2.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * WVec3(1, 0, 0)));
  opt.mNormalAxis1 = inv1.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * WVec3(0, 1, 0)));
  opt.mNormalAxis2 = inv2.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * WVec3(0, 1, 0)));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void WJoltHingeConstraintComponent::ApplySettings()
{
  WJoltConstraintComponent::ApplySettings();

  JPH::HingeConstraint* pConstraint = static_cast<JPH::HingeConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionTorque(m_fFriction);

  if (m_LimitMode != WJoltConstraintLimitMode::NoLimit)
  {
    float low = m_LowerLimit.GetRadian();
    float high = m_UpperLimit.GetRadian();

    const float fLowest = WAngle::MakeFromDegree(1.0f).GetRadian();

    // there should be at least some slack
    if (low <= fLowest && high <= fLowest)
    {
      low = fLowest;
      high = fLowest;
    }

    pConstraint->SetLimits(-low, high);
  }
  else
  {
    pConstraint->SetLimits(-JPH::JPH_PI, +JPH::JPH_PI);
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
        pConstraint->SetTargetAngularVelocity(m_DriveTargetValue.GetRadian());
      }
      else
      {
        pConstraint->SetMotorState(JPH::EMotorState::Position);
        pConstraint->SetTargetAngle(m_DriveTargetValue.GetRadian());
      }

      const float strength = (m_fDriveStrength == 0) ? FLT_MAX : m_fDriveStrength;

      pConstraint->GetMotorSettings().mSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
      pConstraint->GetMotorSettings().mSpringSettings.mFrequency = 20.0f;
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

bool WJoltHingeConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::HingeConstraint*>(m_pConstraint))
  {
    if (m_fBreakForce > 0)
    {
      if (pConstraint->GetTotalLambdaPosition().ReduceMax() >= m_fBreakForce)
      {
        return true;
      }
    }

    if (m_fBreakTorque > 0)
    {
      if (pConstraint->GetTotalLambdaRotation()[0] >= m_fBreakTorque ||
          pConstraint->GetTotalLambdaRotation()[1] >= m_fBreakTorque)
      {
        return true;
      }
    }
  }

  return false;
}

W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltHingeConstraintComponent);
