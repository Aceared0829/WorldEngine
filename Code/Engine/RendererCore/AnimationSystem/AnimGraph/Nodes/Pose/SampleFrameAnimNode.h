#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

/// Samples a specific frame of an animation clip without time-based playback.
///
/// This node outputs a static pose from a specific frame of an animation. Useful for pose-based animations,
/// aim offsets, or manual frame selection. The sample position can be specified as a normalized value (0-1)
/// or absolute frame number.
class W_RENDERERCORE_DLL WSampleFrameAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WSampleFrameAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WSampleFrameAnimNode

public:
  void SetClip(const char* szClip);
  const char* GetClip() const;

  WHashedString m_sClip;                                 // [ property ]
  float m_fNormalizedSamplePosition = 0.0f;               // [ property ]

private:
  WAnimGraphNumberInputPin m_InNormalizedSamplePosition; // [ property ]
  WAnimGraphNumberInputPin m_InAbsoluteSamplePosition;   // [ property ]
  WAnimGraphLocalPoseOutputPin m_OutPose;                // [ property ]
};
