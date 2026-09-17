
inline WBitflags<WGALShaderResourceCategory> WGALShaderResourceCategory::MakeFromShaderDescriptorType(WGALShaderResourceType::Enum type)
{
  switch (type)
  {
    case WGALShaderResourceType::Sampler:
      return WGALShaderResourceCategory::Sampler;
    case WGALShaderResourceType::ConstantBuffer:
    case WGALShaderResourceType::PushConstants:
      return WGALShaderResourceCategory::ConstantBuffer;
    case WGALShaderResourceType::Texture:
      return WGALShaderResourceCategory::TextureSRV;
    case WGALShaderResourceType::TexelBuffer:
    case WGALShaderResourceType::StructuredBuffer:
    case WGALShaderResourceType::ByteAddressBuffer:
      return WGALShaderResourceCategory::BufferSRV;
    case WGALShaderResourceType::TextureRW:
      return WGALShaderResourceCategory::TextureUAV;
    case WGALShaderResourceType::TexelBufferRW:
    case WGALShaderResourceType::StructuredBufferRW:
    case WGALShaderResourceType::ByteAddressBufferRW:
      return WGALShaderResourceCategory::BufferUAV;
    case WGALShaderResourceType::TextureAndSampler:
      return WGALShaderResourceCategory::TextureSRV | WGALShaderResourceCategory::Sampler;
    default:
      W_REPORT_FAILURE("Missing enum");
      return {};
  }
}

inline bool WGALShaderTextureType::IsArray(WGALShaderTextureType::Enum format)
{
  switch (format)
  {
    case WGALShaderTextureType::Texture1DArray:
    case WGALShaderTextureType::Texture2DArray:
    case WGALShaderTextureType::Texture2DMSArray:
    case WGALShaderTextureType::TextureCubeArray:
      return true;
    default:
      return false;
  }
}

inline bool WGALShaderTextureType::IsMSAA(WGALShaderTextureType::Enum format)
{
  switch (format)
  {
    case WGALShaderTextureType::Texture2DMS:
    case WGALShaderTextureType::Texture2DMSArray:
      return true;
    default:
      return false;
  }
}

inline WGALTextureType::Enum WGALShaderTextureType::GetTextureType(WGALShaderTextureType::Enum format)
{
  switch (format)
  {
    case WGALShaderTextureType::Texture2D:
    case WGALShaderTextureType::Texture2DMS:
      return WGALTextureType::Texture2D;
    case WGALShaderTextureType::Texture2DArray:
    case WGALShaderTextureType::Texture2DMSArray:
      return WGALTextureType::Texture2DArray;
    case WGALShaderTextureType::Texture3D:
      return WGALTextureType::Texture3D;
    case WGALShaderTextureType::TextureCube:
      return WGALTextureType::TextureCube;
    case WGALShaderTextureType::TextureCubeArray:
      return WGALTextureType::TextureCubeArray;
    case WGALShaderTextureType::Unknown:
    case WGALShaderTextureType::Texture1D:
    case WGALShaderTextureType::Texture1DArray:
    default:
      W_REPORT_FAILURE("Unknown shader texture type");
      return WGALTextureType::Invalid;
  }
}
