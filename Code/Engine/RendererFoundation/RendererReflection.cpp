#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WGALResourceFormat, 1)
  W_ENUM_CONSTANT(WGALResourceFormat::RGBAFloat),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAUInt),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAInt),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBFloat),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBUInt),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBInt),
    W_ENUM_CONSTANT(WGALResourceFormat::B5G6R5UNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::BGRAUByteNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::BGRAUByteNormalizedsRGB),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAHalf),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAUShort),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAUShortNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAShort),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAShortNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RGFloat),
    W_ENUM_CONSTANT(WGALResourceFormat::RGUInt),
    W_ENUM_CONSTANT(WGALResourceFormat::RGInt),
    W_ENUM_CONSTANT(WGALResourceFormat::RGB10A2UInt),
    W_ENUM_CONSTANT(WGALResourceFormat::RGB10A2UIntNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RG11B10Float),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAUByteNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAUByteNormalizedsRGB),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAUByte),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAByteNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RGBAByte),
    W_ENUM_CONSTANT(WGALResourceFormat::RGHalf),
    W_ENUM_CONSTANT(WGALResourceFormat::RGUShort),
    W_ENUM_CONSTANT(WGALResourceFormat::RGUShortNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RGShort),
    W_ENUM_CONSTANT(WGALResourceFormat::RGShortNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RGUByte),
    W_ENUM_CONSTANT(WGALResourceFormat::RGUByteNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RGByte),
    W_ENUM_CONSTANT(WGALResourceFormat::RGByteNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::DFloat),
    W_ENUM_CONSTANT(WGALResourceFormat::RFloat),
    W_ENUM_CONSTANT(WGALResourceFormat::RUInt),
    W_ENUM_CONSTANT(WGALResourceFormat::RInt),
    W_ENUM_CONSTANT(WGALResourceFormat::RHalf),
    W_ENUM_CONSTANT(WGALResourceFormat::RUShort),
    W_ENUM_CONSTANT(WGALResourceFormat::RUShortNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RShort),
    W_ENUM_CONSTANT(WGALResourceFormat::RShortNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RUByte),
    W_ENUM_CONSTANT(WGALResourceFormat::RUByteNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::RByte),
    W_ENUM_CONSTANT(WGALResourceFormat::RByteNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::AUByteNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::D16),
    W_ENUM_CONSTANT(WGALResourceFormat::D24S8),
    W_ENUM_CONSTANT(WGALResourceFormat::BC1),
    W_ENUM_CONSTANT(WGALResourceFormat::BC1sRGB),
    W_ENUM_CONSTANT(WGALResourceFormat::BC2),
    W_ENUM_CONSTANT(WGALResourceFormat::BC2sRGB),
    W_ENUM_CONSTANT(WGALResourceFormat::BC3),
    W_ENUM_CONSTANT(WGALResourceFormat::BC3sRGB),
    W_ENUM_CONSTANT(WGALResourceFormat::BC4UNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::BC4Normalized),
    W_ENUM_CONSTANT(WGALResourceFormat::BC5UNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::BC5Normalized),
    W_ENUM_CONSTANT(WGALResourceFormat::BC6UFloat),
    W_ENUM_CONSTANT(WGALResourceFormat::BC6Float),
    W_ENUM_CONSTANT(WGALResourceFormat::BC7UNormalized),
    W_ENUM_CONSTANT(WGALResourceFormat::BC7UNormalizedsRGB)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WGALMSAASampleCount, 1)
  W_ENUM_CONSTANTS(WGALMSAASampleCount::None, WGALMSAASampleCount::TwoSamples, WGALMSAASampleCount::FourSamples, WGALMSAASampleCount::EightSamples)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WGALTextureType, 1)
  W_ENUM_CONSTANTS(WGALTextureType::Invalid, WGALTextureType::Texture2D, WGALTextureType::TextureCube, WGALTextureType::Texture3D, WGALTextureType::Texture2DProxy, WGALTextureType::Texture2DShared, WGALTextureType::Texture2DArray, WGALTextureType::TextureCubeArray)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WGALShaderResourceType, 1)
  W_ENUM_CONSTANTS(WGALShaderResourceType::Unknown,
  WGALShaderResourceType::Sampler,
  WGALShaderResourceType::ConstantBuffer,
  WGALShaderResourceType::PushConstants,
  WGALShaderResourceType::Texture,
  WGALShaderResourceType::TextureAndSampler,
  WGALShaderResourceType::TexelBuffer,
  WGALShaderResourceType::StructuredBuffer,
  WGALShaderResourceType::ByteAddressBuffer,
  WGALShaderResourceType::TextureRW)
  W_ENUM_CONSTANTS(WGALShaderResourceType::TexelBufferRW,
  WGALShaderResourceType::StructuredBufferRW,
  WGALShaderResourceType::ByteAddressBufferRW)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WGALShaderTextureType, 1)
  W_ENUM_CONSTANTS(WGALShaderTextureType::Unknown,
  WGALShaderTextureType::Texture1D,
  WGALShaderTextureType::Texture1DArray,
  WGALShaderTextureType::Texture2D,
  WGALShaderTextureType::Texture2DArray,
  WGALShaderTextureType::Texture2DMS,
  WGALShaderTextureType::Texture2DMSArray,
  WGALShaderTextureType::Texture3D,
  WGALShaderTextureType::TextureCube,
  WGALShaderTextureType::TextureCubeArray)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WGALResourceAccess, WNoBase, 1, WRTTIDefaultAllocator<WGALResourceAccess>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Immutable", m_bImmutable),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WGALTextureUsageFlags, 1)
  W_BITFLAGS_CONSTANTS(WGALTextureUsageFlags::ShaderResource, WGALTextureUsageFlags::UnorderedAccess, WGALTextureUsageFlags::RenderTarget, WGALTextureUsageFlags::Presentable)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_TYPE(WGALTextureCreationDescription, WNoBase, 1, WRTTIDefaultAllocator<WGALTextureCreationDescription>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Width", m_uiWidth),
    W_MEMBER_PROPERTY("Height", m_uiHeight),
    W_MEMBER_PROPERTY("Depth", m_uiDepth),
    W_MEMBER_PROPERTY("MipLevelCount", m_uiMipLevelCount),
    W_MEMBER_PROPERTY("ArraySize", m_uiArraySize),
    W_ENUM_MEMBER_PROPERTY("Format", WGALResourceFormat, m_Format),
    W_ENUM_MEMBER_PROPERTY("SampleCount", WGALMSAASampleCount, m_SampleCount),
    W_ENUM_MEMBER_PROPERTY("Type", WGALTextureType, m_Type),
    W_BITFLAGS_MEMBER_PROPERTY("TextureFlags", WGALTextureUsageFlags, m_TextureFlags),
    W_MEMBER_PROPERTY("ResourceAccess", m_ResourceAccess),
    // m_pExisitingNativeObject deliberately not reflected as it can't be serialized in any meaningful way.
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WGALPlatformSharedHandle, WNoBase, 1, WRTTIDefaultAllocator<WGALPlatformSharedHandle>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SharedTexture", m_hSharedTexture),
    W_MEMBER_PROPERTY("Semaphore", m_hSemaphore),
    W_MEMBER_PROPERTY("ProcessId", m_uiProcessId),
    W_MEMBER_PROPERTY("MemoryTypeIndex", m_uiMemoryTypeIndex),
    W_MEMBER_PROPERTY("Size", m_uiSize),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WGALShaderStage, 1)
  W_ENUM_CONSTANTS(WGALShaderStage::VertexShader, WGALShaderStage::HullShader, WGALShaderStage::DomainShader, WGALShaderStage::GeometryShader, WGALShaderStage::PixelShader, WGALShaderStage::ComputeShader)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WGALShaderStageFlags, 1)
  W_BITFLAGS_CONSTANTS(WGALShaderStageFlags::VertexShader, WGALShaderStageFlags::HullShader, WGALShaderStageFlags::DomainShader, WGALShaderStageFlags::GeometryShader, WGALShaderStageFlags::PixelShader, WGALShaderStageFlags::ComputeShader, WGALShaderStageFlags::Auto)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WGALResourceState, 1)
  W_BITFLAGS_CONSTANTS(WGALResourceState::ShaderResource, WGALResourceState::ConstantBuffer, WGALResourceState::VertexBuffer, WGALResourceState::IndexBuffer, WGALResourceState::DrawIndirect, WGALResourceState::DepthStencilRead, WGALResourceState::CopySource, WGALResourceState::ResolveSource)
  W_BITFLAGS_CONSTANTS(WGALResourceState::UnorderedAccess, WGALResourceState::RenderTarget, WGALResourceState::DepthStencilWrite, WGALResourceState::CopyDestination, WGALResourceState::ResolveDestination)
  W_BITFLAGS_CONSTANTS(WGALResourceState::Discard, WGALResourceState::Present, WGALResourceState::CpuRead, WGALResourceState::CpuWrite)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WGALShaderResourceCategory, 1)
  W_BITFLAGS_CONSTANTS(WGALShaderResourceCategory::Sampler, WGALShaderResourceCategory::ConstantBuffer, WGALShaderResourceCategory::TextureSRV, WGALShaderResourceCategory::BufferSRV, WGALShaderResourceCategory::TextureUAV, WGALShaderResourceCategory::BufferUAV)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_ENUM(WGALPresentMode, 1)
  W_ENUM_CONSTANTS(WGALPresentMode::Immediate, WGALPresentMode::VSync)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WGALVertexAttributeSemantic, 1)
  W_ENUM_CONSTANTS(WGALVertexAttributeSemantic::Position, WGALVertexAttributeSemantic::Normal, WGALVertexAttributeSemantic::Tangent,
  WGALVertexAttributeSemantic::Color0, WGALVertexAttributeSemantic::Color1, WGALVertexAttributeSemantic::Color2, WGALVertexAttributeSemantic::Color3,
  WGALVertexAttributeSemantic::Color4, WGALVertexAttributeSemantic::Color5, WGALVertexAttributeSemantic::Color6, WGALVertexAttributeSemantic::Color7)
  W_ENUM_CONSTANTS(WGALVertexAttributeSemantic::TexCoord0, WGALVertexAttributeSemantic::TexCoord1, WGALVertexAttributeSemantic::TexCoord2, WGALVertexAttributeSemantic::TexCoord3,
  WGALVertexAttributeSemantic::TexCoord4, WGALVertexAttributeSemantic::TexCoord5, WGALVertexAttributeSemantic::TexCoord6, WGALVertexAttributeSemantic::TexCoord7,
  WGALVertexAttributeSemantic::TexCoord8, WGALVertexAttributeSemantic::TexCoord9)
  W_ENUM_CONSTANTS(WGALVertexAttributeSemantic::BiTangent,
  WGALVertexAttributeSemantic::BoneIndices0, WGALVertexAttributeSemantic::BoneIndices1,
  WGALVertexAttributeSemantic::BoneWeights0, WGALVertexAttributeSemantic::BoneWeights1,
  WGALVertexAttributeSemantic::DataOffsets)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WGALBufferUsageFlags, 1)
  W_BITFLAGS_CONSTANTS(WGALBufferUsageFlags::VertexBuffer, WGALBufferUsageFlags::IndexBuffer, WGALBufferUsageFlags::ConstantBuffer, WGALBufferUsageFlags::TexelBuffer, WGALBufferUsageFlags::StructuredBuffer, WGALBufferUsageFlags::ByteAddressBuffer)
  W_BITFLAGS_CONSTANTS(WGALBufferUsageFlags::ShaderResource, WGALBufferUsageFlags::UnorderedAccess, WGALBufferUsageFlags::DrawIndirect, WGALBufferUsageFlags::Transient)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_ENUM(WGALQueryType, 1)
  W_ENUM_CONSTANTS(WGALQueryType::NumSamplesPassed, WGALQueryType::AnySamplesPassed)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WGALSharedTextureType, 1)
  W_ENUM_CONSTANTS(WGALSharedTextureType::None, WGALSharedTextureType::Exported, WGALSharedTextureType::Imported)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WGALQueueType, 1)
  W_ENUM_CONSTANTS(WGALQueueType::Graphics, WGALQueueType::Compute, WGALQueueType::Transfer)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

W_STATICLINK_FILE(RendererFoundation, RendererFoundation_RendererReflection);
