#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

WResult WTexConvProcessor::Assemble2DTexture(const WImageHeader& refImg, WImage& dst) const
{
  W_PROFILE_SCOPE("Assemble2DTexture");

  dst.ResetAndAlloc(refImg);

  WColor* pPixelOut = dst.GetPixelPointer<WColor>();

  return Assemble2DSlice(m_Descriptor.m_ChannelMappings[0], refImg.GetWidth(), refImg.GetHeight(), pPixelOut);
}

WResult WTexConvProcessor::Assemble2DSlice(const WTexConvSliceChannelMapping& mapping, WUInt32 uiResolutionX, WUInt32 uiResolutionY, WColor* pPixelOut) const
{
  WTempHybridArray<const WColor*, 16> pSource;
  for (WUInt32 i = 0; i < m_Descriptor.m_InputImages.GetCount(); ++i)
  {
    pSource.ExpandAndGetRef() = m_Descriptor.m_InputImages[i].GetPixelPointer<WColor>();
  }

  const float fZero = 0.0f;
  const float fOne = 1.0f;
  const float* pSourceValues[4] = {nullptr, nullptr, nullptr, nullptr};
  WUInt32 uiSourceStrides[4] = {0, 0, 0, 0};

  for (WUInt32 channel = 0; channel < 4; ++channel)
  {
    const auto& cm = mapping.m_Channel[channel];
    const WInt32 inputIndex = cm.m_iInputImageIndex;

    if (inputIndex != -1)
    {
      const WColor* pSourcePixel = pSource[inputIndex];
      uiSourceStrides[channel] = 4;

      switch (cm.m_ChannelValue)
      {
        case WTexConvChannelValue::Red:
          pSourceValues[channel] = &pSourcePixel->r;
          break;
        case WTexConvChannelValue::Green:
          pSourceValues[channel] = &pSourcePixel->g;
          break;
        case WTexConvChannelValue::Blue:
          pSourceValues[channel] = &pSourcePixel->b;
          break;
        case WTexConvChannelValue::Alpha:
          pSourceValues[channel] = &pSourcePixel->a;
          break;

        default:
          W_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }
    else
    {
      uiSourceStrides[channel] = 0; // because of the constant value

      switch (cm.m_ChannelValue)
      {
        case WTexConvChannelValue::Black:
          pSourceValues[channel] = &fZero;
          break;

        case WTexConvChannelValue::White:
          pSourceValues[channel] = &fOne;
          break;

        default:
          if (channel == 3)
            pSourceValues[channel] = &fOne;
          else
            pSourceValues[channel] = &fZero;
          break;
      }
    }
  }

  const bool bFlip = m_Descriptor.m_bFlipHorizontal;

  if (!bFlip && (pSourceValues[0] + 1 == pSourceValues[1]) && (pSourceValues[1] + 1 == pSourceValues[2]) &&
      (pSourceValues[2] + 1 == pSourceValues[3]))
  {
    W_PROFILE_SCOPE("Assemble2DSlice(memcpy)");

    WMemoryUtils::Copy<WColor>(pPixelOut, reinterpret_cast<const WColor*>(pSourceValues[0]), uiResolutionX * uiResolutionY);
  }
  else
  {
    W_PROFILE_SCOPE("Assemble2DSlice(gather)");

    for (WUInt32 y = 0; y < uiResolutionY; ++y)
    {
      const WUInt32 pixelWriteRowOffset = uiResolutionX * (bFlip ? (uiResolutionY - y - 1) : y);

      for (WUInt32 x = 0; x < uiResolutionX; ++x)
      {
        float* dst = &pPixelOut[pixelWriteRowOffset + x].r;

        for (WUInt32 c = 0; c < 4; ++c)
        {
          dst[c] = *pSourceValues[c];
          pSourceValues[c] += uiSourceStrides[c];
        }
      }
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::DetermineTargetResolution(const WImage& image, WEnum<WImageFormat> OutputImageFormat, WUInt32& out_uiTargetResolutionX, WUInt32& out_uiTargetResolutionY) const
{
  W_PROFILE_SCOPE("DetermineResolution");

  W_ASSERT_DEV(out_uiTargetResolutionX == 0 && out_uiTargetResolutionY == 0, "Target resolution already determined");

  const WUInt32 uiOrgResX = image.GetWidth();
  const WUInt32 uiOrgResY = image.GetHeight();

  out_uiTargetResolutionX = uiOrgResX;
  out_uiTargetResolutionY = uiOrgResY;

  out_uiTargetResolutionX /= (1 << m_Descriptor.m_uiDownscaleSteps);
  out_uiTargetResolutionY /= (1 << m_Descriptor.m_uiDownscaleSteps);

  out_uiTargetResolutionX = WMath::Clamp(out_uiTargetResolutionX, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);
  out_uiTargetResolutionY = WMath::Clamp(out_uiTargetResolutionY, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);

  // keep original aspect ratio
  if (uiOrgResX > uiOrgResY)
  {
    out_uiTargetResolutionY = (out_uiTargetResolutionX * uiOrgResY) / uiOrgResX;
  }
  else if (uiOrgResX < uiOrgResY)
  {
    out_uiTargetResolutionX = (out_uiTargetResolutionY * uiOrgResX) / uiOrgResY;
  }

  if (m_Descriptor.m_OutputType == WTexConvOutputType::Volume)
  {
    WUInt32 uiScaleFactor = uiOrgResY / out_uiTargetResolutionY;
    out_uiTargetResolutionX = uiOrgResX / uiScaleFactor;
  }

  if (OutputImageFormat != WImageFormat::UNKNOWN && WImageFormat::RequiresFirstLevelBlockAlignment(OutputImageFormat))
  {
    const WUInt32 blockWidth = WImageFormat::GetBlockWidth(OutputImageFormat);

    WUInt32 currentWidth = out_uiTargetResolutionX;
    WUInt32 currentHeight = out_uiTargetResolutionY;
    bool issueWarning = false;

    if (out_uiTargetResolutionX % blockWidth != 0)
    {
      out_uiTargetResolutionX = WMath::RoundUp(out_uiTargetResolutionX, static_cast<WUInt16>(blockWidth));
      issueWarning = true;
    }

    WUInt32 blockHeight = WImageFormat::GetBlockHeight(OutputImageFormat);
    if (out_uiTargetResolutionY % blockHeight != 0)
    {
      out_uiTargetResolutionY = WMath::RoundUp(out_uiTargetResolutionY, static_cast<WUInt16>(blockHeight));
      issueWarning = true;
    }

    if (issueWarning)
    {
      WLog::Warning(
        "Chosen output image format is compressed, but target resolution does not fulfill block size requirements. {}x{} -> downscale {} / "
        "clamp({}, {}) -> {}x{}, adjusted to {}x{}",
        uiOrgResX, uiOrgResY, m_Descriptor.m_uiDownscaleSteps, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution, currentWidth,
        currentHeight, out_uiTargetResolutionX, out_uiTargetResolutionY);
    }
  }

  return W_SUCCESS;
}
