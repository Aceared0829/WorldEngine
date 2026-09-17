#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationClipResource, 1, WRTTIDefaultAllocator<WAnimationClipResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WAnimationClipResource);
// clang-format on

WAnimationClipResource::WAnimationClipResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WAnimationClipResource, WAnimationClipResourceDescriptor)
{
  m_pDescriptor = W_DEFAULT_NEW(WAnimationClipResourceDescriptor);
  *m_pDescriptor = std::move(descriptor);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

WResourceLoadDesc WAnimationClipResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WAnimationClipResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WAnimationClipResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = W_DEFAULT_NEW(WAnimationClipResourceDescriptor);
  m_pDescriptor->Deserialize(*Stream).IgnoreResult();

  res.m_State = WResourceState::Loaded;
  return res;
}

void WAnimationClipResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WAnimationClipResource);

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += static_cast<WUInt32>(m_pDescriptor->GetHeapMemoryUsage());
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct WAnimationClipResourceDescriptor::OzzImpl
{
  struct CachedAnim
  {
    WUInt32 m_uiResourceChangeCounter = 0;
    ozz::unique_ptr<ozz::animation::Animation> m_pAnim;
  };

  WMap<const WSkeletonResource*, CachedAnim> m_MappedOzzAnimations;
};

WAnimationClipResourceDescriptor::WAnimationClipResourceDescriptor()
{
  m_pOzzImpl = W_DEFAULT_NEW(OzzImpl);
}

WAnimationClipResourceDescriptor::WAnimationClipResourceDescriptor(WAnimationClipResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

WAnimationClipResourceDescriptor::~WAnimationClipResourceDescriptor() = default;

void WAnimationClipResourceDescriptor::operator=(WAnimationClipResourceDescriptor&& rhs) noexcept
{
  m_pOzzImpl = std::move(rhs.m_pOzzImpl);

  m_JointInfos = std::move(rhs.m_JointInfos);
  m_Transforms = std::move(rhs.m_Transforms);
  m_uiNumTotalPositions = rhs.m_uiNumTotalPositions;
  m_uiNumTotalRotations = rhs.m_uiNumTotalRotations;
  m_uiNumTotalScales = rhs.m_uiNumTotalScales;
  m_Duration = rhs.m_Duration;
}

WResult WAnimationClipResourceDescriptor::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(11);

  WUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  WCompressedStreamWriterZstd compressor(&inout_stream, 0, WCompressedStreamWriterZstd::Compression::Average);
  WStreamWriter& stream = compressor;
#else
  WStreamWriter& stream = inout_stream;
#endif

  // the compression mode is written to the uncompressed stream, everything after it is compressed
  inout_stream << uiCompressionMode;

  const WUInt16 uiNumJoints = static_cast<WUInt16>(m_JointInfos.GetCount());
  stream << uiNumJoints;
  for (WUInt32 i = 0; i < m_JointInfos.GetCount(); ++i)
  {
    const auto& val = m_JointInfos.GetValue(i);

    stream << m_JointInfos.GetKey(i);
    stream << val.m_uiPositionIdx;
    stream << val.m_uiPositionCount;
    stream << val.m_uiRotationIdx;
    stream << val.m_uiRotationCount;
    stream << val.m_uiScaleIdx;
    stream << val.m_uiScaleCount;
  }

  stream << m_Duration;
  stream << m_uiNumTotalPositions;
  stream << m_uiNumTotalRotations;
  stream << m_uiNumTotalScales;

  W_SUCCEED_OR_RETURN(stream.WriteArray(m_Transforms));

  stream << m_vConstantRootMotion;

  m_EventTrack.Save(stream);

  stream << m_bAdditive;

  const WUInt16 uiNumCustomCurves = static_cast<WUInt16>(m_CustomCurves.GetCount());
  stream << uiNumCustomCurves;

  for (const auto& cc : m_CustomCurves)
  {
    stream << cc.m_sName;
    cc.m_Curve.Save(stream);
  }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  W_SUCCEED_OR_RETURN(compressor.FinishCompressedStream());
#endif

  return W_SUCCESS;
}

