#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

/// Samples a single animation clip with playback control.
///
/// This node plays an animation clip over time, supporting looping, speed control, and root motion extraction.
/// Trigger outputs signal when the animation starts and finishes. Common use cases include playing walk cycles,
/// idle animations, or one-shot actions like attacks.
class W_RENDERERCORE_DLL WSampleAnimClipAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WSampleAnimClipAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WSampleAnimClipAnimNode

  void SetClip(const char* szClip);
  const char* GetClip() const;

public:
  WSampleAnimClipAnimNode();
  ~WSampleAnimClipAnimNode();

private:
  WHashedString m_sClip;                      // [ property ]
  bool m_bLoop = true;                         // [ property ]
  float m_fRootMotionAmount = 0.0f;            // [ property ]
  float m_fPlaybackSpeed = 1.0f;               // [ property ]

  WAnimGraphTriggerInputPin m_InStart;        // [ property ]
  WAnimGraphBoolInputPin m_InLoop;            // [ property ]
  WAnimGraphNumberInputPin m_InSpeed;         // [ property ]

  WAnimGraphLocalPoseOutputPin m_OutPose;     // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnStarted;  // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnFinished; // [ property ]

  struct InstanceData
  {
    WTime m_PlaybackTime = WTime::MakeFromHours(1000);
  };
};
