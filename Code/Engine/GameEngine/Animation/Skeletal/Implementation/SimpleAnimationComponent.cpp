#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/SimpleAnimationComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>

using namespace ozz;
using namespace ozz::animation;
using namespace ozz::math;

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSimpleAnimationComponent, 3, WComponentMode::Static);
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("AnimationClip", m_hAnimationClip)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation"), new WRequiredAttribute()),
    W_ENUM_MEMBER_PROPERTY("AnimationMode", WPropertyAnimMode, m_AnimationMode),
    W_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_ENUM_MEMBER_PROPERTY("RootMotionMode", WRootMotionMode, m_RootMotionMode),
    W_ENUM_MEMBER_PROPERTY("InvisibleUpdateRate", WAnimationInvisibleUpdateRate, m_InvisibleUpdateRate)->AddAttributes(new WDefaultValueAttribute(WAnimationInvisibleUpdateRate::Pause)),
    W_MEMBER_PROPERTY("EnableIK", m_bEnableIK),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
      new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WSimpleAnimationComponent::WSimpleAnimationComponent() = default;
WSimpleAnimationComponent::~WSimpleAnimationComponent() = default;

void WSimpleAnimationComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_AnimationMode;
  s << m_fSpeed;
  s << m_hAnimationClip;
  s << m_RootMotionMode;
  s << m_InvisibleUpdateRate;
  s << m_bEnableIK;
}

void WSimpleAnimationComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_AnimationMode;
  s >> m_fSpeed;
  s >> m_hAnimationClip;
  s >> m_RootMotionMode;

  if (uiVersion >= 2)
  {
    s >> m_InvisibleUpdateRate;
  }

  if (uiVersion >= 3)
  {
    s >> m_bEnableIK;
  }
}

void WSimpleAnimationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  WMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  m_hSkeleton = msg.m_hSkeleton;
}

void WSimpleAnimationComponent::SetNormalizedPlaybackPosition(float fPosition)
{
  m_fNormalizedPlaybackPosition = fPosition;

  // force update next time
  SetUserFlag(1, true);
}

