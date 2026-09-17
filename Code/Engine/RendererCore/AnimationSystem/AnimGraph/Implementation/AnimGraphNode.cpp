#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphNode, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("CustomTitle", GetCustomNodeTitle, SetCustomNodeTitle),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAnimGraphNode::WAnimGraphNode() = default;
WAnimGraphNode::~WAnimGraphNode() = default;

WResult WAnimGraphNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  // no need to serialize this, not used at runtime
  // stream << m_CustomNodeTitle;

  return W_SUCCESS;
}

WResult WAnimGraphNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  // no need to serialize this, not used at runtime
  // stream >> m_CustomNodeTitle;

  return W_SUCCESS;
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphNode);
