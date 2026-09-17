#pragma once

#include <Foundation/Strings/String.h>
#include <Texture/Image/Image.h>
#include <Texture/TextureDLL.h>

/// Input options for WTexComparer
class W_TEXTURE_DLL WTexCompareDesc
{
  W_DISALLOW_COPY_AND_ASSIGN(WTexCompareDesc);

public:
  WTexCompareDesc() = default;

  /// Path to a file to load as a reference image. Optional, if m_ExpectedImage is already filled out.
  WString m_sExpectedFile;

  /// Path to a file to load as the input image. Optional, if m_ActualImage is already filled out.
  WString m_sActualFile;

  /// The reference image to compare. Ignored if m_sExpectedFile is filled out.
  WImage m_ExpectedImage;

  /// The image to compare. Ignored if m_sActualFile is filled out.
  WImage m_ActualImage;

  /// If enabled, the image comparison allows for more wiggle room.
  /// For images containing single-pixel rasterized lines.
  bool m_bRelaxedComparison = false;

  /// If the comparison yields a larger MSE than this, the images are considered to be too different.
  WUInt32 m_MeanSquareErrorThreshold = 100;
};

/// Compares two images and generates various outputs.
class W_TEXTURE_DLL WTexComparer
{
  W_DISALLOW_COPY_AND_ASSIGN(WTexComparer);

public:
  WTexComparer();

  /// The input data to compare.
  WTexCompareDesc m_Descriptor;

  /// Executes the comparison and fill out the public variables to describe the result.
  WResult Compare();

  /// If true, the mean-square error of the difference was larger than the threshold.
  bool m_bExceededMSE = false;
  /// The MSE of the difference image.
  WUInt32 m_OutputMSE = 0;

  /// The (normalized) difference image.
  WImage m_OutputImageDiff;
  /// Only the RGB part of the (normalized) difference image.
  WImage m_OutputImageDiffRgb;
  /// Only the Alpha part of the (normalized) difference image.
  WImage m_OutputImageDiffAlpha;

  /// Only the RGB part of the actual input image.
  WImage m_ExtractedActualRgb;
  /// Only the RGB part of the reference input image.
  WImage m_ExtractedExpectedRgb;
  /// Only the Alpha part of the actual input image.
  WImage m_ExtractedActualAlpha;
  /// Only the Alpha part of the reference input image.
  WImage m_ExtractedExpectedAlpha;

  /// Min/Max difference of the RGB and Alpha images.
  WUInt8 m_uiOutputMinDiffRgb = 0;
  WUInt8 m_uiOutputMaxDiffRgb = 0;
  WUInt8 m_uiOutputMinDiffAlpha = 0;
  WUInt8 m_uiOutputMaxDiffAlpha = 0;

private:
  WResult LoadInputImages();
  WResult ComputeMSE();
  WResult ExtractImages();
};
