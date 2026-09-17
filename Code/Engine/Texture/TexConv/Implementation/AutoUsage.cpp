#include <Texture/TexturePCH.h>

#include <Texture/TexConv/TexConvProcessor.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

struct FileSuffixToUsage
{
  const char* m_szSuffix = nullptr;
  const WTexConvUsage::Enum m_Usage = WTexConvUsage::Auto;
};

static FileSuffixToUsage suffixToUsageMap[] = {
  //
  {"_d", WTexConvUsage::Color},          //
  {"diff", WTexConvUsage::Color},        //
  {"diffuse", WTexConvUsage::Color},     //
  {"albedo", WTexConvUsage::Color},      //
  {"col", WTexConvUsage::Color},         //
  {"color", WTexConvUsage::Color},       //
  {"emissive", WTexConvUsage::Color},    //
  {"emit", WTexConvUsage::Color},        //

  {"_n", WTexConvUsage::NormalMap},      //
  {"nrm", WTexConvUsage::NormalMap},     //
  {"norm", WTexConvUsage::NormalMap},    //
  {"normal", WTexConvUsage::NormalMap},  //
  {"normals", WTexConvUsage::NormalMap}, //

  {"_r", WTexConvUsage::Linear},         //
  {"_rgh", WTexConvUsage::Linear},       //
  {"_rough", WTexConvUsage::Linear},     //
  {"roughness", WTexConvUsage::Linear},  //

  {"_m", WTexConvUsage::Linear},         //
  {"_met", WTexConvUsage::Linear},       //
  {"_metal", WTexConvUsage::Linear},     //
  {"metallic", WTexConvUsage::Linear},   //

  {"_h", WTexConvUsage::Linear},         //
  {"height", WTexConvUsage::Linear},     //
  {"_disp", WTexConvUsage::Linear},      //

  {"_ao", WTexConvUsage::Linear},        //
  {"occlusion", WTexConvUsage::Linear},  //

  {"_alpha", WTexConvUsage::Linear},     //
};


static WTexConvUsage::Enum DetectUsageFromFilename(WStringView sFile)
{
  WStringBuilder name = WPathUtils::GetFileName(sFile);
  name.ToLower();

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(suffixToUsageMap); ++i)
  {
    if (name.EndsWith_NoCase(suffixToUsageMap[i].m_szSuffix))
    {
      return suffixToUsageMap[i].m_Usage;
    }
  }

  return WTexConvUsage::Auto;
}

static WTexConvUsage::Enum DetectUsageFromImage(const WImage& image)
{
  const WImageHeader& header = image.GetHeader();
  const WImageFormat::Enum format = header.GetImageFormat();

  if (header.GetDepth() > 1)
  {
    // unsupported
    return WTexConvUsage::Auto;
  }

  if (WImageFormat::IsSrgb(format))
  {
    // already sRGB so must be color
    return WTexConvUsage::Color;
  }

  if (format == WImageFormat::BC5_UNORM)
  {
    return WTexConvUsage::NormalMap;
  }

  if (WImageFormat::GetBitsPerChannel(format, WImageFormatChannel::R) > 8 || format == WImageFormat::BC6H_SF16 ||
      format == WImageFormat::BC6H_UF16)
  {
    return WTexConvUsage::Hdr;
  }

  if (WImageFormat::GetNumChannels(format) <= 2)
  {
    return WTexConvUsage::Linear;
  }

  const WImage* pImgRGBA = &image;
  WImage convertedRGBA;

  if (image.GetImageFormat() != WImageFormat::R8G8B8A8_UNORM)
  {
    pImgRGBA = &convertedRGBA;
    if (WImageConversion::Convert(image, convertedRGBA, WImageFormat::R8G8B8A8_UNORM).Failed())
    {
      // cannot convert to RGBA -> maybe some weird lookup table format
      return WTexConvUsage::Auto;
    }
  }

  // analyze the image content
  {
    WUInt32 sr = 0;
    WUInt32 sg = 0;
    WUInt32 sb = 0;

    WUInt32 uiExtremeNormals = 0;

    WUInt32 uiNumPixels = header.GetWidth() * header.GetHeight();
    W_ASSERT_DEBUG(uiNumPixels > 0, "Unexpected empty image");

    // Sample no more than 10000 pixels
    WUInt32 uiStride = WMath::Max(1U, uiNumPixels / 10000);
    uiNumPixels /= uiStride;

    const WUInt8* pPixel = pImgRGBA->GetPixelPointer<WUInt8>();

    for (WUInt32 uiPixel = 0; uiPixel < uiNumPixels; ++uiPixel)
    {
      // definitely not a normal map, if any Z vector points that much backwards
      uiExtremeNormals += (pPixel[2] < 90) ? 1 : 0;

      sr += pPixel[0];
      sg += pPixel[1];
      sb += pPixel[2];

      pPixel += 4 * uiStride;
    }

    // the average color in the image
    sr /= uiNumPixels; // NOLINT: not a division by zero
    sg /= uiNumPixels; // NOLINT: not a division by zero
    sb /= uiNumPixels; // NOLINT: not a division by zero

    if (sb < 230 || sr < 128 - 60 || sr > 128 + 60 || sg < 128 - 60 || sg > 128 + 60)
    {
      // if the average color is not a proper hue of blue, it cannot be a normal map
      return WTexConvUsage::Color;
    }

    if (uiExtremeNormals > uiNumPixels / 100)
    {
      // more than 1 percent of normals pointing backwards ? => probably not a normalmap
      return WTexConvUsage::Color;
    }

    // it might just be a normal map, it does have the proper hue of blue
    return WTexConvUsage::NormalMap;
  }
}

WResult WTexConvProcessor::AdjustUsage(WStringView sFilename, const WImage& srcImg, WEnum<WTexConvUsage>& inout_Usage)
{
  W_PROFILE_SCOPE("AdjustUsage");

  if (inout_Usage == WTexConvUsage::Auto)
  {
    inout_Usage = DetectUsageFromFilename(sFilename);
  }

  if (inout_Usage == WTexConvUsage::Auto)
  {
    inout_Usage = DetectUsageFromImage(srcImg);
  }

  if (inout_Usage == WTexConvUsage::Auto)
  {
    WLog::Error("Failed to deduce target format.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}
