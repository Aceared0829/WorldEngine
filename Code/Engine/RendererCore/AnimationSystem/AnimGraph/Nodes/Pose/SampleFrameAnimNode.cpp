#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Pose/SampleFrameAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSampleFrameAnimNode, 1, WRTTIDefaultAllocator<WSampleFrameAnimNode>)
  {
    W_BEGIN_PROPERTIES
    {
      W_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new WDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      W_MEMBER_PROPERTY("NormPos", m_fNormalizedSamplePosition)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 1.0f)),

      W_MEMBER_PROPERTY("InNormPos", m_InNormalizedSamplePosition)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("InAbsPos", m_InAbsoluteSamplePosition)->AddAttributes(new WHiddenAttribute()),

      W_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new WHiddenAttribute()),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WCategoryAttribute("Pose Generation"),
      new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue)),
      new WTitleAttribute("Sample Frame: '{Clip}'"),
    }
    W_END_ATTRIBUTES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WSampleFrameAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sClip;
  stream << m_fNormalizedSamplePosition;

  W_SUCCEED_OR_RETURN(m_InNormalizedSamplePosition.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InAbsoluteSamplePosition.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return W_SUCCESS;
}

WResult WSampleFrameAnimNode::DeserializeNode(WStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sClip;
  stream >> m_fNormalizedSamplePosition;

  W_SUCCEED_OR_RETURN(m_InNormalizedSamplePosition.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InAbsoluteSamplePosition.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return W_SUCCESS;
}

void WSampleFrameAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  const auto& clip = ref_controller.GetAnimationClipInfo(m_sClip);

  if (clip.m_hClip.IsValid())
  {
    WResourceLock<WAnimationClipResource> pAnimClip(clip.m_hClip, WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pAnimClip.GetAcquireResult() != WResourceAcquireResult::Final)
      return;

    float fNormPos = static_cast<float>(m_InNormalizedSamplePosition.GetNumber(ref_graph, m_fNormalizedSamplePosition));

    if (m_InAbsoluteSamplePosition.IsConnected())
    {
      const WTime tDuration = pAnimClip->GetDescriptor().GetDuration();
      const float fInvDuration = 1.0f / tDuration.AsFloatInSeconds();
      fNormPos = static_cast<float>(m_InAbsoluteSamplePosition.GetNumber(ref_graph) * fInvDuration);
    }

    fNormPos = WMath::Clamp(fNormPos, 0.0f, 1.0f);

    const void* pThis = this;
    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(WHashingUtils::xxHash32(&pThis, sizeof(pThis)));

    cmd.m_hAnimationClip = clip.m_hClip;
    cmd.m_fPreviousNormalizedSamplePos = fNormPos;
    cmd.m_fNormalizedSamplePos = fNormPos;
    cmd.m_EventSampling = WAnimPoseEventTrackSampleMode::None;

    {
      WAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_pWeights = nullptr;
      pLocalTransforms->m_bUseRootMotion = false;
      pLocalTransforms->m_fOverallWeight = 1.0f;
      pLocalTransforms->m_CommandID = cmd.GetCommandID();

      m_OutPose.SetPose(ref_graph, pLocalTransforms);
    }
  }
  else
  {
    const void* pThis = this;
    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandRestPose();

    {
      WAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_pWeights = nullptr;
      pLocalTransforms->m_bUseRootMotion = false;
      pLocalTransforms->m_fOverallWeight = 1.0f;
      pLocalTransforms->m_CommandID = cmd.GetCommandID();

      m_OutPose.SetPose(ref_graph, pLocalTransforms);
    }
  }
}

void WSampleFrameAnimNode::SetClip(const char* szClip)
{
  m_sClip.Assign(szClip);
}

const char* WSampleFrameAnimNode::GetClip() const
{
  return m_sClip.GetData();
}


W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Pose_SampleFrameAnimNode);
