#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Constraints/JoltSwingTwistConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltSwingTwistConstraintComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("SwingLimitY", GetSwingLimitY, SetSwingLimitY)->AddAttributes(new WClampValueAttribute(WAngle(), WAngle::MakeFromDegree(175))),
    W_ACCESSOR_PROPERTY("SwingLimitZ", GetSwingLimitZ, SetSwingLimitZ)->AddAttributes(new WClampValueAttribute(WAngle(), WAngle::MakeFromDegree(175))),

    W_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),

    W_ACCESSOR_PROPERTY("LowerTwistLimit", GetLowerTwistLimit, SetLowerTwistLimit)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(5), WAngle::MakeFromDegree(175)), new WDefaultValueAttribute(WAngle::MakeFromDegree(90))),
    W_ACCESSOR_PROPERTY("UpperTwistLimit", GetUpperTwistLimit, SetUpperTwistLimit)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(5), WAngle::MakeFromDegree(175)), new WDefaultValueAttribute(WAngle::MakeFromDegree(90))),

    //W_ENUM_ACCESSOR_PROPERTY("TwistDriveMode", WJoltConstraintDriveMode, GetTwistDriveMode, SetTwistDriveMode),
    //W_ACCESSOR_PROPERTY("TwistDriveTargetValue", GetTwistDriveTargetValue, SetTwistDriveTargetValue),
    //W_ACCESSOR_PROPERTY("TwistDriveStrength", GetTwistDriveStrength, SetTwistDriveStrength)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WMinValueTextAttribute("Maximum"))
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WConeVisualizerAttribute(WBasisAxis::PositiveX, "SwingLimitY", 0.3f, nullptr)
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltSwingTwistConstraintComponent::WJoltSwingTwistConstraintComponent() = default;
WJoltSwingTwistConstraintComponent::~WJoltSwingTwistConstraintComponent() = default;

void WJoltSwingTwistConstraintComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_SwingLimitY;
  s << m_SwingLimitZ;

  s << m_LowerTwistLimit;
  s << m_UpperTwistLimit;

  s << m_fFriction;

  // s << m_TwistDriveMode;
  // s << m_TwistDriveTargetValue;
  // s << m_fTwistDriveStrength;
}

void WJoltSwingTwistConstraintComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_SwingLimitY;
  s >> m_SwingLimitZ;

  s >> m_LowerTwistLimit;
  s >> m_UpperTwistLimit;

  s >> m_fFriction;

  // s >> m_TwistDriveMode;
  // s >> m_TwistDriveTargetValue;
  // s >> m_fTwistDriveStrength;
}

void WJoltSwingTwistConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::SwingTwistConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPosition1 = inv1 * WJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPosition2 = inv2 * WJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mPlaneHalfConeAngle = m_SwingLimitY.GetRadian() * 0.5f;
  opt.mNormalHalfConeAngle = m_SwingLimitZ.GetRadian() * 0.5f;
  opt.mMaxFrictionTorque = m_fFriction;
  opt.mTwistAxis1 = inv1.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * WVec3::MakeAxisX()));
  opt.mTwistAxis2 = inv2.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * WVec3::MakeAxisX()));
  opt.mTwistMinAngle = -m_LowerTwistLimit.GetRadian();
  opt.mTwistMaxAngle = m_UpperTwistLimit.GetRadian();
  opt.mPlaneAxis1 = inv1.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * WVec3::MakeAxisY()));
  opt.mPlaneAxis2 = inv2.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * WVec3::MakeAxisY()));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void WJoltSwingTwistConstraintComponent::ApplySettings()
{
  WJoltConstraintComponent::ApplySettings();

  auto pConstraint = static_cast<JPH::SwingTwistConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionTorque(m_fFriction);
  pConstraint->SetPlaneHalfConeAngle(m_SwingLimitY.GetRadian() * 0.5f);
  pConstraint->SetNormalHalfConeAngle(m_SwingLimitZ.GetRadian() * 0.5f);
  pConstraint->SetTwistMinAngle(-m_LowerTwistLimit.GetRadian());
  pConstraint->SetTwistMaxAngle(m_UpperTwistLimit.GetRadian());

  // drive
  //{
  //  if (m_TwistDriveMode == WJoltConstraintDriveMode::NoDrive)
  //  {
  //    pConstraint->SetTwistMotorState(JPH::EMotorState::Off);
  //  }
  //  else
  //  {
  //    if (m_TwistDriveMode == WJoltConstraintDriveMode::DriveVelocity)
  //    {
  //      pConstraint->SetTwistMotorState(JPH::EMotorState::Velocity);
  //      pConstraint->SetTargetAngularVelocityCS(JPH::Vec3::sReplicate(m_TwistDriveTargetValue.GetRadian()));
  //    }
  //    else
  //    {
  //      pConstraint->SetTwistMotorState(JPH::EMotorState::Position);
  //      //pConstraint->SetTargetOrientationCS(m_TwistDriveTargetValue.GetRadian());
  //    }

  //    const float strength = (m_fTwistDriveStrength == 0) ? FLT_MAX : m_fTwistDriveStrength;

  //    pConstraint->GetTwistMotorSettings().mFrequency = 2.0f;
  //    pConstraint->GetTwistMotorSettings().SetForceLimit(strength);
  //    pConstraint->GetTwistMotorSettings().SetTorqueLimit(strength);
  //  }
  //}

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

bool WJoltSwingTwistConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::SwingTwistConstraint*>(m_pConstraint))
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
      if (pConstraint->GetTotalLambdaSwingY() >= m_fBreakTorque ||
          pConstraint->GetTotalLambdaSwingZ() >= m_fBreakTorque ||
          pConstraint->GetTotalLambdaTwist() >= m_fBreakTorque)
      {
        return true;
      }
    }
  }

  return false;
}

void WJoltSwingTwistConstraintComponent::SetSwingLimitZ(WAngle f)
{
  m_SwingLimitZ = f;
  QueueApplySettings();
}

void WJoltSwingTwistConstraintComponent::SetSwingLimitY(WAngle f)
{
  m_SwingLimitY = f;
  QueueApplySettings();
}

void WJoltSwingTwistConstraintComponent::SetFriction(float f)
{
  m_fFriction = f;
  QueueApplySettings();
}

void WJoltSwingTwistConstraintComponent::SetLowerTwistLimit(WAngle f)
{
  m_LowerTwistLimit = f;
  QueueApplySettings();
}

void WJoltSwingTwistConstraintComponent::SetUpperTwistLimit(WAngle f)
{
  m_UpperTwistLimit = f;
  QueueApplySettings();
}

// void WJoltSwingTwistConstraintComponent::SetTwistDriveMode(WJoltConstraintDriveMode::Enum mode)
//{
//   m_TwistDriveMode = mode;
//   QueueApplySettings();
// }
//
// void WJoltSwingTwistConstraintComponent::SetTwistDriveTargetValue(WAngle f)
//{
//   m_TwistDriveTargetValue = f;
//   QueueApplySettings();
// }
//
// void WJoltSwingTwistConstraintComponent::SetTwistDriveStrength(float f)
//{
//   m_fTwistDriveStrength = f;
//   QueueApplySettings();
// }


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltSwingTwistConstraintComponent);
