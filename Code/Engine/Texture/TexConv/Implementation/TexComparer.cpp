#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexComparer.h>

WTexComparer::WTexComparer() = default;

WResult WTexComparer::Compare()
{
  W_PROFILE_SCOPE("Compare");

  W_SUCCEED_OR_RETURN(LoadInputImages());

  if ((m_Descriptor.m_ActualImage.GetWidth() != m_Descriptor.m_ExpectedImage.GetWidth()) ||
      (m_Descriptor.m_ActualImage.GetHeight() != m_Descriptor.m_ExpectedImage.GetHeight()))
  {
    WLog::Error("Image sizes are not identical: {}x{} != {}x{}", m_Descriptor.m_ActualImage.GetWidth(), m_Descriptor.m_ActualImage.GetHeight(), m_Descriptor.m_ExpectedImage.GetWidth(), m_Descriptor.m_ExpectedImage.GetHeight());
    return W_FAILURE;
  }

  W_SUCCEED_OR_RETURN(ComputeMSE());

  if (m_OutputMSE > m_Descriptor.m_MeanSquareErrorThreshold)
  {
    m_bExceededMSE = true;

    W_SUCCEED_OR_RETURN(ExtractImages());
  }

  return W_SUCCESS;
}

WResult WTexComparer::LoadInputImages()
{
  W_PROFILE_SCOPE("Load Images");

  if (!m_Descriptor.m_sActualFile.IsEmpty())
  {
    if (m_Descriptor.m_ActualImage.LoadFrom(m_Descriptor.m_sActualFile).Failed())
    {
      WLog::Error("Could not load image file '{0}'.", WArgSensitive(m_Descriptor.m_sActualFile, "File"));
      return W_FAILURE;
    }
  }

  if (!m_Descriptor.m_sExpectedFile.IsEmpty())
  {
    if (m_Descriptor.m_ExpectedImage.LoadFrom(m_Descriptor.m_sExpectedFile).Failed())
    {
      WLog::Error("Could not load reference file '{0}'.", WArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
      return W_FAILURE;
    }
  }

  if (!m_Descriptor.m_ActualImage.IsValid())
  {
    WLog::Error("No image available.");
    return W_FAILURE;
  }

  if (!m_Descriptor.m_ExpectedImage.IsValid())
  {
    WLog::Error("No reference image available.");
    return W_FAILURE;
  }

  if (m_Descriptor.m_ActualImage.GetImageFormat() == WImageFormat::UNKNOWN)
  {
    WLog::Error("Unknown image format for '{}'", WArgSensitive(m_Descriptor.m_sActualFile, "File"));
    return W_FAILURE;
  }

  if (m_Descriptor.m_ExpectedImage.GetImageFormat() == WImageFormat::UNKNOWN)
  {
    WLog::Error("Unknown image format for '{}'", WArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
    return W_FAILURE;
  }

  if (WImageConversion::Convert(m_Descriptor.m_ActualImage, m_Descriptor.m_ActualImage, WImageFormat::R8G8B8A8_UNORM).Failed())
  {
    WLog::Error("Could not convert to RGBA8: '{}'", WArgSensitive(m_Descriptor.m_sActualFile, "File"));
    return W_FAILURE;
  }

  if (WImageConversion::Convert(m_Descriptor.m_ExpectedImage, m_Descriptor.m_ExpectedImage, WImageFormat::R8G8B8A8_UNORM).Failed())
  {
    WLog::Error("Could not convert to RGBA8: '{}'", WArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WTexComparer::ComputeMSE()
{
  W_PROFILE_SCOPE("ComputeMSE");

  if (m_Descriptor.m_bRelaxedComparison)
    WImageUtils::ComputeImageDifferenceABSRelaxed(m_Descriptor.m_ActualImage, m_Descriptor.m_ExpectedImage, m_OutputImageDiff);
  else
    WImageUtils::ComputeImageDifferenceABS(m_Descriptor.m_ActualImage, m_Descriptor.m_ExpectedImage, m_OutputImageDiff);

  m_OutputMSE = WImageUtils::ComputeMeanSquareError(m_OutputImageDiff, 32);

  return W_SUCCESS;
}

WResult WTexComparer::ExtractImages()
{
  W_PROFILE_SCOPE("ExtractImages");

  WImageUtils::Normalize(m_OutputImageDiff, m_uiOutputMinDiffRgb, m_uiOutputMaxDiffRgb, m_uiOutputMinDiffAlpha, m_uiOutputMaxDiffAlpha);

  W_SUCCEED_OR_RETURN(WImageConversion::Convert(m_OutputImageDiff, m_OutputImageDiffRgb, WImageFormat::R8G8B8_UNORM));

  WImageUtils::ExtractAlphaChannel(m_OutputImageDiff, m_OutputImageDiffAlpha);

  W_SUCCEED_OR_RETURN(WImageConversion::Convert(m_Descriptor.m_ActualImage, m_ExtractedActualRgb, WImageFormat::R8G8B8_UNORM));
  WImageUtils::ExtractAlphaChannel(m_Descriptor.m_ActualImage, m_ExtractedActualAlpha);

  W_SUCCEED_OR_RETURN(WImageConversion::Convert(m_Descriptor.m_ExpectedImage, m_ExtractedExpectedRgb, WImageFormat::R8G8B8_UNORM));
  WImageUtils::ExtractAlphaChannel(m_Descriptor.m_ExpectedImage, m_ExtractedExpectedAlpha);

  return W_SUCCESS;
}
