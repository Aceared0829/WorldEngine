#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/JointAttachmentComponent.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJointAttachmentComponent, 1, WComponentMode::Dynamic);
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
    W_MEMBER_PROPERTY("PositionOffset", m_vLocalPositionOffset),
    W_MEMBER_PROPERTY("RotationOffset", m_vLocalRotationOffset),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
      new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgAnimationPoseUpdated, OnAnimationPoseUpdated)
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

WJointAttachmentComponent::WJointAttachmentComponent() = default;
WJointAttachmentComponent::~WJointAttachmentComponent() = default;

void WJointAttachmentComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sJointToAttachTo;
  s << m_vLocalPositionOffset;
  s << m_vLocalRotationOffset;
}

void WJointAttachmentComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sJointToAttachTo;
  s >> m_vLocalPositionOffset;
  s >> m_vLocalRotationOffset;

  m_uiJointIndex = WInvalidJointIndex;
}

void WJointAttachmentComponent::SetJointName(const char* szName)
{
  m_sJointToAttachTo.Assign(szName);
  m_uiJointIndex = WInvalidJointIndex;
}

const char* WJointAttachmentComponent::GetJointName() const
{
  return m_sJointToAttachTo.GetData();
}

void WJointAttachmentComponent::OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& msg)
{
  if (m_uiJointIndex == WInvalidJointIndex)
  {
    m_uiJointIndex = msg.m_pSkeleton->FindJointByName(m_sJointToAttachTo);
  }

  if (m_uiJointIndex == WInvalidJointIndex)
    return;

  WMat4 bone;
  WQuat boneRot;

  msg.ComputeFullBoneTransform(m_uiJointIndex, bone, boneRot);

  WGameObject* pOwner = GetOwner();
  pOwner->SetLocalPosition(bone.GetTranslationVector() + bone.TransformDirection(m_vLocalPositionOffset));
  pOwner->SetLocalRotation(boneRot * m_vLocalRotationOffset);
}

W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_JointAttachmentComponent);
