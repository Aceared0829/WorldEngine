#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>

struct WPerSpriteData;
class WRenderDataBatch;
using WShaderResourceHandle = WTypedResourceHandle<class WShaderResource>;

/// Implements rendering of sprites.
///
/// Sprites are rendered as quads that always face the camera. All sprites in a batch are rendered with a single draw call.
class W_RENDERERCORE_DLL WSpriteRenderer : public WRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WSpriteRenderer, WRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WSpriteRenderer);

public:
  WSpriteRenderer();
  ~WSpriteRenderer();

  // WRenderer implementation
  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual void RenderBatch(const WRenderViewContext& renderContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;

  static float s_fShapeIconScale;
  static float s_fShapeIconFadeDistance;

protected:
  /// Creates a GPU buffer for per-sprite instance data.
  WGALBufferHandle CreateSpriteDataBuffer(WUInt32 uiBufferSize) const;

  /// Destroys a previously created sprite data buffer.
  void DeleteSpriteDataBuffer(WGALBufferHandle hBuffer) const;

  /// Fills the sprite data array with per-instance information from the batch.
  void FillSpriteData(const WRenderDataBatch& batch) const;

  WShaderResourceHandle m_hShader;
  mutable WDynamicArray<WPerSpriteData, WAlignedAllocatorWrapper> m_SpriteData;
};
