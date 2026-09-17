
#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>
#include <Texture/Image/ImageEnums.h>

class WWindowBase;
class WGALDevice;

/// Bind group layout for a single bind group.
/// Auto created by shader resource. Mostly used to quickly determine if a bind group still matches after e.g. switching the shader.
struct WGALBindGroupLayoutCreationDescription
{
  WUInt32 CalculateHash() const;

  WDynamicArray<WShaderResourceBinding> m_ResourceBindings;    ///< Must be sorted by m_iSlot. m_iBindGroup must be the same for all bindings in this array.
  WHybridArray<WShaderResourceBinding, 1> m_ImmutableSamplers; ///< If supported by the platform, contains immutable samplers. See WGALImmutableSamplers.
};

/// Push constant info
/// Used by WGALPipelineLayoutCreationDescription.
struct WGALPushConstant
{
  WUInt16 m_uiSize = 0;
  WUInt16 m_uiOffset = 0;
  WBitflags<WGALShaderStageFlags> m_Stages;
};

/// Pipeline layout.
/// Auto created by shader resource. Mostly used for de-duplication of native resources in case pipelines share the same layout.
struct WGALPipelineLayoutCreationDescription : public WHashableStruct<WGALPipelineLayoutCreationDescription>
{
  WGALBindGroupLayoutHandle m_BindGroups[W_GAL_MAX_BIND_GROUPS]; ///< One for each bind group used in the shader. BG_FRAME, BG_RENDER_PASS, BG_MATERIAL, BG_DRAW_CALL.
  WGALPushConstant m_PushConstants;                               ///< Only one push constant block is supported right now.
};

/// Defines the complete state of a graphics pipeline, excluding bound resources (e.g. textures, buffers) and dynamic states (e.g. viewport).
/// All handles must be set except for m_hVertexDeclaration which is optional. Creating a graphics pipeline increases the reference count on all valid handles.
struct WGALGraphicsPipelineCreationDescription : public WHashableStruct<WGALGraphicsPipelineCreationDescription>
{
  WGALShaderHandle m_hShader;                       ///< Also defines pipeline layout
  WGALVertexDeclarationHandle m_hVertexDeclaration; ///< Optional

  WGALRasterizerStateHandle m_hRasterizerState;
  WGALBlendStateHandle m_hBlendState;
  WGALDepthStencilStateHandle m_hDepthStencilState;

  WEnum<WGALPrimitiveTopology> m_Topology;

  WGALRenderPassDescriptor m_RenderPass; ///< Use WGALRenderingSetup::GetRenderPass to set this.
};

/// Defines the complete state of a compute pipeline, excluding bound resources (e.g. textures, buffers).
/// Creating a compute pipeline increases the reference count on the shader handle.
struct WGALComputePipelineCreationDescription : public WHashableStruct<WGALComputePipelineCreationDescription>
{
  WGALShaderHandle m_hShader; ///< Also defines pipeline layout
};

struct WGALWindowSwapChainCreationDescription : public WHashableStruct<WGALWindowSwapChainCreationDescription>
{
  WWindowBase* m_pWindow = nullptr;

  // Describes the format that should be used for the backbuffer.
  // Note however, that different platforms may enforce restrictions on this.
  WGALMSAASampleCount::Enum m_SampleCount = WGALMSAASampleCount::None;
  WGALResourceFormat::Enum m_BackBufferFormat = WGALResourceFormat::RGBAUByteNormalizedsRGB;
  WEnum<WGALPresentMode> m_InitialPresentMode = WGALPresentMode::VSync;

  bool m_bDoubleBuffered = true;
};

struct WGALSwapChainCreationDescription : public WHashableStruct<WGALSwapChainCreationDescription>
{
  const WRTTI* m_pSwapChainType = nullptr;
};

struct WGALDeviceCreationDescription
{
  bool m_bDebugDevice = false;
};

