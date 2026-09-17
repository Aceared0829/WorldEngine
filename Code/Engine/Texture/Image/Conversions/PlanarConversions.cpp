#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageConversion.h>

namespace
{
  // https://docs.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering#converting-8-bit-yuv-to-rgb888
  WVec3I32 RGB2YUV(WVec3I32 vRgb)
  {
    WVec3I32 yuv;
    yuv.x = ((66 * vRgb.x + 129 * vRgb.y + 25 * vRgb.z + 128) >> 8) + 16;
    yuv.y = ((-38 * vRgb.x - 74 * vRgb.y + 112 * vRgb.z + 128) >> 8) + 128;
    yuv.z = ((112 * vRgb.x - 94 * vRgb.y - 18 * vRgb.z + 128) >> 8) + 128;
    return yuv;
  }

  WVec3I32 YUV2RGB(WVec3I32 vYuv)
  {
    WVec3I32 rgb;

    WInt32 C = vYuv.x - 16;
    WInt32 D = vYuv.y - 128;
    WInt32 E = vYuv.z - 128;

    rgb.x = WMath::Clamp((298 * C + 409 * E + 128) >> 8, 0, 255);
    rgb.y = WMath::Clamp((298 * C - 100 * D - 208 * E + 128) >> 8, 0, 255);
    rgb.z = WMath::Clamp((298 * C + 516 * D + 128) >> 8, 0, 255);
    return rgb;
  }
} // namespace

struct WImageConversion_NV12_sRGB : public WImageConversionStepDeplanarize
{
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::NV12, WImageFormat::R8G8B8A8_UNORM_SRGB, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WArrayPtr<WImageView> source, WImage target, WUInt32 uiNumPixelsX, WUInt32 uiNumPixelsY, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);
    W_IGNORE_UNUSED(targetFormat);

    for (WUInt32 y = 0; y < uiNumPixelsY; y += 2)
    {
      const WUInt8* luma0 = source[0].GetPixelPointer<WUInt8>(0, 0, 0, 0, y);
      const WUInt8* luma1 = source[0].GetPixelPointer<WUInt8>(0, 0, 0, 0, y + 1);
      const WUInt8* chroma = source[1].GetPixelPointer<WUInt8>(0, 0, 0, 0, y / 2);

      WUInt8* rgba0 = target.GetPixelPointer<WUInt8>(0, 0, 0, 0, y);
      WUInt8* rgba1 = target.GetPixelPointer<WUInt8>(0, 0, 0, 0, y + 1);

      for (WUInt32 x = 0; x < uiNumPixelsX; x += 2)
      {
        WVec3I32 p00 = YUV2RGB(WVec3I32(luma0[0], chroma[0], chroma[1]));
        WVec3I32 p01 = YUV2RGB(WVec3I32(luma0[1], chroma[0], chroma[1]));
        WVec3I32 p10 = YUV2RGB(WVec3I32(luma1[0], chroma[0], chroma[1]));
        WVec3I32 p11 = YUV2RGB(WVec3I32(luma1[1], chroma[0], chroma[1]));

        rgba0[0] = static_cast<WUInt8>(p00.x);
        rgba0[1] = static_cast<WUInt8>(p00.y);
        rgba0[2] = static_cast<WUInt8>(p00.z);
        rgba0[3] = static_cast<WUInt8>(0xff);
        rgba0[4] = static_cast<WUInt8>(p01.x);
        rgba0[5] = static_cast<WUInt8>(p01.y);
        rgba0[6] = static_cast<WUInt8>(p01.z);
        rgba0[7] = static_cast<WUInt8>(0xff);

        rgba1[0] = static_cast<WUInt8>(p10.x);
        rgba1[1] = static_cast<WUInt8>(p10.y);
        rgba1[2] = static_cast<WUInt8>(p10.z);
        rgba1[3] = static_cast<WUInt8>(0xff);
        rgba1[4] = static_cast<WUInt8>(p11.x);
        rgba1[5] = static_cast<WUInt8>(p11.y);
        rgba1[6] = static_cast<WUInt8>(p11.z);
        rgba1[7] = static_cast<WUInt8>(0xff);

        luma0 += 2;
        luma1 += 2;
        chroma += 2;

        rgba0 += 8;
        rgba1 += 8;
      }
    }

    return W_SUCCESS;
  }
};

struct WImageConversion_sRGB_NV12 : public WImageConversionStepPlanarize
{
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM_SRGB, WImageFormat::NV12, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(const WImageView& source, WArrayPtr<WImage> target, WUInt32 uiNumPixelsX, WUInt32 uiNumPixelsY, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);
    W_IGNORE_UNUSED(targetFormat);

    for (WUInt32 y = 0; y < uiNumPixelsY; y += 2)
    {
      const WUInt8* rgba0 = source.GetPixelPointer<WUInt8>(0, 0, 0, 0, y);
      const WUInt8* rgba1 = source.GetPixelPointer<WUInt8>(0, 0, 0, 0, y + 1);

      WUInt8* luma0 = target[0].GetPixelPointer<WUInt8>(0, 0, 0, 0, y);
      WUInt8* luma1 = target[0].GetPixelPointer<WUInt8>(0, 0, 0, 0, y + 1);
      WUInt8* chroma = target[1].GetPixelPointer<WUInt8>(0, 0, 0, 0, y / 2);

      for (WUInt32 x = 0; x < uiNumPixelsX; x += 2)
      {
        WVec3I32 p00 = RGB2YUV(WVec3I32(rgba0[0], rgba0[1], rgba0[2]));
        WVec3I32 p01 = RGB2YUV(WVec3I32(rgba0[4], rgba0[5], rgba0[6]));
        WVec3I32 p10 = RGB2YUV(WVec3I32(rgba1[0], rgba1[1], rgba1[2]));
        WVec3I32 p11 = RGB2YUV(WVec3I32(rgba1[4], rgba1[5], rgba1[6]));

        luma0[0] = static_cast<WUInt8>(p00.x);
        luma0[1] = static_cast<WUInt8>(p01.x);
        luma1[0] = static_cast<WUInt8>(p10.x);
        luma1[1] = static_cast<WUInt8>(p11.x);

        WVec3I32 c = (p00 + p01 + p10 + p11);

        chroma[0] = static_cast<WUInt8>(c.y >> 2);
        chroma[1] = static_cast<WUInt8>(c.z >> 2);

        luma0 += 2;
        luma1 += 2;
        chroma += 2;

        rgba0 += 8;
        rgba1 += 8;
      }
    }

    return W_SUCCESS;
  }
};

W_STATICLINK_FORCE static WImageConversion_NV12_sRGB s_conversion_NV12_sRGB;
W_STATICLINK_FORCE static WImageConversion_sRGB_NV12 s_conversion_sRGB_NV12;



W_STATICLINK_FILE(Texture, Texture_Image_Conversions_PlanarConversions);
