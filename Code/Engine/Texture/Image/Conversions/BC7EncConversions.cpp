#include <Texture/TexturePCH.h>

#if W_USE_BC7ENC

#  include <bc7enc_rdo/rdo_bc_encoder.h>

#  include <Foundation/System/SystemInformation.h>
#  include <Texture/Image/ImageConversion.h>

WImageConversionEntry g_BC7EncConversions[] = {
  // Even at the lowest quality level of BC7Enc, BC1 encoding times are more than a magnitude worse than DXTexConv.
  // WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, WImageFormat::BC1_UNORM, WImageConversionFlags::Default),
  // WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM_SRGB, WImageFormat::BC1_UNORM_SRGB, WImageConversionFlags::Default),
  WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, WImageFormat::BC7_UNORM, WImageConversionFlags::Default),
  WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM_SRGB, WImageFormat::BC7_UNORM_SRGB, WImageConversionFlags::Default),
};

class WImageConversion_CompressBC7Enc : public WImageConversionStepCompressBlocks
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    return g_BC7EncConversions;
  }

  virtual WResult CompressBlocks(WConstByteBlobPtr source, WByteBlobPtr target, WUInt32 numBlocksX, WUInt32 numBlocksY,
    WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    WSystemInformation info = WSystemInformation::Get();
    const WInt32 iCpuCores = info.GetCPUCoreCount();

    rdo_bc::rdo_bc_params rp;
    rp.m_rdo_max_threads = WMath::Clamp<WInt32>(iCpuCores - 2, 2, 8);
    rp.m_status_output = false;
    rp.m_bc1_quality_level = 18;

    switch (targetFormat)
    {
      case WImageFormat::BC7_UNORM:
      case WImageFormat::BC7_UNORM_SRGB:
        rp.m_dxgi_format = DXGI_FORMAT_BC7_UNORM;
        break;
      case WImageFormat::BC1_UNORM:
      case WImageFormat::BC1_UNORM_SRGB:
        rp.m_dxgi_format = DXGI_FORMAT_BC1_UNORM;
        break;
      default:
        W_ASSERT_NOT_IMPLEMENTED;
    }

    utils::image_u8 source_image(numBlocksX * 4, numBlocksY * 4);
    auto& pixels = source_image.get_pixels();
    WMemoryUtils::Copy<WUInt32>(reinterpret_cast<WUInt32*>(pixels.data()), reinterpret_cast<const WUInt32*>(source.GetPtr()), numBlocksX * 4 * numBlocksY * 4);

    rdo_bc::rdo_bc_encoder encoder;
    if (!encoder.init(source_image, rp))
    {
      WLog::Error("rdo_bc_encoder::init() failed!");
      return W_FAILURE;
    }

    if (!encoder.encode())
    {
      WLog::Error("rdo_bc_encoder::encode() failed!");
      return W_FAILURE;
    }

    const WUInt32 uiTotalBytes = encoder.get_total_blocks_size_in_bytes();
    if (uiTotalBytes != target.GetCount())
    {
      WLog::Error("Encoder output of {} byte does not match the expected size of {} bytes", uiTotalBytes, target.GetCount());
      return W_FAILURE;
    }
    WMemoryUtils::Copy<WUInt8>(reinterpret_cast<WUInt8*>(target.GetPtr()), reinterpret_cast<const WUInt8*>(encoder.get_blocks()), uiTotalBytes);
    return W_SUCCESS;
  }
};

W_STATICLINK_FORCE static WImageConversion_CompressBC7Enc s_conversion_compressBC7Enc;

#endif

W_STATICLINK_FILE(Texture, Texture_Image_Conversions_BC7EncConversions);
