#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WTexConvCompressionMode, 1)
  W_ENUM_CONSTANTS(WTexConvCompressionMode::None, WTexConvCompressionMode::Medium, WTexConvCompressionMode::High)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WTexConvMipmapMode, 1)
  W_ENUM_CONSTANTS(WTexConvMipmapMode::None, WTexConvMipmapMode::Linear, WTexConvMipmapMode::Kaiser)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WTexConvUsage, 1)
  W_ENUM_CONSTANT(WTexConvUsage::Auto), W_ENUM_CONSTANT(WTexConvUsage::Color), W_ENUM_CONSTANT(WTexConvUsage::Linear),
  W_ENUM_CONSTANT(WTexConvUsage::Hdr), W_ENUM_CONSTANT(WTexConvUsage::NormalMap), W_ENUM_CONSTANT(WTexConvUsage::NormalMap_Inverted),
  W_ENUM_CONSTANT(WTexConvUsage::BumpMap),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WTexConvProcessor::WTexConvProcessor() = default;

WResult WTexConvProcessor::Process()
{
  W_PROFILE_SCOPE("WTexConvProcessor::Process");

  if (m_Descriptor.m_OutputType == WTexConvOutputType::Atlas)
  {
    WMemoryStreamWriter stream(&m_TextureAtlas);
    W_SUCCEED_OR_RETURN(GenerateTextureAtlas(stream));
  }
  else
  {
    W_SUCCEED_OR_RETURN(LoadInputImages());

    W_SUCCEED_OR_RETURN(AdjustUsage(m_Descriptor.m_InputFiles[0], m_Descriptor.m_InputImages[0], m_Descriptor.m_Usage));

    WLog::Info("-usage is '{}'", WArgEnum(m_Descriptor.m_Usage));

    W_SUCCEED_OR_RETURN(ForceSRGBFormats());

    WUInt32 uiNumChannelsUsed = 0;
    W_SUCCEED_OR_RETURN(DetectNumChannels(m_Descriptor.m_ChannelMappings, uiNumChannelsUsed));

    WEnum<WImageFormat> OutputImageFormat;

    W_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, m_Descriptor.m_Usage, uiNumChannelsUsed));

    WLog::Info("Output image format is '{}'", WImageFormat::GetName(OutputImageFormat));

    WUInt32 uiTargetResolutionX = 0;
    WUInt32 uiTargetResolutionY = 0;

    W_SUCCEED_OR_RETURN(DetermineTargetResolution(m_Descriptor.m_InputImages[0], OutputImageFormat, uiTargetResolutionX, uiTargetResolutionY));

    WLog::Info("Target resolution is '{} x {}'", uiTargetResolutionX, uiTargetResolutionY);

    W_SUCCEED_OR_RETURN(ConvertAndScaleInputImages(uiTargetResolutionX, uiTargetResolutionY, m_Descriptor.m_Usage));

    W_SUCCEED_OR_RETURN(ClampInputValues(m_Descriptor.m_InputImages, m_Descriptor.m_fMaxValue));

    if (m_Descriptor.m_Usage == WTexConvUsage::BumpMap)
    {
      W_SUCCEED_OR_RETURN(ConvertToNormalMap(m_Descriptor.m_InputImages));
      m_Descriptor.m_Usage = WTexConvUsage::NormalMap;
    }

    WImage assembledImg;
    if (m_Descriptor.m_OutputType == WTexConvOutputType::Texture2D || m_Descriptor.m_OutputType == WTexConvOutputType::None)
    {
      W_SUCCEED_OR_RETURN(Assemble2DTexture(m_Descriptor.m_InputImages[0].GetHeader(), assembledImg));

      W_SUCCEED_OR_RETURN(InvertNormalMap(assembledImg));

      W_SUCCEED_OR_RETURN(DilateColor2D(assembledImg));
    }
    else if (m_Descriptor.m_OutputType == WTexConvOutputType::Cubemap)
    {
      W_SUCCEED_OR_RETURN(AssembleCubemap(assembledImg));
    }
    else if (m_Descriptor.m_OutputType == WTexConvOutputType::Volume)
    {
      W_SUCCEED_OR_RETURN(Assemble3DTexture(assembledImg));
    }
    else if (m_Descriptor.m_OutputType == WTexConvOutputType::Texture2DArray)
    {
      W_SUCCEED_OR_RETURN(Assemble2DArrayTexture(assembledImg));
      W_SUCCEED_OR_RETURN(InvertNormalMap(assembledImg));
    }

    W_SUCCEED_OR_RETURN(AdjustHdrExposure(assembledImg));

    W_SUCCEED_OR_RETURN(GenerateMipmaps(assembledImg, 0, uiNumChannelsUsed == 1 ? MipmapChannelMode::SingleChannel : MipmapChannelMode::AllChannels));

    W_SUCCEED_OR_RETURN(PremultiplyAlpha(assembledImg));

    W_SUCCEED_OR_RETURN(GenerateOutput(std::move(assembledImg), m_OutputImage, OutputImageFormat));

    W_SUCCEED_OR_RETURN(GenerateThumbnailOutput(m_OutputImage, m_ThumbnailOutputImage, m_Descriptor.m_uiThumbnailOutputResolution));

    W_SUCCEED_OR_RETURN(GenerateLowResOutput(m_OutputImage, m_LowResOutputImage, m_Descriptor.m_uiLowResMipmaps));
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::DetectNumChannels(WArrayPtr<const WTexConvSliceChannelMapping> channelMapping, WUInt32& uiNumChannels)
{
  W_PROFILE_SCOPE("DetectNumChannels");

  uiNumChannels = 0;

  for (const auto& mapping : channelMapping)
  {
    for (WUInt32 i = 0; i < 4; ++i)
    {
      if (mapping.m_Channel[i].m_iInputImageIndex != -1 || mapping.m_Channel[i].m_ChannelValue == WTexConvChannelValue::Black)
      {
        uiNumChannels = WMath::Max(uiNumChannels, i + 1);
      }
    }
  }

  if (uiNumChannels == 0)
  {
    WLog::Error("No proper channel mapping provided.");
    return W_FAILURE;
  }

  // special case handling to detect when the alpha channel will end up white anyway and thus uiNumChannels could be 3 instead of 4
  // which enables us to use more optimized output formats
  if (uiNumChannels == 4)
  {
    uiNumChannels = 3;

    for (const auto& mapping : channelMapping)
    {
      if (mapping.m_Channel[3].m_ChannelValue == WTexConvChannelValue::Black)
      {
        // sampling a texture without an alpha channel always returns 1, so to use all 0, we do need the channel
        uiNumChannels = 4;
        return W_SUCCESS;
      }

      if (mapping.m_Channel[3].m_iInputImageIndex == -1)
      {
        // no fourth channel is needed for this
        continue;
      }

      WImage& img = m_Descriptor.m_InputImages[mapping.m_Channel[3].m_iInputImageIndex];

      const WUInt32 uiNumRequiredChannels = (WUInt32)mapping.m_Channel[3].m_ChannelValue + 1;
      const WUInt32 uiNumActualChannels = WImageFormat::GetNumChannels(img.GetImageFormat());

      if (uiNumActualChannels < uiNumRequiredChannels)
      {
        // channel not available -> not needed
        continue;
      }

      if (img.Convert(WImageFormat::R32G32B32A32_FLOAT).Failed())
      {
        // can't convert -> will fail later anyway
        continue;
      }

      const float* pColors = img.GetPixelPointer<float>();
      pColors += (uiNumRequiredChannels - 1); // offset by 0 to 3 to read red, green, blue or alpha

      W_ASSERT_DEV(img.GetRowPitch() == img.GetWidth() * sizeof(float) * 4, "Unexpected row pitch");

      for (WUInt32 i = 0; i < img.GetWidth() * img.GetHeight(); ++i)
      {
        if (!WMath::IsEqual(*pColors, 1.0f, 1.0f / 255.0f))
        {
          // value is not 1.0f -> the channel is needed
          uiNumChannels = 4;
          return W_SUCCESS;
        }

        pColors += 4;
      }
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::GenerateOutput(WImage&& src, WImage& dst, WEnum<WImageFormat> format)
{
  W_PROFILE_SCOPE("GenerateOutput");

  dst.ResetAndMove(std::move(src));

  if (dst.Convert(format).Failed())
  {
    WLog::Error("Failed to convert result image to output format '{}'", WImageFormat::GetName(format));
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::GenerateThumbnailOutput(const WImage& srcImg, WImage& dstImg, WUInt32 uiTargetRes)
{
  if (uiTargetRes == 0)
    return W_SUCCESS;

  W_PROFILE_SCOPE("GenerateThumbnailOutput");

  WUInt32 uiBestMip = 0;

  for (WUInt32 m = 0; m < srcImg.GetNumMipLevels(); ++m)
  {
    if (srcImg.GetWidth(m) <= uiTargetRes && srcImg.GetHeight(m) <= uiTargetRes)
    {
      uiBestMip = m;
      break;
    }

    uiBestMip = m;
  }

  WImage scratch1, scratch2;
  WImage* pCurrentScratch = &scratch1;
  WImage* pOtherScratch = &scratch2;

  pCurrentScratch->ResetAndCopy(srcImg.GetSubImageView(uiBestMip, 0));

  if (pCurrentScratch->GetWidth() > uiTargetRes || pCurrentScratch->GetHeight() > uiTargetRes)
  {
    if (pCurrentScratch->GetWidth() > pCurrentScratch->GetHeight())
    {
      const float fAspectRatio = (float)pCurrentScratch->GetWidth() / (float)uiTargetRes;
      WUInt32 uiTargetHeight = (WUInt32)(pCurrentScratch->GetHeight() / fAspectRatio);

      uiTargetHeight = WMath::Max(uiTargetHeight, 4U);

      if (WImageUtils::Scale(*pCurrentScratch, *pOtherScratch, uiTargetRes, uiTargetHeight).Failed())
      {
        WLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", pCurrentScratch->GetWidth(), pCurrentScratch->GetHeight(), uiTargetRes,
          uiTargetHeight);
        return W_FAILURE;
      }
    }
    else
    {
      const float fAspectRatio = (float)pCurrentScratch->GetHeight() / (float)uiTargetRes;
      WUInt32 uiTargetWidth = (WUInt32)(pCurrentScratch->GetWidth() / fAspectRatio);

      uiTargetWidth = WMath::Max(uiTargetWidth, 4U);

      if (WImageUtils::Scale(*pCurrentScratch, *pOtherScratch, uiTargetWidth, uiTargetRes).Failed())
      {
        WLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", pCurrentScratch->GetWidth(), pCurrentScratch->GetHeight(), uiTargetWidth,
          uiTargetRes);
        return W_FAILURE;
      }
    }

    WMath::Swap(pCurrentScratch, pOtherScratch);
  }

  dstImg.ResetAndMove(std::move(*pCurrentScratch));

  // we want to write out the thumbnail unchanged, so make sure it has a non-sRGB format
  dstImg.ReinterpretAs(WImageFormat::AsLinear(dstImg.GetImageFormat()));

  if (dstImg.Convert(WImageFormat::R8G8B8A8_UNORM).Failed())
  {
    WLog::Error("Failed to convert thumbnail image to RGBA8.");
    return W_FAILURE;
  }

  // generate alpha checkerboard pattern
  {
    const float fTileSize = 16.0f;

    WColorLinearUB* pPixels = dstImg.GetPixelPointer<WColorLinearUB>();
    const WUInt64 rowPitch = dstImg.GetRowPitch();

    WInt32 checkCounter = 0;
    WColor tiles[2]{WColor::LightGray, WColor::DarkGray};


    for (WUInt32 y = 0; y < dstImg.GetHeight(); ++y)
    {
      checkCounter = (WInt32)WMath::Floor(y / fTileSize);

      for (WUInt32 x = 0; x < dstImg.GetWidth(); ++x)
      {
        WColorLinearUB& col = pPixels[x];

        if (col.a < 255)
        {
          const WColor colF = col;
          const WInt32 tileIdx = (checkCounter + (WInt32)WMath::Floor(x / fTileSize)) % 2;

          col = WMath::Lerp(tiles[tileIdx], colF, WMath::Sqrt(colF.a)).WithAlpha(colF.a);
        }
      }

      pPixels = WMemoryUtils::AddByteOffset(pPixels, static_cast<ptrdiff_t>(rowPitch));
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::GenerateLowResOutput(const WImage& srcImg, WImage& dstImg, WUInt32 uiLowResMip)
{
  if (uiLowResMip == 0)
    return W_SUCCESS;

  W_PROFILE_SCOPE("GenerateLowResOutput");

  // don't early out here in this case, otherwise external processes may consider the output to be incomplete
  // if (srcImg.GetNumMipLevels() <= uiLowResMip)
  //{
  //  // probably just a low-resolution input image, do not generate output, but also do not fail
  //  WLog::Warning("LowRes image not generated, original resolution is already below threshold.");
  //  return W_SUCCESS;
  //}

  if (WImageUtils::ExtractLowerMipChain(srcImg, dstImg, uiLowResMip).Failed())
  {
    WLog::Error("Failed to extract low-res mipmap chain from output image.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}



W_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Processor);