WResult WAnimationClipResourceDescriptor::Deserialize(WStreamReader& inout_stream)
{
  const WTypeVersion uiVersion = inout_stream.ReadVersion(11);

  if (uiVersion < 6)
    return W_FAILURE;

  WStreamReader* pStream = &inout_stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  WCompressedStreamReaderZstd decompressor;
#endif

  if (uiVersion >= 11)
  {
    WUInt8 uiCompressionMode = 0;
    inout_stream >> uiCompressionMode;

    switch (uiCompressionMode)
    {
      case 0:
        break;

      case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        decompressor.SetInputStream(&inout_stream);
        pStream = &decompressor;
        break;
#else
        WLog::Error("Animation clip is compressed with zstandard, but support for this compression scheme is not compiled in.");
        return W_FAILURE;
#endif

      default:
        WLog::Error("Animation clip uses an unknown compression mode {}.", uiCompressionMode);
        return W_FAILURE;
    }
  }

  WStreamReader& stream = *pStream;

  WUInt16 uiNumJoints = 0;
  stream >> uiNumJoints;

  m_JointInfos.Reserve(uiNumJoints);

  WHashedString hs;

  for (WUInt16 i = 0; i < uiNumJoints; ++i)
  {
    stream >> hs;

    JointInfo ji;
    stream >> ji.m_uiPositionIdx;
    stream >> ji.m_uiPositionCount;
    stream >> ji.m_uiRotationIdx;
    stream >> ji.m_uiRotationCount;
    stream >> ji.m_uiScaleIdx;
    stream >> ji.m_uiScaleCount;

    m_JointInfos.Insert(hs, ji);
  }

  m_JointInfos.Sort();

  stream >> m_Duration;
  stream >> m_uiNumTotalPositions;
  stream >> m_uiNumTotalRotations;
  stream >> m_uiNumTotalScales;

  W_SUCCEED_OR_RETURN(stream.ReadArray(m_Transforms));

  if (uiVersion >= 7)
  {
    stream >> m_vConstantRootMotion;
  }

  if (uiVersion >= 8)
  {
    m_EventTrack.Load(stream);
  }

  if (uiVersion >= 9)
  {
    stream >> m_bAdditive;
  }

  if (uiVersion >= 10)
  {
    WUInt16 uiNumCustomCurves = 0;
    stream >> uiNumCustomCurves;

    m_CustomCurves.SetCount(uiNumCustomCurves);
    for (auto& cc : m_CustomCurves)
    {
      stream >> cc.m_sName;
      cc.m_Curve.Load(stream);
      cc.m_Curve.SortControlPoints();
      cc.m_Curve.CreateLinearApproximation();
    }
  }

  return W_SUCCESS;
}

WUInt64 WAnimationClipResourceDescriptor::GetHeapMemoryUsage() const
{
  W_LOCK(m_Mutex);
  return m_Transforms.GetHeapMemoryUsage() + m_JointInfos.GetHeapMemoryUsage() + m_pOzzImpl->m_MappedOzzAnimations.GetHeapMemoryUsage();
}

WUInt16 WAnimationClipResourceDescriptor::GetNumJoints() const
{
  return static_cast<WUInt16>(m_JointInfos.GetCount());
}

WTime WAnimationClipResourceDescriptor::GetDuration() const
{
  return m_Duration;
}

void WAnimationClipResourceDescriptor::SetDuration(WTime duration)
{
  m_Duration = duration;
}

W_FORCE_INLINE void ez2ozz(const WVec3& vIn, ozz::math::Float3& ref_out)
{
  ref_out.x = vIn.x;
  ref_out.y = vIn.y;
  ref_out.z = vIn.z;
}

W_FORCE_INLINE void ez2ozz(const WQuat& qIn, ozz::math::Quaternion& ref_out)
{
  ref_out.x = qIn.x;
  ref_out.y = qIn.y;
  ref_out.z = qIn.z;
  ref_out.w = qIn.w;
}

