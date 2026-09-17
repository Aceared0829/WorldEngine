#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Pose/SampleAnimClipSequenceAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSampleAnimClipSequenceAnimNode, 2, WRTTIDefaultAllocator<WSampleAnimClipSequenceAnimNode>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, {})),
      W_MEMBER_PROPERTY("Loop", m_bLoop),
      W_MEMBER_PROPERTY("RootMotionAmount", m_fRootMotionAmount)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 100.0f)),
      W_ACCESSOR_PROPERTY("StartClip", GetStartClip, SetStartClip)->AddAttributes(new WDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      W_ARRAY_ACCESSOR_PROPERTY("MiddleClips", Clips_GetCount, Clips_GetValue, Clips_SetValue, Clips_Insert, Clips_Remove)->AddAttributes(new WDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      W_ACCESSOR_PROPERTY("EndClip", GetEndClip, SetEndClip)->AddAttributes(new WDynamicStringEnumAttribute("AnimationClipMappingEnum")),

      W_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("ClipIndex", m_ClipIndexPin)->AddAttributes(new WHiddenAttribute()),

      W_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("OutOnMiddleStarted", m_OutOnMiddleStarted)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("OutOnEndStarted", m_OutOnEndStarted)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new WHiddenAttribute()),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WCategoryAttribute("Pose Generation"),
      new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue)),
      new WTitleAttribute("Sample Sequence: '{StartClip}' '{Clip}' '{EndClip}'"),
    }
    W_END_ATTRIBUTES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSampleAnimClipSequenceAnimNode::WSampleAnimClipSequenceAnimNode() = default;
WSampleAnimClipSequenceAnimNode::~WSampleAnimClipSequenceAnimNode() = default;

WResult WSampleAnimClipSequenceAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(2);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sStartClip;
  W_SUCCEED_OR_RETURN(stream.WriteArray(m_Clips));
  stream << m_sEndClip;
  stream << m_fRootMotionAmount;
  stream << m_bLoop;
  stream << m_fPlaybackSpeed;

  W_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_ClipIndexPin.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnMiddleStarted.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnEndStarted.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return W_SUCCESS;
}

WResult WSampleAnimClipSequenceAnimNode::DeserializeNode(WStreamReader& stream)
{
  const auto version = stream.ReadVersion(2);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sStartClip;
  W_SUCCEED_OR_RETURN(stream.ReadArray(m_Clips));
  stream >> m_sEndClip;

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

  stream >> m_bLoop;
  stream >> m_fPlaybackSpeed;

  W_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_ClipIndexPin.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnMiddleStarted.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnEndStarted.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return W_SUCCESS;
}

WUInt32 WSampleAnimClipSequenceAnimNode::Clips_GetCount() const
{
  return m_Clips.GetCount();
}

const char* WSampleAnimClipSequenceAnimNode::Clips_GetValue(WUInt32 uiIndex) const
{
  return m_Clips[uiIndex];
}

void WSampleAnimClipSequenceAnimNode::Clips_SetValue(WUInt32 uiIndex, const char* szValue)
{
  m_Clips[uiIndex].Assign(szValue);
}

void WSampleAnimClipSequenceAnimNode::Clips_Insert(WUInt32 uiIndex, const char* szValue)
{
  WHashedString s;
  s.Assign(szValue);
  m_Clips.InsertAt(uiIndex, s);
}

void WSampleAnimClipSequenceAnimNode::Clips_Remove(WUInt32 uiIndex)
{
  m_Clips.RemoveAtAndCopy(uiIndex);
}

void WSampleAnimClipSequenceAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  InstanceData* pState = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pState->m_PlaybackTime > WTime::MakeFromHours(10))
  {
    if (!m_InStart.IsConnected())
    {
      pState->m_State = State::Start;
    }

    pState->m_PlaybackTime = WTime::MakeZero();
  }

  if (m_InStart.IsTriggered(ref_graph))
  {
    pState->m_PlaybackTime = WTime::MakeZero();
    pState->m_State = State::Start;
  }

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  // currently we only support playing clips forwards
  const float fPlaySpeed = WMath::Max(0.0f, static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)));

  WTime tPrevSamplePos = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fPlaySpeed;

  WAnimationClipResourceHandle hCurClip;
  WTime tCurDuration;

  while (pState->m_State != State::Off)
  {
    if (pState->m_State == State::Start)
    {
      const auto& startClip = ref_controller.GetAnimationClipInfo(m_sStartClip);

      if (!startClip.m_hClip.IsValid())
      {
        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = static_cast<WUInt8>(m_ClipIndexPin.GetNumber(ref_graph, 0xFF));
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }

        pState->m_State = State::Middle;
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        continue;
      }

      WResourceLock<WAnimationClipResource> pAnimClip(startClip.m_hClip, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != WResourceAcquireResult::Final)
      {
        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = static_cast<WUInt8>(m_ClipIndexPin.GetNumber(ref_graph, 0xFF));
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }

        pState->m_State = State::Middle;
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      W_ASSERT_DEBUG(tCurDuration >= WTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        tPrevSamplePos = WTime::MakeZero();
        pState->m_PlaybackTime -= tCurDuration;
        pState->m_State = State::Middle;

        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = static_cast<WUInt8>(m_ClipIndexPin.GetNumber(ref_graph, 0xFF));
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }
        continue;
      }

      hCurClip = startClip.m_hClip;
      break;
    }

    if (pState->m_State == State::Middle)
    {
      if (m_Clips.IsEmpty())
      {
        pState->m_State = State::End;
        m_OutOnEndStarted.SetTriggered(ref_graph);
        continue;
      }

      const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_Clips[pState->m_uiMiddleClipIdx]);

      if (!clipInfo.m_hClip.IsValid())
      {
        pState->m_State = State::End;
        m_OutOnEndStarted.SetTriggered(ref_graph);
        continue;
      }

      WResourceLock<WAnimationClipResource> pAnimClip(clipInfo.m_hClip, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != WResourceAcquireResult::Final)
      {
        pState->m_State = State::End;
        m_OutOnEndStarted.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      W_ASSERT_DEBUG(tCurDuration >= WTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        tPrevSamplePos = WTime::MakeZero();
        pState->m_PlaybackTime -= tCurDuration;

        if (bLoop)
        {
          m_OutOnMiddleStarted.SetTriggered(ref_graph);
          pState->m_State = State::Middle;

          pState->m_uiMiddleClipIdx = static_cast<WUInt8>(m_ClipIndexPin.GetNumber(ref_graph, 0xFF));
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }
        else
        {
          m_OutOnEndStarted.SetTriggered(ref_graph);
          pState->m_State = State::End;
        }
        continue;
      }

      hCurClip = clipInfo.m_hClip;
      break;
    }

    if (pState->m_State == State::End)
    {
      const auto& endClip = ref_controller.GetAnimationClipInfo(m_sEndClip);

      if (!endClip.m_hClip.IsValid())
      {
        pState->m_State = State::HoldMiddleFrame;
        m_OutOnFinished.SetTriggered(ref_graph);
        continue;
      }

      WResourceLock<WAnimationClipResource> pAnimClip(endClip.m_hClip, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != WResourceAcquireResult::Final)
      {
        pState->m_State = State::HoldMiddleFrame;
        m_OutOnFinished.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      W_ASSERT_DEBUG(tCurDuration >= WTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        m_OutOnFinished.SetTriggered(ref_graph);
        pState->m_State = State::HoldEndFrame;
        continue;
      }

      hCurClip = endClip.m_hClip;
      break;
    }

    if (pState->m_State == State::HoldEndFrame)
    {
      const auto& endClip = ref_controller.GetAnimationClipInfo(m_sEndClip);
      hCurClip = endClip.m_hClip;

      WResourceLock<WAnimationClipResource> pAnimClip(endClip.m_hClip, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      pState->m_PlaybackTime = tCurDuration;
      break;
    }

    if (pState->m_State == State::HoldMiddleFrame)
    {
      if (m_Clips.IsEmpty() || pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
      {
        pState->m_State = State::HoldStartFrame;
        continue;
      }

      const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_Clips[pState->m_uiMiddleClipIdx]);

      if (!clipInfo.m_hClip.IsValid())
      {
        pState->m_State = State::HoldStartFrame;
        continue;
      }

      WResourceLock<WAnimationClipResource> pAnimClip(clipInfo.m_hClip, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != WResourceAcquireResult::Final)
      {
        pState->m_State = State::HoldStartFrame;
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      pState->m_PlaybackTime = tCurDuration;
      hCurClip = clipInfo.m_hClip;
      break;
    }

    if (pState->m_State == State::HoldStartFrame)
    {
      const auto& startClip = ref_controller.GetAnimationClipInfo(m_sStartClip);

      if (!startClip.m_hClip.IsValid())
      {
        pState->m_State = State::Off;
        continue;
      }

      WResourceLock<WAnimationClipResource> pAnimClip(startClip.m_hClip, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != WResourceAcquireResult::Final)
      {
        pState->m_State = State::Off;
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      W_ASSERT_DEBUG(tCurDuration >= WTime::MakeFromMilliseconds(5), "Too short clip");

      hCurClip = startClip.m_hClip;
      pState->m_PlaybackTime = tCurDuration;
      break;
    }
  }

  if (!hCurClip.IsValid())
    return;

  const float fInvDuration = 1.0f / tCurDuration.AsFloatInSeconds();

  const void* pThis = this;
  auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(WHashingUtils::xxHash32(&pThis, sizeof(pThis)));


  cmd.m_hAnimationClip = hCurClip;

  // if we are already holding the last frame, we can skip event sampling
  if (pState->m_State >= State::HoldStartFrame)
  {
    cmd.m_EventSampling = WAnimPoseEventTrackSampleMode::None;

    cmd.m_fPreviousNormalizedSamplePos = 1.0f;
    cmd.m_fNormalizedSamplePos = 1.0f;
  }
  else
  {
    cmd.m_EventSampling = WAnimPoseEventTrackSampleMode::OnlyBetween;

    cmd.m_fPreviousNormalizedSamplePos = WMath::Clamp(tPrevSamplePos.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
    cmd.m_fNormalizedSamplePos = WMath::Clamp(pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
  }

  {
    WAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights = nullptr;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    if (m_fRootMotionAmount != 0.0f)
    {
      pLocalTransforms->m_bUseRootMotion = true;

      const float fCurNormPos = WMath::Clamp(pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
      WResourceLock<WAnimationClipResource> pAnimClip(hCurClip, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * fPlaySpeed * m_fRootMotionAmount;

      const double fSampleTimeSecs = (double)fCurNormPos * pAnimClip->GetDescriptor().GetDuration().GetSeconds();
      for (const auto& cc : pAnimClip->GetDescriptor().m_CustomCurves)
      {
        auto& cv = pLocalTransforms->m_CustomCurveValues.ExpandAndGetRef();
        cv.m_sName = cc.m_sName;
        cv.m_fValue = static_cast<float>(cc.m_Curve.Evaluate(fSampleTimeSecs));
      }
    }

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
}

void WSampleAnimClipSequenceAnimNode::SetStartClip(const char* szClip)
{
  m_sStartClip.Assign(szClip);
}

const char* WSampleAnimClipSequenceAnimNode::GetStartClip() const
{
  return m_sStartClip;
}

void WSampleAnimClipSequenceAnimNode::SetEndClip(const char* szClip)
{
  m_sEndClip.Assign(szClip);
}

const char* WSampleAnimClipSequenceAnimNode::GetEndClip() const
{
  return m_sEndClip;
}

bool WSampleAnimClipSequenceAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WSampleAnimClipSequenceAnimNodePatch_1_2 : public WGraphPatch
{
public:
  WSampleAnimClipSequenceAnimNodePatch_1_2()
    : WGraphPatch("WSampleAnimClipSequenceAnimNode", 2)
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

WSampleAnimClipSequenceAnimNodePatch_1_2 g_WSampleAnimClipSequenceAnimNodePatch_1_2;

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Pose_SampleAnimClipSequenceAnimNode);
