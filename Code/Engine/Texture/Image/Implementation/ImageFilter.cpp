#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageFilter.h>

WSimdFloat WImageFilter::GetWidth() const
{
  return m_fWidth;
}

WImageFilter::WImageFilter(float width)
  : m_fWidth(width)
{
}

WImageFilterBox::WImageFilterBox(float fWidth)
  : WImageFilter(fWidth)
{
}

WSimdFloat WImageFilterBox::SamplePoint(const WSimdFloat& x) const
{
  WSimdFloat absX = x.Abs();

  if (absX <= GetWidth())
  {
    return 1.0f;
  }
  else
  {
    return 0.0f;
  }
}

WImageFilterTriangle::WImageFilterTriangle(float fWidth)
  : WImageFilter(fWidth)
{
}

WSimdFloat WImageFilterTriangle::SamplePoint(const WSimdFloat& x) const
{
  WSimdFloat absX = x.Abs();

  WSimdFloat width = GetWidth();

  if (absX <= width)
  {
    return width - absX;
  }
  else
  {
    return 0.0f;
  }
}

static WSimdFloat sinc(const WSimdFloat& x)
{
  WSimdFloat absX = x.Abs();

  // Use Taylor expansion for small values to avoid division
  if (absX < 0.0001f)
  {
    // sin(x) / x = (x - x^3/6 + x^5/120 - ...) / x = 1 - x^2/6 + x^4/120 - ...
    return WSimdFloat(1.0f) - x * x * WSimdFloat(1.0f / 6.0f);
  }
  else
  {
    return WMath::Sin(WAngle::MakeFromRadian(x)) / x;
  }
}

static WSimdFloat modifiedBessel0(const WSimdFloat& x)
{
  // Implementation as I0(x) = sum((1/4 * x * x) ^ k / (k!)^2, k, 0, inf), see
  // http://mathworld.wolfram.com/ModifiedBesselFunctionoftheFirstKind.html

  WSimdFloat sum = 1.0f;

  WSimdFloat xSquared = x * x * WSimdFloat(0.25f);

  WSimdFloat currentTerm = xSquared;

  for (WUInt32 i = 2; currentTerm > 0.001f; ++i)
  {
    sum += currentTerm;
    currentTerm *= xSquared / WSimdFloat(i * i);
  }

  return sum;
}

WImageFilterSincWithKaiserWindow::WImageFilterSincWithKaiserWindow(float fWidth, float fBeta)
  : WImageFilter(fWidth)
  , m_fBeta(fBeta)
  , m_fInvBesselBeta(1.0f / modifiedBessel0(m_fBeta))
{
}

WSimdFloat WImageFilterSincWithKaiserWindow::SamplePoint(const WSimdFloat& x) const
{
  WSimdFloat scaledX = x / GetWidth();

  WSimdFloat xSq = 1.0f - scaledX * scaledX;

  if (xSq <= 0.0f)
  {
    return 0.0f;
  }
  else
  {
    return sinc(x * WSimdFloat(WMath::Pi<float>())) * modifiedBessel0(m_fBeta * xSq.GetSqrt()) * m_fInvBesselBeta;
  }
}

WImageFilterWeights::WImageFilterWeights(const WImageFilter& filter, WUInt32 uiSrcSamples, WUInt32 uiDstSamples)
{
  // Filter weights repeat after the common phase
  WUInt32 commonPhase = WMath::GreatestCommonDivisor(uiSrcSamples, uiDstSamples);

  uiSrcSamples /= commonPhase;
  uiDstSamples /= commonPhase;

  m_uiDstSamplesReduced = uiDstSamples;

  m_fSourceToDestScale = float(uiDstSamples) / float(uiSrcSamples);
  m_fDestToSourceScale = float(uiSrcSamples) / float(uiDstSamples);

  WSimdFloat filterScale, invFilterScale;

  if (uiDstSamples > uiSrcSamples)
  {
    // When upsampling, reconstruct the source by applying the filter in source space and resampling
    filterScale = 1.0f;
    invFilterScale = 1.0f;
  }
  else
  {
    // When downsampling, widen the filter in order to narrow its frequency spectrum, which effectively combines reconstruction + low-pass
    // filter
    filterScale = m_fDestToSourceScale;
    invFilterScale = m_fSourceToDestScale;
  }

  m_fWidthInSourceSpace = filter.GetWidth() * filterScale;

  m_uiNumWeights = WUInt32(WMath::Ceil(m_fWidthInSourceSpace * WSimdFloat(2.0f))) + 1;

  m_Weights.SetCountUninitialized(uiDstSamples * m_uiNumWeights);

  for (WUInt32 dstSample = 0; dstSample < uiDstSamples; ++dstSample)
  {
    WSimdFloat dstSampleInSourceSpace = (WSimdFloat(dstSample) + WSimdFloat(0.5f)) * m_fDestToSourceScale;

    WInt32 firstSourceIdx = GetFirstSourceSampleIndex(dstSample);

    WSimdFloat totalWeight = 0.0f;

    for (WUInt32 weightIdx = 0; weightIdx < m_uiNumWeights; ++weightIdx)
    {
      WSimdFloat sourceSample = WSimdFloat(firstSourceIdx + WInt32(weightIdx)) + WSimdFloat(0.5f);

      WSimdFloat weight = filter.SamplePoint((dstSampleInSourceSpace - sourceSample) * invFilterScale);
      totalWeight += weight;
      m_Weights[dstSample * m_uiNumWeights + weightIdx] = weight;
    }

    // Normalize weights
    WSimdFloat invWeight = 1.0f / totalWeight;

    for (WUInt32 weightIdx = 0; weightIdx < m_uiNumWeights; ++weightIdx)
    {
      m_Weights[dstSample * m_uiNumWeights + weightIdx] *= invWeight;
    }
  }
}

WUInt32 WImageFilterWeights::GetNumWeights() const
{
  return m_uiNumWeights;
}

WSimdFloat WImageFilterWeights::GetWeight(WUInt32 uiDstSampleIndex, WUInt32 uiWeightIndex) const
{
  W_ASSERT_DEBUG(uiWeightIndex < m_uiNumWeights, "Invalid weight index {} (should be < {})", uiWeightIndex, m_uiNumWeights);

  return WSimdFloat(m_Weights[(uiDstSampleIndex % m_uiDstSamplesReduced) * m_uiNumWeights + uiWeightIndex]);
}
