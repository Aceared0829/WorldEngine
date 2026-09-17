#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Basics.h>
#include <Foundation/Math/Size.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

/// This class can be used to define the render targets to be used by an WView.
struct W_RENDERERFOUNDATION_DLL WGALRenderTargets
{
  bool operator==(const WGALRenderTargets& other) const;
  bool operator!=(const WGALRenderTargets& other) const;

  WGALTextureHandle m_hRTs[W_GAL_MAX_RENDERTARGET_COUNT];
  WGALTextureHandle m_hDSTarget;
};

/// This class is used to describe the render pass setup of a graphics pipeline.
/// This should not be filled out manually, but rather be created by the WGALRenderingSetup.
/// The render pass descriptor is eventually used by the renderer to generate both a render pass as well as compatible graphics pipeline state objects.
/// \sa WGALRenderingSetup
struct WGALRenderPassDescriptor : public WHashableStruct<WGALRenderPassDescriptor>
{
  WUInt8 m_uiRTCount = 0;
  WEnum<WGALMSAASampleCount> m_Msaa;
  WEnum<WGALResourceFormat> m_DepthFormat = WGALResourceFormat::Invalid;
  WEnum<WGALRenderTargetLoadOp> m_DepthLoadOp;
  WEnum<WGALRenderTargetStoreOp> m_DepthStoreOp;
  WEnum<WGALRenderTargetLoadOp> m_StencilLoadOp;
  WEnum<WGALRenderTargetStoreOp> m_StencilStoreOp;
  WEnum<WGALResourceFormat> m_ColorFormat[W_GAL_MAX_RENDERTARGET_COUNT];
  WEnum<WGALRenderTargetLoadOp> m_ColorLoadOp[W_GAL_MAX_RENDERTARGET_COUNT];
  WEnum<WGALRenderTargetStoreOp> m_ColorStoreOp[W_GAL_MAX_RENDERTARGET_COUNT];
};

/// This class is used to describe the frame buffer of a graphics pipeline.
/// This should not be filled out manually, but rather be created by the WGALRenderingSetup.
/// The frame buffer descriptor is used by the renderer to set up render targets to render into.
/// \sa WGALRenderingSetup
struct WGALFrameBufferDescriptor : public WHashableStruct<WGALFrameBufferDescriptor>
{
  WGALRenderTargetViewHandle m_hColorTarget[W_GAL_MAX_RENDERTARGET_COUNT];
  WGALRenderTargetViewHandle m_hDepthTarget;
  WSizeU32 m_Size = {0, 0};
  WUInt32 m_uiSliceCount = 0;
};

/// This class sets up the render targets for a graphics pipeline, used by WRenderContext::BeginRendering or WGALCommandEncoder::BeginRendering.
///
/// The usage pattern is very strict to prevent creating invalid render target configs: SetColorTarget must be called starting at uiIndex 0 before you can call it with index 1 and so fourth. To call any of the SetClear* functions, you first must have called the corresponding SetColorTarget or SetDepthStencilTarget functions. All clear methods have reasonable default values (color: black, depth: 1.0, stencil: 0). SetColorTarget and SetDepthStencilTarget can be called multiple times as long as the format of the view remains identical to the previously assigned one. Thus, while not very expensive, it is still a good idea to cache these objects and only swap out the textures when they are not fixed, e.g. when using the WGPUResourcePool.
/// The class produces a WGALRenderPassDescriptor and a WGALFrameBufferDescriptor that can be retrieved via GetRenderPass and GetFrameBuffer respectively.
///
/// Example usage:
/// \code{.cpp}
/// renderTargetSetup.SetColorTarget(0, hRT)).SetClearColor(0, WColor::White)
/// renderTargetSetup.SetDepthStencilTarget(hDepth)).SetClearDepth().SetClearStencil();
/// \endcode
///
/// \sa WGALRenderPassDescriptor, WGALFrameBufferDescriptor, WRenderContext::BeginRendering, WGALCommandEncoder::BeginRendering
struct W_RENDERERFOUNDATION_DLL WGALRenderingSetup : public WHashableStruct<WGALRenderingSetup>
{
public:
  /// \name Setup render targets
  ///@{

  /// Sets the color render target at the given index.
  /// Note that you must set index 0 before you can call this with index 1 and so fourth. You can call the function for an already set index again as long as the format / size matches.
  /// If not set, the load and store operations default to 'Load' and 'Store' respectively.
  WGALRenderingSetup& SetColorTarget(WUInt8 uiIndex, WGALRenderTargetViewHandle hRenderTarget, WEnum<WGALRenderTargetLoadOp> loadOp = {}, WEnum<WGALRenderTargetStoreOp> storeOp = {});

  /// Sets the depth / stencil render target.
  /// If not set, the load and store operations default to 'Load' and 'Store' respectively for both depth and stencil.
  WGALRenderingSetup& SetDepthStencilTarget(WGALRenderTargetViewHandle hDSTarget, WEnum<WGALRenderTargetLoadOp> depthLoadOp = {}, WEnum<WGALRenderTargetStoreOp> depthStoreOp = {}, WEnum<WGALRenderTargetLoadOp> stencilLoadOp = {}, WEnum<WGALRenderTargetStoreOp> stencilStoreOp = {});

  /// Sets the clear color of the given render target and switches the load op to clear.
  /// Note that you first must have called SetColorTarget with the same uiIndex to be able to set the clear color.
  WGALRenderingSetup& SetClearColor(WUInt8 uiIndex, const WColor& color = WColor(0, 0, 0, 0));

  /// Sets the clear depth value.
  /// Note that you first must have called SetDepthStencilTarget to be able to set the clear depth value.
  WGALRenderingSetup& SetClearDepth(float fDepthClear = 1.0f);

  /// Sets the clear depth value.
  /// Note that you first must have called SetDepthStencilTarget to be able to set the clear depth value.
  WGALRenderingSetup& SetClearStencil(WUInt8 uiStencilClear = 0);

  ///@}
  /// \name Accessors
  ///@{

  inline WUInt8 GetColorTargetCount() const;
  /// Returns the clear color of the render target at the given index. Note that uiIndex must be less than GetColorTargetCount().
  inline const WColor& GetClearColor(WUInt8 uiIndex) const;

  inline bool HasDepthStencilTarget() const;
  /// Returns the depth clear value. This call is only valid if HasDepthStencilTarget() returns true.
  inline float GetClearDepth() const;
  /// Returns the stencil clear value. This call is only valid if HasDepthStencilTarget() returns true.
  inline WUInt8 GetClearStencil() const;

  /// Returns the render pass description. Used to create a render pass or feed into the creation of a graphics pipeline state object.
  inline const WGALRenderPassDescriptor& GetRenderPass() const;
  /// Returns the frame buffer description. Used by the renderer to setup render targets.
  inline const WGALFrameBufferDescriptor& GetFrameBuffer() const;

  ///@}

  /// Same as creating a new instance.
  void Reset();


protected:
  WUInt8 m_uiClearStencil = 0;
  float m_fClearDepth = 1.0f;
  WColor m_ClearColor[W_GAL_MAX_RENDERTARGET_COUNT] = {WColor(0, 0, 0, 0)};

  WGALRenderPassDescriptor m_RenderPass;
  WGALFrameBufferDescriptor m_FrameBuffer;
};

#include <RendererFoundation/Resources/Implementation/RenderTargetSetup_inl.h>
