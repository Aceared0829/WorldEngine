#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

static WImageFormat::Enum DetermineOutputFormatPC(
  WTexConvUsage::Enum targetFormat, WTexConvCompressionMode::Enum compressionMode, WUInt32 uiNumChannels)
{
  if (targetFormat == WTexConvUsage::NormalMap || targetFormat == WTexConvUsage::NormalMap_Inverted || targetFormat == WTexConvUsage::BumpMap)
  {
    if (compressionMode >= WTexConvCompressionMode::High)
      return WImageFormat::BC5_UNORM;

    if (compressionMode >= WTexConvCompressionMode::Medium)
      return WImageFormat::R8G8_UNORM;

    // TODO: in the rare case that the input texture has higher precision, we could use R16G16_UNORM or R16G16_FLOAT here
    // R16G16_UNORM isn't supported on all platforms, so R16G16_FLOAT may be better
    // return WImageFormat::R16G16_FLOAT;
    return WImageFormat::R8G8_UNORM;
  }

  if (targetFormat == WTexConvUsage::Color)
  {
    if (compressionMode >= WTexConvCompressionMode::High && uiNumChannels < 4)
      return WImageFormat::BC1_UNORM_SRGB;

    if (compressionMode >= WTexConvCompressionMode::Medium)
      return WImageFormat::BC7_UNORM_SRGB;

    return WImageFormat::R8G8B8A8_UNORM_SRGB;
  }

  if (targetFormat == WTexConvUsage::Linear)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= WTexConvCompressionMode::Medium)
          return WImageFormat::BC4_UNORM;

        return WImageFormat::R8_UNORM;

      case 2:
        if (compressionMode >= WTexConvCompressionMode::Medium)
          return WImageFormat::BC5_UNORM;

        return WImageFormat::R8G8_UNORM;

      case 3:
        if (compressionMode >= WTexConvCompressionMode::High)
          return WImageFormat::BC1_UNORM;

        if (compressionMode >= WTexConvCompressionMode::Medium)
          return WImageFormat::BC7_UNORM;

        return WImageFormat::R8G8B8A8_UNORM;

      case 4:
        if (compressionMode >= WTexConvCompressionMode::Medium)
          return WImageFormat::BC7_UNORM;

        return WImageFormat::R8G8B8A8_UNORM;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (targetFormat == WTexConvUsage::Hdr)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= WTexConvCompressionMode::High)
          return WImageFormat::BC6H_UF16;

        return WImageFormat::R16_FLOAT;

      case 2:
        return WImageFormat::R16G16_FLOAT;

      case 3:
        if (compressionMode >= WTexConvCompressionMode::High)
          return WImageFormat::BC6H_UF16;

        if (compressionMode >= WTexConvCompressionMode::Medium)
          return WImageFormat::R11G11B10_FLOAT;

        return WImageFormat::R16G16B16A16_FLOAT;

      case 4:
        return WImageFormat::R16G16B16A16_FLOAT;
    }
  }

  return WImageFormat::UNKNOWN;
}

WResult WTexConvProcessor::ChooseOutputFormat(WEnum<WImageFormat>& out_Format, WEnum<WTexConvUsage> usage, WUInt32 uiNumChannels) const
{
  W_PROFILE_SCOPE("ChooseOutputFormat");

  W_ASSERT_DEV(out_Format == WImageFormat::UNKNOWN, "Output format already set");

  switch (m_Descriptor.m_TargetPlatform)
  {
      // case  WTexConvTargetPlatform::Android:
      //  out_Format = DetermineOutputFormatAndroid(m_Descriptor.m_TargetFormat, m_Descriptor.m_CompressionMode);
      //  break;

    case WTexConvTargetPlatform::PC:
      out_Format = DetermineOutputFormatPC(usage, m_Descriptor.m_CompressionMode, uiNumChannels);
      break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  if (out_Format == WImageFormat::UNKNOWN)
  {
    WLog::Error("Failed to decide for an output image format.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}
