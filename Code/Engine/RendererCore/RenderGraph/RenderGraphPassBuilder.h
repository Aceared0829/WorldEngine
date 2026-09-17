#pragma once

#include <Foundation/Math/Color.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererCore/RenderGraph/RenderGraphContext.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Returned by the render graph when adding a pass. The caller declares all resource accesses through this builder, then sets an execution callback. Only one instance of this class can exist at any time for each graph.
///
/// For render passes, the render graph will call BeginRendering/EndRendering automatically based on SetColorTarget/SetDepthStencilTarget declarations. The execute callback only needs to bind shaders, resources, and draw.
class W_RENDERERCORE_DLL WRenderGraphPassBuilder
{
public:
  WRenderGraphPassBuilder() = default;
  WRenderGraphPassBuilder(WRenderGraphPassBuilder&& rhs) noexcept;
  ~WRenderGraphPassBuilder();
  void operator=(WRenderGraphPassBuilder&& rhs) noexcept;

  /// \name Texture
  ///@{

  /// Declare that this pass reads a texture. `stage` is optional and should only be used in graphics passes when a later stage reads a texture so allow for more concurrent work on the GPU.
  ///
  /// For depth textures, the correct SRV state is `WGALResourceState::DepthStencilRead` instead of `WGALResourceState::ShaderResource`, but this function automatically switches this for your convenience.
  WRenderGraphPassBuilder& ReadTexture(WRenderGraphTextureHandle hTexture,
    WGALTextureRange range = {},
    WBitflags<WGALResourceState> access = WGALResourceState::ShaderResource,
    WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  /// Declare that this pass writes to a texture. `stage` is optional and should only be used in graphics passes when a later stage reads a texture so allow for more concurrent work on the GPU.
  WRenderGraphPassBuilder& WriteTexture(WRenderGraphTextureHandle hTexture,
    WGALTextureRange range = {},
    WBitflags<WGALResourceState> access = WGALResourceState::UnorderedAccess,
    WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  ///@}
  /// \name Buffer
  ///@{

  /// Declare that this pass reads a buffer. `stage` is optional and should only be used in graphics passes when a later stage reads a buffer so allow for more concurrent work on the GPU.
  WRenderGraphPassBuilder& ReadBuffer(WRenderGraphBufferHandle hBuffer,
    WBitflags<WGALResourceState> access = WGALResourceState::ShaderResource,
    WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  /// Declare that this pass writes to a buffer. `stage` is optional and should only be used in graphics passes when a later stage reads a buffer so allow for more concurrent work on the GPU.
  WRenderGraphPassBuilder& WriteBuffer(WRenderGraphBufferHandle hBuffer,
    WBitflags<WGALResourceState> access = WGALResourceState::UnorderedAccess,
    WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  ///@}
  /// \name Render Targets
  ///@{

  /// Adds a texture as a color render target. This will implicitly result in WriteTexture so there is no need to declare read / write operations for render targets.
  WRenderGraphPassBuilder& AddColorTarget(WRenderGraphTextureHandle hTexture,
    WGALRenderTargetRange range = {},
    WEnum<WGALRenderTargetLoadOp> loadOp = {},
    WEnum<WGALRenderTargetStoreOp> storeOp = {},
    WEnum<WGALResourceFormat> overrideViewFormat = WGALResourceFormat::Invalid,
    WEnum<WGALTextureType> overrideViewType = WGALTextureType::Invalid);

  /// Sets the clear color for the color target at the given index and switches the load op to `Clear`. SetColorTarget must have been called for this index first.
  WRenderGraphPassBuilder& SetClearColor(WUInt8 uiIndex, const WColor& color = WColor(0, 0, 0, 0));

  /// Bind a texture as the depth/stencil target. This will implicitly result in WriteTexture so there is no need to declare read / write operations for render targets.
  WRenderGraphPassBuilder& AddDepthStencilTarget(WRenderGraphTextureHandle hTexture,
    WGALRenderTargetRange range = {},
    WEnum<WGALRenderTargetLoadOp> depthLoadOp = {},
    WEnum<WGALRenderTargetStoreOp> depthStoreOp = {},
    WEnum<WGALRenderTargetLoadOp> stencilLoadOp = {},
    WEnum<WGALRenderTargetStoreOp> stencilStoreOp = {},
    bool bReadOnly = false);

  /// Sets the clear depth value and switches the depth load op to `Clear`. SetDepthStencilTarget must have been called first.
  WRenderGraphPassBuilder& SetClearDepth(float fDepthClear = 1.0f);

  /// Sets the clear stencil value and switches the stencil load op to `Clear`. SetDepthStencilTarget must have been called first.
  WRenderGraphPassBuilder& SetClearStencil(WUInt8 uiStencilClear = 0);


  ///@}
  /// \name Pass Configuration
  ///@{

  /// Marks this pass as having externally visible side effects (readback, etc.). Prevents the pass from being culled even if no other pass reads its outputs.
  WRenderGraphPassBuilder& HasSideEffects();

  /// Marks this pass as stereoscopic. Only valid for graphics passes.
  WRenderGraphPassBuilder& SetStereoscopic(bool bStereoscopic = true);

  /// Set the callback invoked when this pass executes. For render passes, BeginRendering will already be active when the callback is invoked.
  /// As the execute callback is called at a later date, make sure to capture variables by value if using a lambda.
  WRenderGraphPassBuilder& SetExecuteCallback(WRenderGraphExecuteFunction callback);

  ///@}

private:
  friend class WRenderGraph;
  WRenderGraphPassBuilder(WRenderGraph* pParent);

private:
  WRenderGraph* m_pParent = nullptr;
};
