
#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/AssetFileHeader.h>

#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/WTexFormat/WTexFormat.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTexture3DResource, 1, WRTTIDefaultAllocator<WTexture3DResource>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

W_RESOURCE_IMPLEMENT_COMMON_CODE(WTexture3DResource);

WTexture3DResource::WTexture3DResource()
  : WResource(DoUpdate::OnGraphicsResourceThreads, WTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

WTexture3DResource::WTexture3DResource(WResource::DoUpdate ResourceUpdateThread)
  : WResource(ResourceUpdateThread, WTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

WResourceLoadDesc WTexture3DResource::UnloadData(Unload WhatToUnload)
{
  if (m_uiLoadedTextures > 0)
  {
    for (WInt32 r = 0; r < 2; ++r)
    {
      --m_uiLoadedTextures;

      WGALDevice::GetDefaultDevice()->DestroyTexture(m_hGALTexture[m_uiLoadedTextures]);

      m_uiMemoryGPU[m_uiLoadedTextures] = 0;

      if (WhatToUnload == Unload::OneQualityLevel || m_uiLoadedTextures == 0)
        break;
    }
  }

  if (WhatToUnload == Unload::AllQualityLevels)
  {
    WGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
  }

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable = 2 - m_uiLoadedTextures;
  res.m_State = m_uiLoadedTextures == 0 ? WResourceState::Unloaded : WResourceState::Loaded;
  return res;
}

void WTexture3DResource::FillOutDescriptor(WTexture3DResourceDescriptor& ref_td, const WImage* pImage, bool bSRGB, WUInt32 uiNumMipLevels,
  WUInt32& out_uiMemoryUsed, WHybridArray<WGALSystemMemoryDescription, 32>& ref_initData)
{
  const WUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  const WGALResourceFormat::Enum format = WTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), bSRGB);

  ref_td.m_DescGAL.m_Format = format;
  ref_td.m_DescGAL.m_uiWidth = pImage->GetWidth(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiHeight = pImage->GetHeight(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiMipLevelCount = uiNumMipLevels;
  ref_td.m_DescGAL.m_uiArraySize = pImage->GetNumArrayIndices();
  ref_td.m_DescGAL.m_ResourceAccess.m_bImmutable = true;

  if (ref_td.m_DescGAL.m_uiDepth > 1)
    ref_td.m_DescGAL.m_Type = WGALTextureType::Texture3D;

  out_uiMemoryUsed = 0;

  ref_initData.Clear();

  for (WUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (WUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (WUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        WGALSystemMemoryDescription& id = ref_initData.ExpandAndGetRef();

        id.m_pData = pImage->GetSubImageView(mip, face, array_index).GetByteBlobPtr();

        if (WImageFormat::GetType(pImage->GetImageFormat()) == WImageFormatType::BLOCK_COMPRESSED)
        {
          const WUInt32 uiMemPitchFactor = WGALResourceFormat::GetBitsPerElement(format) * 4 / 8;

          id.m_uiRowPitch = WMath::Max<WUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiRowPitch = static_cast<WUInt32>(pImage->GetRowPitch(mip));
        }

        W_ASSERT_DEV(pImage->GetDepthPitch(mip) < WMath::MaxValue<WUInt32>(), "Depth pitch exceeds WGAL limits.");
        id.m_uiSlicePitch = static_cast<WUInt32>(pImage->GetDepthPitch(mip));

        out_uiMemoryUsed += id.m_uiSlicePitch;
      }
    }
  }

  const WArrayPtr<WGALSystemMemoryDescription> InitDataPtr(ref_initData);

  ref_td.m_InitialContent = InitDataPtr;
}


WResourceLoadDesc WTexture3DResource::UpdateContent(WStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    WResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = WResourceState::LoadedResourceMissing;

    return res;
  }

  WTexture3DResourceDescriptor td;
  WImage* pImage = nullptr;
  bool bIsFallback = false;
  WTexFormat texFormat;

  // load image data
  {
    Stream->ReadBytes(&pImage, sizeof(WImage*));
    *Stream >> bIsFallback;
    texFormat.ReadHeader(*Stream);

    td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
    td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
    td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  }

  const bool bIsRenderTarget = texFormat.m_iRenderTargetResolutionX != 0;
  W_ASSERT_DEV(!bIsRenderTarget, "Render targets are not supported by regular 2D texture resources");

  {

    const WUInt32 uiNumMipmapsLowRes =
      WTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : WMath::Min(pImage->GetNumMipLevels(), 6U);
    WUInt32 uiUploadNumMipLevels = 0;
    bool bCouldLoadMore = false;

    if (bIsFallback)
    {
      if (m_uiLoadedTextures == 0)
      {
        // only upload fallback textures, if we don't have any texture data at all, yet
        bCouldLoadMore = true;
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        // ignore this texture entirely, if we already have low res data
        // but assume we could load a higher resolution version
        bCouldLoadMore = true;
        WLog::Debug("Ignoring fallback texture data, low-res resource data is already loaded.");
      }
      else
      {
        WLog::Debug("Ignoring fallback texture data, resource is already fully loaded.");
      }
    }
    else
    {
      if (m_uiLoadedTextures == 0)
      {
        bCouldLoadMore = uiNumMipmapsLowRes < pImage->GetNumMipLevels();
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        uiUploadNumMipLevels = pImage->GetNumMipLevels();
      }
      else
      {
        // ignore the texture, if we already have fully loaded data
        WLog::Debug("Ignoring texture data, resource is already fully loaded.");
      }
    }

    if (uiUploadNumMipLevels > 0)
    {
      W_ASSERT_DEBUG(m_uiLoadedTextures < 2, "Invalid texture upload");

      WTempHybridArray<WGALSystemMemoryDescription, 32> initData;
      FillOutDescriptor(td, pImage, texFormat.m_bSRGB, uiUploadNumMipLevels, m_uiMemoryGPU[m_uiLoadedTextures], initData);

      WTextureUtils::ConfigureSampler(static_cast<WTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

      // ignore its return value here, we build our own
      CreateResource(std::move(td));
    }

    {
      WResourceLoadDesc res;
      res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
      res.m_uiQualityLevelsLoadable = bCouldLoadMore ? 1 : 0;
      res.m_State = WResourceState::Loaded;

      return res;
    }
  }
}

void WTexture3DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WTexture3DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WTexture3DResource, WTexture3DResourceDescriptor)
{
  WResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State = WResourceState::Loaded;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  m_Type = descriptor.m_DescGAL.m_Type;
  m_Format = descriptor.m_DescGAL.m_Format;
  m_uiWidth = descriptor.m_DescGAL.m_uiWidth;
  m_uiHeight = descriptor.m_DescGAL.m_uiHeight;
  m_uiDepth = descriptor.m_DescGAL.m_uiDepth;

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_DescGAL, descriptor.m_InitialContent);
  W_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  pDevice->GetTexture(m_hGALTexture[m_uiLoadedTextures])->SetDebugName(GetResourceDescription());

  if (!m_hSamplerState.IsInvalidated())
  {
    pDevice->DestroySamplerState(m_hSamplerState);
  }

  m_hSamplerState = pDevice->CreateSamplerState(descriptor.m_SamplerDesc);
  W_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Textures_Texture3DResource);
