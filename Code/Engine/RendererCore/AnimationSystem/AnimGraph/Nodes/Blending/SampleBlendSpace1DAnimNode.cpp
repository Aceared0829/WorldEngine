#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Blending/SampleBlendSpace1DAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WAnimationClip1D, WNoBase, 1, WRTTIDefaultAllocator<WAnimationClip1D>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new WDynamicStringEnumAttribute("AnimationClipMappingEnum")),
    W_MEMBER_PROPERTY("Position", m_fPosition),
    W_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f)),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSampleBlendSpace1DAnimNode, 2, WRTTIDefaultAllocator<WSampleBlendSpace1DAnimNode>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new WDefaultValueAttribute(true)),
      W_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, {})),
      W_MEMBER_PROPERTY("RootMotionAmount", m_fRootMotionAmount)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 100.0f)),
      W_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

      W_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("InLerp", m_InLerp)->AddAttributes(new WHiddenAttribute()),

      W_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new WHiddenAttribute()),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WCategoryAttribute("Pose Generation"),
      new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue)),
      new WTitleAttribute("BlendSpace 1D: '{Clips[0]}' '{Clips[1]}' '{Clips[2]}'"),
    }
    W_END_ATTRIBUTES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WAnimationClip1D::SetAnimationFile(const char* szFile)
{
  m_sClip.Assign(szFile);
}

const char* WAnimationClip1D::GetAnimationFile() const
{
  return m_sClip;
}

WSampleBlendSpace1DAnimNode::WSampleBlendSpace1DAnimNode() = default;
WSampleBlendSpace1DAnimNode::~WSampleBlendSpace1DAnimNode() = default;

WResult WSampleBlendSpace1DAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(2);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_Clips.GetCount();
  for (WUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_sClip;
    stream << m_Clips[i].m_fPosition;
    stream << m_Clips[i].m_fSpeed;
  }

  stream << m_bLoop;
  stream << m_fRootMotionAmount;
  stream << m_fPlaybackSpeed;

  W_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InLerp.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return W_SUCCESS;
}

WResult WSampleBlendSpace1DAnimNode::DeserializeNode(WStreamReader& stream)
{
  const auto version = stream.ReadVersion(2);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  WUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (WUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_sClip;
    stream >> m_Clips[i].m_fPosition;
    stream >> m_Clips[i].m_fSpeed;
  }

  stream >> m_bLoop;

  if (version == 1)
  {
    bool bApplyRootMotion = false;
    stream >> bApplyRootMotion;
    m_fRootMotionAmount = bApplyRootMotion ? 1.0f : 0.0f;
  }
  else if (version >= 2)
  {
    stream >> m_fRootMotionAmount;
  }

  stream >> m_fPlaybackSpeed;

  W_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InLerp.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return W_SUCCESS;
}

