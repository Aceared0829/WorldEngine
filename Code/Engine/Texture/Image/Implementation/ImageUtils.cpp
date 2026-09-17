#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageUtils.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Time/Timestamp.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

template <typename TYPE>
static void SetDiff(const WImageView& imageA, const WImageView& imageB, WImage& out_difference, WUInt32 w, WUInt32 h, WUInt32 d, WUInt32 uiComp)
{
  const TYPE* pA = imageA.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  const TYPE* pB = imageB.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (WUInt32 i = 0; i < uiComp; ++i)
    pR[i] = pB[i] > pA[i] ? (pB[i] - pA[i]) : (pA[i] - pB[i]);
}

template <typename TYPE, typename ACCU, int COMP>
static void SetCompMinDiff(const WImageView& newDifference, WImage& out_minDifference, WUInt32 w, WUInt32 h, WUInt32 d, WUInt32 uiComp)
{
  const TYPE* pNew = newDifference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_minDifference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (WUInt32 i = 0; i < uiComp; i += COMP)
  {
    ACCU minDiff = 0;
    ACCU newDiff = 0;
    for (WUInt32 c = 0; c < COMP; c++)
    {
      minDiff += pR[i + c];
      newDiff += pNew[i + c];
    }
    if (minDiff > newDiff)
    {
      for (WUInt32 c = 0; c < COMP; c++)
        pR[i + c] = pNew[i + c];
    }
  }
}

template <typename TYPE>
static WUInt32 GetError(const WImageView& difference, WUInt32 w, WUInt32 h, WUInt32 d, WUInt32 uiComp, WUInt32 uiPixel)
{
  const TYPE* pR = difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  WUInt32 uiErrorSum = 0;

  for (WUInt32 p = 0; p < uiPixel; ++p)
  {
    WUInt32 error = 0;

    for (WUInt32 c = 0; c < uiComp; ++c)
    {
      error += *pR;
      ++pR;
    }

    error /= uiComp;
    uiErrorSum += error * error;
  }

  return uiErrorSum;
}

void WImageUtils::ComputeImageDifferenceABS(const WImageView& imageA, const WImageView& imageB, WImage& out_difference)
{
  W_PROFILE_SCOPE("WImageUtils::ComputeImageDifferenceABS");

  W_ASSERT_DEV(imageA.GetWidth() == imageB.GetWidth(), "Dimensions do not match");
  W_ASSERT_DEV(imageA.GetHeight() == imageB.GetHeight(), "Dimensions do not match");
  W_ASSERT_DEV(imageA.GetDepth() == imageB.GetDepth(), "Dimensions do not match");
  W_ASSERT_DEV(imageA.GetImageFormat() == imageB.GetImageFormat(), "Format does not match");

  WImageHeader differenceHeader;

  differenceHeader.SetWidth(imageA.GetWidth());
  differenceHeader.SetHeight(imageA.GetHeight());
  differenceHeader.SetDepth(imageA.GetDepth());
  differenceHeader.SetImageFormat(imageA.GetImageFormat());
  out_difference.ResetAndAlloc(differenceHeader);

  const WUInt32 uiSize2D = imageA.GetHeight() * imageA.GetWidth();

  for (WUInt32 d = 0; d < imageA.GetDepth(); ++d)
  {
    // for (WUInt32 h = 0; h < ImageA.GetHeight(); ++h)
    {
      // for (WUInt32 w = 0; w < ImageA.GetWidth(); ++w)
      {
        switch (imageA.GetImageFormat())
        {
          case WImageFormat::R8G8B8A8_UNORM:
          case WImageFormat::R8G8B8A8_UNORM_SRGB:
          case WImageFormat::R8G8B8A8_UINT:
          case WImageFormat::R8G8B8A8_SNORM:
          case WImageFormat::R8G8B8A8_SINT:
          case WImageFormat::B8G8R8A8_UNORM:
          case WImageFormat::B8G8R8X8_UNORM:
          case WImageFormat::B8G8R8A8_UNORM_SRGB:
          case WImageFormat::B8G8R8X8_UNORM_SRGB:
          {
            SetDiff<WUInt8>(imageA, imageB, out_difference, 0, 0, d, 4 * uiSize2D);
          }
          break;

          case WImageFormat::B8G8R8_UNORM:
          {
            SetDiff<WUInt8>(imageA, imageB, out_difference, 0, 0, d, 3 * uiSize2D);
          }
          break;

          default:
            W_REPORT_FAILURE("The WImageFormat {0} is not implemented", (WUInt32)imageA.GetImageFormat());
            return;
        }
      }
    }
  }
}


void WImageUtils::ComputeImageDifferenceABSRelaxed(const WImageView& imageA, const WImageView& imageB, WImage& out_difference)
{
  W_ASSERT_ALWAYS(imageA.GetDepth() == 1 && imageA.GetNumMipLevels() == 1, "Depth slices and mipmaps are not supported");

  W_PROFILE_SCOPE("WImageUtils::ComputeImageDifferenceABSRelaxed");

  ComputeImageDifferenceABS(imageA, imageB, out_difference);

  WImage tempB;
  tempB.ResetAndCopy(imageB);
  WImage tempDiff;
  tempDiff.ResetAndCopy(out_difference);

  for (WInt32 yOffset = -1; yOffset <= 1; ++yOffset)
  {
    for (WInt32 xOffset = -1; xOffset <= 1; ++xOffset)
    {
      if (yOffset == 0 && xOffset == 0)
        continue;

      WImageUtils::Copy(imageB, WRectU32(WMath::Max(xOffset, 0), WMath::Max(yOffset, 0), imageB.GetWidth() - WMath::Abs(xOffset), imageB.GetHeight() - WMath::Abs(yOffset)), tempB, WVec3U32(-WMath::Min(xOffset, 0), -WMath::Min(yOffset, 0), 0)).AssertSuccess("");

      ComputeImageDifferenceABS(imageA, tempB, tempDiff);

      const WUInt32 uiSize2D = imageA.GetHeight() * imageA.GetWidth();
      switch (imageA.GetImageFormat())
      {
        case WImageFormat::R8G8B8A8_UNORM:
        case WImageFormat::R8G8B8A8_UNORM_SRGB:
        case WImageFormat::R8G8B8A8_UINT:
        case WImageFormat::R8G8B8A8_SNORM:
        case WImageFormat::R8G8B8A8_SINT:
        case WImageFormat::B8G8R8A8_UNORM:
        case WImageFormat::B8G8R8X8_UNORM:
        case WImageFormat::B8G8R8A8_UNORM_SRGB:
        case WImageFormat::B8G8R8X8_UNORM_SRGB:
        {
          SetCompMinDiff<WUInt8, WUInt32, 4>(tempDiff, out_difference, 0, 0, 0, 4 * uiSize2D);
        }
        break;

        case WImageFormat::B8G8R8_UNORM:
        {
          SetCompMinDiff<WUInt8, WUInt32, 3>(tempDiff, out_difference, 0, 0, 0, 3 * uiSize2D);
        }
        break;

        default:
          W_REPORT_FAILURE("The WImageFormat {0} is not implemented", (WUInt32)imageA.GetImageFormat());
          return;
      }
    }
  }
}

WUInt32 WImageUtils::ComputeMeanSquareError(const WImageView& differenceImage, WUInt8 uiBlockSize, WUInt32 uiOffsetx, WUInt32 uiOffsety)
{
  W_PROFILE_SCOPE("WImageUtils::ComputeMeanSquareError(detail)");

  W_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  WUInt32 uiNumComponents = WImageFormat::GetNumChannels(differenceImage.GetImageFormat());

  WUInt32 uiWidth = WMath::Min(differenceImage.GetWidth(), uiOffsetx + uiBlockSize) - uiOffsetx;
  WUInt32 uiHeight = WMath::Min(differenceImage.GetHeight(), uiOffsety + uiBlockSize) - uiOffsety;

  // Treat image as single-component format and scale the width instead
  uiWidth *= uiNumComponents;

  if (uiWidth == 0 || uiHeight == 0)
    return 0;

  switch (differenceImage.GetImageFormat())
  {
      // Supported formats
    case WImageFormat::R8G8B8A8_UNORM:
    case WImageFormat::R8G8B8A8_UNORM_SRGB:
    case WImageFormat::R8G8B8A8_UINT:
    case WImageFormat::R8G8B8A8_SNORM:
    case WImageFormat::R8G8B8A8_SINT:
    case WImageFormat::B8G8R8A8_UNORM:
    case WImageFormat::B8G8R8A8_UNORM_SRGB:
    case WImageFormat::B8G8R8_UNORM:
      break;

    default:
      W_REPORT_FAILURE("The WImageFormat {0} is not implemented", (WUInt32)differenceImage.GetImageFormat());
      return 0;
  }


  WUInt32 error = 0;

  WUInt64 uiRowPitch = differenceImage.GetRowPitch();
  WUInt64 uiDepthPitch = differenceImage.GetDepthPitch();

  const WUInt32 uiSize2D = uiWidth * uiHeight;
  const WUInt8* pSlicePointer = differenceImage.GetPixelPointer<WUInt8>(0, 0, 0, uiOffsetx, uiOffsety);

  for (WUInt32 d = 0; d < differenceImage.GetDepth(); ++d)
  {
    const WUInt8* pRowPointer = pSlicePointer;

    for (WUInt32 y = 0; y < uiHeight; ++y)
    {
      const WUInt8* pPixelPointer = pRowPointer;
      for (WUInt32 x = 0; x < uiWidth; ++x)
      {
        WUInt32 uiDiff = *pPixelPointer;
        error += uiDiff * uiDiff;

        pPixelPointer++;
      }

      pRowPointer += uiRowPitch;
    }

    pSlicePointer += uiDepthPitch;
  }

  error /= uiSize2D;
  return error;
}

WUInt32 WImageUtils::ComputeMeanSquareError(const WImageView& differenceImage, WUInt8 uiBlockSize)
{
  W_PROFILE_SCOPE("WImageUtils::ComputeMeanSquareError");

  W_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  const WUInt32 uiHalfBlockSize = uiBlockSize / 2;

  const WUInt32 uiBlocksX = (differenceImage.GetWidth() / uiHalfBlockSize) + 1;
  const WUInt32 uiBlocksY = (differenceImage.GetHeight() / uiHalfBlockSize) + 1;

  WUInt32 uiMaxError = 0;

  for (WUInt32 by = 0; by < uiBlocksY; ++by)
  {
    for (WUInt32 bx = 0; bx < uiBlocksX; ++bx)
    {
      const WUInt32 uiBlockError = ComputeMeanSquareError(differenceImage, uiBlockSize, bx * uiHalfBlockSize, by * uiHalfBlockSize);

      uiMaxError = WMath::Max(uiMaxError, uiBlockError);
    }
  }

  return uiMaxError;
}

template <typename Func, typename ImageType>
static void ApplyFunc(ImageType& inout_image, Func func)
{
  WUInt32 uiWidth = inout_image.GetWidth();
  WUInt32 uiHeight = inout_image.GetHeight();
  WUInt32 uiDepth = inout_image.GetDepth();

  W_IGNORE_UNUSED(uiDepth);
  W_ASSERT_DEV(uiWidth > 0 && uiHeight > 0 && uiDepth > 0, "The image passed to FindMinMax has illegal dimension {}x{}x{}.", uiWidth, uiHeight, uiDepth);

  WUInt64 uiRowPitch = inout_image.GetRowPitch();
  WUInt64 uiDepthPitch = inout_image.GetDepthPitch();
  WUInt32 uiNumChannels = WImageFormat::GetNumChannels(inout_image.GetImageFormat());

  auto pSlicePointer = inout_image.template GetPixelPointer<WUInt8>();

  for (WUInt32 z = 0; z < inout_image.GetDepth(); ++z)
  {
    auto pRowPointer = pSlicePointer;

    for (WUInt32 y = 0; y < uiHeight; ++y)
    {
      auto pPixelPointer = pRowPointer;
      for (WUInt32 x = 0; x < uiWidth; ++x)
      {
        for (WUInt32 c = 0; c < uiNumChannels; ++c)
        {
          func(pPixelPointer++, x, y, z, c);
        }
      }

      pRowPointer += uiRowPitch;
    }

    pSlicePointer += uiDepthPitch;
  }
}