void WSimpleAnimationComponent::Update()
{
  if (!m_hSkeleton.IsValid() || !m_hAnimationClip.IsValid())
    return;

  if (m_fSpeed == 0.0f && !GetUserFlag(1))
    return;

  WTime tMinStep = WTime::MakeFromSeconds(0);
  WVisibilityState::Enum visType = GetOwner()->GetVisibilityState();

  if (visType != WVisibilityState::Direct)
  {
    if (m_InvisibleUpdateRate == WAnimationInvisibleUpdateRate::Pause && visType == WVisibilityState::Invisible)
      return;

    tMinStep = WAnimationInvisibleUpdateRate::GetTimeStep(m_InvisibleUpdateRate);
  }

  m_ElapsedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return;

  W_PROFILE_SCOPE("WSimpleAnimationComponent::Update");

  // if we did this, the animation would fully stop, when the component is really invisible (not even indirectly visible)
  // this breaks the setting 'InvisibleUpdateRate', which is supposed to let the user override the update rate for this case
  const bool bVisible = true; // visType != WVisibilityState::Invisible;

  WResourceLock<WAnimationClipResource> pAnimation(m_hAnimationClip, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimation.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  const WTime tDiff = m_ElapsedTimeSinceUpdate;
  m_ElapsedTimeSinceUpdate = WTime::MakeZero();

  const WAnimationClipResourceDescriptor& animDesc = pAnimation->GetDescriptor();

  m_Duration = animDesc.GetDuration();

  const float fPrevPlaybackPos = m_fNormalizedPlaybackPosition;

  WAnimPoseEventTrackSampleMode mode = WAnimPoseEventTrackSampleMode::None;

  if (!UpdatePlaybackTime(tDiff, animDesc.m_EventTrack, mode))
    return;

  if (animDesc.m_EventTrack.IsEmpty())
  {
    mode = WAnimPoseEventTrackSampleMode::None;
  }

  // no need to do anything, if we can't get events and are currently invisible
  if (!bVisible && mode == WAnimPoseEventTrackSampleMode::None && m_RootMotionMode == WRootMotionMode::Ignore)
    return;

  WResourceLock<WSkeletonResource> pSkeleton(m_hSkeleton, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  WAnimPoseGenerator poseGen;
  poseGen.Reset(pSkeleton.GetPointer(), GetOwner());

  auto& cmdSample = poseGen.AllocCommandSampleTrack(0);
  cmdSample.m_hAnimationClip = m_hAnimationClip;
  cmdSample.m_fNormalizedSamplePos = m_fNormalizedPlaybackPosition;
  cmdSample.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
  cmdSample.m_EventSampling = mode;

  if (bVisible)
  {
    auto& cmdL2M = poseGen.AllocCommandLocalToModelPose();
    cmdL2M.m_pSendLocalPoseMsgTo = GetOwner();

    if (animDesc.m_bAdditive)
    {
      auto& cmdComb = poseGen.AllocCommandCombinePoses();
      cmdComb.m_Inputs.PushBack(cmdSample.GetCommandID());
      cmdComb.m_InputWeights.PushBack(1.0f);

      cmdL2M.m_Inputs.PushBack(cmdComb.GetCommandID());
    }
    else
    {
      cmdL2M.m_Inputs.PushBack(cmdSample.GetCommandID());
    }

    WAnimPoseGeneratorCommandID prevCmdID = cmdL2M.GetCommandID();
    poseGen.SetFinalCommand(prevCmdID);
  }

  poseGen.UpdatePose(m_bEnableIK);

  if (m_RootMotionMode != WRootMotionMode::Ignore)
  {
    m_vPendingRootMotion = tDiff.AsFloatInSeconds() * m_fSpeed * animDesc.m_vConstantRootMotion;

    const bool bReverse = GetUserFlag(0);
    if (bReverse)
    {
      m_vPendingRootMotion = -m_vPendingRootMotion;
    }
  }

  if (!animDesc.m_CustomCurves.IsEmpty())
  {
    const double fTimeSecs = (double)m_fNormalizedPlaybackPosition * animDesc.GetDuration().GetSeconds();
    for (const auto& curve : animDesc.m_CustomCurves)
    {
      const float fValue = (float)curve.m_Curve.Evaluate(fTimeSecs);

      WMsgAnimationCurveValue msg;
      msg.m_sCurveName = curve.m_sName;
      msg.m_fMin = fValue;
      msg.m_fMax = fValue;
      msg.m_fAverage = fValue;
      GetOwner()->PostEventMessage(msg, nullptr, WTime::MakeZero());
    }
  }

  if (poseGen.GetCurrentPose().IsEmpty())
    return;

  // inform child nodes/components that a new pose is available
  if (poseGen.ShouldSendPoseResultMsg())
  {
    WMsgAnimationPoseUpdated msg2;
    msg2.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    msg2.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    msg2.m_ModelTransforms = poseGen.GetCurrentPose();

    // recursive, so that objects below the mesh can also listen in on these changes
    // for example bone attachments
    GetOwner()->SendMessageRecursive(msg2);

    if (msg2.m_bContinueAnimating == false)
    {
      SetActiveFlag(false);
    }
  }
}

void WSimpleAnimationComponent::ApplyRootMotion()
{
  // only applies positional root motion; called from the PostAsync phase to allow safe game object modification
  WRootMotionMode::Apply(m_RootMotionMode, GetOwner(), m_vPendingRootMotion, WAngle(), WAngle(), WAngle());

  m_vPendingRootMotion = WVec3::MakeZero();
}

bool WSimpleAnimationComponent::UpdatePlaybackTime(WTime tDiff, const WEventTrack& eventTrack, WAnimPoseEventTrackSampleMode& out_trackSampling)
{
  if (tDiff.IsZero() || m_fSpeed == 0.0f)
  {
    if (GetUserFlag(1))
    {
      SetUserFlag(1, false);
      return true;
    }

    return false;
  }

  out_trackSampling = WAnimPoseEventTrackSampleMode::OnlyBetween;

  const float tDiffNorm = static_cast<float>(tDiff.GetSeconds() / m_Duration.GetSeconds());
  const float tPrefNorm = m_fNormalizedPlaybackPosition;

  switch (m_AnimationMode)
  {
    case WPropertyAnimMode::Once:
    {
      m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;
      m_fNormalizedPlaybackPosition = WMath::Clamp(m_fNormalizedPlaybackPosition, 0.0f, 1.0f);
      break;
    }

    case WPropertyAnimMode::Loop:
    {
      m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;

      if (m_fNormalizedPlaybackPosition < 0.0f)
      {
        m_fNormalizedPlaybackPosition += 1.0f;

        out_trackSampling = WAnimPoseEventTrackSampleMode::LoopAtStart;
      }
      else if (m_fNormalizedPlaybackPosition > 1.0f)
      {
        m_fNormalizedPlaybackPosition -= 1.0f;

        out_trackSampling = WAnimPoseEventTrackSampleMode::LoopAtEnd;
      }

      break;
    }

    case WPropertyAnimMode::BackAndForth:
    {
      const bool bReverse = GetUserFlag(0);

      if (bReverse)
        m_fNormalizedPlaybackPosition -= tDiffNorm * m_fSpeed;
      else
        m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;

      if (m_fNormalizedPlaybackPosition > 1.0f)
      {
        SetUserFlag(0, !bReverse);

        m_fNormalizedPlaybackPosition = 2.0f - m_fNormalizedPlaybackPosition;

        out_trackSampling = WAnimPoseEventTrackSampleMode::BounceAtEnd;
      }
      else if (m_fNormalizedPlaybackPosition < 0.0f)
      {
        SetUserFlag(0, !bReverse);

        m_fNormalizedPlaybackPosition = -m_fNormalizedPlaybackPosition;

        out_trackSampling = WAnimPoseEventTrackSampleMode::BounceAtStart;
      }

      break;
    }
  }

  return tPrefNorm != m_fNormalizedPlaybackPosition;
}


//////////////////////////////////////////////////////////////////////////

WSimpleAnimationComponentManager::WSimpleAnimationComponentManager(WWorld* pWorld)
  : WComponentManager(pWorld)
{
}

WSimpleAnimationComponentManager::~WSimpleAnimationComponentManager() = default;

void WSimpleAnimationComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WSimpleAnimationComponentManager::Update, this);
    desc.m_Phase = WWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_uiAsyncPhaseBatchSize = 2;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WSimpleAnimationComponentManager::ApplyRootMotion, this);
    desc.m_Phase = WWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void WSimpleAnimationComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update();
    }
  }
}

void WSimpleAnimationComponentManager::ApplyRootMotion(const WWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->m_RootMotionMode != WRootMotionMode::Ignore && it->IsActiveAndInitialized())
    {
      it->ApplyRootMotion();
    }
  }
}

W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_SimpleAnimationComponent);
