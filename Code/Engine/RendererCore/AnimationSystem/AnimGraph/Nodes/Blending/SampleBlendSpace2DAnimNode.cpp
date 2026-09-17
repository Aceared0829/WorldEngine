#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Blending/SampleBlendSpace2DAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WAnimationClip2D, WNoBase, 1, WRTTIDefaultAllocator<WAnimationClip2D>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new WDynamicStringEnumAttribute("AnimationClipMappingEnum")),
    W_MEMBER_PROPERTY("Position", m_vPosition),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSampleBlendSpace2DAnimNode, 2, WRTTIDefaultAllocator<WSampleBlendSpace2DAnimNode>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new WDefaultValueAttribute(true)),
      W_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, {})),
      W_MEMBER_PROPERTY("RootMotionAmount", m_fRootMotionAmount)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 100.0f)),
      W_MEMBER_PROPERTY("InputResponse", m_InputResponse)->AddAttributes(new WDefaultValueAttribute(WTime::MakeFromMilliseconds(100))),
    W_ACCESSOR_PROPERTY("CenterClip", GetCenterClipFile, SetCenterClipFile)->AddAttributes(new WDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      W_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

      W_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("X", m_InCoordX)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("Y", m_InCoordY)->AddAttributes(new WHiddenAttribute()),

      W_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new WHiddenAttribute()),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WCategoryAttribute("Pose Generation"),
      new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue)),
      new WTitleAttribute("BlendSpace 2D: '{CenterClip}'"),
    }
    W_END_ATTRIBUTES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WAnimationClip2D::SetAnimationFile(const char* szFile)
{
  m_sClip.Assign(szFile);
}

const char* WAnimationClip2D::GetAnimationFile() const
{
  return m_sClip;
}

WSampleBlendSpace2DAnimNode::WSampleBlendSpace2DAnimNode() = default;
WSampleBlendSpace2DAnimNode::~WSampleBlendSpace2DAnimNode() = default;

WResult WSampleBlendSpace2DAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(3);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sCenterClip;

  stream << m_Clips.GetCount();
  for (WUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_sClip;
    stream << m_Clips[i].m_vPosition;
  }

  stream << m_bLoop;
  stream << m_fRootMotionAmount;
  stream << m_fPlaybackSpeed;
  stream << m_InputResponse;

  W_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InCoordX.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InCoordY.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return W_SUCCESS;
}

WResult WSampleBlendSpace2DAnimNode::DeserializeNode(WStreamReader& stream)
{
  const auto version = stream.ReadVersion(3);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sCenterClip;

  WUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (WUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_sClip;
    stream >> m_Clips[i].m_vPosition;
  }

  stream >> m_bLoop;

  if (version <= 2)
  {
    bool bApplyRootMotion = false;
    stream >> bApplyRootMotion;
    m_fRootMotionAmount = bApplyRootMotion ? 1.0f : 0.0f;
  }

  if (version >= 2)
  {
    stream >> m_fRootMotionAmount;
  }

  stream >> m_fPlaybackSpeed;
  stream >> m_InputResponse;

  W_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InCoordX.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InCoordY.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return W_SUCCESS;
}

void WSampleBlendSpace2DAnimNode::SetCenterClipFile(const char* szFile)
{
  m_sCenterClip.Assign(szFile);
}

const char* WSampleBlendSpace2DAnimNode::GetCenterClipFile() const
{
  return m_sCenterClip;
}

