#pragma once

#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WSkeletonResource;
class WStreamWriter;
class WStreamReader;
struct WAnimGraphSharedBoneWeights;

/// Creates per-bone weight masks for partial animation blending.
///
/// This node generates weight masks that control which bones are affected by animations. Specify root bones
/// to define hierarchies (e.g., spine for upper body). Outputs both normal and inverted weights, enabling
/// different animations on different body parts (upper body aim, lower body locomotion).
class W_RENDERERCORE_DLL WBoneWeightsAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WBoneWeightsAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WBoneWeightsAnimNode

public:
  WBoneWeightsAnimNode();
  ~WBoneWeightsAnimNode();

  float m_fWeight = 1.0f;                                       // [ property ]

  WUInt32 RootBones_GetCount() const;                          // [ property ]
  const char* RootBones_GetValue(WUInt32 uiIndex) const;       // [ property ]
  void RootBones_SetValue(WUInt32 uiIndex, const char* value); // [ property ]
  void RootBones_Insert(WUInt32 uiIndex, const char* value);   // [ property ]
  void RootBones_Remove(WUInt32 uiIndex);                      // [ property ]

private:
  WAnimGraphBoneWeightsOutputPin m_WeightsPin;                 // [ property ]
  WAnimGraphBoneWeightsOutputPin m_InverseWeightsPin;          // [ property ]

  WHybridArray<WHashedString, 2> m_RootBones;

  struct InstanceData
  {
    WSharedPtr<WAnimGraphSharedBoneWeights> m_pSharedBoneWeights;
    WSharedPtr<WAnimGraphSharedBoneWeights> m_pSharedInverseBoneWeights;
  };
};
