#include <JoltPlugin/JoltPluginPCH.h>

#if 0

#  include <Core/WorldSerializer/WorldReader.h>
#  include <Core/WorldSerializer/WorldWriter.h>
#  include <JoltPlugin/Constraints/Jolt6DOFConstraintComponent.h>
#  include <JoltPlugin/System/JoltCore.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(WJoltAxis, 1)
  W_BITFLAGS_CONSTANT(WJoltAxis::X),
  W_BITFLAGS_CONSTANT(WJoltAxis::Y),
  W_BITFLAGS_CONSTANT(WJoltAxis::Z),
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_COMPONENT_TYPE(WJolt6DOFConstraintComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_BITFLAGS_ACCESSOR_PROPERTY("FreeLinearAxis", WJoltAxis, GetFreeLinearAxis, SetFreeLinearAxis),
    W_ENUM_ACCESSOR_PROPERTY("LinearLimitMode", WJoltConstraintLimitMode, GetLinearLimitMode, SetLinearLimitMode),
    W_ACCESSOR_PROPERTY("LinearRangeX", GetLinearRangeX, SetLinearRangeX),
    W_ACCESSOR_PROPERTY("LinearRangeY", GetLinearRangeY, SetLinearRangeY),
    W_ACCESSOR_PROPERTY("LinearRangeZ", GetLinearRangeZ, SetLinearRangeZ),
    W_ACCESSOR_PROPERTY("LinearStiffness", GetLinearStiffness, SetLinearStiffness)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("LinearDamping", GetLinearDamping, SetLinearDamping)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_BITFLAGS_ACCESSOR_PROPERTY("FreeAngularAxis", WJoltAxis, GetFreeAngularAxis, SetFreeAngularAxis),
    W_ENUM_ACCESSOR_PROPERTY("SwingLimitMode", WJoltConstraintLimitMode, GetSwingLimitMode, SetSwingLimitMode),
    W_ACCESSOR_PROPERTY("SwingLimit", GetSwingLimit, SetSwingLimit)->AddAttributes(new WClampValueAttribute(WAngle(), WAngle::MakeFromDegree(175))),
    W_ACCESSOR_PROPERTY("SwingStiffness", GetSwingStiffness, SetSwingStiffness)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("SwingDamping", GetSwingDamping, SetSwingDamping)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ENUM_ACCESSOR_PROPERTY("TwistLimitMode", WJoltConstraintLimitMode, GetTwistLimitMode, SetTwistLimitMode),
    W_ACCESSOR_PROPERTY("LowerTwistLimit", GetLowerTwistLimit, SetLowerTwistLimit)->AddAttributes(new WClampValueAttribute(-WAngle::MakeFromDegree(175), WAngle::MakeFromDegree(175))),
    W_ACCESSOR_PROPERTY("UpperTwistLimit", GetUpperTwistLimit, SetUpperTwistLimit)->AddAttributes(new WClampValueAttribute(-WAngle::MakeFromDegree(175), WAngle::MakeFromDegree(175))),
    W_ACCESSOR_PROPERTY("TwistStiffness", GetTwistStiffness, SetTwistStiffness)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("TwistDamping", GetTwistDamping, SetTwistDamping)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 0.2, WColor::SlateGray)
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJolt6DOFConstraintComponent::WJolt6DOFConstraintComponent() = default;
WJolt6DOFConstraintComponent::~WJolt6DOFConstraintComponent() = default;

void WJolt6DOFConstraintComponent::SerializeComponent(WWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_FreeLinearAxis;
  s << m_FreeAngularAxis;
  s << m_fLinearStiffness;
  s << m_fLinearDamping;
  s << m_fSwingStiffness;
  s << m_fSwingDamping;

  s << m_LinearLimitMode;
  s << m_vLinearRangeX;
  s << m_vLinearRangeY;
  s << m_vLinearRangeZ;

  s << m_SwingLimitMode;
  s << m_SwingLimit;

  s << m_TwistLimitMode;
  s << m_LowerTwistLimit;
  s << m_UpperTwistLimit;
  s << m_fTwistStiffness;
  s << m_fTwistDamping;
}