struct WGALShaderCreationDescription : public WHashableStruct<WGALShaderCreationDescription>
{
  WGALShaderCreationDescription();
  /// Needs to be overwritten as the base class impl can only handle pod types.
  WGALShaderCreationDescription(const WGALShaderCreationDescription& other);
  ~WGALShaderCreationDescription();
  /// Needs to be overwritten as the base class impl can only handle pod types.
  void operator=(const WGALShaderCreationDescription& other);

  bool HasByteCodeForStage(WGALShaderStage::Enum stage) const;

  WSharedPtr<WGALShaderByteCode> m_ByteCodes[WGALShaderStage::ENUM_COUNT];
};

struct WGALRenderTargetBlendDescription : public WHashableStruct<WGALRenderTargetBlendDescription>
{
  WEnum<WGALBlend> m_SourceBlend = WGALBlend::One;
  WEnum<WGALBlend> m_DestBlend = WGALBlend::One;
  WEnum<WGALBlendOp> m_BlendOp = WGALBlendOp::Add;

  WEnum<WGALBlend> m_SourceBlendAlpha = WGALBlend::One;
  WEnum<WGALBlend> m_DestBlendAlpha = WGALBlend::One;
  WEnum<WGALBlendOp> m_BlendOpAlpha = WGALBlendOp::Add;

  WUInt8 m_uiWriteMask = 0xFF;    ///< Enables writes to color channels. Bit1 = Red Channel, Bit2 = Green Channel, Bit3 = Blue Channel, Bit4 = Alpha
                                   ///< Channel, Bit 5-8 are unused
  bool m_bBlendingEnabled = false; ///< If enabled, the color will be blended into the render target. Otherwise it will overwrite the render target.
                                   ///< Set m_uiWriteMask to 0 to disable all writes to the render target.
};

struct WGALBlendStateCreationDescription : public WHashableStruct<WGALBlendStateCreationDescription>
{
  WGALRenderTargetBlendDescription m_RenderTargetBlendDescriptions[W_GAL_MAX_RENDERTARGET_COUNT];

  bool m_bAlphaToCoverage = false;  ///< Alpha-to-coverage can only be used with MSAA render targets. Default is false.
  bool m_bIndependentBlend = false; ///< If disabled, the blend state of the first render target is used for all render targets. Otherwise each
                                    ///< render target uses a different blend state.
};

struct WGALStencilOpDescription : public WHashableStruct<WGALStencilOpDescription>
{
  WEnum<WGALStencilOp> m_FailOp = WGALStencilOp::Keep;
  WEnum<WGALStencilOp> m_DepthFailOp = WGALStencilOp::Keep;
  WEnum<WGALStencilOp> m_PassOp = WGALStencilOp::Keep;

  WEnum<WGALCompareFunc> m_StencilFunc = WGALCompareFunc::Always;
};

struct WGALDepthStencilStateCreationDescription : public WHashableStruct<WGALDepthStencilStateCreationDescription>
{
  WGALStencilOpDescription m_FrontFaceStencilOp;
  WGALStencilOpDescription m_BackFaceStencilOp;

  WEnum<WGALCompareFunc> m_DepthTestFunc = WGALCompareFunc::Less;

  bool m_bDepthEnable = true;
  bool m_bDepthWrite = true;
  bool m_bStencilEnable = false;
  WUInt8 m_uiStencilReadMask = 0xFF;
  WUInt8 m_uiStencilWriteMask = 0xFF;
};

/// Describes the settings for a new rasterizer state. See WGALDevice::CreateRasterizerState
struct WGALRasterizerStateCreationDescription : public WHashableStruct<WGALRasterizerStateCreationDescription>
{
  WEnum<WGALCullMode> m_CullMode = WGALCullMode::Back; ///< Which sides of a triangle to cull. Default is WGALCullMode::Back
  WInt32 m_iDepthBias = 0;                               ///< The pixel depth bias. Default is 0
  float m_fDepthBiasClamp = 0.0f;                         ///< The pixel depth bias clamp. Default is 0
  float m_fSlopeScaledDepthBias = 0.0f;                   ///< The pixel slope scaled depth bias clamp. Default is 0
  bool m_bWireFrame = false;                              ///< Whether triangles are rendered filled or as wireframe. Default is false
  bool m_bFrontCounterClockwise = false;                  ///< Sets which triangle winding order defines the 'front' of a triangle. If true, the front of a triangle
                                                          ///< is the one where the vertices appear in counter clockwise order. Default is false
  bool m_bScissorTest = false;
  bool m_bConservativeRasterization = false;              ///< Whether conservative rasterization is enabled
};

