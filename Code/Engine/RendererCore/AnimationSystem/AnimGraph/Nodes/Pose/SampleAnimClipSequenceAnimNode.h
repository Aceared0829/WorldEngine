#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

/// Plays a sequence of animation clips with automatic transitions.
///
/// This node plays multiple animation clips in sequence (start → middle clips → end), with optional looping.
/// Useful for complex animations composed of intro, loop, and outro sections (e.g., sprint start, sprint loop, sprint end).
/// Trigger outputs signal when transitioning between clips.
class W_RENDERERCORE_DLL WSampleAnimClipSequenceAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WSampleAnimClipSequenceAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WSampleAnimClipSequenceAnimNode

public:
  WSampleAnimClipSequenceAnimNode();
  ~WSampleAnimClipSequenceAnimNode();

  void SetStartClip(const char* szClip);
  const char* GetStartClip() const;

  WUInt32 Clips_GetCount() const;                            // [ property ]
  const char* Clips_GetValue(WUInt32 uiIndex) const;         // [ property ]
  void Clips_SetValue(WUInt32 uiIndex, const char* szValue); // [ property ]
  void Clips_Insert(WUInt32 uiIndex, const char* szValue);   // [ property ]
  void Clips_Remove(WUInt32 uiIndex);                        // [ property ]

  void SetEndClip(const char* szClip);
  const char* GetEndClip() const;

private:
  WHashedString m_sStartClip;                      // [ property ]
  WHybridArray<WHashedString, 1> m_Clips;         // [ property ]
  WHashedString m_sEndClip;                        // [ property ]
  float m_fRootMotionAmount = 0.0f;                 // [ property ]
  bool m_bLoop = false;                             // [ property ]
  float m_fPlaybackSpeed = 1.0f;                    // [ property ]

  WAnimGraphTriggerInputPin m_InStart;             // [ property ]
  WAnimGraphBoolInputPin m_InLoop;                 // [ property ]
  WAnimGraphNumberInputPin m_InSpeed;              // [ property ]
  WAnimGraphNumberInputPin m_ClipIndexPin;         // [ property ]

  WAnimGraphLocalPoseOutputPin m_OutPose;          // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnMiddleStarted; // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnEndStarted;    // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnFinished;      // [ property ]

  enum class State : WUInt8
  {
    Off,
    Start,
    Middle,
    End,
    HoldStartFrame,
    HoldMiddleFrame,
    HoldEndFrame,
  };

  struct InstanceData
  {
    WTime m_PlaybackTime = WTime::MakeFromHours(1000);
    State m_State = State::Off;
    WUInt8 m_uiMiddleClipIdx = 0;
  };
};