void WSampleBlendSpace2DAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || (!m_InCoordX.IsConnected() && !m_InCoordY.IsConnected()) || m_Clips.IsEmpty())
    return;

  InstanceData* pState = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (!m_InStart.IsConnected() && pState->m_CenterPlaybackTime > WTime::MakeFromHours(10))
  {
    pState->m_CenterPlaybackTime = WTime::MakeZero();
    pState->m_fOtherPlaybackPosNorm = 0.0f;
  }

  if (m_InStart.IsTriggered(ref_graph))
  {
    pState->m_CenterPlaybackTime = WTime::MakeZero();
    pState->m_fOtherPlaybackPosNorm = 0.0f;

    m_OutOnStarted.SetTriggered(ref_graph);
  }

  const float x = static_cast<float>(m_InCoordX.GetNumber(ref_graph));
  const float y = static_cast<float>(m_InCoordY.GetNumber(ref_graph));

  if (m_InputResponse.IsZeroOrNegative())
  {
    pState->m_fLastValueX = x;
    pState->m_fLastValueY = y;
  }
  else
  {
    const float lerp = static_cast<float>(WMath::Min(1.0, tDiff.GetSeconds() * (1.0 / m_InputResponse.GetSeconds())));
    pState->m_fLastValueX = WMath::Lerp(pState->m_fLastValueX, x, lerp);
    pState->m_fLastValueY = WMath::Lerp(pState->m_fLastValueY, y, lerp);
  }

  const auto& centerInfo = ref_controller.GetAnimationClipInfo(m_sCenterClip);

  WUInt32 uiMaxWeightClip = 0;
  WTempHybridArray<ClipToPlay, 8> clips;
  ComputeClipsAndWeights(ref_controller, centerInfo, WVec2(pState->m_fLastValueX, pState->m_fLastValueY), clips, uiMaxWeightClip);

  PlayClips(ref_controller, centerInfo, pState, ref_graph, tDiff, clips, uiMaxWeightClip);
}

void WSampleBlendSpace2DAnimNode::ComputeClipsAndWeights(WAnimController& ref_controller, const WAnimController::AnimClipInfo& centerInfo, const WVec2& p, WDynamicArray<ClipToPlay>& clips, WUInt32& out_uiMaxWeightClip) const
{
  out_uiMaxWeightClip = 0;
  float fMaxWeight = -1.0f;

  if (m_Clips.GetCount() == 1 && !centerInfo.m_hClip.IsValid())
  {
    auto& clip = clips.ExpandAndGetRef();
    clip.m_uiIndex = 0;
    clip.m_pClipInfo = &centerInfo;
  }
  else
  {
    // this algorithm is taken from http://runevision.com/thesis chapter 6.3 "Gradient Band Interpolation"
    // also see http://answers.unity.com/answers/1208837/view.html

    float fWeightNormalization = 0.0f;

    for (WUInt32 i = 0; i < m_Clips.GetCount(); ++i)
    {
      const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_Clips[i].m_sClip);
      if (!clipInfo.m_hClip.IsValid())
        continue;

      const WVec2 pi = m_Clips[i].m_vPosition;
      float fMinWeight = 1.0f;

      for (WUInt32 j = 0; j < m_Clips.GetCount(); ++j)
      {
        const WVec2 pj = m_Clips[j].m_vPosition;

        const float fLenSqr = (pi - pj).GetLengthSquared();
        const float fProjLenSqr = (pi - p).Dot(pi - pj);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = WMath::Min(fMinWeight, fWeight);
      }

      // also check against center clip
      if (centerInfo.m_hClip.IsValid())
      {
        const float fLenSqr = pi.GetLengthSquared();
        const float fProjLenSqr = (pi - p).Dot(pi);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = WMath::Min(fMinWeight, fWeight);
      }

      if (fMinWeight > 0.0f)
      {
        auto& c = clips.ExpandAndGetRef();
        c.m_uiIndex = i;
        c.m_fWeight = fMinWeight;
        c.m_pClipInfo = &clipInfo;

        fWeightNormalization += fMinWeight;
      }
    }

    // also compute weight for center clip
    if (centerInfo.m_hClip.IsValid())
    {
      float fMinWeight = 1.0f;

      for (WUInt32 j = 0; j < m_Clips.GetCount(); ++j)
      {
        const WVec2 pj = m_Clips[j].m_vPosition;

        const float fLenSqr = pj.GetLengthSquared();
        const float fProjLenSqr = (-p).Dot(-pj);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = WMath::Min(fMinWeight, fWeight);
      }

      if (fMinWeight > 0.0f)
      {
        auto& c = clips.ExpandAndGetRef();
        c.m_uiIndex = 0xFFFFFFFF;
        c.m_fWeight = fMinWeight;
        c.m_pClipInfo = &centerInfo;

        fWeightNormalization += fMinWeight;
      }
    }

    fWeightNormalization = 1.0f / fWeightNormalization;

    for (WUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      auto& c = clips[i];

      c.m_fWeight *= fWeightNormalization;

      if (c.m_fWeight > fMaxWeight)
      {
        fMaxWeight = c.m_fWeight;
        out_uiMaxWeightClip = i;
      }
    }
  }
}

