#include <RendererCore/Pipeline/SubGraphNode.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSubGraphTextureInputNode, 1, WRTTIDefaultAllocator<WSubGraphTextureInputNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Value", m_Value),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Subgraph"),
    new WTitleAttribute("Texture Input: {Name}"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSubGraphTextureOutputNode, 1, WRTTIDefaultAllocator<WSubGraphTextureOutputNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Value", m_Value),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Subgraph"),
    new WTitleAttribute("Texture Output: {Name}"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSubGraphBufferInputNode, 1, WRTTIDefaultAllocator<WSubGraphBufferInputNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Value", m_Value),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Subgraph"),
    new WTitleAttribute("Buffer Input: {Name}"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Teal)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSubGraphBufferOutputNode, 1, WRTTIDefaultAllocator<WSubGraphBufferOutputNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Value", m_Value),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Subgraph"),
    new WTitleAttribute("Buffer Output: {Name}"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Teal)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSubGraphNode, 1, WRTTIDefaultAllocator<WSubGraphNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Pipeline", m_sPipeline)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_RenderPipeline", WDependencyFlags::Transform)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Subgraph"),
    new WTitleAttribute("Subgraph: {Pipeline}"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSubGraphTextureInputNode::WSubGraphTextureInputNode()
  : WRenderPipelinePass("TextureInput")
{
}

WSubGraphTextureOutputNode::WSubGraphTextureOutputNode()
  : WRenderPipelinePass("TextureOutput")
{
}

WSubGraphBufferInputNode::WSubGraphBufferInputNode()
  : WRenderPipelinePass("BufferInput")
{
}

WSubGraphBufferOutputNode::WSubGraphBufferOutputNode()
  : WRenderPipelinePass("BufferOutput")
{
}

WSubGraphNode::WSubGraphNode()
  : WRenderPipelineNode()
{
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineSubgraph);