void WJolt6DOFConstraintComponent::DeserializeComponent(WWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_FreeLinearAxis;
  s >> m_FreeAngularAxis;
  s >> m_fLinearStiffness;
  s >> m_fLinearDamping;
  s >> m_fSwingStiffness;
  s >> m_fSwingDamping;

  s >> m_LinearLimitMode;
  s >> m_vLinearRangeX;
  s >> m_vLinearRangeY;
  s >> m_vLinearRangeZ;
  s >> m_SwingLimitMode;
  s >> m_SwingLimit;

  s >> m_TwistLimitMode;
  s >> m_LowerTwistLimit;
  s >> m_UpperTwistLimit;
  s >> m_fTwistStiffness;
  s >> m_fTwistDamping;
}

void WJolt6DOFConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  //W_ASSERT_DEV(localFrame0.isFinite() && localFrame0.isValid() && localFrame0.isSane(), "frame 0");
  //W_ASSERT_DEV(localFrame1.isFinite() && localFrame1.isValid() && localFrame1.isSane(), "frame 1");
  //
  //  m_pJoint = PxD6JointCreate(*(WJolt::GetSingleton()->GetJoltAPI()), actor0, localFrame0, actor1, localFrame1);
}

void WJolt6DOFConstraintComponent::ApplySettings()
{
  WJoltConstraintComponent::ApplySettings();

  //JoltD6Joint* pJoint = static_cast<PxD6Joint*>(m_pJoint);

  //if (m_LinearLimitMode == WJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eX, m_FreeLinearAxis.IsSet(WJoltAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eY, m_FreeLinearAxis.IsSet(WJoltAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eZ, m_FreeLinearAxis.IsSet(WJoltAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeLinearAxis;

  //  if (m_LinearLimitMode == WJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (WMath::IsEqual(m_vLinearRangeX.x, m_vLinearRangeX.y, 0.05f))
  //      freeAxis.Remove(WJoltAxis::X);
  //    if (WMath::IsEqual(m_vLinearRangeY.x, m_vLinearRangeY.y, 0.05f))
  //      freeAxis.Remove(WJoltAxis::Y);
  //    if (WMath::IsEqual(m_vLinearRangeZ.x, m_vLinearRangeZ.y, 0.05f))
  //      freeAxis.Remove(WJoltAxis::Z);
  //  }

  //  pJoint->setMotion(PxD6Axis::eX, freeAxis.IsSet(WJoltAxis::X) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eY, freeAxis.IsSet(WJoltAxis::Y) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eZ, freeAxis.IsSet(WJoltAxis::Z) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  PxJointLinearLimitPair l(0, 0, PxSpring(0, 0));

  //  if (m_LinearLimitMode == WJoltConstraintLimitMode::SoftLimit)
  //  {
  //    l.stiffness = m_fLinearStiffness;
  //    l.damping = m_fLinearDamping;
  //  }
  //  else
  //  {
  //    l.restitution = m_fLinearStiffness;
  //    l.bounceThreshold = m_fLinearDamping;
  //  }

  //  if (freeAxis.IsSet(WJoltAxis::X))
  //  {
  //    l.lower = m_vLinearRangeX.x;
  //    l.upper = m_vLinearRangeX.y;

  //    if (l.lower > l.upper)
  //      WMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eX, l);
  //  }

  //  if (freeAxis.IsSet(WJoltAxis::Y))
  //  {
  //    l.lower = m_vLinearRangeY.x;
  //    l.upper = m_vLinearRangeY.y;

  //    if (l.lower > l.upper)
  //      WMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eY, l);
  //  }

  //  if (freeAxis.IsSet(WJoltAxis::Z))
  //  {
  //    l.lower = m_vLinearRangeZ.x;
  //    l.upper = m_vLinearRangeZ.y;

  //    if (l.lower > l.upper)
  //      WMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eZ, l);
  //  }
  //}


  //if (m_SwingLimitMode == WJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eSWING1, m_FreeAngularAxis.IsSet(WJoltAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eSWING2, m_FreeAngularAxis.IsSet(WJoltAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeAngularAxis;

  //  if (m_SwingLimitMode == WJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (WMath::IsZero(m_SwingLimit.GetDegree(), 1.0f))
  //    {
  //      freeAxis.Remove(WJoltAxis::Y);
  //      freeAxis.Remove(WJoltAxis::Z);
  //    }
  //  }

  //  pJoint->setMotion(PxD6Axis::eSWING1, freeAxis.IsSet(WJoltAxis::Y) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eSWING2, freeAxis.IsSet(WJoltAxis::Z) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  if (freeAxis.IsAnySet(WJoltAxis::Y | WJoltAxis::Z))
  //  {
  //    const float fSwingLimit = WMath::Max(WAngle::MakeFromDegree(0.5f).GetRadian(), m_SwingLimit.GetRadian());

  //    PxJointLimitCone l(fSwingLimit, fSwingLimit);

  //    if (m_SwingLimitMode == WJoltConstraintLimitMode::SoftLimit)
  //    {
  //      l.stiffness = m_fSwingStiffness;
  //      l.damping = m_fSwingDamping;
  //    }
  //    else
  //    {
  //      l.restitution = m_fSwingStiffness;
  //      l.bounceThreshold = m_fSwingDamping;
  //    }

  //    pJoint->setSwingLimit(l);
  //  }
  //}

  //if (m_TwistLimitMode == WJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eTWIST, m_FreeAngularAxis.IsSet(WJoltAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeAngularAxis;

  //  if (m_SwingLimitMode == WJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (WMath::IsEqual(m_LowerTwistLimit.GetDegree(), m_UpperTwistLimit.GetDegree(), 1.0f))
  //    {
  //      freeAxis.Remove(WJoltAxis::X);
  //    }
  //  }

  //  pJoint->setMotion(PxD6Axis::eTWIST, freeAxis.IsSet(WJoltAxis::X) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  if (freeAxis.IsSet(WJoltAxis::X))
  //  {
  //    PxJointAngularLimitPair l(m_LowerTwistLimit.GetRadian(), m_UpperTwistLimit.GetRadian());

  //    if (l.lower > l.upper)
  //    {
  //      WMath::Swap(l.lower, l.upper);
  //    }

  //    if (WMath::IsEqual(l.lower, l.upper, WAngle::MakeFromDegree(0.5f).GetRadian()))
  //    {
  //      l.lower -= WAngle::MakeFromDegree(0.5f).GetRadian();
  //      l.upper += WAngle::MakeFromDegree(0.5f).GetRadian();
  //    }

  //    if (m_TwistLimitMode == WJoltConstraintLimitMode::SoftLimit)
  //    {
  //      l.stiffness = m_fTwistStiffness;
  //      l.damping = m_fTwistDamping;
  //    }
  //    else
  //    {
  //      l.restitution = m_fTwistStiffness;
  //      l.bounceThreshold = m_fTwistDamping;
  //    }

  //    pJoint->setTwistLimit(l);
  //  }
  //}
}

