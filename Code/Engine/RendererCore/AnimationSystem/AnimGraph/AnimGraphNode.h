#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

class WSkeletonResource;
class WGameObject;
class WAnimGraphInstance;
class WAnimController;
class WStreamWriter;
class WStreamReader;
struct WAnimGraphPinDataLocalTransforms;
struct WAnimGraphPinDataBoneWeights;
class WAnimationClipResource;
struct WInstanceDataDesc;

using WAnimationClipResourceHandle = WTypedResourceHandle<class WAnimationClipResource>;

namespace ozz
{
  namespace animation
  {
    class Animation;
  }
} // namespace ozz

/// Base class for all nodes in an WAnimGraph
///
/// Animation graph nodes implement different operations in the animation system such as sampling clips,
/// blending poses, logic operations, or even outputting debug information.
///
/// Nodes define:
/// - Input and output pins (typed connections to other nodes)
/// - Behavior in the Step() method (called each frame during graph evaluation)
/// - Optional per-instance state data (playback time, blend weights, etc.)
///
/// Nodes that need to store state across frames (like playback time or transition progress) should use
/// the instance data pattern. This allows one graph definition (WAnimGraph) to be shared by many
/// runtime instances (WAnimGraphInstance), where each instance has its own state data.
///
/// The same WAnimGraph can be used by hundreds of characters, each with their own
/// WAnimGraphInstance and instance data. The graph definition (nodes, connections, pins) is shared
/// to minimize memory overhead.
class W_RENDERERCORE_DLL WAnimGraphNode : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphNode, WReflectedClass);

public:
  WAnimGraphNode();
  virtual ~WAnimGraphNode();

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

  const char* GetCustomNodeTitle() const { return m_sCustomNodeTitle.GetString(); }
  void SetCustomNodeTitle(const char* szSz) { m_sCustomNodeTitle.Assign(szSz); }

protected:
  friend class WAnimGraphInstance;
  friend class WAnimGraph;
  friend class WAnimGraphResource;

  WHashedString m_sCustomNodeTitle;
  WUInt32 m_uiInstanceDataOffset = WInvalidIndex;

  virtual WResult SerializeNode(WStreamWriter& stream) const = 0;
  virtual WResult DeserializeNode(WStreamReader& stream) = 0;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const = 0;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const { return false; }
};