void WSampleBlendSpace2DAnimNode::PlayClips(WAnimController& ref_controller, const WAnimController::AnimClipInfo& centerInfo, InstanceData* pState, WAnimGraphInstance& ref_graph, WTime tDiff, WArrayPtr<ClipToPlay> clips, WUInt32 uiMaxWeightClip) const
{
  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);
  const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed));

  WTime tAvgDuration = WTime::MakeZero();

  WTempHybridArray<WAnimPoseGeneratorCommandSampleTrack*, 8> pSampleTrack;
  pSampleTrack.SetCountUninitialized(clips.GetCount());

  WVec3 vRootMotion = WVec3::MakeZero();
  WUInt32 uiNumAvgClips = 0;

  WTempHybridArray<const WAnimationClipResourceDescriptor*, 24> pDescs;
  pDescs.SetCount(clips.GetCount());

  for (WUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    const auto& c = clips[i];

    const WHashedString sClip = c.m_uiIndex >= 0xFF ? m_sCenterClip : m_Clips[c.m_uiIndex].m_sClip;

    const auto& clipInfo = *clips[i].m_pClipInfo;

    WResourceLock<WAnimationClipResource> pClip(clipInfo.m_hClip, WResourceAcquireMode::BlockTillLoaded);

    if (c.m_uiIndex < 0xFF) // center clip should not contribute to the average time
    {
      ++uiNumAvgClips;
      tAvgDuration += pClip->GetDescriptor().GetDuration();
    }

    const void* pThis = this;
    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(WHashingUtils::xxHash32(&pThis, sizeof(pThis), i));
    cmd.m_hAnimationClip = clipInfo.m_hClip;
    cmd.m_fNormalizedSamplePos = pClip->GetDescriptor().GetDuration().AsFloatInSeconds(); // will be combined with actual pos below

    pSampleTrack[i] = &cmd;


    // need this later to look up the root motion
    pDescs[i] = &pClip->GetDescriptor();
  }

  if (uiNumAvgClips > 0)
  {
    tAvgDuration = tAvgDuration / uiNumAvgClips;
  }

  tAvgDuration = WMath::Max(tAvgDuration, WTime::MakeFromMilliseconds(16));

  const WTime fPrevCenterPlaybackPos = pState->m_CenterPlaybackTime;
  const float fPrevPlaybackPosNorm = pState->m_fOtherPlaybackPosNorm;

  WAnimPoseEventTrackSampleMode eventSamplingCenter = WAnimPoseEventTrackSampleMode::OnlyBetween;
  WAnimPoseEventTrackSampleMode eventSampling = WAnimPoseEventTrackSampleMode::OnlyBetween;

  const float fInvAvgDuration = 1.0f / tAvgDuration.AsFloatInSeconds();
  const float tDiffNorm = tDiff.AsFloatInSeconds() * fInvAvgDuration;

  // now that we know the duration, we can finally update the playback state
  pState->m_fOtherPlaybackPosNorm += tDiffNorm * fSpeed;
  while (pState->m_fOtherPlaybackPosNorm >= 1.0f)
  {
    if (bLoop)
    {
      pState->m_fOtherPlaybackPosNorm -= 1.0f;
      m_OutOnStarted.SetTriggered(ref_graph);
      eventSampling = WAnimPoseEventTrackSampleMode::LoopAtEnd;
    }
    else
    {
      pState->m_fOtherPlaybackPosNorm = 1.0f;

      if (fPrevPlaybackPosNorm < 1.0f)
      {
        m_OutOnFinished.SetTriggered(ref_graph);
      }
      else
      {
        eventSampling = WAnimPoseEventTrackSampleMode::None;
      }

      break;
    }
  }

  UpdateCenterClipPlaybackTime(centerInfo, pState, ref_graph, tDiff, eventSamplingCenter);

  for (WUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    if (pSampleTrack[i]->m_hAnimationClip == centerInfo.m_hClip)
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevCenterPlaybackPos.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_fNormalizedSamplePos = pState->m_CenterPlaybackTime.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_EventSampling = uiMaxWeightClip == i ? eventSamplingCenter : WAnimPoseEventTrackSampleMode::None;
    }
    else
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevPlaybackPosNorm;
      pSampleTrack[i]->m_fNormalizedSamplePos = pState->m_fOtherPlaybackPosNorm;
      pSampleTrack[i]->m_EventSampling = uiMaxWeightClip == i ? eventSampling : WAnimPoseEventTrackSampleMode::None;

      if (pDescs[i])
      {
        vRootMotion += pDescs[i]->m_vConstantRootMotion * clips[i].m_fWeight;
      }
    }
  }

  WAnimGraphPinDataLocalTransforms* pOutputTransform = ref_controller.AddPinDataLocalTransforms();

  if (m_fRootMotionAmount != 0.0f)
  {
    pOutputTransform->m_bUseRootMotion = true;

    const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed));

    pOutputTransform->m_vRootMotion = tDiff.AsFloatInSeconds() * vRootMotion * fSpeed * m_fRootMotionAmount;
  }

  // accumulate custom curves from all clips weighted by blend weight
  for (WUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    if (!pDescs[i] || clips[i].m_fWeight <= 0.0f)
      continue;

    const bool bIsCenter = (pSampleTrack[i]->m_hAnimationClip == centerInfo.m_hClip);
    const double fSampleTimeSecs = bIsCenter
                                     ? pState->m_CenterPlaybackTime.GetSeconds()
                                     : (double)pState->m_fOtherPlaybackPosNorm * pDescs[i]->GetDuration().GetSeconds();

    for (const auto& cc : pDescs[i]->m_CustomCurves)
    {
      const float fVal = static_cast<float>(cc.m_Curve.Evaluate(fSampleTimeSecs)) * clips[i].m_fWeight;
      bool bFound = false;
      for (auto& cv : pOutputTransform->m_CustomCurveValues)
      {
        if (cv.m_sName == cc.m_sName)
        {
          cv.m_fValue += fVal;
          bFound = true;
          break;
        }
      }
      if (!bFound)
      {
        auto& cv = pOutputTransform->m_CustomCurveValues.ExpandAndGetRef();
        cv.m_sName = cc.m_sName;
        cv.m_fValue = fVal;
      }
    }
  }

  if (clips.GetCount() == 1)
  {
    pOutputTransform->m_CommandID = pSampleTrack[0]->GetCommandID();
  }
  else
  {
    auto& cmdCmb = ref_controller.GetPoseGenerator().AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    cmdCmb.m_InputWeights.SetCountUninitialized(clips.GetCount());
    cmdCmb.m_Inputs.SetCountUninitialized(clips.GetCount());

    for (WUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      cmdCmb.m_InputWeights[i] = clips[i].m_fWeight;
      cmdCmb.m_Inputs[i] = pSampleTrack[i]->GetCommandID();
    }
  }

  m_OutPose.SetPose(ref_graph, pOutputTransform);
}