void WJolt6DOFConstraintComponent::SetFreeLinearAxis(WBitflags<WJoltAxis> flags)
{
  m_FreeLinearAxis = flags;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetFreeAngularAxis(WBitflags<WJoltAxis> flags)
{
  m_FreeAngularAxis = flags;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetLinearLimitMode(WJoltConstraintLimitMode::Enum mode)
{
  m_LinearLimitMode = mode;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetLinearRangeX(const WVec2& value)
{
  m_vLinearRangeX = value;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetLinearRangeY(const WVec2& value)
{
  m_vLinearRangeY = value;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetLinearRangeZ(const WVec2& value)
{
  m_vLinearRangeZ = value;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetLinearStiffness(float f)
{
  m_fLinearStiffness = f;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetLinearDamping(float f)
{
  m_fLinearDamping = f;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetSwingLimitMode(WJoltConstraintLimitMode::Enum mode)
{
  m_SwingLimitMode = mode;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetSwingLimit(WAngle f)
{
  m_SwingLimit = f;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetSwingStiffness(float f)
{
  m_fSwingStiffness = f;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetSwingDamping(float f)
{
  m_fSwingDamping = f;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetTwistLimitMode(WJoltConstraintLimitMode::Enum mode)
{
  m_TwistLimitMode = mode;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetLowerTwistLimit(WAngle f)
{
  m_LowerTwistLimit = f;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetUpperTwistLimit(WAngle f)
{
  m_UpperTwistLimit = f;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetTwistStiffness(float f)
{
  m_fTwistStiffness = f;
  QueueApplySettings();
}

void WJolt6DOFConstraintComponent::SetTwistDamping(float f)
{
  m_fTwistDamping = f;
  QueueApplySettings();
}

#endif


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_Jolt6DOFConstraintComponent);
