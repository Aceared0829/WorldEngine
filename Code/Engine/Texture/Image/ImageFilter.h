#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Texture/TextureDLL.h>

/// Base class for image filtering functions used in scaling and resampling operations.
///
/// Image filters define how pixels are weighted when scaling images up or down.
/// Different filters provide different trade-offs between sharpness, aliasing, and ringing artifacts.
class W_TEXTURE_DLL WImageFilter
{
public:
  /// Evaluates the filter function at the given distance from the center.
  ///
  /// The returned value represents the weight to apply to a sample at this distance.
  /// The function should return 0 for distances beyond the filter width.
  /// Note: The distribution may not be normalized - normalization is handled by the caller.
  virtual WSimdFloat SamplePoint(const WSimdFloat& x) const = 0;

  /// Returns the filter support width (radius).
  ///
  /// The filter function is guaranteed to return 0 for |x| > width.
  /// Larger widths generally mean higher quality but slower filtering.
  WSimdFloat GetWidth() const;

protected:
  WImageFilter(float width);

private:
  WSimdFloat m_fWidth;
};

/// Box filter - fastest, produces blocky results.
///
/// The box filter provides uniform weighting within its support width.
/// Best used for pixel art or when nearest-neighbor-like behavior is desired.
/// Produces sharp edges but can create blocking artifacts.
class W_TEXTURE_DLL WImageFilterBox : public WImageFilter
{
public:
  /// \param fWidth Filter support width, typically 0.5 for standard box filtering
  WImageFilterBox(float fWidth = 0.5f);

  virtual WSimdFloat SamplePoint(const WSimdFloat& x) const override;
};

/// Triangle (bilinear) filter - good balance of speed and quality.
///
/// The triangle filter provides linear weighting that falls to zero at the edges.
/// This is equivalent to bilinear interpolation and provides a good balance
/// between performance and visual quality with minimal ringing artifacts.
class W_TEXTURE_DLL WImageFilterTriangle : public WImageFilter
{
public:
  /// \param fWidth Filter support width, typically 1.0 for standard triangle filtering
  WImageFilterTriangle(float fWidth = 1.0f);

  virtual WSimdFloat SamplePoint(const WSimdFloat& x) const override;
};

/// Kaiser-windowed sinc filter - highest quality but may introduce ringing.
///
/// This filter provides the highest quality scaling with excellent preservation of detail.
/// The Kaiser window helps reduce ringing artifacts compared to an unwindowed sinc.
/// Use higher beta values for less ringing but more blurring.
///
/// **Parameter Guidelines:**
/// - Beta 2-4: Less ringing, more blurring
/// - Beta 4-6: Good balance (recommended range)
/// - Beta 6-8: Sharp but more ringing artifacts
class W_TEXTURE_DLL WImageFilterSincWithKaiserWindow : public WImageFilter
{
public:
  /// Constructs a Kaiser-windowed sinc filter.
  ///
  /// \param fWindowWidth Filter support width. Larger values provide higher quality but slower performance.
  ///                     Typical range: 2.0-4.0, with 3.0 being a good default.
  /// \param fBeta Kaiser window beta parameter controlling the trade-off between ringing and blurring.
  ///              This is alpha*pi in standard Kaiser window definitions. Range: 2.0-8.0, default 4.0.
  WImageFilterSincWithKaiserWindow(float fWindowWidth = 3.0f, float fBeta = 4.0f);

  virtual WSimdFloat SamplePoint(const WSimdFloat& x) const override;

private:
  WSimdFloat m_fBeta;
  WSimdFloat m_fInvBesselBeta;
};

/// Pre-computes the required filter weights for rescaling a sequence of image samples.
class W_TEXTURE_DLL WImageFilterWeights
{
public:
  /// Pre-compute the weights for the given filter for scaling between the given number of samples.
  WImageFilterWeights(const WImageFilter& filter, WUInt32 uiSrcSamples, WUInt32 uiDstSamples);

  /// Returns the number of weights.
  WUInt32 GetNumWeights() const;

  /// Returns the weight used for the source sample GetFirstSourceSampleIndex(dstSampleIndex) + weightIndex
  WSimdFloat GetWeight(WUInt32 uiDstSampleIndex, WUInt32 uiWeightIndex) const;

  /// Returns the index of the first source sample that needs to be weighted to evaluate the destination sample
  inline WInt32 GetFirstSourceSampleIndex(WUInt32 uiDstSampleIndex) const;

  WArrayPtr<const float> ViewWeights() const;

private:
  WHybridArray<float, 16> m_Weights;
  WSimdFloat m_fWidthInSourceSpace;
  WSimdFloat m_fSourceToDestScale;
  WSimdFloat m_fDestToSourceScale;
  WUInt32 m_uiNumWeights;
  WUInt32 m_uiDstSamplesReduced;
};

#include <Texture/Image/Implementation/ImageFilter_inl.h>