void WSampleBlendSpace2DAnimNode::UpdateCenterClipPlaybackTime(const WAnimController::AnimClipInfo& centerInfo, InstanceData* pState, WAnimGraphInstance& ref_graph, WTime tDiff, WAnimPoseEventTrackSampleMode& out_eventSamplingCenter) const
{
  const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed));

  if (centerInfo.m_hClip.IsValid())
  {
    WResourceLock<WAnimationClipResource> pClip(centerInfo.m_hClip, WResourceAcquireMode::BlockTillLoaded);

    const WTime tDur = pClip->GetDescriptor().GetDuration();

    pState->m_CenterPlaybackTime += tDiff * fSpeed;

    // always loop the center clip
    while (pState->m_CenterPlaybackTime > tDur)
    {
      pState->m_CenterPlaybackTime -= tDur;
      out_eventSamplingCenter = WAnimPoseEventTrackSampleMode::LoopAtEnd;
    }
  }
}

bool WSampleBlendSpace2DAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WSampleBlendSpace2DAnimNodePatch_1_2 : public WGraphPatch
{
public:
  WSampleBlendSpace2DAnimNodePatch_1_2()
    : WGraphPatch("WSampleBlendSpace2DAnimNode", 2)
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

WSampleBlendSpace2DAnimNodePatch_1_2 g_WSampleBlendSpace2DAnimNodePatch_1_2;

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Blending_SampleBlendSpace2DAnimNode);