struct WGALSamplerStateCreationDescription : public WHashableStruct<WGALSamplerStateCreationDescription>
{
  WEnum<WGALTextureFilterMode> m_MinFilter;
  WEnum<WGALTextureFilterMode> m_MagFilter;
  WEnum<WGALTextureFilterMode> m_MipFilter;

  WEnum<WImageAddressMode> m_AddressU;
  WEnum<WImageAddressMode> m_AddressV;
  WEnum<WImageAddressMode> m_AddressW;

  WEnum<WGALCompareFunc> m_SampleCompareFunc;

  WColor m_BorderColor = WColor::Black;

  float m_fMipLodBias = 0.0f;
  float m_fMinMip = -1.0f;
  float m_fMaxMip = 42000.0f;

  WUInt8 m_uiMaxAnisotropy = 4;

  /// Quality slot that controls filter and anisotropy for this sampler. When set, these are overridden
  /// by the current quality setting for that slot, and the sampler is automatically recreated when quality changes.
  /// Set to WGALTextureQualitySlot::None (default) to use fixed filter settings from m_MinFilter/m_MagFilter/m_MipFilter/m_uiMaxAnisotropy.
  /// \see WRenderContext::SetDefaultTextureQuality, WTextureFilterSetting
  WEnum<WGALTextureQualitySlot> m_useTextureQualitySlot = WGALTextureQualitySlot::None;
};

struct W_RENDERERFOUNDATION_DLL WGALVertexBinding
{
  WUInt32 m_uiStride = 0;
  WEnum<WGALVertexBindingRate> m_Rate;
};

struct W_RENDERERFOUNDATION_DLL WGALVertexAttribute
{
  W_DECLARE_POD_TYPE();

  WGALVertexAttribute() = default;

  constexpr WGALVertexAttribute(WGALVertexAttributeSemantic::Enum semantic, WGALResourceFormat::Enum format, WUInt8 uiOffset, WUInt8 uiVertexBufferSlot);

  WGALVertexAttributeSemantic::Enum m_eSemantic = WGALVertexAttributeSemantic::Position;
  WGALResourceFormat::Enum m_eFormat = WGALResourceFormat::XYZFloat;
  WUInt8 m_uiOffset = 0;
  WUInt8 m_uiVertexBufferSlot = 0;
};

struct W_RENDERERFOUNDATION_DLL WGALVertexDeclarationCreationDescription : public WHashableStruct<WGALVertexDeclarationCreationDescription>
{
  WGALShaderHandle m_hShader; // Needed for attribute indices
  WStaticArray<WGALVertexAttribute, W_GAL_MAX_VERTEX_ATTRIBUTE_COUNT> m_VertexAttributes;
  WStaticArray<WGALVertexBinding, W_GAL_MAX_VERTEX_BUFFER_COUNT> m_VertexBindings;
};

struct WGALResourceAccess
{
  W_ALWAYS_INLINE bool IsImmutable() const { return m_bImmutable; }

  bool m_bImmutable = false;
};

struct W_RENDERERFOUNDATION_DLL WGALBufferCreationDescription : public WHashableStruct<WGALBufferCreationDescription>
{
  /// Returns the most appropriate default resource state based on the buffer's usage flags.
  WBitflags<WGALResourceState> GetDefaultState() const;