void WAnimationClipResourceDescriptor::CreateMappedOzzAnimation(ozz::unique_ptr<ozz::animation::Animation>& out_pOzzAnim, const WSkeleton& skeleton) const
{
  auto pOzzSkeleton = &skeleton.GetOzzSkeleton();
  const WUInt32 uiNumJoints = pOzzSkeleton->num_joints();

  ozz::animation::offline::RawAnimation rawAnim;
  rawAnim.duration = WMath::Max(1.0f / 60.0f, m_Duration.AsFloatInSeconds());
  rawAnim.tracks.resize(uiNumJoints);

  for (WUInt32 j = 0; j < uiNumJoints; ++j)
  {
    auto& dstTrack = rawAnim.tracks[j];

    const WTempHashedString sJointName = WTempHashedString(pOzzSkeleton->joint_names()[j]);

    const JointInfo* pJointInfo = GetJointInfo(sJointName);

    if (pJointInfo == nullptr)
    {
      dstTrack.translations.resize(1);
      dstTrack.rotations.resize(1);
      dstTrack.scales.resize(1);

      const WUInt16 uiFallbackIdx = skeleton.FindJointByName(sJointName);

      W_ASSERT_DEV(uiFallbackIdx != WInvalidJointIndex, "");

      const auto& fallbackJoint = skeleton.GetJointByIndex(uiFallbackIdx);

      const WTransform& fallbackTransform = m_bAdditive ? WTransform::MakeIdentity() : fallbackJoint.GetRestPoseLocalTransform();

      auto& dstT = dstTrack.translations[0];
      auto& dstR = dstTrack.rotations[0];
      auto& dstS = dstTrack.scales[0];

      dstT.time = 0.0f;
      dstR.time = 0.0f;
      dstS.time = 0.0f;

      ez2ozz(fallbackTransform.m_vPosition, dstT.value);
      ez2ozz(fallbackTransform.m_qRotation, dstR.value);
      ez2ozz(fallbackTransform.m_vScale, dstS.value);
    }
    else
    {
      // positions
      {
        dstTrack.translations.resize(pJointInfo->m_uiPositionCount);
        const WArrayPtr<const KeyframeVec3> keyframes = GetPositionKeyframes(*pJointInfo);

        for (WUInt32 i = 0; i < pJointInfo->m_uiPositionCount; ++i)
        {
          auto& dst = dstTrack.translations[i];

          dst.time = keyframes[i].m_fTimeInSec;
          ez2ozz(keyframes[i].m_Value, dst.value);
        }
      }

      // rotations
      {
        dstTrack.rotations.resize(pJointInfo->m_uiRotationCount);
        const WArrayPtr<const KeyframeQuat> keyframes = GetRotationKeyframes(*pJointInfo);

        for (WUInt32 i = 0; i < pJointInfo->m_uiRotationCount; ++i)
        {
          auto& dst = dstTrack.rotations[i];

          dst.time = keyframes[i].m_fTimeInSec;
          ez2ozz(keyframes[i].m_Value, dst.value);
        }
      }

      // scales
      {
        dstTrack.scales.resize(pJointInfo->m_uiScaleCount);
        const WArrayPtr<const KeyframeVec3> keyframes = GetScaleKeyframes(*pJointInfo);

        for (WUInt32 i = 0; i < pJointInfo->m_uiScaleCount; ++i)
        {
          auto& dst = dstTrack.scales[i];

          dst.time = keyframes[i].m_fTimeInSec;
          ez2ozz(keyframes[i].m_Value, dst.value);
        }
      }
    }
  }

  ozz::animation::offline::AnimationBuilder animBuilder;

  W_ASSERT_DEBUG(rawAnim.Validate(), "Invalid animation data");

  out_pOzzAnim = std::move(animBuilder(rawAnim));
}

const ozz::animation::Animation& WAnimationClipResourceDescriptor::GetMappedOzzAnimation(const WSkeletonResource& skeleton) const
{
  W_LOCK(m_Mutex);
  auto it = m_pOzzImpl->m_MappedOzzAnimations.Find(&skeleton);
  if (it.IsValid())
  {
    if (it.Value().m_uiResourceChangeCounter == skeleton.GetCurrentResourceChangeCounter())
    {
      return *it.Value().m_pAnim.get();
    }
  }

  auto& cached = m_pOzzImpl->m_MappedOzzAnimations[&skeleton];
  CreateMappedOzzAnimation(cached.m_pAnim, skeleton.GetDescriptor().m_Skeleton);
  cached.m_uiResourceChangeCounter = skeleton.GetCurrentResourceChangeCounter();

  return *cached.m_pAnim.get();
}

