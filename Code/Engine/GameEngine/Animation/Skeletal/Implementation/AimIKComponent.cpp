#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AimIKComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WIkJointEntry, WNoBase, 1, WRTTIDefaultAllocator<WIkJointEntry>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Joint", m_sJointName),
    W_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WAimIKComponent, 3, WComponentMode::Dynamic);
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("DebugVisScale", GetDebugVisScale, SetDebugVisScale)->AddAttributes(new WClampValueAttribute(0.0f, 10.0f)),
    W_ENUM_MEMBER_PROPERTY("ForwardVector", WBasisAxis, m_ForwardVector)->AddAttributes(new WDefaultValueAttribute(WBasisAxis::PositiveX)),
    W_ENUM_MEMBER_PROPERTY("UpVector", WBasisAxis, m_UpVector)->AddAttributes(new WDefaultValueAttribute(WBasisAxis::PositiveZ)),
    W_ACCESSOR_PROPERTY("PoleVector", DummyGetter, SetPoleVectorReference)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_MEMBER_PROPERTY("InversePoleVector", m_bInversePoleVector),
    W_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
    W_ARRAY_MEMBER_PROPERTY("Joints", m_Joints),
    W_MEMBER_PROPERTY("Order", m_uiOrder),
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

WResult WIkJointEntry::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_sJointName;
  inout_stream << m_fWeight;
  return W_SUCCESS;
}

WResult WIkJointEntry::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_sJointName;
  inout_stream >> m_fWeight;
  m_uiJointIdx = 0;
  return W_SUCCESS;
}

WAimIKComponent::WAimIKComponent() = default;
WAimIKComponent::~WAimIKComponent() = default;

void WAimIKComponent::SetPoleVectorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hPoleVector = resolver(szReference, GetHandle(), "PoleVector");
}

void WAimIKComponent::SetDebugVisScale(float fScale)
{
  // allow scales from 0.05f to 10.0f
  // map them to range 0 to 200
  m_uiDebugVisScale = static_cast<WUInt8>(WMath::Clamp(WMath::RoundToInt(fScale * 20.0f), 0, 200));
}

float WAimIKComponent::GetDebugVisScale() const
{
  return m_uiDebugVisScale / 20.0f;
}

void WAimIKComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fWeight;
  s << m_ForwardVector;
  s << m_UpVector;
  inout_stream.WriteGameObjectHandle(m_hPoleVector);
  s.WriteArray(m_Joints).AssertSuccess();

  s << m_bInversePoleVector;

  s << m_uiOrder;
}

void WAimIKComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fWeight;
  s >> m_ForwardVector;
  s >> m_UpVector;
  m_hPoleVector = inout_stream.ReadGameObjectHandle();
  s.ReadArray(m_Joints).AssertSuccess();

  if (uiVersion >= 2)
  {
    s >> m_bInversePoleVector;
  }

  if (uiVersion >= 3)
  {
    s >> m_uiOrder;
  }
}

void WAimIKComponent::OnInjectPoseCommands(WMsgInjectPoseCommands& msg) const
{
  if ((m_fWeight <= 0.0f && m_uiDebugVisScale == 0) || m_Joints.IsEmpty())
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
  const WTransform selfTrans = GetOwner()->GetGlobalTransform();
  const WTransform ownerTransform = WTransform::MakeGlobalTransform(targetTrans, msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_RootTransform);
  const WTransform localTarget = WTransform::MakeLocalTransform(ownerTransform, selfTrans);

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

  for (WUInt32 i = 0; i < m_Joints.GetCount(); ++i)
  {
    if (m_Joints[i].m_fWeight <= 0.0f)
      continue;

    if (m_Joints[i].m_uiJointIdx == 0)
    {
      m_Joints[i].m_uiJointIdx = msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_Skeleton.FindJointByName(m_Joints[i].m_sJointName);
    }

    if (m_Joints[i].m_uiJointIdx == WInvalidJointIndex)
      continue;

    auto& cmdIk = msg.m_pGenerator->AllocCommandAimIK();
    cmdIk.m_fDebugVisScale = GetDebugVisScale();
    cmdIk.m_uiJointIdx = m_Joints[i].m_uiJointIdx;
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;
    cmdIk.m_fWeight = m_fWeight * m_Joints[i].m_fWeight;
    cmdIk.m_vForwardVector = WBasisAxis::GetBasisVector(m_ForwardVector);
    cmdIk.m_vUpVector = WBasisAxis::GetBasisVector(m_UpVector);
    cmdIk.m_vPoleVectorPosition = vPoleVectorPos;
    cmdIk.m_bInversePoleVector = m_bInversePoleVector;

    // in theory one could limit which joints get their model poses updated,
    // but in practice this doesn't work unless we know that they are definitely just in one straight line (not the case for spines)
    // if (i + 1 < m_Joints.GetCount())
    //{
    //  cmdIk.m_uiRecalcModelPoseToJointIdx = m_Joints[i + 1].m_uiJointIdx;
    //}

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }
}


W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AimIKComponent);
