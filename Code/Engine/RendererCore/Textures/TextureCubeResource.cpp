#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/WTexFormat/WTexFormat.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureCubeResource, 1, WRTTIDefaultAllocator<WTextureCubeResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WTextureCubeResource);
// clang-format on

WTextureCubeResource::WTextureCubeResource()
  : WResource(DoUpdate::OnGraphicsResourceThreads, WTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
  m_uiLoadedTextures = 0;
  m_uiMemoryGPU[0] = 0;
  m_uiMemoryGPU[1] = 0;
  m_Format = WGALResourceFormat::Invalid;
  m_uiWidthAndHeight = 0;
}

WResourceLoadDesc WTextureCubeResource::UnloadData(Unload WhatToUnload)
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

WResourceLoadDesc WTextureCubeResource::UpdateContent(WStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    WResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = WResourceState::LoadedResourceMissing;

    return res;
  }

  WImage* pImage = nullptr;
  Stream->ReadBytes(&pImage, sizeof(WImage*));

  bool bIsFallback = false;
  *Stream >> bIsFallback;

  WTexFormat texFormat;
  texFormat.ReadHeader(*Stream);

  const WUInt32 uiNumMipmapsLowRes = WTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : 6;

  const WUInt32 uiNumMipLevels = WMath::Min(m_uiLoadedTextures == 0 ? uiNumMipmapsLowRes : pImage->GetNumMipLevels(), pImage->GetNumMipLevels());
  const WUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  if (pImage->GetWidth(uiHighestMipLevel) != pImage->GetHeight(uiHighestMipLevel))
  {
    WLog::Error("Cubemap width '{0}' is not identical to height '{1}'", pImage->GetWidth(uiHighestMipLevel), pImage->GetHeight(uiHighestMipLevel));

    WResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = WResourceState::LoadedResourceMissing;

    return res;
  }

  m_Format = WTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), texFormat.m_bSRGB);
  m_uiWidthAndHeight = pImage->GetWidth(uiHighestMipLevel);

  WGALTextureCreationDescription texDesc;
  texDesc.m_Format = m_Format;
  texDesc.m_uiWidth = m_uiWidthAndHeight;
  texDesc.m_uiHeight = m_uiWidthAndHeight;
  texDesc.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  texDesc.m_uiMipLevelCount = uiNumMipLevels;
  texDesc.m_uiArraySize = pImage->GetNumArrayIndices();
  texDesc.m_ResourceAccess.m_bImmutable = true;

  if (texDesc.m_uiDepth > 1)
    texDesc.m_Type = WGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    texDesc.m_Type = WGALTextureType::TextureCube;

  if (texDesc.m_uiArraySize > 1)
  {
    if (texDesc.m_Type == WGALTextureType::TextureCube)
    {
      texDesc.m_Type = WGALTextureType::TextureCubeArray;
    }
    else if (texDesc.m_Type == WGALTextureType::Texture2D)
    {
      texDesc.m_Type = WGALTextureType::Texture2DArray;
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }
  }

  W_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '{0}')", GetResourceID());

  m_uiMemoryGPU[m_uiLoadedTextures] = 0;

  WTempHybridArray<WGALSystemMemoryDescription, 32> InitData;

  for (WUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (WUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (WUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        WGALSystemMemoryDescription& id = InitData.ExpandAndGetRef();

        id.m_pData = pImage->GetSubImageView(mip, face, array_index).GetByteBlobPtr();

        W_ASSERT_DEV(pImage->GetDepthPitch(mip) < WMath::MaxValue<WUInt32>(), "Depth pitch exceeds WGAL limits.");

        if (WImageFormat::GetType(pImage->GetImageFormat()) == WImageFormatType::BLOCK_COMPRESSED)
        {
          const WUInt32 uiMemPitchFactor = WGALResourceFormat::GetBitsPerElement(m_Format) * 4 / 8;

          id.m_uiRowPitch = WMath::Max<WUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiRowPitch = static_cast<WUInt32>(pImage->GetRowPitch(mip));
        }

        id.m_uiSlicePitch = static_cast<WUInt32>(pImage->GetDepthPitch(mip));

        m_uiMemoryGPU[m_uiLoadedTextures] += id.m_uiSlicePitch;
      }
    }
  }

  const WArrayPtr<WGALSystemMemoryDescription> InitDataPtr(InitData);

  WTextureCubeResourceDescriptor td;
  td.m_DescGAL = texDesc;
  td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
  td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
  td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  td.m_InitialContent = InitDataPtr;

  WTextureUtils::ConfigureSampler(static_cast<WTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

  // ignore its return value here, we build our own
  CreateResource(std::move(td));

  {
    WResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;

    if (uiHighestMipLevel == 0)
      res.m_uiQualityLevelsLoadable = 0;
    else
      res.m_uiQualityLevelsLoadable = 1;

    res.m_State = WResourceState::Loaded;

    return res;
  }
}

void WTextureCubeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WTextureCubeResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WTextureCubeResource, WTextureCubeResourceDescriptor)
{
  WResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State = WResourceState::Loaded;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  W_ASSERT_DEV(descriptor.m_DescGAL.m_uiWidth == descriptor.m_DescGAL.m_uiHeight, "Cubemap width and height must be identical");

  m_Format = descriptor.m_DescGAL.m_Format;
  m_uiWidthAndHeight = descriptor.m_DescGAL.m_uiWidth;

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



W_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureCubeResource);