WAnimationClipResourceDescriptor::JointInfo WAnimationClipResourceDescriptor::CreateJoint(const WHashedString& sJointName, WUInt16 uiNumPositions, WUInt16 uiNumRotations, WUInt16 uiNumScales)
{
  JointInfo ji;
  ji.m_uiPositionIdx = m_uiNumTotalPositions;
  ji.m_uiRotationIdx = m_uiNumTotalRotations;
  ji.m_uiScaleIdx = m_uiNumTotalScales;

  ji.m_uiPositionCount = uiNumPositions;
  ji.m_uiRotationCount = uiNumRotations;
  ji.m_uiScaleCount = uiNumScales;

  m_uiNumTotalPositions += uiNumPositions;
  m_uiNumTotalRotations += uiNumRotations;
  m_uiNumTotalScales += uiNumScales;

  m_JointInfos.Insert(sJointName, ji);

  return ji;
}

const WAnimationClipResourceDescriptor::JointInfo* WAnimationClipResourceDescriptor::GetJointInfo(const WTempHashedString& sJointName) const
{
  WUInt32 uiIndex = m_JointInfos.Find(sJointName);

  if (uiIndex == WInvalidIndex)
    return nullptr;

  return &m_JointInfos.GetValue(uiIndex);
}

void WAnimationClipResourceDescriptor::AllocateJointTransforms()
{
  const WUInt32 uiNumBytes = m_uiNumTotalPositions * sizeof(KeyframeVec3) + m_uiNumTotalRotations * sizeof(KeyframeQuat) + m_uiNumTotalScales * sizeof(KeyframeVec3);

  m_Transforms.SetCountUninitialized(uiNumBytes);
}

WArrayPtr<WAnimationClipResourceDescriptor::KeyframeVec3> WAnimationClipResourceDescriptor::GetPositionKeyframes(const JointInfo& jointInfo)
{
  W_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  WUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiPositionIdx;

  return WArrayPtr<KeyframeVec3>(reinterpret_cast<KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiPositionCount);
}

WArrayPtr<WAnimationClipResourceDescriptor::KeyframeQuat> WAnimationClipResourceDescriptor::GetRotationKeyframes(const JointInfo& jointInfo)
{
  W_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  WUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * jointInfo.m_uiRotationIdx;

  return WArrayPtr<KeyframeQuat>(reinterpret_cast<KeyframeQuat*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiRotationCount);
}

WArrayPtr<WAnimationClipResourceDescriptor::KeyframeVec3> WAnimationClipResourceDescriptor::GetScaleKeyframes(const JointInfo& jointInfo)
{
  W_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  WUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * m_uiNumTotalRotations;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiScaleIdx;

  return WArrayPtr<KeyframeVec3>(reinterpret_cast<KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiScaleCount);
}

WArrayPtr<const WAnimationClipResourceDescriptor::KeyframeVec3> WAnimationClipResourceDescriptor::GetPositionKeyframes(const JointInfo& jointInfo) const
{
  WUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiPositionIdx;

  return WArrayPtr<const KeyframeVec3>(reinterpret_cast<const KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiPositionCount);
}

WArrayPtr<const WAnimationClipResourceDescriptor::KeyframeQuat> WAnimationClipResourceDescriptor::GetRotationKeyframes(const JointInfo& jointInfo) const
{
  WUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * jointInfo.m_uiRotationIdx;

  return WArrayPtr<const KeyframeQuat>(reinterpret_cast<const KeyframeQuat*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiRotationCount);
}

WArrayPtr<const WAnimationClipResourceDescriptor::KeyframeVec3> WAnimationClipResourceDescriptor::GetScaleKeyframes(const JointInfo& jointInfo) const
{
  WUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * m_uiNumTotalRotations;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiScaleIdx;

  return WArrayPtr<const KeyframeVec3>(reinterpret_cast<const KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiScaleCount);
}

// bool WAnimationClipResourceDescriptor::HasRootMotion() const
//{
//  return m_JointNameToIndex.Contains(WTempHashedString("WRootMotionTransform"));
//}
//
// WUInt16 WAnimationClipResourceDescriptor::GetRootMotionJoint() const
//{
//  WUInt16 jointIdx = 0;
//
// #if W_ENABLED(W_COMPILE_FOR_DEBUG)
//
//  const WUInt32 idx = m_JointNameToIndex.Find(WTempHashedString("WRootMotionTransform"));
//  W_ASSERT_DEBUG(idx != WInvalidIndex, "Animation Clip has no root motion transforms");
//
//  jointIdx = m_JointNameToIndex.GetValue(idx);
//  W_ASSERT_DEBUG(jointIdx == 0, "The root motion joint should always be at index 0");
// #endif
//
//  return jointIdx;
//}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationClipResource);
