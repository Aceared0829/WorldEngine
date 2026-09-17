#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Marks a texture input of a render pipeline that is meant to be used as a sub-graph.
///
/// The node's name becomes the name of the input pin on the WSubGraphNode node that references the pipeline. Boundary nodes are removed when a sub-graph is inlined into its parent, so they never end up in a runtime pipeline.
class W_RENDERERCORE_DLL WSubGraphTextureInputNode : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WSubGraphTextureInputNode, WRenderPipelinePass);

public:
  WSubGraphTextureInputNode();

  WRenderPipelineNodeOutputPin m_Value;
};

/// Marks a texture output of a render pipeline that is meant to be used as a sub-graph. See WSubGraphTextureInputNode.
class W_RENDERERCORE_DLL WSubGraphTextureOutputNode : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WSubGraphTextureOutputNode, WRenderPipelinePass);

public:
  WSubGraphTextureOutputNode();

  WRenderPipelineNodeInputPin m_Value;
};

/// Marks a buffer input of a render pipeline that is meant to be used as a sub-graph. See WSubGraphTextureInputNode.
class W_RENDERERCORE_DLL WSubGraphBufferInputNode : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WSubGraphBufferInputNode, WRenderPipelinePass);

public:
  WSubGraphBufferInputNode();

  WRenderPipelineNodeBufferOutputPin m_Value;
};

/// Marks a buffer output of a render pipeline that is meant to be used as a sub-graph. See WSubGraphTextureInputNode.
class W_RENDERERCORE_DLL WSubGraphBufferOutputNode : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WSubGraphBufferOutputNode, WRenderPipelinePass);

public:
  WSubGraphBufferOutputNode();

  WRenderPipelineNodeBufferInputPin m_Value;
};

/// Placeholder for another render pipeline asset that is inlined into this one.
///
/// Its pins are derived from the boundary nodes of the referenced pipeline. This node only exists while authoring, the asset transform replaces it with the contents of the referenced pipeline, which is why it is not an WRenderPipelinePass.
class W_RENDERERCORE_DLL WSubGraphNode : public WRenderPipelineNode
{
  W_ADD_DYNAMIC_REFLECTION(WSubGraphNode, WRenderPipelineNode);

public:
  WSubGraphNode();

  WString m_sPipeline; ///< The render pipeline asset to inline.
};
