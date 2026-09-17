#pragma once

#include <RendererCore/Pipeline/RenderData.h>

/// This is the base class for types that handle rendering of different object types.
///
/// E.g. there are different renderers for meshes, particle effects, light sources, etc.
class W_RENDERERCORE_DLL WRenderer : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WRenderer, WReflectedClass);

public:
  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const = 0;

  virtual void RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const = 0;
};
