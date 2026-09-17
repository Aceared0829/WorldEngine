#include <Texture/TexturePCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

WResult WTexConvProcessor::ForceSRGBFormats()
{
  // if the output is going to be sRGB, assume the incoming RGB data is also already in sRGB
  if (m_Descriptor.m_Usage == WTexConvUsage::Color)
  {
    for (const auto& mapping : m_Descriptor.m_ChannelMappings)
    {
      // do not enforce sRGB conversion for textures that are mapped to the alpha channel
      for (WUInt32 i = 0; i < 3; ++i)
      {
        const WInt32 iTex = mapping.m_Channel[i].m_iInputImageIndex;
        if (iTex != -1)
        {
          auto& img = m_Descriptor.m_InputImages[iTex];
          img.ReinterpretAs(WImageFormat::AsSrgb(img.GetImageFormat()));
        }
      }
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::GenerateMipmaps(WImage& img, WUInt32 uiNumMips, MipmapChannelMode channelMode /*= MipmapChannelMode::AllChannels*/) const
{
  W_PROFILE_SCOPE("GenerateMipmaps");

  WImageUtils::MipMapOptions opt;
  opt.m_numMipMaps = uiNumMips;

  WImageFilterBox filterLinear;
  WImageFilterSincWithKaiserWindow filterKaiser;

  switch (m_Descriptor.m_MipmapMode)
  {
    case WTexConvMipmapMode::None:
      return W_SUCCESS;

    case WTexConvMipmapMode::Linear:
      opt.m_filter = &filterLinear;
      break;

    case WTexConvMipmapMode::Kaiser:
      opt.m_filter = &filterKaiser;
      break;
  }

  opt.m_addressModeU = m_Descriptor.m_AddressModeU;
  opt.m_addressModeV = m_Descriptor.m_AddressModeV;
  opt.m_addressModeW = m_Descriptor.m_AddressModeW;

  opt.m_preserveCoverage = m_Descriptor.m_bPreserveMipmapCoverage;
  opt.m_alphaThreshold = m_Descriptor.m_fMipmapAlphaThreshold;

  opt.m_renormalizeNormals = m_Descriptor.m_Usage == WTexConvUsage::NormalMap || m_Descriptor.m_Usage == WTexConvUsage::NormalMap_Inverted || m_Descriptor.m_Usage == WTexConvUsage::BumpMap;

  // Copy red to alpha channel if we only have a single channel input texture
  if (opt.m_preserveCoverage && channelMode == MipmapChannelMode::SingleChannel)
  {
    auto imgData = img.GetBlobPtr<WColor>();
    auto pData = imgData.GetPtr();
    while (pData < imgData.GetEndPtr())
    {
      pData->a = pData->r;
      ++pData;
    }
  }

  WImage scratch;
  WImageUtils::GenerateMipMaps(img, scratch, opt);
  img.ResetAndMove(std::move(scratch));

  if (img.GetNumMipLevels() <= 1)
  {
    WLog::Error("Mipmap generation failed.");
    return W_FAILURE;
  }

  // Copy alpha channel back to red
  if (opt.m_preserveCoverage && channelMode == MipmapChannelMode::SingleChannel)
  {
    auto imgData = img.GetBlobPtr<WColor>();
    auto pData = imgData.GetPtr();
    while (pData < imgData.GetEndPtr())
    {
      pData->r = pData->a;
      ++pData;
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::PremultiplyAlpha(WImage& image) const
{
  W_PROFILE_SCOPE("PremultiplyAlpha");

  if (!m_Descriptor.m_bPremultiplyAlpha)
    return W_SUCCESS;

  for (WColor& col : image.GetBlobPtr<WColor>())
  {
    col.r *= col.a;
    col.g *= col.a;
    col.b *= col.a;
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::AdjustHdrExposure(WImage& img) const
{
  W_PROFILE_SCOPE("AdjustHdrExposure");

  WImageUtils::ChangeExposure(img, m_Descriptor.m_fHdrExposureBias);
  return W_SUCCESS;
}

WResult WTexConvProcessor::ConvertToNormalMap(WArrayPtr<WImage> imgs) const
{
  W_PROFILE_SCOPE("ConvertToNormalMap");

  for (WImage& img : imgs)
  {
    W_SUCCEED_OR_RETURN(ConvertToNormalMap(img));
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::ConvertToNormalMap(WImage& bumpMap) const
{
  WImageHeader newImageHeader = bumpMap.GetHeader();
  newImageHeader.SetNumMipLevels(1);
  WImage newImage;
  newImage.ResetAndAlloc(newImageHeader);

  struct Accum
  {
    float x = 0.f;
    float y = 0.f;
  };
  WDelegate<Accum(WUInt32, WUInt32)> filterKernel;

  // we'll assume that both the input bump map and the new image are using
  // RGBA 32 bit floating point as an internal format which should be tightly packed
  W_ASSERT_DEV(bumpMap.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT && bumpMap.GetRowPitch() % sizeof(WColor) == 0, "");

  const WColor* bumpPixels = bumpMap.GetPixelPointer<WColor>(0, 0, 0, 0, 0, 0);
  const auto getBumpPixel = [&](WUInt32 x, WUInt32 y) -> float
  {
    const WColor* ptr = bumpPixels + y * bumpMap.GetWidth() + x;
    return ptr->r;
  };

  WColor* newPixels = newImage.GetPixelPointer<WColor>(0, 0, 0, 0, 0, 0);
  auto getNewPixel = [&](WUInt32 x, WUInt32 y) -> WColor&
  {
    WColor* ptr = newPixels + y * newImage.GetWidth() + x;
    return *ptr;
  };

  switch (m_Descriptor.m_BumpMapFilter)
  {
    case WTexConvBumpMapFilter::Finite:
      filterKernel = [&](WUInt32 x, WUInt32 y)
      {
        constexpr float linearKernel[3] = {-1, 0, 1};

        Accum accum;
        for (int i = -1; i <= 1; ++i)
        {
          const WInt32 rx = WMath::Clamp(i + static_cast<WInt32>(x), 0, static_cast<WInt32>(newImage.GetWidth()) - 1);
          const WInt32 ry = WMath::Clamp(i + static_cast<WInt32>(y), 0, static_cast<WInt32>(newImage.GetHeight()) - 1);

          const float depthX = getBumpPixel(rx, y);
          const float depthY = getBumpPixel(x, ry);

          accum.x += depthX * linearKernel[i + 1];
          accum.y += depthY * linearKernel[i + 1];
        }

        return accum;
      };
      break;
    case WTexConvBumpMapFilter::Sobel:
      filterKernel = [&](WUInt32 x, WUInt32 y)
      {
        constexpr float kernel[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
        constexpr float weight = 1.f / 4.f;

        Accum accum;
        for (WInt32 i = -1; i <= 1; ++i)
        {
          for (WInt32 j = -1; j <= 1; ++j)
          {
            const WInt32 rx = WMath::Clamp(j + static_cast<WInt32>(x), 0, static_cast<WInt32>(newImage.GetWidth()) - 1);
            const WInt32 ry = WMath::Clamp(i + static_cast<WInt32>(y), 0, static_cast<WInt32>(newImage.GetHeight()) - 1);

            const float depth = getBumpPixel(rx, ry);

            accum.x += depth * kernel[i + 1][j + 1];
            accum.y += depth * kernel[j + 1][i + 1];
          }
        }

        accum.x *= weight;
        accum.y *= weight;

        return accum;
      };
      break;
    case WTexConvBumpMapFilter::Scharr:
      filterKernel = [&](WUInt32 x, WUInt32 y)
      {
        constexpr float kernel[3][3] = {{-3, 0, 3}, {-10, 0, 10}, {-3, 0, 3}};
        constexpr float weight = 1.f / 16.f;

        Accum accum;
        for (WInt32 i = -1; i <= 1; ++i)
        {
          for (WInt32 j = -1; j <= 1; ++j)
          {
            const WInt32 rx = WMath::Clamp(j + static_cast<WInt32>(x), 0, static_cast<WInt32>(newImage.GetWidth()) - 1);
            const WInt32 ry = WMath::Clamp(i + static_cast<WInt32>(y), 0, static_cast<WInt32>(newImage.GetHeight()) - 1);

            const float depth = getBumpPixel(rx, ry);

            accum.x += depth * kernel[i + 1][j + 1];
            accum.y += depth * kernel[j + 1][i + 1];
          }
        }

        accum.x *= weight;
        accum.y *= weight;

        return accum;
      };
      break;
  };

  for (WUInt32 y = 0; y < bumpMap.GetHeight(); ++y)
  {
    for (WUInt32 x = 0; x < bumpMap.GetWidth(); ++x)
    {
      Accum accum = filterKernel(x, y);

      WVec3 normal = WVec3(1.f, 0.f, accum.x).CrossRH(WVec3(0.f, 1.f, accum.y));
      normal.NormalizeIfNotZero(WVec3(0, 0, 1), 0.001f).IgnoreResult();
      normal.y = -normal.y;

      normal = normal * 0.5f + WVec3(0.5f);

      WColor& newPixel = getNewPixel(x, y);
      newPixel.SetRGBA(normal.x, normal.y, normal.z, 0.f);
    }
  }

  bumpMap.ResetAndMove(std::move(newImage));

  return W_SUCCESS;
}

WResult WTexConvProcessor::ClampInputValues(WArrayPtr<WImage> images, float maxValue) const
{
  for (WImage& image : images)
  {
    W_SUCCEED_OR_RETURN(ClampInputValues(image, maxValue));
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::ClampInputValues(WImage& image, float maxValue) const
{
  // we'll assume that at this point in the processing pipeline, the format is
  // RGBA32F which should result in tightly packed mipmaps.
  W_ASSERT_DEV(image.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT && image.GetRowPitch() % sizeof(float[4]) == 0, "");

  for (auto& value : image.GetBlobPtr<float>())
  {
    if (WMath::IsNaN(value))
    {
      value = 0.f;
    }
    else
    {
      value = WMath::Clamp(value, -maxValue, maxValue);
    }
  }

  return W_SUCCESS;
}

/// Alpha value above which a pixel is considered fully opaque and its color is kept as is.
static constexpr float g_fDilateOpaqueThreshold = 220.0f / 255.0f;

/// Replaces the color of all non-opaque pixels with the average color of all opaque ones.
///
/// Returns false, if either all or no pixels are opaque, in which case there is nothing to dilate.
static bool FillAvgImageColor(WImage& ref_img)
{
  auto pixels = ref_img.GetBlobPtr<WColor>();

  WColor avg = WColor::MakeZero();
  WUInt32 uiValidCount = 0;

  for (const WColor& col : pixels)
  {
    if (col.a >= g_fDilateOpaqueThreshold)
    {
      avg += col;
      ++uiValidCount;
    }
  }

  if (uiValidCount == 0 || uiValidCount == pixels.GetCount())
  {
    // nothing to do
    return false;
  }

  avg /= static_cast<float>(uiValidCount);
  avg.NormalizeToLdrRange();

  for (WColor& col : pixels)
  {
    if (col.a < g_fDilateOpaqueThreshold)
    {
      const float fAlpha = col.a;
      col = avg;
      col.a = fAlpha;
    }
  }

  return true;
}

inline static WColor GetPixelValue(const WColor* pPixels, WInt32 iWidth, WInt32 x, WInt32 y)
{
  return pPixels[y * iWidth + x];
}

inline static void SetPixelValue(WColor* pPixels, WInt32 iWidth, WInt32 x, WInt32 y, const WColor& col)
{
  pPixels[y * iWidth + x] = col;
}

/// Smears the color of valid pixels into their invalid neighbors.
///
/// A pixel is valid, if its entry in validPixels is set. Pixels that receive a color this pass are
/// recorded in out_filledIndices instead of being marked immediately, so that a color doesn't
/// travel more than one pixel per pass. The alpha channel is never modified.
static void DilateColors(WColor* pPixels, WInt32 iWidth, WInt32 iHeight, const WDynamicArray<bool>& validPixels, WDynamicArray<WUInt32>& out_filledIndices)
{
  out_filledIndices.Clear();

  const WInt32 iRadius = 1;

  for (WInt32 y = 0; y < iHeight; ++y)
  {
    for (WInt32 x = 0; x < iWidth; ++x)
    {
      const WUInt32 uiIndex = static_cast<WUInt32>(y * iWidth + x);

      if (validPixels[uiIndex])
        continue;

      WColor avg = WColor::MakeZero();
      WUInt32 uiValidCount = 0;

      for (WInt32 cy = WMath::Max<WInt32>(0, y - iRadius); cy <= WMath::Min<WInt32>(y + iRadius, iHeight - 1); ++cy)
      {
        for (WInt32 cx = WMath::Max<WInt32>(0, x - iRadius); cx <= WMath::Min<WInt32>(x + iRadius, iWidth - 1); ++cx)
        {
          if (!validPixels[cy * iWidth + cx])
            continue;

          avg += GetPixelValue(pPixels, iWidth, cx, cy);
          ++uiValidCount;
        }
      }

      if (uiValidCount == 0)
        continue;

      avg /= static_cast<float>(uiValidCount);
      avg.a = GetPixelValue(pPixels, iWidth, x, y).a;

      SetPixelValue(pPixels, iWidth, x, y, avg);
      out_filledIndices.PushBack(uiIndex);
    }
  }
}

WResult WTexConvProcessor::DilateColor2D(WImage& img) const
{
  if (m_Descriptor.m_uiDilateColor == 0)
    return W_SUCCESS;

  W_PROFILE_SCOPE("DilateColor2D");

  if (!FillAvgImageColor(img))
    return W_SUCCESS;

  const WUInt32 uiNumPasses = m_Descriptor.m_uiDilateColor;

  WColor* pPixels = img.GetPixelPointer<WColor>();
  const WInt32 iWidth = static_cast<WInt32>(img.GetWidth());
  const WInt32 iHeight = static_cast<WInt32>(img.GetHeight());

  WDynamicArray<bool> validPixels;
  validPixels.SetCount(static_cast<WUInt32>(img.GetBlobPtr<WColor>().GetCount()));

  for (WUInt32 i = 0; i < validPixels.GetCount(); ++i)
  {
    validPixels[i] = pPixels[i].a >= g_fDilateOpaqueThreshold;
  }

  WDynamicArray<WUInt32> filledIndices;

  for (WUInt32 pass = 0; pass < uiNumPasses; ++pass)
  {
    DilateColors(pPixels, iWidth, iHeight, validPixels, filledIndices);

    for (WUInt32 uiIndex : filledIndices)
    {
      validPixels[uiIndex] = true;
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::InvertNormalMap(WImage& image)
{
  if (m_Descriptor.m_Usage != WTexConvUsage::NormalMap_Inverted)
    return W_SUCCESS;

  // we'll assume that at this point in the processing pipeline, the format is
  // RGBA32F which should result in tightly packed mipmaps.
  W_ASSERT_DEV(image.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT && image.GetRowPitch() % sizeof(float[4]) == 0, "");

  for (auto& value : image.GetBlobPtr<WColor>())
  {
    value.g = 1.0f - value.g;
  }

  return W_SUCCESS;
}
