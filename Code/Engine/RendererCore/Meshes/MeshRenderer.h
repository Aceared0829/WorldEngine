#pragma once

#include <RendererCore/Pipeline/Renderer.h>

class WMeshRenderData;
struct WPerInstanceData;

/// Implements rendering of static meshes.
///
/// All meshes in one batch are rendered with a single instanced draw call.
class W_RENDERERCORE_DLL WMeshRenderer : public WRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WMeshRenderer, WRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WMeshRenderer);

public:
  WMeshRenderer();
  ~WMeshRenderer();

  // WRenderer implementation
  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual void RenderBatch(const WRenderViewContext& renderContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;

protected:
  /// Sets additional shader data specific to the current render data.
  ///
  /// Can be overridden to bind custom per-object data.
  virtual void SetAdditionalData(const WRenderViewContext& renderViewContext, const WMeshRenderData* pRenderData) const;
};
