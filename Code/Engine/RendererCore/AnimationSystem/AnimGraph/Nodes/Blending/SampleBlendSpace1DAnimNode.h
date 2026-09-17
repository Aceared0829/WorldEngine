#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

struct W_RENDERERCORE_DLL WAnimationClip1D
{
  WHashedString m_sClip;
  float m_fPosition = 0.0f;
  float m_fSpeed = 1.0f;

  void SetAnimationFile(const char* szFile);
  const char* GetAnimationFile() const;
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WAnimationClip1D);

/// Blends between animation clips based on a single parameter (1D blend space).
///
/// This node defines clips at different parameter values (e.g., 0=idle, 0.5=walk, 1=run) and automatically
/// interpolates between them based on input. Commonly used for speed-based locomotion where movement speed
/// determines the animation blend.
class W_RENDERERCORE_DLL WSampleBlendSpace1DAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WSampleBlendSpace1DAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WSampleBlendSpace1DAnimNode

public:
  WSampleBlendSpace1DAnimNode();
  ~WSampleBlendSpace1DAnimNode();

private:
  WHybridArray<WAnimationClip1D, 4> m_Clips; // [ property ]
  bool m_bLoop = true;                         // [ property ]
  float m_fRootMotionAmount = 0.0f;            // [ property ]
  float m_fPlaybackSpeed = 1.0f;               // [ property ]

  WAnimGraphTriggerInputPin m_InStart;        // [ property ]
  WAnimGraphBoolInputPin m_InLoop;            // [ property ]
  WAnimGraphNumberInputPin m_InSpeed;         // [ property ]
  WAnimGraphNumberInputPin m_InLerp;          // [ property ]
  WAnimGraphLocalPoseOutputPin m_OutPose;     // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnStarted;  // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnFinished; // [ property ]


  struct InstanceData
  {
    WTime m_PlaybackTime = WTime::MakeFromHours(1000);
  };
};
