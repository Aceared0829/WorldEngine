#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>

bool WTextureUtils::s_bForceFullQualityAlways = false;

namespace
{
  WUInt32 GetMipSize(WUInt32 uiSize, WUInt32 uiMipLevel)
  {
    for (WUInt32 i = 0; i < uiMipLevel; i++)
    {
      uiSize = uiSize / 2;
    }
    return WMath::Max(1u, uiSize);
  }
} // namespace

WGALResourceFormat::Enum WTextureUtils::ImageFormatToGalFormat(WImageFormat::Enum format, bool bSRGB)
{
  switch (format)
  {
    case WImageFormat::R8G8B8A8_UNORM:
      if (bSRGB)
        return WGALResourceFormat::RGBAUByteNormalizedsRGB;
      else
        return WGALResourceFormat::RGBAUByteNormalized;

      // case WImageFormat::R8G8B8A8_TYPELESS:
    case WImageFormat::R8G8B8A8_UNORM_SRGB:
      return WGALResourceFormat::RGBAUByteNormalizedsRGB;

    case WImageFormat::R8G8B8A8_UINT:
      return WGALResourceFormat::RGBAUInt;

    case WImageFormat::R8G8B8A8_SNORM:
      return WGALResourceFormat::RGBAByteNormalized;

    case WImageFormat::R8G8B8A8_SINT:
      return WGALResourceFormat::RGBAInt;

    case WImageFormat::B8G8R8A8_UNORM:
      if (bSRGB)
        return WGALResourceFormat::BGRAUByteNormalizedsRGB;
      else
        return WGALResourceFormat::BGRAUByteNormalized;

    case WImageFormat::B8G8R8X8_UNORM:
      if (bSRGB)
        return WGALResourceFormat::BGRAUByteNormalizedsRGB;
      else
        return WGALResourceFormat::BGRAUByteNormalized;

      // case WImageFormat::B8G8R8A8_TYPELESS:
    case WImageFormat::B8G8R8A8_UNORM_SRGB:
      return WGALResourceFormat::BGRAUByteNormalizedsRGB;

      // case WImageFormat::B8G8R8X8_TYPELESS:
    case WImageFormat::B8G8R8X8_UNORM_SRGB:
      return WGALResourceFormat::BGRAUByteNormalizedsRGB;

      // case WImageFormat::B8G8R8_UNORM:

      // case WImageFormat::BC1_TYPELESS:
    case WImageFormat::BC1_UNORM:
      if (bSRGB)
        return WGALResourceFormat::BC1sRGB;
      else
        return WGALResourceFormat::BC1;

    case WImageFormat::BC1_UNORM_SRGB:
      return WGALResourceFormat::BC1sRGB;

      // case WImageFormat::BC2_TYPELESS:
    case WImageFormat::BC2_UNORM:
      if (bSRGB)
        return WGALResourceFormat::BC2sRGB;
      else
        return WGALResourceFormat::BC2;

    case WImageFormat::BC2_UNORM_SRGB:
      return WGALResourceFormat::BC2sRGB;

      // case WImageFormat::BC3_TYPELESS:
    case WImageFormat::BC3_UNORM:
      if (bSRGB)
        return WGALResourceFormat::BC3sRGB;
      else
        return WGALResourceFormat::BC3;

    case WImageFormat::BC3_UNORM_SRGB:
      return WGALResourceFormat::BC3sRGB;

      // case WImageFormat::BC4_TYPELESS:
    case WImageFormat::BC4_UNORM:
      return WGALResourceFormat::BC4UNormalized;

    case WImageFormat::BC4_SNORM:
      return WGALResourceFormat::BC4Normalized;

      // case WImageFormat::BC5_TYPELESS:
    case WImageFormat::BC5_UNORM:
      return WGALResourceFormat::BC5UNormalized;

    case WImageFormat::BC5_SNORM:
      return WGALResourceFormat::BC5Normalized;

      // case WImageFormat::BC6H_TYPELESS:
    case WImageFormat::BC6H_UF16:
      return WGALResourceFormat::BC6UFloat;

    case WImageFormat::BC6H_SF16:
      return WGALResourceFormat::BC6Float;

      // case WImageFormat::BC7_TYPELESS:
    case WImageFormat::BC7_UNORM:
      if (bSRGB)
        return WGALResourceFormat::BC7UNormalizedsRGB;
      else
        return WGALResourceFormat::BC7UNormalized;

    case WImageFormat::BC7_UNORM_SRGB:
      return WGALResourceFormat::BC7UNormalizedsRGB;

    case WImageFormat::B5G6R5_UNORM:
      return WGALResourceFormat::B5G6R5UNormalized; /// \todo Not supported by some GPUs ?

    case WImageFormat::R16_FLOAT:
      return WGALResourceFormat::RHalf;

    case WImageFormat::R32_FLOAT:
      return WGALResourceFormat::RFloat;

    case WImageFormat::R16G16_FLOAT:
      return WGALResourceFormat::RGHalf;

    case WImageFormat::R32G32_FLOAT:
      return WGALResourceFormat::RGFloat;

    case WImageFormat::R32G32B32_FLOAT:
      return WGALResourceFormat::RGBFloat;

    case WImageFormat::R16G16B16A16_FLOAT:
      return WGALResourceFormat::RGBAHalf;

    case WImageFormat::R32G32B32A32_FLOAT:
      return WGALResourceFormat::RGBAFloat;

    case WImageFormat::R16G16B16A16_UNORM:
      return WGALResourceFormat::RGBAUShortNormalized;

    case WImageFormat::R8_UNORM:
      return WGALResourceFormat::RUByteNormalized;

    case WImageFormat::R8G8_UNORM:
      return WGALResourceFormat::RGUByteNormalized;

    case WImageFormat::R16G16_UNORM:
      return WGALResourceFormat::RGUShortNormalized;

    case WImageFormat::R11G11B10_FLOAT:
      return WGALResourceFormat::RG11B10Float;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return WGALResourceFormat::Invalid;
}

WImageFormat::Enum WTextureUtils::GalFormatToImageFormat(WGALResourceFormat::Enum format)
{
  switch (format)
  {
    case WGALResourceFormat::RGBAFloat:
      return WImageFormat::R32G32B32A32_FLOAT;
    case WGALResourceFormat::RGBAUInt:
      return WImageFormat::R32G32B32A32_UINT;
    case WGALResourceFormat::RGBAInt:
      return WImageFormat::R32G32B32A32_SINT;
    case WGALResourceFormat::RGBFloat:
      return WImageFormat::R32G32B32_FLOAT;
    case WGALResourceFormat::RGBUInt:
      return WImageFormat::R32G32B32_UINT;
    case WGALResourceFormat::RGBInt:
      return WImageFormat::R32G32B32_SINT;
    case WGALResourceFormat::B5G6R5UNormalized:
      return WImageFormat::B5G6R5_UNORM;
    case WGALResourceFormat::BGRAUByteNormalized:
      return WImageFormat::B8G8R8A8_UNORM;
    case WGALResourceFormat::BGRAUByteNormalizedsRGB:
      return WImageFormat::B8G8R8A8_UNORM_SRGB;
    case WGALResourceFormat::RGBAHalf:
      return WImageFormat::R16G16B16A16_FLOAT;
    case WGALResourceFormat::RGBAUShort:
      return WImageFormat::R16G16B16A16_UINT;
    case WGALResourceFormat::RGBAUShortNormalized:
      return WImageFormat::R16G16B16A16_UNORM;
    case WGALResourceFormat::RGBAShort:
      return WImageFormat::R16G16B16A16_SINT;
    case WGALResourceFormat::RGBAShortNormalized:
      return WImageFormat::R16G16B16A16_SNORM;
    case WGALResourceFormat::RGFloat:
      return WImageFormat::R32G32_FLOAT;
    case WGALResourceFormat::RGUInt:
      return WImageFormat::R32G32_UINT;
    case WGALResourceFormat::RGInt:
      return WImageFormat::R32G32_SINT;
    case WGALResourceFormat::RG11B10Float:
      return WImageFormat::R11G11B10_FLOAT;
    case WGALResourceFormat::RGBAUByteNormalized:
      return WImageFormat::R8G8B8A8_UNORM;
    case WGALResourceFormat::RGBAUByteNormalizedsRGB:
      return WImageFormat::R8G8B8A8_UNORM_SRGB;
    case WGALResourceFormat::RGBAUByte:
      return WImageFormat::R8G8B8A8_UINT;
    case WGALResourceFormat::RGBAByteNormalized:
      return WImageFormat::R8G8B8A8_SNORM;
    case WGALResourceFormat::RGBAByte:
      return WImageFormat::R8G8B8A8_SINT;
    case WGALResourceFormat::RGHalf:
      return WImageFormat::R16G16_FLOAT;
    case WGALResourceFormat::RGUShort:
      return WImageFormat::R16G16_UINT;
    case WGALResourceFormat::RGUShortNormalized:
      return WImageFormat::R16G16_UNORM;
    case WGALResourceFormat::RGShort:
      return WImageFormat::R16G16_SINT;
    case WGALResourceFormat::RGShortNormalized:
      return WImageFormat::R16G16_SNORM;
    case WGALResourceFormat::RGUByte:
      return WImageFormat::R8G8_UINT;
    case WGALResourceFormat::RGUByteNormalized:
      return WImageFormat::R8G8_UNORM;
    case WGALResourceFormat::RGByte:
      return WImageFormat::R8G8_SINT;
    case WGALResourceFormat::RGByteNormalized:
      return WImageFormat::R8G8_SNORM;
    case WGALResourceFormat::DFloat:
      return WImageFormat::R32_FLOAT;
    case WGALResourceFormat::RFloat:
      return WImageFormat::R32_FLOAT;
    case WGALResourceFormat::RUInt:
      return WImageFormat::R32_UINT;
    case WGALResourceFormat::RInt:
      return WImageFormat::R32_SINT;
    case WGALResourceFormat::RHalf:
      return WImageFormat::R16_FLOAT;
    case WGALResourceFormat::RUShort:
      return WImageFormat::R16_UINT;
    case WGALResourceFormat::RUShortNormalized:
      return WImageFormat::R16_UNORM;
    case WGALResourceFormat::RShort:
      return WImageFormat::R16_SINT;
    case WGALResourceFormat::RShortNormalized:
      return WImageFormat::R16_SNORM;
    case WGALResourceFormat::RUByte:
      return WImageFormat::R8_UINT;
    case WGALResourceFormat::RUByteNormalized:
      return WImageFormat::R8_UNORM;
    case WGALResourceFormat::RByte:
      return WImageFormat::R8_SINT;
    case WGALResourceFormat::RByteNormalized:
      return WImageFormat::R8_SNORM;
    case WGALResourceFormat::AUByteNormalized:
      return WImageFormat::R8_UNORM;
    case WGALResourceFormat::D16:
      return WImageFormat::R16_UINT;
    case WGALResourceFormat::BC1:
      return WImageFormat::BC1_UNORM;
    case WGALResourceFormat::BC1sRGB:
      return WImageFormat::BC1_UNORM_SRGB;
    case WGALResourceFormat::BC2:
      return WImageFormat::BC2_UNORM;
    case WGALResourceFormat::BC2sRGB:
      return WImageFormat::BC2_UNORM_SRGB;
    case WGALResourceFormat::BC3:
      return WImageFormat::BC3_UNORM;
    case WGALResourceFormat::BC3sRGB:
      return WImageFormat::BC3_UNORM_SRGB;
    case WGALResourceFormat::BC4UNormalized:
      return WImageFormat::BC4_UNORM;
    case WGALResourceFormat::BC4Normalized:
      return WImageFormat::BC4_SNORM;
    case WGALResourceFormat::BC5UNormalized:
      return WImageFormat::BC5_UNORM;
    case WGALResourceFormat::BC5Normalized:
      return WImageFormat::BC5_SNORM;
    case WGALResourceFormat::BC6UFloat:
      return WImageFormat::BC6H_UF16;
    case WGALResourceFormat::BC6Float:
      return WImageFormat::BC6H_SF16;
    case WGALResourceFormat::BC7UNormalized:
      return WImageFormat::BC7_UNORM;
    case WGALResourceFormat::BC7UNormalizedsRGB:
      return WImageFormat::BC7_UNORM_SRGB;
    case WGALResourceFormat::RGB10A2UInt:
    case WGALResourceFormat::RGB10A2UIntNormalized:
    case WGALResourceFormat::D24S8:
    default:
    {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
      WStringBuilder sFormat;
      W_ASSERT_DEBUG(WReflectionUtils::EnumerationToString(WGetStaticRTTI<WGALResourceFormat>(), format, sFormat, WReflectionUtils::EnumConversionMode::ValueNameOnly), "Cannot convert GAL format '{}' to string", format);
      W_ASSERT_DEBUG(false, "The GL format: '{}' does not have a matching image format.", sFormat);
#endif
    }
  }
  return WImageFormat::UNKNOWN;
}

WImageFormat::Enum WTextureUtils::GalFormatToImageFormat(WGALResourceFormat::Enum format, bool bRemoveSRGB)
{
  WImageFormat::Enum imageFormat = GalFormatToImageFormat(format);
  if (bRemoveSRGB)
  {
    imageFormat = WImageFormat::AsLinear(imageFormat);
  }
  return imageFormat;
}

void WTextureUtils::ConfigureSampler(WTextureFilterSetting::Enum filter, WGALSamplerStateCreationDescription& out_sampler)
{
  out_sampler.m_MinFilter = WGALTextureFilterMode::Linear;
  out_sampler.m_MagFilter = WGALTextureFilterMode::Linear;
  out_sampler.m_MipFilter = WGALTextureFilterMode::Linear;
  out_sampler.m_uiMaxAnisotropy = 1;

  if (filter >= WTextureFilterSetting::LowestQuality)
  {
    out_sampler.m_useTextureQualitySlot = static_cast<WGALTextureQualitySlot::Enum>(filter - WTextureFilterSetting::LowestQuality);
  }
  else
  {
    out_sampler.m_useTextureQualitySlot = WGALTextureQualitySlot::None;
  }

  switch (filter)
  {
    case WTextureFilterSetting::FixedNearest:
      out_sampler.m_MinFilter = WGALTextureFilterMode::Point;
      out_sampler.m_MagFilter = WGALTextureFilterMode::Point;
      out_sampler.m_MipFilter = WGALTextureFilterMode::Point;
      break;

    case WTextureFilterSetting::FixedBilinear:
      out_sampler.m_MipFilter = WGALTextureFilterMode::Point;
      break;

    case WTextureFilterSetting::FixedTrilinear:
      break;

    case WTextureFilterSetting::FixedAnisotropic2x:
      out_sampler.m_MinFilter = WGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = WGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 2;
      break;

    case WTextureFilterSetting::FixedAnisotropic4x:
      out_sampler.m_MinFilter = WGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = WGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 4;
      break;

    case WTextureFilterSetting::FixedAnisotropic8x:
      out_sampler.m_MinFilter = WGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = WGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 8;
      break;

    case WTextureFilterSetting::FixedAnisotropic16x:
      out_sampler.m_MinFilter = WGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = WGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 16;
      break;

    default:
      break;
  }
}

void WTextureUtils::CopySubResourceToImage(const WGALTextureCreationDescription& desc, const WGALTextureSubresource& subResource, const WGALSystemMemoryDescription& memory, WImage& out_image, bool bRemoveSRGB)
{
  WImageHeader headerTemp;
  headerTemp.SetImageFormat(WTextureUtils::GalFormatToImageFormat(desc.m_Format, bRemoveSRGB));
  headerTemp.SetWidth(desc.m_uiWidth);
  headerTemp.SetHeight(desc.m_uiHeight);
  // GetWidth/Height assert that the requested mip level is within range, so the helper header must know the texture's full mip chain.
  headerTemp.SetNumMipLevels(desc.m_uiMipLevelCount);

  WImageHeader header;
  header.SetImageFormat(WTextureUtils::GalFormatToImageFormat(desc.m_Format, bRemoveSRGB));
  header.SetWidth(headerTemp.GetWidth(subResource.m_uiMipLevel));
  header.SetHeight(headerTemp.GetHeight(subResource.m_uiMipLevel));

  out_image.ResetAndAlloc(header);

  if (header.GetRowPitch() == memory.m_uiRowPitch)
  {
    const void* pSource = memory.m_pData.GetPtr();
    WUInt8* pDest = out_image.GetPixelPointer<WUInt8>();
    WUInt32 uiSize = static_cast<WUInt32>(header.GetDepthPitch());
    W_ASSERT_DEBUG(uiSize <= memory.m_pData.GetCount(), "Not enough data in the buffer to create image");
    memcpy(pDest, pSource, uiSize);
  }
  else
  {
    // Copy row by row
    const WUInt32 uiHeight = header.GetNumBlocksY();
    const WUInt32 uiRowSize = static_cast<WUInt32>(header.GetRowPitch());
    for (WUInt32 y = 0; y < uiHeight; ++y)
    {
      const void* pSource = WMemoryUtils::AddByteOffset(memory.m_pData.GetPtr(), y * memory.m_uiRowPitch);
      WUInt8* pDest = out_image.GetPixelPointer<WUInt8>(0, 0, 0, 0, y);
      memcpy(pDest, pSource, uiRowSize);
    }
  }
}

WImageView WTextureUtils::MakeImageViewFromSubResource(const WGALTextureCreationDescription& desc, const WGALTextureSubresource& subResource, const WGALSystemMemoryDescription& memory, WImage& ref_tempImage, bool bRemoveSRGB)
{
  WImageView view;
  WImageHeader headerTemp;
  headerTemp.SetImageFormat(WTextureUtils::GalFormatToImageFormat(desc.m_Format, bRemoveSRGB));
  headerTemp.SetWidth(desc.m_uiWidth);
  headerTemp.SetHeight(desc.m_uiHeight);
  headerTemp.SetNumMipLevels(desc.m_uiMipLevelCount);

  WImageHeader header;
  header.SetImageFormat(WTextureUtils::GalFormatToImageFormat(desc.m_Format, bRemoveSRGB));
  header.SetWidth(headerTemp.GetWidth(subResource.m_uiMipLevel));
  header.SetHeight(headerTemp.GetHeight(subResource.m_uiMipLevel));

  if (header.GetRowPitch() == memory.m_uiRowPitch)
  {
    view.ResetAndViewExternalStorage(header, memory.m_pData);
  }
  else
  {
    CopySubResourceToImage(desc, subResource, memory, ref_tempImage, bRemoveSRGB);
    view = ref_tempImage.GetSubImageView();
  }
  return view;
}

void WTextureUtils::CopySubResourceToMemory(const WGALTextureCreationDescription& desc, const WGALTextureSubresource& subResource, const WGALSystemMemoryDescription& sourceMemory, WArrayPtr<WUInt8> targetData, WUInt32 uiTargetRowPitch)
{
  if (sourceMemory.m_uiRowPitch == uiTargetRowPitch)
  {
    const WUInt32 uiMemorySize = WGALResourceFormat::GetBitsPerElement(desc.m_Format) *
                                  GetMipSize(desc.m_uiWidth, subResource.m_uiMipLevel) *
                                  GetMipSize(desc.m_uiHeight, subResource.m_uiMipLevel) / 8;
    W_ASSERT_DEBUG(uiMemorySize <= sourceMemory.m_pData.GetCount(), "");
    W_ASSERT_DEBUG(uiMemorySize <= targetData.GetCount(), "");
    memcpy(targetData.GetPtr(), sourceMemory.m_pData.GetPtr(), uiMemorySize);
  }
  else
  {
    // Copy row by row
    const WUInt32 uiHeight = GetMipSize(desc.m_uiHeight, subResource.m_uiMipLevel);
    for (WUInt32 y = 0; y < uiHeight; ++y)
    {
      const WUInt8* pSource = WMemoryUtils::AddByteOffset(sourceMemory.m_pData.GetPtr(), y * sourceMemory.m_uiRowPitch);
      WUInt8* pDest = WMemoryUtils::AddByteOffset(targetData.GetPtr(), y * uiTargetRowPitch);

      const WUInt32 uiCopySize = WGALResourceFormat::GetBitsPerElement(desc.m_Format) * GetMipSize(desc.m_uiWidth, subResource.m_uiMipLevel) / 8;
      W_ASSERT_DEBUG(pDest + uiCopySize <= targetData.GetEndPtr(), "");
      W_ASSERT_DEBUG(pSource + uiCopySize <= sourceMemory.m_pData.GetEndPtr(), "");
      memcpy(pDest, pSource, uiCopySize);
    }
  }
}
