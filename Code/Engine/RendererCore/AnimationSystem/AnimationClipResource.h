#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/EventTrack.h>
#include <ozz/base/memory/unique_ptr.h>

class WSkeletonResource;
class WSkeleton;

namespace ozz::animation
{
  class Animation;
}

/// A single named float curve stored inside an animation clip.
struct W_RENDERERCORE_DLL WAnimationClipCustomCurve
{
  WHashedString m_sName;
  WCurve1D m_Curve;
};

struct W_RENDERERCORE_DLL WAnimationClipResourceDescriptor
{
public:
  WAnimationClipResourceDescriptor();
  WAnimationClipResourceDescriptor(WAnimationClipResourceDescriptor&& rhs);
  ~WAnimationClipResourceDescriptor();

  void operator=(WAnimationClipResourceDescriptor&& rhs) noexcept;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);

  WUInt64 GetHeapMemoryUsage() const;

  WUInt16 GetNumJoints() const;
  WTime GetDuration() const;
  void SetDuration(WTime duration);

  void CreateMappedOzzAnimation(ozz::unique_ptr<ozz::animation::Animation>& out_pOzzAnim, const WSkeleton& skeleton) const;
  const ozz::animation::Animation& GetMappedOzzAnimation(const WSkeletonResource& skeleton) const;

  struct JointInfo
  {
    WUInt32 m_uiPositionIdx = 0;
    WUInt32 m_uiRotationIdx = 0;
    WUInt32 m_uiScaleIdx = 0;
    WUInt16 m_uiPositionCount = 0;
    WUInt16 m_uiRotationCount = 0;
    WUInt16 m_uiScaleCount = 0;
  };

  struct KeyframeVec3
  {
    float m_fTimeInSec;
    WVec3 m_Value;
  };

  struct KeyframeQuat
  {
    float m_fTimeInSec;
    WQuat m_Value;
  };

  JointInfo CreateJoint(const WHashedString& sJointName, WUInt16 uiNumPositions, WUInt16 uiNumRotations, WUInt16 uiNumScales);
  const JointInfo* GetJointInfo(const WTempHashedString& sJointName) const;
  void AllocateJointTransforms();

  WArrayPtr<KeyframeVec3> GetPositionKeyframes(const JointInfo& jointInfo);
  WArrayPtr<KeyframeQuat> GetRotationKeyframes(const JointInfo& jointInfo);
  WArrayPtr<KeyframeVec3> GetScaleKeyframes(const JointInfo& jointInfo);

  WArrayPtr<const KeyframeVec3> GetPositionKeyframes(const JointInfo& jointInfo) const;
  WArrayPtr<const KeyframeQuat> GetRotationKeyframes(const JointInfo& jointInfo) const;
  WArrayPtr<const KeyframeVec3> GetScaleKeyframes(const JointInfo& jointInfo) const;

  WVec3 m_vConstantRootMotion = WVec3::MakeZero();

  WDynamicArray<WAnimationClipCustomCurve> m_CustomCurves;

  WEventTrack m_EventTrack;

  bool m_bAdditive = false;

private:
  mutable WMutex m_Mutex; ///< Guards m_JointInfos and m_pOzzImpl against concurrent access during async animation sampling.
  WArrayMap<WHashedString, JointInfo> m_JointInfos;
  WDataBuffer m_Transforms;
  WUInt32 m_uiNumTotalPositions = 0;
  WUInt32 m_uiNumTotalRotations = 0;
  WUInt32 m_uiNumTotalScales = 0;
  WTime m_Duration;

  struct OzzImpl;
  WUniquePtr<OzzImpl> m_pOzzImpl;
};

//////////////////////////////////////////////////////////////////////////

using WAnimationClipResourceHandle = WTypedResourceHandle<class WAnimationClipResource>;

class W_RENDERERCORE_DLL WAnimationClipResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationClipResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WAnimationClipResource);
  W_RESOURCE_DECLARE_CREATEABLE(WAnimationClipResource, WAnimationClipResourceDescriptor);

public:
  WAnimationClipResource();

  const WAnimationClipResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WUniquePtr<WAnimationClipResourceDescriptor> m_pDescriptor;
};
