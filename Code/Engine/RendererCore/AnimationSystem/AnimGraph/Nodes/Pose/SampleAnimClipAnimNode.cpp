#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Pose/SampleAnimClipAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSampleAnimClipAnimNode, 2, WRTTIDefaultAllocator<WSampleAnimClipAnimNode>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new WDefaultValueAttribute(true)),
      W_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, {})),
      W_MEMBER_PROPERTY("RootMotionAmount", m_fRootMotionAmount)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 100.0f)),
      W_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new WDynamicStringEnumAttribute("AnimationClipMappingEnum")),

      W_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new WHiddenAttribute()),

      W_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new WHiddenAttribute()),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WCategoryAttribute("Pose Generation"),
      new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue)),
      new WTitleAttribute("Sample Clip: '{Clip}'"),
    }
    W_END_ATTRIBUTES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSampleAnimClipAnimNode::WSampleAnimClipAnimNode() = default;
WSampleAnimClipAnimNode::~WSampleAnimClipAnimNode() = default;

WResult WSampleAnimClipAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(2);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sClip;
  stream << m_bLoop;
  stream << m_fRootMotionAmount;
  stream << m_fPlaybackSpeed;

  W_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return W_SUCCESS;
}

WResult WSampleAnimClipAnimNode::DeserializeNode(WStreamReader& stream)
{
  const auto version = stream.ReadVersion(2);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sClip;
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
  W_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return W_SUCCESS;
}

void WSampleAnimClipAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_sClip);

  if (!clipInfo.m_hClip.IsValid() || !m_OutPose.IsConnected())
    return;

  WResourceLock<WAnimationClipResource> pAnimClip(clipInfo.m_hClip, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimClip.GetAcquireResult() != WResourceAcquireResult::Final)
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

  const WTime tDuration = pAnimClip->GetDescriptor().GetDuration();
  const float fInvDuration = 1.0f / tDuration.AsFloatInSeconds();

  // currently we only support playing clips forwards
  const float fPlaySpeed = WMath::Max(0.0f, static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)));

  const WTime tPrevSamplePos = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fPlaySpeed;

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  const void* pThis = this;
  auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(WHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_EventSampling = WAnimPoseEventTrackSampleMode::OnlyBetween;

  if (pState->m_PlaybackTime >= tDuration)
  {
    if (bLoop)
    {
      pState->m_PlaybackTime -= tDuration;
      cmd.m_EventSampling = WAnimPoseEventTrackSampleMode::LoopAtEnd;
      m_OutOnStarted.SetTriggered(ref_graph);
    }
    else
    {
      if (tPrevSamplePos < tDuration)
      {
        m_OutOnFinished.SetTriggered(ref_graph);
      }
      else
      {
        // if we are already holding the last frame, we can skip event sampling
        cmd.m_EventSampling = WAnimPoseEventTrackSampleMode::None;
      }
    }
  }

  const float fPrevNormPos = WMath::Clamp(tPrevSamplePos.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
  const float fCurNormPos = WMath::Clamp(pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);

  cmd.m_hAnimationClip = clipInfo.m_hClip;
  cmd.m_fPreviousNormalizedSamplePos = fPrevNormPos;
  cmd.m_fNormalizedSamplePos = fCurNormPos;

  {
    WAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights = nullptr;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    if (m_fRootMotionAmount != 0.0f)
    {
      pLocalTransforms->m_bUseRootMotion = true;

      pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * fPlaySpeed * m_fRootMotionAmount;
    }

    const double fSampleTimeSecs = (double)fCurNormPos * pAnimClip->GetDescriptor().GetDuration().GetSeconds();
    for (const auto& cc : pAnimClip->GetDescriptor().m_CustomCurves)
    {
      auto& cv = pLocalTransforms->m_CustomCurveValues.ExpandAndGetRef();
      cv.m_sName = cc.m_sName;
      cv.m_fValue = static_cast<float>(cc.m_Curve.Evaluate(fSampleTimeSecs));
    }

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
}

void WSampleAnimClipAnimNode::SetClip(const char* szClip)
{
  m_sClip.Assign(szClip);
}

const char* WSampleAnimClipAnimNode::GetClip() const
{
  return m_sClip.GetData();
}

bool WSampleAnimClipAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WSampleAnimClipAnimNodePatch_1_2 : public WGraphPatch
{
public:
  WSampleAnimClipAnimNodePatch_1_2()
    : WGraphPatch("WSampleAnimClipAnimNode", 2)
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

WSampleAnimClipAnimNodePatch_1_2 g_WSampleAnimClipAnimNodePatch_1_2;

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Pose_SampleAnimClipAnimNode);