  WUInt32 m_uiTotalSize = 0;           ///< Total size in bytes. Must always be set > 0.
  WUInt32 m_uiStructSize = 0;          ///< Struct, Index or Vertex size in bytes. Only valid if StructuredBuffer, VertexBuffer or IndexBuffer flag is set.
  WBitflags<WGALBufferUsageFlags> m_BufferFlags;
  WGALResourceAccess m_ResourceAccess;
  WEnum<WGALResourceFormat> m_Format; ///< Only relevant for TexelBuffer to create the default view.
};

struct W_RENDERERFOUNDATION_DLL WGALTextureCreationDescription : public WHashableStruct<WGALTextureCreationDescription>
{
  void SetAsRenderTarget(WUInt32 uiWidth, WUInt32 uiHeight, WGALResourceFormat::Enum format, WGALMSAASampleCount::Enum sampleCount = WGALMSAASampleCount::None);
  void SetAsRenderTarget(WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiArraySize, WGALResourceFormat::Enum format, WGALMSAASampleCount::Enum sampleCount = WGALMSAASampleCount::None);
  WResult Validate(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> initialData = {}) const;
  WUInt32 GetNumberOfSlices() const;
  WVec3U32 GetMipMapSize(WUInt32 uiMipLevel) const;

  /// Returns the most appropriate default resource state based on the texture's allowed views and format.
  WBitflags<WGALResourceState> GetDefaultState() const;

  WUInt32 m_uiWidth = 0;
  WUInt32 m_uiHeight = 0;
  WUInt32 m_uiDepth = 1;
  WUInt32 m_uiArraySize = 1; ///< In case of cube maps, the number of cubes instead of faces.
  WUInt8 m_uiMipLevelCount = 1;

  WEnum<WGALResourceFormat> m_Format = WGALResourceFormat::Invalid;
  WEnum<WGALMSAASampleCount> m_SampleCount = WGALMSAASampleCount::None;
  WEnum<WGALTextureType> m_Type = WGALTextureType::Texture2D;

  WBitflags<WGALTextureUsageFlags> m_TextureFlags = WGALTextureUsageFlags::ShaderResource;

  WGALResourceAccess m_ResourceAccess;

  void* m_pExisitingNativeObject = nullptr; ///< Can be used to encapsulate existing native textures in objects usable by the GAL
};

struct WGALRenderTargetViewCreationDescription : public WHashableStruct<WGALRenderTargetViewCreationDescription>
{
  WGALTextureHandle m_hTexture;

  WUInt32 m_uiMipLevel = 0;
  WUInt32 m_uiFirstSlice = 0;
  WUInt32 m_uiSliceCount = 1;
  WEnum<WGALResourceFormat> m_OverrideViewFormat = WGALResourceFormat::Invalid;
  WEnum<WGALTextureType> m_OverrideViewType = WGALTextureType::Invalid;

  bool m_bReadOnly = false; ///< Can be used for depth stencil views to create read only views (e.g. for soft particles using the native depth buffer)
};

/// Type for important GAL events.
struct WGALDeviceEvent
{
  enum Type
  {
    BeforeInit,
    AfterInit,
    BeforeShutdown,
    AfterShutdown,
    BeforeBeginFrame,
    AfterBeginFrame,
    BeforeEndFrame,
    AfterEndFrame,
    BeforeBeginCommands,
    AfterBeginCommands,
    BeforeEndCommands,
    AfterEndCommands,
    // could add resource creation/destruction events, if this would be useful
  };

  Type m_Type;
  class WGALDevice* m_pDevice = nullptr;
  WGALCommandEncoder* m_pCommandEncoder = nullptr;
};

// Opaque platform specific handle
// Typically holds a platform specific handle for the texture and it's synchronization primitive
struct WGALPlatformSharedHandle : public WHashableStruct<WGALPlatformSharedHandle>
{
  WUInt64 m_hSharedTexture = 0;
  WUInt64 m_hSemaphore = 0;
  WUInt32 m_uiProcessId = 0;
  WUInt32 m_uiMemoryTypeIndex = 0;
  WUInt64 m_uiSize = 0;
};

#include <RendererFoundation/Descriptors/Implementation/Descriptors_inl.h>