void WSampleBlendSpace1DAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || !m_InLerp.IsConnected() || m_Clips.IsEmpty())
    return;

  InstanceData* pState = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (!m_InStart.IsConnected() && pState->m_PlaybackTime > WTime::MakeFromHours(10))
  {
    pState->m_PlaybackTime = WTime::MakeZero();
  }

  if (m_InStart.IsTriggered(ref_graph))
  {
    pState->m_PlaybackTime = WTime::MakeZero();

    m_OutOnStarted.SetTriggered(ref_graph);
  }

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  WUInt32 uiClip1 = 0;
  WUInt32 uiClip2 = 0;

  const float fLerpPos = (float)m_InLerp.GetNumber(ref_graph);

  if (m_Clips.GetCount() > 1)
  {
    float fDist1 = 1000000.0f;
    float fDist2 = 1000000.0f;

    for (WUInt32 i = 0; i < m_Clips.GetCount(); ++i)
    {
      const float dist = WMath::Abs(m_Clips[i].m_fPosition - fLerpPos);

      if (dist < fDist1)
      {
        fDist2 = fDist1;
        uiClip2 = uiClip1;

        fDist1 = dist;
        uiClip1 = i;
      }
      else if (dist < fDist2)
      {
        fDist2 = dist;
        uiClip2 = i;
      }
    }

    if (WMath::IsZero(fDist1, WMath::SmallEpsilon<float>()))
    {
      uiClip2 = uiClip1;
    }
  }

  const auto& clip1 = ref_controller.GetAnimationClipInfo(m_Clips[uiClip1].m_sClip);
  const auto& clip2 = ref_controller.GetAnimationClipInfo(m_Clips[uiClip2].m_sClip);

  if (!clip1.m_hClip.IsValid() || !clip2.m_hClip.IsValid())
    return;

  WResourceLock<WAnimationClipResource> pAnimClip1(clip1.m_hClip, WResourceAcquireMode::BlockTillLoaded);
  WResourceLock<WAnimationClipResource> pAnimClip2(clip2.m_hClip, WResourceAcquireMode::BlockTillLoaded);

  if (pAnimClip1.GetAcquireResult() != WResourceAcquireResult::Final || pAnimClip2.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  float fLerpFactor = 0.0f;

  if (uiClip1 != uiClip2)
  {
    const float len = m_Clips[uiClip2].m_fPosition - m_Clips[uiClip1].m_fPosition;
    fLerpFactor = (fLerpPos - m_Clips[uiClip1].m_fPosition) / len;

    // clamp and reduce to single sample when possible
    if (fLerpFactor <= 0.0f)
    {
      fLerpFactor = 0.0f;
      uiClip2 = uiClip1;
    }
    else if (fLerpFactor >= 1.0f)
    {
      fLerpFactor = 1.0f;
      uiClip1 = uiClip2;
    }
  }

  const float fAvgClipSpeed = WMath::Lerp(m_Clips[uiClip1].m_fSpeed, m_Clips[uiClip2].m_fSpeed, fLerpFactor);
  const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)) * fAvgClipSpeed;

  const auto& animDesc1 = pAnimClip1->GetDescriptor();
  const auto& animDesc2 = pAnimClip2->GetDescriptor();

  const WTime avgDuration = WMath::Lerp(animDesc1.GetDuration(), animDesc2.GetDuration(), fLerpFactor);
  const float fInvDuration = 1.0f / avgDuration.AsFloatInSeconds();

  const WTime tPrevPlayback = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fSpeed;

  WAnimPoseEventTrackSampleMode eventSampling = WAnimPoseEventTrackSampleMode::OnlyBetween;

  if (pState->m_PlaybackTime >= avgDuration)
  {
    if (bLoop)
    {
      pState->m_PlaybackTime -= avgDuration;
      eventSampling = WAnimPoseEventTrackSampleMode::LoopAtEnd;
      m_OutOnStarted.SetTriggered(ref_graph);
    }
    else
    {
      pState->m_PlaybackTime = avgDuration;

      if (tPrevPlayback < avgDuration)
      {
        m_OutOnFinished.SetTriggered(ref_graph);
      }
      else
      {
        // if we are already holding the last frame, we can skip event sampling
        eventSampling = WAnimPoseEventTrackSampleMode::None;
      }
    }
  }

  WAnimGraphPinDataLocalTransforms* pOutputTransform = ref_controller.AddPinDataLocalTransforms();

  auto& poseGen = ref_controller.GetPoseGenerator();

  const float fPrevPosNorm = tPrevPlayback.AsFloatInSeconds() * fInvDuration;
  const float fCurPosNorm = pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration;

  if (clip1.m_hClip == clip2.m_hClip)
  {
    const void* pThis = this;
    auto& cmd = poseGen.AllocCommandSampleTrack(WHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
    cmd.m_hAnimationClip = clip1.m_hClip;
    cmd.m_fPreviousNormalizedSamplePos = fPrevPosNorm;
    cmd.m_fNormalizedSamplePos = fCurPosNorm;
    cmd.m_EventSampling = eventSampling;

    pOutputTransform->m_CommandID = cmd.GetCommandID();
  }
  else
  {
    auto& cmdCmb = poseGen.AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    // sample animation 1
    {
      const void* pThis = this;
      auto& cmd = poseGen.AllocCommandSampleTrack(WHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
      cmd.m_hAnimationClip = clip1.m_hClip;
      cmd.m_fPreviousNormalizedSamplePos = fPrevPosNorm;
      cmd.m_fNormalizedSamplePos = fCurPosNorm;
      cmd.m_EventSampling = fLerpFactor <= 0.5f ? eventSampling : WAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(1.0f - fLerpFactor);
    }

    // sample animation 2
    {
      const void* pThis = this;
      auto& cmd = poseGen.AllocCommandSampleTrack(WHashingUtils::xxHash32(&pThis, sizeof(pThis), 1));
      cmd.m_hAnimationClip = clip2.m_hClip;
      cmd.m_fPreviousNormalizedSamplePos = fPrevPosNorm;
      cmd.m_fNormalizedSamplePos = fCurPosNorm;
      cmd.m_EventSampling = fLerpFactor > 0.5f ? eventSampling : WAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(fLerpFactor);
    }
  }

  // send to output
  {
    if (m_fRootMotionAmount != 0.0f)
    {
      pOutputTransform->m_bUseRootMotion = true;

      pOutputTransform->m_vRootMotion = WMath::Lerp(animDesc1.m_vConstantRootMotion, animDesc2.m_vConstantRootMotion, fLerpFactor) * tDiff.AsFloatInSeconds() * fSpeed * m_fRootMotionAmount;
    }

    // blend custom curves from both clips
    const double fSampleTimeSecs1 = (double)fCurPosNorm * animDesc1.GetDuration().GetSeconds();
    const double fSampleTimeSecs2 = (double)fCurPosNorm * animDesc2.GetDuration().GetSeconds();

    for (const auto& cc : animDesc1.m_CustomCurves)
    {
      auto& cv = pOutputTransform->m_CustomCurveValues.ExpandAndGetRef();
      cv.m_sName = cc.m_sName;
      cv.m_fValue = static_cast<float>(cc.m_Curve.Evaluate(fSampleTimeSecs1)) * (1.0f - fLerpFactor);
    }

    for (const auto& cc : animDesc2.m_CustomCurves)
    {
      const float fVal2 = static_cast<float>(cc.m_Curve.Evaluate(fSampleTimeSecs2)) * fLerpFactor;
      bool bFound = false;
      for (auto& cv : pOutputTransform->m_CustomCurveValues)
      {
        if (cv.m_sName == cc.m_sName)
        {
          cv.m_fValue += fVal2;
          bFound = true;
          break;
        }
      }
      if (!bFound)
      {
        auto& cv = pOutputTransform->m_CustomCurveValues.ExpandAndGetRef();
        cv.m_sName = cc.m_sName;
        cv.m_fValue = fVal2;
      }
    }

    m_OutPose.SetPose(ref_graph, pOutputTransform);
  }
}

bool WSampleBlendSpace1DAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WSampleBlendSpace1DAnimNodePatch_1_2 : public WGraphPatch
{
public:
  WSampleBlendSpace1DAnimNodePatch_1_2()
    : WGraphPatch("WSampleBlendSpace1DAnimNode", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    if (auto pProp = pNode->FindProperty("ApplyRootMotion"))
    {
      if (pProp->m_Value.IsA<bool>())
      {
        const bool bApply = pProp->m_Value.Get<bool>();

        if (bApply)
        {
          pNode->AddProperty("RootMotionAmount", 1.0f);
        }
      }
    }
  }
};

WSampleBlendSpace1DAnimNodePatch_1_2 g_WSampleBlendSpace1DAnimNodePatch_1_2;

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Blending_SampleBlendSpace1DAnimNode);
