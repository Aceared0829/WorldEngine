#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Types/Bitflags.h>

/// Defines which operations can be performed on an WGALResourceFormat
/// \sa WGALDeviceCapabilities::m_FormatSupport
struct WGALResourceFormatSupport
{
  using StorageType = WUInt8;

  enum Enum
  {
    None = 0,
    Texture = W_BIT(0),         ///< Can be used as a texture and bound to a WGALShaderResourceType::Texture slot.
    RenderTarget = W_BIT(1),    ///< Can be used as a texture and bound as a render target.
    TextureRW = W_BIT(2),       ///< Can be used as a texture and bound to a WGALShaderResourceType::TextureRW slot.
    MSAA2x = W_BIT(3),          ///< The format supports 2x MSAA
    MSAA4x = W_BIT(4),          ///< The format supports 4x MSAA
    MSAA8x = W_BIT(5),          ///< The format supports 8x MSAA
    VertexAttribute = W_BIT(6), ///< The format can be used as a vertex attribute.
    Default = 0
  };

  struct Bits
  {
    StorageType Texture : 1;
    StorageType RenderTarget : 1;
    StorageType TextureRW : 1;
    StorageType MSAA2x : 1;
    StorageType MSAA4x : 1;
    StorageType MSAA8x : 1;
    StorageType VertexAttribute : 1;
  };
};
W_DECLARE_FLAGS_OPERATORS(WGALResourceFormatSupport);

struct WGALBufferLayout
{
  using StorageType = WUInt8;
  enum Enum
  {
    Vulkan_Std140_relaxed, // Vulkan uniform buffer
    Vulkan_Std430_relaxed, // Vulkan structured buffer
    DirectX_ConstantButter,
    DirectX_StructuredButter,
    Default = DirectX_ConstantButter
  };
};

/// This struct holds information about the rendering device capabilities (e.g. what shader stages are supported and more)
/// To get the device capabilities you need to call the GetCapabilities() function on an WGALDevice object.
struct W_RENDERERFOUNDATION_DLL WGALDeviceCapabilities
{
  // Device description
  WString m_sAdapterName = "Unknown";
  WUInt64 m_uiDedicatedVRAM = 0;
  WUInt64 m_uiDedicatedSystemRAM = 0;
  WUInt64 m_uiSharedSystemRAM = 0;
  bool m_bHardwareAccelerated = false;

  // General capabilities
  bool m_bSupportsMultithreadedResourceCreation = false; ///< whether creating resources is allowed on other threads than the main thread
  bool m_bSupportsMultipleBindGroups = false;
  WEnum<WGALBufferLayout> m_materialBufferLayout;

  // Draw related capabilities
  bool m_bShaderStageSupported[WGALShaderStage::ENUM_COUNT] = {};
  bool m_bSupportsIndirectDraw = false;
  bool m_bSupportsConservativeRasterization = false;
  bool m_bSupportsDepthBiasClamp = false;  ///< Whether WGALRasterizerStateCreationDescription::m_fDepthBiasClamp is honored. If false, the value is silently treated as 0.
  bool m_bSupportsWireframe = false;
  bool m_bSupportsVSRenderTargetArrayIndex = false;
  bool m_bSupportsTexelBuffer = false;     ///< Whether WGALBufferUsageFlags::TexelBuffer is supported. Hardcoded per platform as it must match SUPPORTS_TEXEL_BUFFER shader define.
  bool m_bSupportsMultipleSRVTypes = true; ///< Whether more than one of WGALBufferUsageFlags::StructuredBuffer, WGALBufferUsageFlags::TexelBuffer and WGALBufferUsageFlags::ByteAddressBuffer is supported on a buffer.
  bool m_bSupportsMultiSampledArrays = false;
  WUInt16 m_uiMaxPushConstantsSize = 0;


  // Texture related capabilities
  bool m_bSupportsSharedTextures = false;
  WDynamicArray<WBitflags<WGALResourceFormatSupport>> m_FormatSupport;
};