static void FindMinMax(const WImageView& image, WUInt8& out_uiMinRgb, WUInt8& out_uiMaxRgb, WUInt8& out_uiMinAlpha, WUInt8& out_uiMaxAlpha)
{
  WImageFormat::Enum imageFormat = image.GetImageFormat();
  W_IGNORE_UNUSED(imageFormat);
  W_ASSERT_DEV(WImageFormat::GetBitsPerChannel(imageFormat, WImageFormatChannel::R) == 8 && WImageFormat::GetDataType(imageFormat) == WImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in FindMinMax");

  out_uiMinRgb = 255u;
  out_uiMinAlpha = 255u;
  out_uiMaxRgb = 0u;
  out_uiMaxAlpha = 0u;

  auto minMax = [&](const WUInt8* pPixel, WUInt32 /*x*/, WUInt32 /*y*/, WUInt32 /*z*/, WUInt32 c)
  {
    WUInt8 val = *pPixel;

    if (c < 3)
    {
      out_uiMinRgb = WMath::Min(out_uiMinRgb, val);
      out_uiMaxRgb = WMath::Max(out_uiMaxRgb, val);
    }
    else
    {
      out_uiMinAlpha = WMath::Min(out_uiMinAlpha, val);
      out_uiMaxAlpha = WMath::Max(out_uiMaxAlpha, val);
    }
  };
  ApplyFunc(image, minMax);
}

void WImageUtils::Normalize(WImage& inout_image)
{
  WUInt8 uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha;
  Normalize(inout_image, uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha);
}

void WImageUtils::Normalize(WImage& inout_image, WUInt8& out_uiMinRgb, WUInt8& out_uiMaxRgb, WUInt8& out_uiMinAlpha, WUInt8& out_uiMaxAlpha)
{
  W_PROFILE_SCOPE("WImageUtils::Normalize");

  WImageFormat::Enum imageFormat = inout_image.GetImageFormat();

  W_ASSERT_DEV(WImageFormat::GetBitsPerChannel(imageFormat, WImageFormatChannel::R) == 8 && WImageFormat::GetDataType(imageFormat) == WImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in NormalizeImage");

  bool ignoreAlpha = false;
  if (imageFormat == WImageFormat::B8G8R8X8_UNORM || imageFormat == WImageFormat::B8G8R8X8_UNORM_SRGB)
  {
    ignoreAlpha = true;
  }

  FindMinMax(inout_image, out_uiMinRgb, out_uiMaxRgb, out_uiMinAlpha, out_uiMaxAlpha);
  WUInt8 uiRangeRgb = out_uiMaxRgb - out_uiMinRgb;
  WUInt8 uiRangeAlpha = out_uiMaxAlpha - out_uiMinAlpha;

  auto normalize = [&](WUInt8* pPixel, WUInt32 /*x*/, WUInt32 /*y*/, WUInt32 /*z*/, WUInt32 c)
  {
    WUInt8 val = *pPixel;
    if (c < 3)
    {
      // color channels are uniform when min == max, in that case keep original value as scaling is not meaningful
      if (uiRangeRgb != 0)
      {
        *pPixel = static_cast<WUInt8>(255u * (static_cast<float>(val - out_uiMinRgb) / (uiRangeRgb)));
      }
    }
    else
    {
      // alpha is uniform when minAlpha == maxAlpha, in that case keep original alpha as scaling is not meaningful
      if (!ignoreAlpha && uiRangeAlpha != 0)
      {
        *pPixel = static_cast<WUInt8>(255u * (static_cast<float>(val - out_uiMinAlpha) / (uiRangeAlpha)));
      }
    }
  };
  ApplyFunc(inout_image, normalize);
}

void WImageUtils::ExtractAlphaChannel(const WImageView& inputImage, WImage& inout_outputImage)
{
  W_PROFILE_SCOPE("WImageUtils::ExtractAlphaChannel");

  switch (WImageFormat::Enum imageFormat = inputImage.GetImageFormat())
  {
    case WImageFormat::R8G8B8A8_UNORM:
    case WImageFormat::R8G8B8A8_UNORM_SRGB:
    case WImageFormat::R8G8B8A8_UINT:
    case WImageFormat::R8G8B8A8_SNORM:
    case WImageFormat::R8G8B8A8_SINT:
    case WImageFormat::B8G8R8A8_UNORM:
    case WImageFormat::B8G8R8A8_UNORM_SRGB:
      break;
    default:
      W_REPORT_FAILURE("ExtractAlpha needs an image with 8bpp and 4 channel. The WImageFormat {} is not supported.", (WUInt32)imageFormat);
      return;
  }

  WImageHeader outputHeader = inputImage.GetHeader();
  outputHeader.SetImageFormat(WImageFormat::R8_UNORM);
  inout_outputImage.ResetAndAlloc(outputHeader);

  const WUInt8* pInputSlice = inputImage.GetPixelPointer<WUInt8>();
  WUInt8* pOutputSlice = inout_outputImage.GetPixelPointer<WUInt8>();

  WUInt64 uiInputRowPitch = inputImage.GetRowPitch();
  WUInt64 uiInputDepthPitch = inputImage.GetDepthPitch();

  WUInt64 uiOutputRowPitch = inout_outputImage.GetRowPitch();
  WUInt64 uiOutputDepthPitch = inout_outputImage.GetDepthPitch();

  for (WUInt32 d = 0; d < inputImage.GetDepth(); ++d)
  {
    const WUInt8* pInputRow = pInputSlice;
    WUInt8* pOutputRow = pOutputSlice;

    for (WUInt32 y = 0; y < inputImage.GetHeight(); ++y)
    {
      const WUInt8* pInputPixel = pInputRow;
      WUInt8* pOutputPixel = pOutputRow;
      for (WUInt32 x = 0; x < inputImage.GetWidth(); ++x)
      {
        *pOutputPixel = pInputPixel[3];

        pInputPixel += 4;
        ++pOutputPixel;
      }

      pInputRow += uiInputRowPitch;
      pOutputRow += uiOutputRowPitch;
    }

    pInputSlice += uiInputDepthPitch;
    pOutputSlice += uiOutputDepthPitch;
  }
}

void WImageUtils::CropImage(const WImageView& input, const WVec2I32& vOffset, const WSizeU32& newsize, WImage& out_output)
{
  W_PROFILE_SCOPE("WImageUtils::CropImage");

  W_ASSERT_DEV(vOffset.x >= 0, "Offset is invalid");
  W_ASSERT_DEV(vOffset.y >= 0, "Offset is invalid");
  W_ASSERT_DEV(vOffset.x < (WInt32)input.GetWidth(), "Offset is invalid");
  W_ASSERT_DEV(vOffset.y < (WInt32)input.GetHeight(), "Offset is invalid");

  const WUInt32 uiNewWidth = WMath::Min(vOffset.x + newsize.width, input.GetWidth()) - vOffset.x;
  const WUInt32 uiNewHeight = WMath::Min(vOffset.y + newsize.height, input.GetHeight()) - vOffset.y;

  WImageHeader outputHeader;
  outputHeader.SetWidth(uiNewWidth);
  outputHeader.SetHeight(uiNewHeight);
  outputHeader.SetImageFormat(input.GetImageFormat());
  out_output.ResetAndAlloc(outputHeader);

  for (WUInt32 y = 0; y < uiNewHeight; ++y)
  {
    for (WUInt32 x = 0; x < uiNewWidth; ++x)
    {
      switch (input.GetImageFormat())
      {
        case WImageFormat::R8G8B8A8_UNORM:
        case WImageFormat::R8G8B8A8_UNORM_SRGB:
        case WImageFormat::R8G8B8A8_UINT:
        case WImageFormat::R8G8B8A8_SNORM:
        case WImageFormat::R8G8B8A8_SINT:
        case WImageFormat::B8G8R8A8_UNORM:
        case WImageFormat::B8G8R8X8_UNORM:
        case WImageFormat::B8G8R8A8_UNORM_SRGB:
        case WImageFormat::B8G8R8X8_UNORM_SRGB:
          out_output.GetPixelPointer<WUInt32>(0, 0, 0, x, y)[0] = input.GetPixelPointer<WUInt32>(0, 0, 0, vOffset.x + x, vOffset.y + y)[0];
          break;

        case WImageFormat::B8G8R8_UNORM:
          out_output.GetPixelPointer<WUInt8>(0, 0, 0, x, y)[0] = input.GetPixelPointer<WUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[0];
          out_output.GetPixelPointer<WUInt8>(0, 0, 0, x, y)[1] = input.GetPixelPointer<WUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[1];
          out_output.GetPixelPointer<WUInt8>(0, 0, 0, x, y)[2] = input.GetPixelPointer<WUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[2];
          break;

        default:
          W_REPORT_FAILURE("The WImageFormat {0} is not implemented", (WUInt32)input.GetImageFormat());
          return;
      }
    }
  }
}

namespace
{
  template <typename T>
  void rotate180(T* pStart, T* pEnd)
  {
    pEnd = pEnd - 1;
    while (pStart < pEnd)
    {
      WMath::Swap(*pStart, *pEnd);
      pStart++;
      pEnd--;
    }
  }
} // namespace

void WImageUtils::RotateSubImage180(WImage& inout_image, WUInt32 uiMipLevel /*= 0*/, WUInt32 uiFace /*= 0*/, WUInt32 uiArrayIndex /*= 0*/)
{
  W_PROFILE_SCOPE("WImageUtils::RotateSubImage180");

  WUInt8* start = inout_image.GetPixelPointer<WUInt8>(uiMipLevel, uiFace, uiArrayIndex);
  WUInt8* end = start + inout_image.GetDepthPitch(uiMipLevel);

  WUInt32 bytesPerPixel = WImageFormat::GetBitsPerPixel(inout_image.GetImageFormat()) / 8;

  switch (bytesPerPixel)
  {
    case 4:
      rotate180<WUInt32>(reinterpret_cast<WUInt32*>(start), reinterpret_cast<WUInt32*>(end));
      break;
    case 12:
      rotate180<WVec3>(reinterpret_cast<WVec3*>(start), reinterpret_cast<WVec3*>(end));
      break;
    case 16:
      rotate180<WVec4>(reinterpret_cast<WVec4*>(start), reinterpret_cast<WVec4*>(end));
      break;
    default:
      // fallback version
      {
        end -= bytesPerPixel;
        while (start < end)
        {
          for (WUInt32 i = 0; i < bytesPerPixel; i++)
          {
            WMath::Swap(start[i], end[i]);
          }
          start += bytesPerPixel;
          end -= bytesPerPixel;
        }
      }
  }
}

WResult WImageUtils::Copy(const WImageView& srcImg, const WRectU32& srcRect, WImage& inout_dstImg, const WVec3U32& vDstOffset, WUInt32 uiDstMipLevel /*= 0*/, WUInt32 uiDstFace /*= 0*/, WUInt32 uiDstArrayIndex /*= 0*/)
{
  if (inout_dstImg.GetImageFormat() != srcImg.GetImageFormat())   // Can only copy when the image formats are identical
    return W_FAILURE;

  if (WImageFormat::IsCompressed(inout_dstImg.GetImageFormat())) // Compressed formats are not supported
    return W_FAILURE;

  W_PROFILE_SCOPE("WImageUtils::Copy");

  const WUInt64 uiDstRowPitch = inout_dstImg.GetRowPitch(uiDstMipLevel);
  const WUInt64 uiSrcRowPitch = srcImg.GetRowPitch(uiDstMipLevel);
  const WUInt32 uiCopyBytesPerRow = WImageFormat::GetBitsPerPixel(srcImg.GetImageFormat()) * srcRect.width / 8;

  WUInt8* dstPtr = inout_dstImg.GetPixelPointer<WUInt8>(uiDstMipLevel, uiDstFace, uiDstArrayIndex, vDstOffset.x, vDstOffset.y, vDstOffset.z);
  const WUInt8* srcPtr = srcImg.GetPixelPointer<WUInt8>(0, 0, 0, srcRect.x, srcRect.y);

  for (WUInt32 y = 0; y < srcRect.height; y++)
  {
    WMemoryUtils::Copy(dstPtr, srcPtr, uiCopyBytesPerRow);

    dstPtr += uiDstRowPitch;
    srcPtr += uiSrcRowPitch;
  }

  return W_SUCCESS;
}

WResult WImageUtils::ExtractLowerMipChain(const WImageView& srcImg, WImage& ref_dstImg, WUInt32 uiNumMips)
{
  const WImageHeader& srcImgHeader = srcImg.GetHeader();

  W_PROFILE_SCOPE("WImageUtils::ExtractLowerMipChain");

  uiNumMips = WMath::Min(uiNumMips, srcImgHeader.GetNumMipLevels());

  WUInt32 startMipLevel = srcImgHeader.GetNumMipLevels() - uiNumMips;

  WImageFormat::Enum format = srcImgHeader.GetImageFormat();

  if (WImageFormat::RequiresFirstLevelBlockAlignment(format))
  {
    // Some block compressed image formats require resolutions that are divisible by block size,
    // therefore adjust startMipLevel accordingly
    while (srcImgHeader.GetWidth(startMipLevel) % WImageFormat::GetBlockWidth(format) != 0 || srcImgHeader.GetHeight(startMipLevel) % WImageFormat::GetBlockHeight(format) != 0)
    {
      if (uiNumMips >= srcImgHeader.GetNumMipLevels())
        return W_FAILURE;

      if (startMipLevel == 0)
        return W_FAILURE;

      ++uiNumMips;
      --startMipLevel;
    }
  }

  WImageHeader dstImgHeader = srcImgHeader;
  dstImgHeader.SetWidth(srcImgHeader.GetWidth(startMipLevel));
  dstImgHeader.SetHeight(srcImgHeader.GetHeight(startMipLevel));
  dstImgHeader.SetDepth(srcImgHeader.GetDepth(startMipLevel));
  dstImgHeader.SetNumFaces(srcImgHeader.GetNumFaces());
  dstImgHeader.SetNumArrayIndices(srcImgHeader.GetNumArrayIndices());
  dstImgHeader.SetNumMipLevels(uiNumMips);

  const WUInt32 uiNumFaces = srcImgHeader.GetNumFaces();
  const WUInt32 uiNumArrayIndices = srcImgHeader.GetNumArrayIndices();

  if (uiNumFaces == 1 && uiNumArrayIndices == 1)
  {
    // Fast path: mip levels are contiguous in memory for simple 2D textures.
    const WUInt8* pDataBegin = srcImg.GetPixelPointer<WUInt8>(startMipLevel);
    const WUInt8* pDataEnd = srcImg.GetByteBlobPtr().GetEndPtr();
    const ptrdiff_t dataSize = reinterpret_cast<ptrdiff_t>(pDataEnd) - reinterpret_cast<ptrdiff_t>(pDataBegin);

    const WConstByteBlobPtr lowResData(pDataBegin, static_cast<WUInt64>(dataSize));

    WImageView dataview;
    dataview.ResetAndViewExternalStorage(dstImgHeader, lowResData);

    ref_dstImg.ResetAndCopy(dataview);
  }
  else
  {
    // For array/cube textures, mip levels of different slices are not contiguous,
    // so each sub-image must be copied individually.
    ref_dstImg.ResetAndAlloc(dstImgHeader);

    for (WUInt32 uiArrayIndex = 0; uiArrayIndex < uiNumArrayIndices; ++uiArrayIndex)
    {
      for (WUInt32 uiFace = 0; uiFace < uiNumFaces; ++uiFace)
      {
        for (WUInt32 uiMip = 0; uiMip < uiNumMips; ++uiMip)
        {
          const WUInt32 uiSrcMip = startMipLevel + uiMip;
          const WImageView srcSubImage = srcImg.GetSubImageView(uiSrcMip, uiFace, uiArrayIndex);
          WUInt8* pDst = ref_dstImg.GetPixelPointer<WUInt8>(uiMip, uiFace, uiArrayIndex);
          WMemoryUtils::Copy(pDst, srcSubImage.GetByteBlobPtr().GetPtr(), srcSubImage.GetByteBlobPtr().GetCount());
        }
      }
    }
  }

  return W_SUCCESS;
}

WUInt32 WImageUtils::GetSampleIndex(WUInt32 uiNumTexels, WInt32 iIndex, WImageAddressMode::Enum addressMode, bool& out_bUseBorderColor)
{
  out_bUseBorderColor = false;
  if (WUInt32(iIndex) >= uiNumTexels)
  {
    switch (addressMode)
    {
      case WImageAddressMode::Repeat:
        iIndex %= uiNumTexels;

        if (iIndex < 0)
        {
          iIndex += uiNumTexels;
        }
        return iIndex;

      case WImageAddressMode::Mirror:
      {
        if (iIndex < 0)
        {
          iIndex = -iIndex - 1;
        }
        bool flip = (iIndex / uiNumTexels) & 1;
        iIndex %= uiNumTexels;
        if (flip)
        {
          iIndex = uiNumTexels - iIndex - 1;
        }
        return iIndex;
      }

      case WImageAddressMode::Clamp:
        return WMath::Clamp<WInt32>(iIndex, 0, uiNumTexels - 1);

      case WImageAddressMode::ClampBorder:
        out_bUseBorderColor = true;
        return 0;

      default:
        W_ASSERT_NOT_IMPLEMENTED
        return 0;
    }
  }
  return iIndex;
}

static WSimdVec4f LoadSample(const WSimdVec4f* pSource, WUInt32 uiNumSourceElements, WUInt32 uiStride, WInt32 iIndex, WImageAddressMode::Enum addressMode, const WSimdVec4f& vBorderColor)
{
  bool useBorderColor = false;
  // result is in the range [-(w-1), (w-1)], bring it to [0, w - 1]
  iIndex = WImageUtils::GetSampleIndex(uiNumSourceElements, iIndex, addressMode, useBorderColor);
  if (useBorderColor)
  {
    return vBorderColor;
  }
  return pSource[iIndex * uiStride];
}

inline static void FilterLine(
  WUInt32 uiNumSourceElements, const WSimdVec4f* __restrict pSourceBegin, WSimdVec4f* __restrict pTargetBegin, WUInt32 uiStride, const WImageFilterWeights& weights, WArrayPtr<const WInt32> firstSampleIndices, WImageAddressMode::Enum addressMode, const WSimdVec4f& vBorderColor)
{
  // Convolve the image using the precomputed weights
  const WUInt32 numWeights = weights.GetNumWeights();

  // When the first source index for the output is between 0 and this value,
  // we can fetch all numWeights inputs without taking addressMode into consideration,
  // which makes the inner loop a lot faster.
  const WInt32 trivialSourceIndicesEnd = static_cast<WInt32>(uiNumSourceElements) - static_cast<WInt32>(numWeights);
  const auto weightsView = weights.ViewWeights();
  const float* __restrict nextWeightPtr = weightsView.GetPtr();
  W_ASSERT_DEBUG((static_cast<WUInt32>(weightsView.GetCount()) % numWeights) == 0, "");
  for (WInt32 firstSourceIdx : firstSampleIndices)
  {
    WSimdVec4f total(0.0f, 0.0f, 0.0f, 0.0f);

    if (firstSourceIdx >= 0 && firstSourceIdx < trivialSourceIndicesEnd)
    {
      const auto* __restrict sourcePtr = pSourceBegin + firstSourceIdx * uiStride;
      for (WUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = WSimdVec4f::MulAdd(*sourcePtr, WSimdVec4f(*nextWeightPtr++), total);
        sourcePtr += uiStride;
      }
    }
    else
    {
      // Very slow fallback case that respects the addressMode
      // (not a lot of pixels are taking this path, so it's probably fine)
      WInt32 sourceIdx = firstSourceIdx;
      for (WUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = WSimdVec4f::MulAdd(LoadSample(pSourceBegin, uiNumSourceElements, uiStride, sourceIdx, addressMode, vBorderColor), WSimdVec4f(*nextWeightPtr++), total);
        sourceIdx++;
      }
    }
    // It's ok to check this once per source index, see the assert above
    // (number of weights in weightsView is divisible by numWeights)
    if (nextWeightPtr == weightsView.GetEndPtr())
    {
      nextWeightPtr = weightsView.GetPtr();
    }
    *pTargetBegin = total;
    pTargetBegin += uiStride;
  }
}

static void DownScaleFastLine(WUInt32 uiPixelStride, const WUInt8* pSrc, WUInt8* pDest, WUInt32 uiLengthIn, WUInt32 uiStrideIn, WUInt32 uiLengthOut, WUInt32 uiStrideOut)
{
  const WUInt32 downScaleFactor = uiLengthIn / uiLengthOut;
  W_ASSERT_DEBUG(downScaleFactor >= 1, "Can't upscale");

  const WUInt32 downScaleFactorLog2 = WMath::Log2i(static_cast<WUInt32>(downScaleFactor));
  const WUInt32 roundOffset = downScaleFactor / 2;

  for (WUInt32 offset = 0; offset < uiLengthOut; ++offset)
  {
    for (WUInt32 channel = 0; channel < uiPixelStride; ++channel)
    {
      const WUInt32 destOffset = offset * uiStrideOut + channel;

      WUInt32 curChannel = roundOffset;
      for (WUInt32 index = 0; index < downScaleFactor; ++index)
      {
        curChannel += static_cast<WUInt32>(pSrc[channel + index * uiStrideIn]);
      }

      curChannel = curChannel >> downScaleFactorLog2;
      pDest[destOffset] = static_cast<WUInt8>(curChannel);
    }

    pSrc += downScaleFactor * uiStrideIn;
  }
}

static void DownScaleFast(const WImageView& image, WImage& out_result, WUInt32 uiWidth, WUInt32 uiHeight)
{
  WImageFormat::Enum format = image.GetImageFormat();

  WUInt32 originalWidth = image.GetWidth();
  WUInt32 originalHeight = image.GetHeight();
  WUInt32 numArrayElements = image.GetNumArrayIndices();
  WUInt32 numFaces = image.GetNumFaces();

  WUInt32 pixelStride = WImageFormat::GetBitsPerPixel(format) / 8;

  WImageHeader intermediateHeader;
  intermediateHeader.SetWidth(uiWidth);
  intermediateHeader.SetHeight(originalHeight);
  intermediateHeader.SetNumArrayIndices(numArrayElements);
  intermediateHeader.SetNumFaces(numFaces);
  intermediateHeader.SetImageFormat(format);

  WImage intermediate;
  intermediate.ResetAndAlloc(intermediateHeader);

  for (WUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (WUInt32 face = 0; face < numFaces; face++)
    {
      for (WUInt32 row = 0; row < originalHeight; row++)
      {
        DownScaleFastLine(pixelStride, image.GetPixelPointer<WUInt8>(0, face, arrayIndex, 0, row), intermediate.GetPixelPointer<WUInt8>(0, face, arrayIndex, 0, row), originalWidth, pixelStride, uiWidth, pixelStride);
      }
    }
  }

  // input and output images may be the same, so we can't access the original image below this point

  WImageHeader outHeader;
  outHeader.SetWidth(uiWidth);
  outHeader.SetHeight(uiHeight);
  outHeader.SetNumArrayIndices(numArrayElements);
  outHeader.SetNumArrayIndices(numFaces);
  outHeader.SetImageFormat(format);

  out_result.ResetAndAlloc(outHeader);

  W_ASSERT_DEBUG(intermediate.GetRowPitch() < WMath::MaxValue<WUInt32>(), "Row pitch exceeds WUInt32 max value.");
  W_ASSERT_DEBUG(out_result.GetRowPitch() < WMath::MaxValue<WUInt32>(), "Row pitch exceeds WUInt32 max value.");

  for (WUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (WUInt32 face = 0; face < numFaces; face++)
    {
      for (WUInt32 col = 0; col < uiWidth; col++)
      {
        DownScaleFastLine(pixelStride, intermediate.GetPixelPointer<WUInt8>(0, face, arrayIndex, col), out_result.GetPixelPointer<WUInt8>(0, face, arrayIndex, col), originalHeight, static_cast<WUInt32>(intermediate.GetRowPitch()), uiHeight, static_cast<WUInt32>(out_result.GetRowPitch()));
      }
    }
  }
}

static float EvaluateAverageCoverage(WBlobPtr<const WColor> colors, float fAlphaThreshold)
{
  W_PROFILE_SCOPE("EvaluateAverageCoverage");

  WUInt64 totalPixels = colors.GetCount();
  WUInt64 count = 0;
  for (WUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    count += colors[idx].a >= fAlphaThreshold;
  }

  return float(count) / float(totalPixels);
}

static void NormalizeCoverage(WImage& inout_currentMip, const WImageHeader& fullImageHeader, const WImageUtils::MipMapOptions& mipOptions, float fTargetCoverage)
{
  W_PROFILE_SCOPE("NormalizeCoverage");

  // Based on the idea in http://the-witness.net/news/2010/09/computing-alpha-mipmaps/. Note we're using a histogram
  // to find the new alpha threshold here rather than bisecting.

  // Early out for very small mips since the algorithm produces unpredictable results for them
  if (inout_currentMip.GetWidth() <= 2 || inout_currentMip.GetHeight() <= 2)
  {
    return;
  }

  // First bilinear upscale to original resolution
  WImage upscaled;
  WImageUtils::Scale(inout_currentMip, upscaled, fullImageHeader.GetWidth(), fullImageHeader.GetHeight(), nullptr, mipOptions.m_addressModeU, mipOptions.m_addressModeV).IgnoreResult();

  auto upscaledColors = upscaled.GetBlobPtr<WColor>();

  // Generate histogram of alpha values
  WUInt64 totalPixels = upscaledColors.GetCount();

  constexpr WUInt32 histogramBits = 8;
  constexpr WUInt32 histogramSize = 1 << histogramBits;
  WUInt32 alphaHistogram[histogramSize] = {};
  for (WUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    alphaHistogram[WMath::ColorFloatToUnsignedInt<histogramBits>(upscaledColors[idx].a)]++;
  }

  // Find a new alpha threshold so the number of covered pixels matches by summing up the histogram
  WInt32 targetCount = WInt32(fTargetCoverage * totalPixels);
  WInt32 coverageCount = 0;
  WInt32 newThreshold = histogramSize - 1;
  for (; newThreshold >= 0; newThreshold--)
  {
    coverageCount += alphaHistogram[newThreshold];

    if (coverageCount >= targetCount)
    {
      break;
    }
  }

  // Rescale alpha values
  auto colors = inout_currentMip.GetBlobPtr<WColor>();

  const float fNewThreshold = float(newThreshold) / float(histogramSize - 1);
  const float alphaScale = mipOptions.m_alphaThreshold / fNewThreshold;
  for (WUInt64 idx = 0; idx < colors.GetCount(); ++idx)
  {
    colors[idx].a *= alphaScale;
  }
}


WResult WImageUtils::Scale(const WImageView& source, WImage& ref_target, WUInt32 uiWidth, WUInt32 uiHeight, const WImageFilter* pFilter, WImageAddressMode::Enum addressModeU, WImageAddressMode::Enum addressModeV, const WColor& borderColor)
{
  return Scale3D(source, ref_target, uiWidth, uiHeight, 1, pFilter, addressModeU, addressModeV, WImageAddressMode::Clamp, borderColor);
}

WResult WImageUtils::Scale3D(const WImageView& source, WImage& ref_target, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiDepth, const WImageFilter* pFilter /*= W_NULL*/, WImageAddressMode::Enum addressModeU /*= WImageAddressMode::Clamp*/,
  WImageAddressMode::Enum addressModeV /*= WImageAddressMode::Clamp*/, WImageAddressMode::Enum addressModeW /*= WImageAddressMode::Clamp*/, const WColor& borderColor /*= WColors::Black*/)
{
  W_PROFILE_SCOPE("WImageUtils::Scale3D");

  if (uiWidth == 0 || uiHeight == 0 || uiDepth == 0)
  {
    WImageHeader header;
    header.SetImageFormat(source.GetImageFormat());
    ref_target.ResetAndAlloc(header);
    return W_SUCCESS;
  }

  const WImageFormat::Enum format = source.GetImageFormat();

  const WUInt32 originalWidth = source.GetWidth();
  const WUInt32 originalHeight = source.GetHeight();
  const WUInt32 originalDepth = source.GetDepth();
  const WUInt32 numFaces = source.GetNumFaces();
  const WUInt32 numArrayElements = source.GetNumArrayIndices();

  if (originalWidth == uiWidth && originalHeight == uiHeight && originalDepth == uiDepth)
  {
    ref_target.ResetAndCopy(source);
    return W_SUCCESS;
  }

  // Scaling down by an even factor?
  const WUInt32 downScaleFactorX = originalWidth / uiWidth;
  const WUInt32 downScaleFactorY = originalHeight / uiHeight;

  if (pFilter == nullptr && (format == WImageFormat::R8G8B8A8_UNORM || format == WImageFormat::B8G8R8A8_UNORM || format == WImageFormat::B8G8R8_UNORM) && downScaleFactorX * uiWidth == originalWidth && downScaleFactorY * uiHeight == originalHeight && uiDepth == 1 && originalDepth == 1 &&
      WMath::IsPowerOf2(downScaleFactorX) && WMath::IsPowerOf2(downScaleFactorY))
  {
    DownScaleFast(source, ref_target, uiWidth, uiHeight);
    return W_SUCCESS;
  }

  // Fallback to default filter
  WImageFilterTriangle defaultFilter;
  if (!pFilter)
  {
    pFilter = &defaultFilter;
  }

  const WImageView* stepSource;

  // Manage scratch images for intermediate conversion or filtering
  const WUInt32 maxNumScratchImages = 2;
  WImage scratch[maxNumScratchImages];
  bool scratchUsed[maxNumScratchImages] = {};
  auto allocateScratch = [&]() -> WImage&
  {
    for (WUInt32 i = 0;; ++i)
    {
      W_ASSERT_DEV(i < maxNumScratchImages, "Failed to allocate scratch image");
      if (!scratchUsed[i])
      {
        scratchUsed[i] = true;
        return scratch[i];
      }
    }
  };
  auto releaseScratch = [&](const WImageView& image)
  {
    for (WUInt32 i = 0; i < maxNumScratchImages; ++i)
    {
      if (&scratch[i] == &image)
      {
        scratchUsed[i] = false;
        return;
      }
    }
  };

  if (format == WImageFormat::R32G32B32A32_FLOAT)
  {
    stepSource = &source;
  }
  else
  {
    WImage& conversionScratch = allocateScratch();
    if (WImageConversion::Convert(source, conversionScratch, WImageFormat::R32G32B32A32_FLOAT).Failed())
    {
      return W_FAILURE;
    }

    stepSource = &conversionScratch;
  };

  WTempHybridArray<WInt32, 256> firstSampleIndices;
  firstSampleIndices.Reserve(WMath::Max(uiWidth, uiHeight, uiDepth));

  if (uiWidth != originalWidth)
  {
    WImageFilterWeights weights(*pFilter, originalWidth, uiWidth);
    firstSampleIndices.SetCountUninitialized(uiWidth);
    for (WUInt32 x = 0; x < uiWidth; ++x)
    {
      firstSampleIndices[x] = weights.GetFirstSourceSampleIndex(x);
    }

    WImage* stepTarget;
    if (uiHeight == originalHeight && uiDepth == originalDepth && format == WImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    WImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetWidth(uiWidth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (WUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (WUInt32 face = 0; face < numFaces; ++face)
      {
        for (WUInt32 z = 0; z < originalDepth; ++z)
        {
          for (WUInt32 y = 0; y < originalHeight; ++y)
          {
            const WSimdVec4f* filterSource = stepSource->GetPixelPointer<WSimdVec4f>(0, face, arrayIndex, 0, y, z);
            WSimdVec4f* filterTarget = stepTarget->GetPixelPointer<WSimdVec4f>(0, face, arrayIndex, 0, y, z);
            FilterLine(originalWidth, filterSource, filterTarget, 1, weights, firstSampleIndices, addressModeU, WSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (uiHeight != originalHeight)
  {
    WImageFilterWeights weights(*pFilter, originalHeight, uiHeight);
    firstSampleIndices.SetCount(uiHeight);
    for (WUInt32 y = 0; y < uiHeight; ++y)
    {
      firstSampleIndices[y] = weights.GetFirstSourceSampleIndex(y);
    }

    WImage* stepTarget;
    if (uiDepth == originalDepth && format == WImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    WImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetHeight(uiHeight);
    stepTarget->ResetAndAlloc(stepHeader);

    for (WUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (WUInt32 face = 0; face < numFaces; ++face)
      {
        for (WUInt32 z = 0; z < originalDepth; ++z)
        {
          for (WUInt32 x = 0; x < uiWidth; ++x)
          {
            const WSimdVec4f* filterSource = stepSource->GetPixelPointer<WSimdVec4f>(0, face, arrayIndex, x, 0, z);
            WSimdVec4f* filterTarget = stepTarget->GetPixelPointer<WSimdVec4f>(0, face, arrayIndex, x, 0, z);
            FilterLine(originalHeight, filterSource, filterTarget, uiWidth, weights, firstSampleIndices, addressModeV, WSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (uiDepth != originalDepth)
  {
    WImageFilterWeights weights(*pFilter, originalDepth, uiDepth);
    firstSampleIndices.SetCount(uiDepth);
    for (WUInt32 z = 0; z < uiDepth; ++z)
    {
      firstSampleIndices[z] = weights.GetFirstSourceSampleIndex(z);
    }

    WImage* stepTarget;
    if (format == WImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    WImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetDepth(uiDepth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (WUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (WUInt32 face = 0; face < numFaces; ++face)
      {
        for (WUInt32 y = 0; y < uiHeight; ++y)
        {
          for (WUInt32 x = 0; x < uiWidth; ++x)
          {
            const WSimdVec4f* filterSource = stepSource->GetPixelPointer<WSimdVec4f>(0, face, arrayIndex, x, y, 0);
            WSimdVec4f* filterTarget = stepTarget->GetPixelPointer<WSimdVec4f>(0, face, arrayIndex, x, y, 0);
            FilterLine(originalHeight, filterSource, filterTarget, uiWidth * uiHeight, weights, firstSampleIndices, addressModeW, WSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  // Convert back to original format - no-op if stepSource and target are the same
  return WImageConversion::Convert(*stepSource, ref_target, format);
}

void WImageUtils::GenerateMipMaps(const WImageView& source, WImage& ref_target, const MipMapOptions& options)
{
  W_PROFILE_SCOPE("WImageUtils::GenerateMipMaps");

  WImageHeader header = source.GetHeader();
  W_ASSERT_DEV(header.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT, "The source image must be a RGBA 32-bit float format.");
  W_ASSERT_DEV(&source != &ref_target, "Source and target must not be the same image.");

  // Make a local copy to be able to tweak some of the options
  WImageUtils::MipMapOptions mipMapOptions = options;

  // alpha thresholds with extreme values are not supported at the moment
  mipMapOptions.m_alphaThreshold = WMath::Clamp(mipMapOptions.m_alphaThreshold, 0.05f, 0.95f);

  // Enforce CLAMP addressing mode for cubemaps
  if (source.GetNumFaces() == 6)
  {
    mipMapOptions.m_addressModeU = WImageAddressMode::Clamp;
    mipMapOptions.m_addressModeV = WImageAddressMode::Clamp;
  }

  WUInt32 numMipMaps = header.ComputeNumberOfMipMaps();
  if (mipMapOptions.m_numMipMaps > 0 && mipMapOptions.m_numMipMaps < numMipMaps)
  {
    numMipMaps = mipMapOptions.m_numMipMaps;
  }
  header.SetNumMipLevels(numMipMaps);

  ref_target.ResetAndAlloc(header);

  for (WUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (WUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      WImageHeader currentMipMapHeader = header;
      currentMipMapHeader.SetNumMipLevels(1);
      currentMipMapHeader.SetNumFaces(1);
      currentMipMapHeader.SetNumArrayIndices(1);

      auto sourceView = source.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();
      auto targetView = ref_target.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();

      memcpy(targetView.GetPtr(), sourceView.GetPtr(), static_cast<size_t>(targetView.GetCount()));

      float targetCoverage = 0.0f;
      if (mipMapOptions.m_preserveCoverage)
      {
        targetCoverage = EvaluateAverageCoverage(source.GetSubImageView(0, face, arrayIndex).GetBlobPtr<WColor>(), mipMapOptions.m_alphaThreshold);
      }

      for (WUInt32 mipMapLevel = 0; mipMapLevel < numMipMaps - 1; mipMapLevel++)
      {
        WImageHeader nextMipMapHeader = currentMipMapHeader;
        nextMipMapHeader.SetWidth(WMath::Max(1u, nextMipMapHeader.GetWidth() / 2));
        nextMipMapHeader.SetHeight(WMath::Max(1u, nextMipMapHeader.GetHeight() / 2));
        nextMipMapHeader.SetDepth(WMath::Max(1u, nextMipMapHeader.GetDepth() / 2));

        auto sourceData = ref_target.GetSubImageView(mipMapLevel, face, arrayIndex).GetByteBlobPtr();
        WImage currentMipMap;
        currentMipMap.ResetAndUseExternalStorage(currentMipMapHeader, sourceData);

        auto dstData = ref_target.GetSubImageView(mipMapLevel + 1, face, arrayIndex).GetByteBlobPtr();
        WImage nextMipMap;
        nextMipMap.ResetAndUseExternalStorage(nextMipMapHeader, dstData);

        WImageUtils::Scale3D(currentMipMap, nextMipMap, nextMipMapHeader.GetWidth(), nextMipMapHeader.GetHeight(), nextMipMapHeader.GetDepth(), mipMapOptions.m_filter, mipMapOptions.m_addressModeU, mipMapOptions.m_addressModeV, mipMapOptions.m_addressModeW, mipMapOptions.m_borderColor)
          .IgnoreResult();

        if (mipMapOptions.m_preserveCoverage)
        {
          NormalizeCoverage(nextMipMap, header, mipMapOptions, targetCoverage);
        }

        if (mipMapOptions.m_renormalizeNormals)
        {
          RenormalizeNormalMap(nextMipMap);
        }

        currentMipMapHeader = nextMipMapHeader;
      }
    }
  }
}

void WImageUtils::ReconstructNormalZ(WImage& ref_image)
{
  W_PROFILE_SCOPE("WImageUtils::ReconstructNormalZ");

  W_ASSERT_DEV(ref_image.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  WSimdVec4f* cur = ref_image.GetBlobPtr<WSimdVec4f>().GetPtr();
  WSimdVec4f* const end = ref_image.GetBlobPtr<WSimdVec4f>().GetEndPtr();

  WSimdFloat oneScalar = 1.0f;

  WSimdVec4f two(2.0f);

  WSimdVec4f minusOne(-1.0f);

  WSimdVec4f half(0.5f);

  for (; cur < end; cur++)
  {
    WSimdVec4f normal;
    // unpack from [0,1] to [-1, 1]
    normal = WSimdVec4f::MulAdd(*cur, two, minusOne);

    // compute Z component
    normal.SetZ((oneScalar - normal.Dot<2>(normal)).GetSqrt());

    // pack back to [0,1]
    *cur = WSimdVec4f::MulAdd(half, normal, half);
  }
}

void WImageUtils::RenormalizeNormalMap(WImage& ref_image)
{
  W_PROFILE_SCOPE("WImageUtils::RenormalizeNormalMap");

  W_ASSERT_DEV(ref_image.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  WSimdVec4f* start = ref_image.GetBlobPtr<WSimdVec4f>().GetPtr();
  WSimdVec4f* const end = ref_image.GetBlobPtr<WSimdVec4f>().GetEndPtr();

  WSimdVec4f two(2.0f);

  WSimdVec4f minusOne(-1.0f);

  WSimdVec4f half(0.5f);

  for (; start < end; start++)
  {
    WSimdVec4f normal;
    normal = WSimdVec4f::MulAdd(*start, two, minusOne);
    normal.Normalize<3>();
    *start = WSimdVec4f::MulAdd(half, normal, half);
  }
}

void WImageUtils::AdjustRoughness(WImage& ref_roughnessMap, const WImageView& normalMap)
{
  W_PROFILE_SCOPE("WImageUtils::AdjustRoughness");

  W_ASSERT_DEV(ref_roughnessMap.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");
  W_ASSERT_DEV(normalMap.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  W_ASSERT_DEV(ref_roughnessMap.GetWidth() >= normalMap.GetWidth() && ref_roughnessMap.GetHeight() >= normalMap.GetHeight(), "The roughness map needs to be bigger or same size than the normal map.");

  WImage filteredNormalMap;
  WImageUtils::MipMapOptions options;

  // Box filter normal map without re-normalization so we have the average normal length in each mip map.
  if (ref_roughnessMap.GetWidth() != normalMap.GetWidth() || ref_roughnessMap.GetHeight() != normalMap.GetHeight())
  {
    WImage temp;
    WImageUtils::Scale(normalMap, temp, ref_roughnessMap.GetWidth(), ref_roughnessMap.GetHeight()).IgnoreResult();
    WImageUtils::RenormalizeNormalMap(temp);
    WImageUtils::GenerateMipMaps(temp, filteredNormalMap, options);
  }
  else
  {
    WImageUtils::GenerateMipMaps(normalMap, filteredNormalMap, options);
  }

  W_ASSERT_DEV(ref_roughnessMap.GetNumMipLevels() == filteredNormalMap.GetNumMipLevels(), "Roughness and normal map must have the same number of mip maps");

  WSimdVec4f two(2.0f);
  WSimdVec4f minusOne(-1.0f);

  WUInt32 numMipLevels = ref_roughnessMap.GetNumMipLevels();
  for (WUInt32 mipLevel = 1; mipLevel < numMipLevels; ++mipLevel)
  {
    WBlobPtr<WSimdVec4f> roughnessData = ref_roughnessMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<WSimdVec4f>();
    WBlobPtr<WSimdVec4f> normalData = filteredNormalMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<WSimdVec4f>();

    for (WUInt64 i = 0; i < roughnessData.GetCount(); ++i)
    {
      WSimdVec4f normal = WSimdVec4f::MulAdd(normalData[i], two, minusOne);

      float avgNormalLength = normal.GetLength<3>();
      if (avgNormalLength < 1.0f)
      {
        float avgNormalLengthSquare = avgNormalLength * avgNormalLength;
        float kappa = (3.0f * avgNormalLength - avgNormalLength * avgNormalLengthSquare) / (1.0f - avgNormalLengthSquare);
        float variance = 1.0f / (2.0f * kappa);

        float oldRoughness = roughnessData[i].GetComponent<0>();
        float newRoughness = WMath::Sqrt(oldRoughness * oldRoughness + variance);

        roughnessData[i].Set(newRoughness);
      }
    }
  }
}

void WImageUtils::ChangeExposure(WImage& ref_image, float fBias)
{
  W_ASSERT_DEV(ref_image.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT, "This function expects an RGBA 32 float image as input");

  if (fBias == 0.0f)
    return;

  W_PROFILE_SCOPE("WImageUtils::ChangeExposure");

  const float multiplier = WMath::Pow2(fBias);

  for (WColor& col : ref_image.GetBlobPtr<WColor>())
  {
    col = multiplier * col;
  }
}

static WResult CopyImageRectToFace(WImage& ref_dstImg, const WImageView& srcImg, WUInt32 uiOffsetX, WUInt32 uiOffsetY, WUInt32 uiFaceIndex)
{
  WRectU32 r;
  r.x = uiOffsetX;
  r.y = uiOffsetY;
  r.width = ref_dstImg.GetWidth();
  r.height = r.width;

  return WImageUtils::Copy(srcImg, r, ref_dstImg, WVec3U32(0), 0, uiFaceIndex);
}

WResult WImageUtils::CreateCubemapFromSingleFile(WImage& ref_dstImg, const WImageView& srcImg)
{
  W_PROFILE_SCOPE("WImageUtils::CreateCubemapFromSingleFile");

  if (srcImg.GetNumFaces() == 6)
  {
    ref_dstImg.ResetAndCopy(srcImg);
    return W_SUCCESS;
  }
  else if (srcImg.GetNumFaces() == 1)
  {
    if (srcImg.GetWidth() % 3 == 0 && srcImg.GetHeight() % 4 == 0 && srcImg.GetWidth() / 3 == srcImg.GetHeight() / 4)
    {
      // Vertical cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+
      // | X-| Z+| X+|
      // +---+---+---+
      //     | Y-|
      //     +---+
      //     | Z-|
      //     +---+
      const WUInt32 faceSize = srcImg.GetWidth() / 3;

      WImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 3, 5));
      WImageUtils::RotateSubImage180(ref_dstImg, 0, 5);
    }
    else if (srcImg.GetWidth() % 4 == 0 && srcImg.GetHeight() % 3 == 0 && srcImg.GetWidth() / 4 == srcImg.GetHeight() / 3)
    {
      // Horizontal cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+---+
      // | X-| Z+| X+| Z-|
      // +---+---+---+---+
      //     | Y-|
      //     +---+
      const WUInt32 faceSize = srcImg.GetWidth() / 4;

      WImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 3, faceSize, 5));
    }
    else
    {
      // Spherical mapping
      if (srcImg.GetWidth() % 4 != 0)
      {
        WLog::Error("Width of the input image should be a multiple of 4");
        return W_FAILURE;
      }

      const WUInt32 faceSize = srcImg.GetWidth() / 4;

      WImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // Corners of the UV space for the respective faces in model space
      const WVec3 faceCorners[] = {
        WVec3(0.5, 0.5, 0.5),   // X+
        WVec3(-0.5, 0.5, -0.5), // X-
        WVec3(-0.5, 0.5, -0.5), // Y+
        WVec3(-0.5, -0.5, 0.5), // Y-
        WVec3(-0.5, 0.5, 0.5),  // Z+
        WVec3(0.5, 0.5, -0.5)   // Z-
      };

      // UV Axis of the respective faces in model space
      const WVec3 faceAxis[] = {
        WVec3(0, 0, -1), WVec3(0, -1, 0), // X+
        WVec3(0, 0, 1), WVec3(0, -1, 0),  // X-
        WVec3(1, 0, 0), WVec3(0, 0, 1),   // Y+
        WVec3(1, 0, 0), WVec3(0, 0, -1),  // Y-
        WVec3(1, 0, 0), WVec3(0, -1, 0),  // Z+
        WVec3(-1, 0, 0), WVec3(0, -1, 0)  // Z-
      };

      const float fFaceSize = (float)faceSize;
      const float fHalfPixel = 0.5f / fFaceSize;
      const float fPixel = 1.0f / fFaceSize;

      const float fHalfSrcWidth = srcImg.GetWidth() / 2.0f;
      const float fSrcHeight = (float)srcImg.GetHeight();

      const WUInt32 srcWidthMinus1 = srcImg.GetWidth() - 1;
      const WUInt32 srcHeightMinus1 = srcImg.GetHeight() - 1;

      W_ASSERT_DEBUG(srcImg.GetRowPitch() % sizeof(WColor) == 0, "Row pitch should be a multiple of sizeof(WColor)");
      const WUInt64 srcRowPitch = srcImg.GetRowPitch() / sizeof(WColor);

      W_ASSERT_DEBUG(ref_dstImg.GetRowPitch() % sizeof(WColor) == 0, "Row pitch should be a multiple of sizeof(WColor)");
      const WUInt64 faceRowPitch = ref_dstImg.GetRowPitch() / sizeof(WColor);

      const WColor* srcData = srcImg.GetPixelPointer<WColor>();
      const float InvPi = 1.0f / WMath::Pi<float>();

      for (WUInt32 faceIndex = 0; faceIndex < 6; faceIndex++)
      {
        WColor* faceData = ref_dstImg.GetPixelPointer<WColor>(0, faceIndex);
        for (WUInt32 y = 0; y < faceSize; y++)
        {
          const float dstV = (float)y * fPixel + fHalfPixel;

          for (WUInt32 x = 0; x < faceSize; x++)
          {
            const float dstU = (float)x * fPixel + fHalfPixel;
            const WVec3 modelSpacePos = faceCorners[faceIndex] + dstU * faceAxis[faceIndex * 2] + dstV * faceAxis[faceIndex * 2 + 1];
            const WVec3 modelSpaceDir = modelSpacePos.GetNormalized();

            const float phi = WMath::ATan2(modelSpaceDir.x, modelSpaceDir.z).GetRadian() + WMath::Pi<float>();
            const float r = WMath::Sqrt(modelSpaceDir.x * modelSpaceDir.x + modelSpaceDir.z * modelSpaceDir.z);
            const float theta = WMath::ATan2(modelSpaceDir.y, r).GetRadian() + WMath::Pi<float>() * 0.5f;

            W_ASSERT_DEBUG(phi >= 0.0f && phi <= 2.0f * WMath::Pi<float>(), "");
            W_ASSERT_DEBUG(theta >= 0.0f && theta <= WMath::Pi<float>(), "");

            const float srcU = phi * InvPi * fHalfSrcWidth;
            const float srcV = (1.0f - theta * InvPi) * fSrcHeight;

            WUInt32 x1 = (WUInt32)WMath::Floor(srcU);
            WUInt32 x2 = x1 + 1;
            WUInt32 y1 = (WUInt32)WMath::Floor(srcV);
            WUInt32 y2 = y1 + 1;

            const float fracX = srcU - x1;
            const float fracY = srcV - y1;

            x1 = WMath::Clamp(x1, 0u, srcWidthMinus1);
            x2 = WMath::Clamp(x2, 0u, srcWidthMinus1);
            y1 = WMath::Clamp(y1, 0u, srcHeightMinus1);
            y2 = WMath::Clamp(y2, 0u, srcHeightMinus1);

            WColor A = srcData[x1 + y1 * srcRowPitch];
            WColor B = srcData[x2 + y1 * srcRowPitch];
            WColor C = srcData[x1 + y2 * srcRowPitch];
            WColor D = srcData[x2 + y2 * srcRowPitch];

            WColor interpolated = A * (1 - fracX) * (1 - fracY) + B * (fracX) * (1 - fracY) + C * (1 - fracX) * fracY + D * fracX * fracY;
            faceData[x + y * faceRowPitch] = interpolated;
          }
        }
      }
    }

    return W_SUCCESS;
  }

  WLog::Error("Unexpected number of faces in cubemap input image.");
  return W_FAILURE;
}

WResult WImageUtils::CreateCubemapFrom6Files(WImage& ref_dstImg, const WImageView* pSourceImages)
{
  W_PROFILE_SCOPE("WImageUtils::CreateCubemapFrom6Files");

  WImageHeader header = pSourceImages[0].GetHeader();
  header.SetNumFaces(6);

  if (header.GetWidth() != header.GetHeight())
    return W_FAILURE;

  if (!WMath::IsPowerOf2(header.GetWidth()))
    return W_FAILURE;

  ref_dstImg.ResetAndAlloc(header);

  for (WUInt32 i = 0; i < 6; ++i)
  {
    if (pSourceImages[i].GetImageFormat() != ref_dstImg.GetImageFormat())
      return W_FAILURE;

    if (pSourceImages[i].GetWidth() != ref_dstImg.GetWidth())
      return W_FAILURE;

    if (pSourceImages[i].GetHeight() != ref_dstImg.GetHeight())
      return W_FAILURE;

    W_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, pSourceImages[i], 0, 0, i));
  }

  return W_SUCCESS;
}

WResult WImageUtils::CreateVolumeTextureFromSingleFile(WImage& ref_dstImg, const WImageView& srcImg)
{
  W_PROFILE_SCOPE("WImageUtils::CreateVolumeTextureFromSingleFile");

  const WUInt32 uiWidthHeight = srcImg.GetHeight();
  const WUInt32 uiDepth = srcImg.GetWidth() / uiWidthHeight;

  if (!WMath::IsPowerOf2(uiWidthHeight))
    return W_FAILURE;
  if (!WMath::IsPowerOf2(uiDepth))
    return W_FAILURE;

  WImageHeader header;
  header.SetWidth(uiWidthHeight);
  header.SetHeight(uiWidthHeight);
  header.SetDepth(uiDepth);
  header.SetImageFormat(srcImg.GetImageFormat());

  ref_dstImg.ResetAndAlloc(header);

  const WImageView view = srcImg.GetSubImageView();

  for (WUInt32 d = 0; d < uiDepth; ++d)
  {
    WRectU32 r;
    r.x = uiWidthHeight * d;
    r.y = 0;
    r.width = uiWidthHeight;
    r.height = uiWidthHeight;

    W_SUCCEED_OR_RETURN(Copy(view, r, ref_dstImg, WVec3U32(0, 0, d)));
  }

  return W_SUCCESS;
}

WColor WImageUtils::NearestSample(const WImageView& image, WImageAddressMode::Enum addressMode, WVec2 vUv)
{
  W_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  W_ASSERT_DEBUG(image.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return NearestSample(image.GetPixelPointer<WColor>(), image.GetWidth(), image.GetHeight(), addressMode, vUv);
}

WColor WImageUtils::NearestSample(const WColor* pPixelPointer, WUInt32 uiWidth, WUInt32 uiHeight, WImageAddressMode::Enum addressMode, WVec2 vUv)
{
  const WInt32 w = uiWidth;
  const WInt32 h = uiHeight;

  vUv = vUv.CompMul(WVec2(static_cast<float>(w), static_cast<float>(h)));
  const WInt32 intX = (WInt32)WMath::Floor(vUv.x);
  const WInt32 intY = (WInt32)WMath::Floor(vUv.y);

  WInt32 x = intX;
  WInt32 y = intY;

  if (addressMode == WImageAddressMode::Clamp)
  {
    x = WMath::Clamp(x, 0, w - 1);
    y = WMath::Clamp(y, 0, h - 1);
  }
  else if (addressMode == WImageAddressMode::Repeat)
  {
    x = x % w;
    x = x < 0 ? x + w : x;
    y = y % h;
    y = y < 0 ? y + h : y;
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }

  return *(pPixelPointer + (y * w) + x);
}

WColor WImageUtils::BilinearSample(const WImageView& image, WImageAddressMode::Enum addressMode, WVec2 vUv)
{
  W_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  W_ASSERT_DEBUG(image.GetImageFormat() == WImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return BilinearSample(image.GetPixelPointer<WColor>(), image.GetWidth(), image.GetHeight(), addressMode, vUv);
}

WColor WImageUtils::BilinearSample(const WColor* pData, WUInt32 uiWidth, WUInt32 uiHeight, WImageAddressMode::Enum addressMode, WVec2 vUv)
{
  WInt32 w = uiWidth;
  WInt32 h = uiHeight;

  vUv = vUv.CompMul(WVec2(static_cast<float>(w), static_cast<float>(h))) - WVec2(0.5f);
  const float floorX = WMath::Floor(vUv.x);
  const float floorY = WMath::Floor(vUv.y);
  const float fractionX = vUv.x - floorX;
  const float fractionY = vUv.y - floorY;
  const WInt32 intX = (WInt32)floorX;
  const WInt32 intY = (WInt32)floorY;

  WColor c[4];
  for (WUInt32 i = 0; i < 4; ++i)
  {
    WInt32 x = intX + (i % 2);
    WInt32 y = intY + (i / 2);

    if (addressMode == WImageAddressMode::Clamp)
    {
      x = WMath::Clamp(x, 0, w - 1);
      y = WMath::Clamp(y, 0, h - 1);
    }
    else if (addressMode == WImageAddressMode::Repeat)
    {
      x = x % w;
      x = x < 0 ? x + w : x;
      y = y % h;
      y = y < 0 ? y + h : y;
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }

    c[i] = *(pData + (y * w) + x);
  }

  const WColor cr0 = WMath::Lerp(c[0], c[1], fractionX);
  const WColor cr1 = WMath::Lerp(c[2], c[3], fractionX);

  return WMath::Lerp(cr0, cr1, fractionY);
}

namespace
{
  template <typename SrcType, typename DstType, WUInt8 SrcStride, WUInt8 DstStride>
  void CopyChannelLoop(const SrcType* pSrc, DstType* pDst, WUInt32 uiNumPixels)
  {
    for (WUInt32 i = 0; i < uiNumPixels; ++i)
    {
      *pDst = static_cast<DstType>(*pSrc);
      pSrc += SrcStride;
      pDst += DstStride;
    }
  }

  template <typename SrcType, typename DstType>
  void CopyChannelImpl(const SrcType* pSrc, WUInt8 uiSrcStride, DstType* pDst, WUInt8 uiDstStride, WUInt32 uiNumPixels)
  {
    // Encode both strides into a single value: src in the upper nibble, dst in the lower.
    // All 16 combinations of stride 1-4 are spelled out so the compiler sees compile-time constants.
    const WUInt8 uiKey = (uiSrcStride << 4) | uiDstStride;

    // clang-format off
    switch (uiKey)
    {
      case 0x11: CopyChannelLoop<SrcType, DstType, 1, 1>(pSrc, pDst, uiNumPixels); break;
      case 0x12: CopyChannelLoop<SrcType, DstType, 1, 2>(pSrc, pDst, uiNumPixels); break;
      case 0x13: CopyChannelLoop<SrcType, DstType, 1, 3>(pSrc, pDst, uiNumPixels); break;
      case 0x14: CopyChannelLoop<SrcType, DstType, 1, 4>(pSrc, pDst, uiNumPixels); break;
      case 0x21: CopyChannelLoop<SrcType, DstType, 2, 1>(pSrc, pDst, uiNumPixels); break;
      case 0x22: CopyChannelLoop<SrcType, DstType, 2, 2>(pSrc, pDst, uiNumPixels); break;
      case 0x23: CopyChannelLoop<SrcType, DstType, 2, 3>(pSrc, pDst, uiNumPixels); break;
      case 0x24: CopyChannelLoop<SrcType, DstType, 2, 4>(pSrc, pDst, uiNumPixels); break;
      case 0x31: CopyChannelLoop<SrcType, DstType, 3, 1>(pSrc, pDst, uiNumPixels); break;
      case 0x32: CopyChannelLoop<SrcType, DstType, 3, 2>(pSrc, pDst, uiNumPixels); break;
      case 0x33: CopyChannelLoop<SrcType, DstType, 3, 3>(pSrc, pDst, uiNumPixels); break;
      case 0x34: CopyChannelLoop<SrcType, DstType, 3, 4>(pSrc, pDst, uiNumPixels); break;
      case 0x41: CopyChannelLoop<SrcType, DstType, 4, 1>(pSrc, pDst, uiNumPixels); break;
      case 0x42: CopyChannelLoop<SrcType, DstType, 4, 2>(pSrc, pDst, uiNumPixels); break;
      case 0x43: CopyChannelLoop<SrcType, DstType, 4, 3>(pSrc, pDst, uiNumPixels); break;
      case 0x44: CopyChannelLoop<SrcType, DstType, 4, 4>(pSrc, pDst, uiNumPixels); break;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
        break;
    }
    // clang-format on
  }
} // namespace

WResult WImageUtils::CopyChannel(WImage& ref_dstImg, WUInt8 uiDstChannelIdx, const WImage& srcImg, WUInt8 uiSrcChannelIdx)
{
  W_PROFILE_SCOPE("WImageUtils::CopyChannel");

  if (srcImg.GetWidth() != ref_dstImg.GetWidth())
    return W_FAILURE;

  if (srcImg.GetHeight() != ref_dstImg.GetHeight())
    return W_FAILURE;

  const WImageFormat::Enum srcFormat = srcImg.GetImageFormat();
  const WImageFormat::Enum dstFormat = ref_dstImg.GetImageFormat();
  const WUInt8 uiSrcChannels = static_cast<WUInt8>(WImageFormat::GetNumChannels(srcFormat));
  const WUInt8 uiDstChannels = static_cast<WUInt8>(WImageFormat::GetNumChannels(dstFormat));
  const WImageFormatType::Enum srcType = WImageFormat::GetType(srcFormat);
  const WImageFormatType::Enum dstType = WImageFormat::GetType(dstFormat);
  if (srcType != dstType || srcType != WImageFormatType::LINEAR)
    return W_FAILURE;

  const WImageFormatDataType::Enum srcDataType = WImageFormat::GetDataType(srcFormat);
  const WImageFormatDataType::Enum dstDataType = WImageFormat::GetDataType(dstFormat);
  if (srcDataType != dstDataType || srcDataType >= WImageFormatDataType::DEPTH_STENCIL)
    return W_FAILURE;

  // Require uniform bits per channel so the stride-based pixel pointer arithmetic is valid.
  const WUInt32 srcBitsPerChannel = WImageFormat::GetBitsPerChannel(srcFormat, WImageFormatChannel::R);
  for (size_t i = 1; i < uiSrcChannels; i++)
  {
    if (WImageFormat::GetBitsPerChannel(srcFormat, static_cast<WImageFormatChannel::Enum>(i)) != srcBitsPerChannel)
      return W_FAILURE;
  }
  const WUInt32 dstBitsPerChannel = WImageFormat::GetBitsPerChannel(dstFormat, WImageFormatChannel::R);
  for (size_t i = 1; i < uiDstChannels; i++)
  {
    if (WImageFormat::GetBitsPerChannel(dstFormat, static_cast<WImageFormatChannel::Enum>(i)) != dstBitsPerChannel)
      return W_FAILURE;
  }

  if (srcBitsPerChannel != dstBitsPerChannel)
    return W_FAILURE;

  if (uiSrcChannelIdx >= uiSrcChannels || uiDstChannelIdx >= uiDstChannels)
    return W_FAILURE;

  const WUInt32 uiNumPixels = srcImg.GetWidth() * srcImg.GetHeight();

  switch (srcBitsPerChannel)
  {
    case 8:
    {
      const WUInt8* pSrc = srcImg.GetPixelPointer<WUInt8>() + uiSrcChannelIdx;
      WUInt8* pDst = ref_dstImg.GetPixelPointer<WUInt8>() + uiDstChannelIdx;
      CopyChannelImpl(pSrc, uiSrcChannels, pDst, uiDstChannels, uiNumPixels);
    }
    break;
    case 16:
    {
      const WUInt16* pSrc = srcImg.GetPixelPointer<WUInt16>() + uiSrcChannelIdx;
      WUInt16* pDst = ref_dstImg.GetPixelPointer<WUInt16>() + uiDstChannelIdx;
      CopyChannelImpl(pSrc, uiSrcChannels, pDst, uiDstChannels, uiNumPixels);
    }
    break;
    case 32:
    {
      const WUInt32* pSrc = srcImg.GetPixelPointer<WUInt32>() + uiSrcChannelIdx;
      WUInt32* pDst = ref_dstImg.GetPixelPointer<WUInt32>() + uiDstChannelIdx;
      CopyChannelImpl(pSrc, uiSrcChannels, pDst, uiDstChannels, uiNumPixels);
    }
    break;
    default:
      return W_FAILURE;
  }

  return W_SUCCESS;
}

static const char s_Base64EncodingTable[64] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
  'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static const WUInt8 BASE64_CHARS_PER_LINE = 76;

static WUInt32 GetBase64EncodedLength(WUInt32 uiInputLength, bool bInsertLineBreaks)
{
  WUInt32 outputLength = (uiInputLength + 2) / 3 * 4;

  if (bInsertLineBreaks)
  {
    outputLength += outputLength / BASE64_CHARS_PER_LINE;
  }

  return outputLength;
}

static WDynamicArray<char> ArrayToBase64(WArrayPtr<const WUInt8> in, bool bInsertLineBreaks = true)
{
  WDynamicArray<char> out;
  out.SetCountUninitialized(GetBase64EncodedLength(in.GetCount(), bInsertLineBreaks));

  WUInt32 offsetIn = 0;
  WUInt32 offsetOut = 0;

  WUInt32 blocksTillNewline = BASE64_CHARS_PER_LINE / 4;
  while (offsetIn < in.GetCount())
  {
    WUInt8 ibuf[3] = {0};

    WUInt32 ibuflen = WMath::Min(in.GetCount() - offsetIn, 3u);

    for (WUInt32 i = 0; i < ibuflen; ++i)
    {
      ibuf[i] = in[offsetIn++];
    }

    char obuf[4];
    obuf[0] = s_Base64EncodingTable[(ibuf[0] >> 2)];
    obuf[1] = s_Base64EncodingTable[((ibuf[0] << 4) & 0x30) | (ibuf[1] >> 4)];
    obuf[2] = s_Base64EncodingTable[((ibuf[1] << 2) & 0x3c) | (ibuf[2] >> 6)];
    obuf[3] = s_Base64EncodingTable[(ibuf[2] & 0x3f)];

    if (ibuflen >= 3)
    {
      out[offsetOut++] = obuf[0];
      out[offsetOut++] = obuf[1];
      out[offsetOut++] = obuf[2];
      out[offsetOut++] = obuf[3];
    }
    else // need to pad up to 4
    {
      switch (ibuflen)
      {
        case 1:
          out[offsetOut++] = obuf[0];
          out[offsetOut++] = obuf[1];
          out[offsetOut++] = '=';
          out[offsetOut++] = '=';
          break;
        case 2:
          out[offsetOut++] = obuf[0];
          out[offsetOut++] = obuf[1];
          out[offsetOut++] = obuf[2];
          out[offsetOut++] = '=';
          break;
      }
    }

    if (--blocksTillNewline == 0)
    {
      if (bInsertLineBreaks)
      {
        out[offsetOut++] = '\n';
      }
      blocksTillNewline = 19;
    }
  }

  W_ASSERT_DEV(offsetOut == out.GetCount(), "All output data should have been written");
  return out;
}

void WImageUtils::EmbedImageData(WStringBuilder& out_sHtml, const WImage& image)
{
  const WImageFileFormat* format = WImageFileFormat::GetWriterFormat("png");
  W_ASSERT_DEV(format != nullptr, "No PNG writer found");

  WDynamicArray<WUInt8> imgData;
  WMemoryStreamContainerWrapperStorage<WDynamicArray<WUInt8>> storage(&imgData);
  WMemoryStreamWriter writer(&storage);
  format->WriteImage(writer, image, "png").IgnoreResult();

  WDynamicArray<char> imgDataBase64 = ArrayToBase64(imgData.GetArrayPtr());
  WStringView imgDataBase64StringView(imgDataBase64.GetArrayPtr().GetPtr(), imgDataBase64.GetArrayPtr().GetEndPtr());
  out_sHtml.AppendFormat("data:image/png;base64,{0}", imgDataBase64StringView);
}

void WImageUtils::CreateImageDiffHtml(WStringBuilder& out_sHtml, WStringView sTitle, const WImage& referenceImgRgb, const WImage& referenceImgAlpha, const WImage& capturedImgRgb, const WImage& capturedImgAlpha, const WImage& diffImgRgb, const WImage& diffImgAlpha, WUInt32 uiError, WUInt32 uiThreshold, WUInt8 uiMinDiffRgb, WUInt8 uiMaxDiffRgb, WUInt8 uiMinDiffAlpha, WUInt8 uiMaxDiffAlpha)
{
  WStringBuilder& output = out_sHtml;
  output.Append("<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
                "<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
                "<HTML> <HEAD>\n");

  output.AppendFormat("<TITLE>{}</TITLE>\n", sTitle);
  output.Append("<script type = \"text/javascript\">\n"
                "function showReferenceImage()\n"
                "{\n"
                "    document.getElementById('image_current_rgb').style.display = 'none'\n"
                "    document.getElementById('image_current_a').style.display = 'none'\n"
                "    document.getElementById('image_reference_rgb').style.display = 'inline-block'\n"
                "    document.getElementById('image_reference_a').style.display = 'inline-block'\n"
                "    document.getElementById('image_caption_rgb').innerHTML = 'Displaying: Reference Image RGB'\n"
                "    document.getElementById('image_caption_a').innerHTML = 'Displaying: Reference Image Alpha'\n"
                "}\n"
                "function showCurrentImage()\n"
                "{\n"
                "    document.getElementById('image_current_rgb').style.display = 'inline-block'\n"
                "    document.getElementById('image_current_a').style.display = 'inline-block'\n"
                "    document.getElementById('image_reference_rgb').style.display = 'none'\n"
                "    document.getElementById('image_reference_a').style.display = 'none'\n"
                "    document.getElementById('image_caption_rgb').innerHTML = 'Displaying: Current Image RGB'\n"
                "    document.getElementById('image_caption_a').innerHTML = 'Displaying: Current Image Alpha'\n"
                "}\n"
                "function imageover()\n"
                "{\n"
                "    var mode = document.querySelector('input[name=\"image_interaction_mode\"]:checked').value\n"
                "    if (mode == 'interactive')\n"
                "    {\n"
                "        showReferenceImage()\n"
                "    }\n"
                "}\n"
                "function imageout()\n"
                "{\n"
                "    var mode = document.querySelector('input[name=\"image_interaction_mode\"]:checked').value\n"
                "    if (mode == 'interactive')\n"
                "    {\n"
                "        showCurrentImage()\n"
                "    }\n"
                "}\n"
                "function handleModeClick(clickedItem)\n"
                "{\n"
                "    if (clickedItem.value == 'current_image' || clickedItem.value == 'interactive')\n"
                "    {\n"
                "        showCurrentImage()\n"
                "    }\n"
                "    else if (clickedItem.value == 'reference_image')\n"
                "    {\n"
                "        showReferenceImage()\n"
                "    }\n"
                "}\n"
                "</script>\n"
                "</HEAD>\n"
                "<BODY bgcolor=\"#ccdddd\">\n"
                "<div style=\"line-height: 1.5; margin-top: 0px; margin-left: 10px; font-family: sans-serif;\">\n");

  output.AppendFormat("<b>Test result for \"{}\" from ", sTitle);
  WDateTime dateTime = WDateTime::MakeFromTimestamp(WTimestamp::CurrentTimestamp());
  output.AppendFormat("{}-{}-{} {}:{}:{}</b><br>\n", dateTime.GetYear(), WArgI(dateTime.GetMonth(), 2, true), WArgI(dateTime.GetDay(), 2, true), WArgI(dateTime.GetHour(), 2, true), WArgI(dateTime.GetMinute(), 2, true), WArgI(dateTime.GetSecond(), 2, true));

  output.Append("<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\">\n");

  output.Append("<!-- STATS-TABLE-START -->\n");

  output.AppendFormat("<tr>\n"
                      "<td>Error metric:</td>\n"
                      "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                      "</tr>\n",
    uiError);
  output.AppendFormat("<tr>\n"
                      "<td>Error threshold:</td>\n"
                      "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                      "</tr>\n",
    uiThreshold);

  output.Append("<!-- STATS-TABLE-END -->\n");

  output.Append("</table>\n"
                "<div style=\"margin-top: 0.5em; margin-bottom: -0.75em\">\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"interactive\" "
                "checked=\"checked\"> Mouse-Over Image Switching\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"current_image\"> "
                "Current Image\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"reference_image\"> "
                "Reference Image\n"
                "</div>\n");

  output.AppendFormat("<div style=\"width:{}px;display: inline-block;\">\n", capturedImgRgb.GetWidth());

  output.Append("<p id=\"image_caption_rgb\">Displaying: Current Image RGB</p>\n"

                "<div style=\"block;\" onmouseover=\"imageover()\" onmouseout=\"imageout()\">\n"
                "<img id=\"image_current_rgb\" alt=\"Captured Image RGB\" src=\"");
  EmbedImageData(output, capturedImgRgb);
  output.Append("\" />\n"
                "<img id=\"image_reference_rgb\" style=\"display: none\" alt=\"Reference Image RGB\" src=\"");
  EmbedImageData(output, referenceImgRgb);
  output.Append("\" />\n"
                "</div>\n"
                "<div style=\"display: block;\">\n");
  output.AppendFormat("<p>RGB Difference (min: {}, max: {}):</p>\n", uiMinDiffRgb, uiMaxDiffRgb);
  output.Append("<img alt=\"Diff Image RGB\" src=\"");
  EmbedImageData(output, diffImgRgb);
  output.Append("\" />\n"
                "</div>\n"
                "</div>\n");

  output.AppendFormat("<div style=\"width:{}px;display: inline-block;\">\n", capturedImgAlpha.GetWidth());

  output.Append("<p id=\"image_caption_a\">Displaying: Current Image Alpha</p>\n"
                "<div style=\"display: block;\" onmouseover=\"imageover()\" onmouseout=\"imageout()\">\n"
                "<img id=\"image_current_a\" alt=\"Captured Image Alpha\" src=\"");
  EmbedImageData(output, capturedImgAlpha);
  output.Append("\" />\n"
                "<img id=\"image_reference_a\" style=\"display: none\" alt=\"Reference Image Alpha\" src=\"");
  EmbedImageData(output, referenceImgAlpha);
  output.Append("\" />\n"
                "</div>\n"
                "<div style=\"px;display: block;\">\n");
  output.AppendFormat("<p>Alpha Difference (min: {}, max: {}):</p>\n", uiMinDiffAlpha, uiMaxDiffAlpha);
  output.Append("<img alt=\"Diff Image Alpha\" src=\"");
  EmbedImageData(output, diffImgAlpha);
  output.Append("\" />\n"
                "</div>\n"
                "</div>\n"
                "</div>\n"
                "</BODY> </HTML>");
}
