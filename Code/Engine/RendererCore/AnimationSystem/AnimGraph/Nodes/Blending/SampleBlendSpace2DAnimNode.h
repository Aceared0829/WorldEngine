#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

struct W_RENDERERCORE_DLL WAnimationClip2D
{
  WHashedString m_sClip;
  WVec2 m_vPosition;

  void SetAnimationFile(const char* szFile);
  const char* GetAnimationFile() const;
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WAnimationClip2D);

/// Blends between animation clips based on two parameters (2D blend space).
///
/// This node defines clips at 2D positions and uses triangular interpolation to blend between them.
/// Commonly used for directional locomotion (forward/backward, left/right) or aim offsets (pitch/yaw).
/// The center clip provides the base animation, with surrounding clips modifying it based on input coordinates.
class W_RENDERERCORE_DLL WSampleBlendSpace2DAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WSampleBlendSpace2DAnimNode, WAnimGraphNode);

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
  WSampleBlendSpace2DAnimNode();
  ~WSampleBlendSpace2DAnimNode();

  void SetCenterClipFile(const char* szFile);
  const char* GetCenterClipFile() const;

private:
  WHashedString m_sCenterClip;                               // [ property ]
  WHybridArray<WAnimationClip2D, 8> m_Clips;                // [ property ]
  WTime m_InputResponse = WTime::MakeFromMilliseconds(100); // [ property ]
  bool m_bLoop = true;                                        // [ property ]
  float m_fRootMotionAmount = 0.0f;                           // [ property ]
  float m_fPlaybackSpeed = 1.0f;                              // [ property ]

  WAnimGraphTriggerInputPin m_InStart;                       // [ property ]
  WAnimGraphBoolInputPin m_InLoop;                           // [ property ]
  WAnimGraphNumberInputPin m_InSpeed;                        // [ property ]
  WAnimGraphNumberInputPin m_InCoordX;                       // [ property ]
  WAnimGraphNumberInputPin m_InCoordY;                       // [ property ]
  WAnimGraphLocalPoseOutputPin m_OutPose;                    // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnStarted;                 // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnFinished;                // [ property ]

  struct ClipToPlay
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_uiIndex;
    float m_fWeight = 1.0f;
    const WAnimController::AnimClipInfo* m_pClipInfo = nullptr;
  };

  struct InstanceData
  {
    WTime m_CenterPlaybackTime = WTime::MakeFromHours(1000);
    float m_fOtherPlaybackPosNorm = 0.0f;
    float m_fLastValueX = 0.0f;
    float m_fLastValueY = 0.0f;
  };

  void UpdateCenterClipPlaybackTime(const WAnimController::AnimClipInfo& centerInfo, InstanceData* pState, WAnimGraphInstance& ref_graph, WTime tDiff, WAnimPoseEventTrackSampleMode& out_eventSamplingCenter) const;
  void PlayClips(WAnimController& ref_controller, const WAnimController::AnimClipInfo& centerInfo, InstanceData* pState, WAnimGraphInstance& ref_graph, WTime tDiff, WArrayPtr<ClipToPlay> clips, WUInt32 uiMaxWeightClip) const;
  void ComputeClipsAndWeights(WAnimController& ref_controller, const WAnimController::AnimClipInfo& centerInfo, const WVec2& p, WDynamicArray<ClipToPlay>& out_Clips, WUInt32& out_uiMaxWeightClip) const;
};
