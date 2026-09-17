#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/TwoBoneIKComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WTwoBoneIKComponent, 3, WComponentMode::Dynamic);
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("DebugVisScale", GetDebugVisScale, SetDebugVisScale)->AddAttributes(new WClampValueAttribute(0.0f, 10.0f)),
    W_MEMBER_PROPERTY("JointStart", m_sJointStart),
    W_MEMBER_PROPERTY("JointMiddle", m_sJointMiddle),
    W_MEMBER_PROPERTY("JointEnd", m_sJointEnd),
    W_ENUM_MEMBER_PROPERTY("MidAxis", WBasisAxis, m_MidAxis)->AddAttributes(new WDefaultValueAttribute(WBasisAxis::PositiveZ)),
    W_ACCESSOR_PROPERTY("PoleVector", DummyGetter, SetPoleVectorReference)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("Order", m_uiOrder),
    //W_MEMBER_PROPERTY("Soften", m_fSoften)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
    //W_MEMBER_PROPERTY("TwistAngle", m_TwistAngle)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(-180), WAngle::MakeFromDegree(180))),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
      new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgInjectPoseCommands, OnInjectPoseCommands)
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

WTwoBoneIKComponent::WTwoBoneIKComponent() = default;
WTwoBoneIKComponent::~WTwoBoneIKComponent() = default;

void WTwoBoneIKComponent::SetPoleVectorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hPoleVector = resolver(szReference, GetHandle(), "PoleVector");
}

void WTwoBoneIKComponent::SetDebugVisScale(float fScale)
{
  // allow scales from 0.05f to 10.0f
  // map them to range 0 to 200
  m_uiDebugVisScale = static_cast<WUInt8>(WMath::Clamp(WMath::RoundToInt(fScale * 20.0f), 0, 200));
}

float WTwoBoneIKComponent::GetDebugVisScale() const
{
  return m_uiDebugVisScale / 20.0f;
}

void WTwoBoneIKComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fWeight;
  s << m_sJointStart;
  s << m_sJointMiddle;
  s << m_sJointEnd;
  s << m_MidAxis;
  inout_stream.WriteGameObjectHandle(m_hPoleVector);

  // version 2
  s << m_uiDebugVisScale;

  // s << m_fSoften;
  // s << m_TwistAngle;

  // version 3
  s << m_uiOrder;
}

void WTwoBoneIKComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fWeight;
  s >> m_sJointStart;
  s >> m_sJointMiddle;
  s >> m_sJointEnd;
  s >> m_MidAxis;
  m_hPoleVector = inout_stream.ReadGameObjectHandle();

  if (uiVersion >= 2)
  {
    s >> m_uiDebugVisScale;
  }

  // s >> m_fSoften;
  // s >> m_TwistAngle;

  if (uiVersion >= 3)
  {
    s >> m_uiOrder;
  }
}

void WTwoBoneIKComponent::OnInjectPoseCommands(WMsgInjectPoseCommands& msg) const
{
  if (m_fWeight <= 0.0f && m_uiDebugVisScale == 0)
    return;

  // if we are already past this, just return
  if (m_uiOrder < msg.m_uiOrderNow)
    return;

  // if we haven't reached this yet, put it in the queue
  if (m_uiOrder > msg.m_uiOrderNow)
  {
    msg.m_uiOrderNext = WMath::Min(msg.m_uiOrderNext, m_uiOrder);
    return;
  }

  const WTransform targetTrans = msg.m_pGenerator->GetTargetObject()->GetGlobalTransform();
  const WTransform ownerTransform = WTransform::MakeGlobalTransform(targetTrans, msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_RootTransform);
  const WTransform localTarget = WTransform::MakeLocalTransform(ownerTransform, GetOwner()->GetGlobalTransform());

  WVec3 vPoleVectorPos;

  const WGameObject* pPoleVector;
  if (!m_hPoleVector.IsInvalidated() && GetWorld()->TryGetObject(m_hPoleVector, pPoleVector))
  {
    vPoleVectorPos = WTransform::MakeLocalTransform(ownerTransform, WTransform(pPoleVector->GetGlobalPosition())).m_vPosition;
  }
  else
  {
    // hard-coded "up vector" as pole target
    vPoleVectorPos = WTransform::MakeLocalTransform(ownerTransform, WTransform(targetTrans * WVec3(0, 0, 10))).m_vPosition;
  }

  if (m_uiJointIdxStart == 0 && m_uiJointIdxMiddle == 0)
  {
    auto& skel = msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_Skeleton;
    m_uiJointIdxStart = skel.FindJointByName(m_sJointStart);
    m_uiJointIdxMiddle = skel.FindJointByName(m_sJointMiddle);
    m_uiJointIdxEnd = skel.FindJointByName(m_sJointEnd);
  }

  if (m_uiJointIdxStart != WInvalidJointIndex && m_uiJointIdxMiddle != WInvalidJointIndex && m_uiJointIdxEnd != WInvalidJointIndex)
  {
    auto& cmdIk = msg.m_pGenerator->AllocCommandTwoBoneIK();
    cmdIk.m_fDebugVisScale = GetDebugVisScale();
    cmdIk.m_uiJointIdxStart = m_uiJointIdxStart;
    cmdIk.m_uiJointIdxMiddle = m_uiJointIdxMiddle;
    cmdIk.m_uiJointIdxEnd = m_uiJointIdxEnd;
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;
    cmdIk.m_vPoleVectorPosition = vPoleVectorPos;
    cmdIk.m_vMidAxis = WBasisAxis::GetBasisVector(m_MidAxis);
    cmdIk.m_fWeight = m_fWeight;
    cmdIk.m_fSoften = 1.0f;                   // m_fSoften;
    cmdIk.m_TwistAngle = WAngle::MakeZero(); // m_TwistAngle;

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }
}


W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_TwoBoneIKComponent);
