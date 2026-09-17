#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Constraints/JoltConeConstraintComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltConeConstraintComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("ConeAngle", GetConeAngle, SetConeAngle)->AddAttributes(new WClampValueAttribute(WAngle(), WAngle::MakeFromDegree(175))),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WConeVisualizerAttribute(WBasisAxis::PositiveX, "ConeAngle", 0.3f, nullptr)
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltConeConstraintComponent::WJoltConeConstraintComponent() = default;
WJoltConeConstraintComponent::~WJoltConeConstraintComponent() = default;

void WJoltConeConstraintComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_ConeAngle;
}

void WJoltConeConstraintComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_ConeAngle;
}

void WJoltConeConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::ConeConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * WJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * WJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mHalfConeAngle = m_ConeAngle.GetRadian() * 0.5f;
  opt.mTwistAxis1 = inv1.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * WVec3::MakeAxisX()));
  opt.mTwistAxis2 = inv2.Multiply3x3(WJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * WVec3::MakeAxisX()));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void WJoltConeConstraintComponent::ApplySettings()
{
  WJoltConstraintComponent::ApplySettings();

  auto pConstraint = static_cast<JPH::ConeConstraint*>(m_pConstraint);
  pConstraint->SetHalfConeAngle(m_ConeAngle.GetRadian() * 0.5f);

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

bool WJoltConeConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::ConeConstraint*>(m_pConstraint))
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
      if (pConstraint->GetTotalLambdaRotation() >= m_fBreakTorque)
      {
        return true;
      }
    }
  }

  return false;
}

void WJoltConeConstraintComponent::SetConeAngle(WAngle f)
{
  m_ConeAngle = f;
  QueueApplySettings();
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltConeConstraintComponent);
