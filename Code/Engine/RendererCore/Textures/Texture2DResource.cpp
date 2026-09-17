#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/WTexFormat/WTexFormat.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTexture2DResource, 1, WRTTIDefaultAllocator<WTexture2DResource>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCVarInt cvar_RenderingOffscreenTargetResolution1("Rendering.Offscreen.TargetResolution1", 256, WCVarFlags::Default, "Configurable render target resolution");
WCVarInt cvar_RenderingOffscreenTargetResolution2("Rendering.Offscreen.TargetResolution2", 512, WCVarFlags::Default, "Configurable render target resolution");

W_RESOURCE_IMPLEMENT_COMMON_CODE(WTexture2DResource);

WTexture2DResource::WTexture2DResource()
  : WResource(DoUpdate::OnGraphicsResourceThreads, WTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

WTexture2DResource::WTexture2DResource(WResource::DoUpdate ResourceUpdateThread)
  : WResource(ResourceUpdateThread, WTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

WResourceLoadDesc WTexture2DResource::UnloadData(Unload WhatToUnload)
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

void WTexture2DResource::FillOutDescriptor(WTexture2DResourceDescriptor& ref_td, const WImage* pImage, bool bSRGB, WUInt32 uiNumMipLevels,
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

  if (WImageFormat::GetType(pImage->GetImageFormat()) == WImageFormatType::BLOCK_COMPRESSED)
  {
    ref_td.m_DescGAL.m_uiWidth = WMath::RoundUp(ref_td.m_DescGAL.m_uiWidth, 4);
    ref_td.m_DescGAL.m_uiHeight = WMath::RoundUp(ref_td.m_DescGAL.m_uiHeight, 4);
  }

  if (ref_td.m_DescGAL.m_uiDepth > 1)
    ref_td.m_DescGAL.m_Type = WGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    ref_td.m_DescGAL.m_Type = WGALTextureType::TextureCube;

  if (ref_td.m_DescGAL.m_uiArraySize > 1)
  {
    if (ref_td.m_DescGAL.m_Type == WGALTextureType::TextureCube)
    {
      ref_td.m_DescGAL.m_Type = WGALTextureType::TextureCubeArray;
    }
    else if (ref_td.m_DescGAL.m_Type == WGALTextureType::Texture2D)
    {
      ref_td.m_DescGAL.m_Type = WGALTextureType::Texture2DArray;
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }
  }

  W_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces");

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

          id.m_uiRowPitch = WMath::RoundUp(pImage->GetWidth(mip), 4) * uiMemPitchFactor;
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


WResourceLoadDesc WTexture2DResource::UpdateContent(WStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    WResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = WResourceState::LoadedResourceMissing;

    return res;
  }

  WTexture2DResourceDescriptor td;
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

    const WUInt32 uiNumMipmapsLowRes = WTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : WMath::Min(pImage->GetNumMipLevels(), 6U);
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

void WTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WTexture2DResource, WTexture2DResourceDescriptor)
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

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_DescGAL, descriptor.m_InitialContent);
  W_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  WStringBuilder name;
  name.SetFormat("{} ([{}] - {}x{})", GetResourceIdOrDescription(), m_uiLoadedTextures, m_uiWidth, m_uiHeight);
  pDevice->GetTexture(m_hGALTexture[m_uiLoadedTextures])->SetDebugName(name);

  if (!m_hSamplerState.IsInvalidated())
  {
    pDevice->DestroySamplerState(m_hSamplerState);
  }

  m_hSamplerState = pDevice->CreateSamplerState(descriptor.m_SamplerDesc);
  W_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// TODO (resources): move into separate file

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderToTexture2DResource, 1, WRTTIDefaultAllocator<WRenderToTexture2DResource>);
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, Texture2D)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    WResourceManager::RegisterResourceOverrideType(WGetStaticRTTI<WRenderToTexture2DResource>(), [](const WStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".WBinRenderTarget");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WResourceManager::UnregisterResourceOverrideType(WGetStaticRTTI<WRenderToTexture2DResource>());
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

W_RESOURCE_IMPLEMENT_COMMON_CODE(WRenderToTexture2DResource);

W_RESOURCE_IMPLEMENT_CREATEABLE(WRenderToTexture2DResource, WRenderToTexture2DResourceDescriptor)
{
  WResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = WResourceState::Loaded;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  m_Type = WGALTextureType::Texture2D;
  m_Format = descriptor.m_Format;
  m_uiWidth = descriptor.m_uiWidth;
  m_uiHeight = descriptor.m_uiHeight;

  WGALTextureCreationDescription descGAL;
  descGAL.SetAsRenderTarget(m_uiWidth, m_uiHeight, m_Format, descriptor.m_SampleCount);

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descGAL, descriptor.m_InitialContent);
  W_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture data could not be uploaded to the GPU");

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

WResourceLoadDesc WRenderToTexture2DResource::UnloadData(Unload WhatToUnload)
{
  for (WInt32 r = 0; r < 2; ++r)
  {
    if (!m_hGALTexture[r].IsInvalidated())
    {
      WGALDevice::GetDefaultDevice()->DestroyTexture(m_hGALTexture[r]);
      m_hGALTexture[r].Invalidate();
    }

    m_uiMemoryGPU[r] = 0;
  }

  m_uiLoadedTextures = 0;

  if (!m_hSamplerState.IsInvalidated())
  {
    WGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
    m_hSamplerState.Invalidate();
  }

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable = 2 - m_uiLoadedTextures;
  res.m_State = WResourceState::Unloaded;
  return res;
}

WGALRenderTargetViewHandle WRenderToTexture2DResource::GetRenderTargetView() const
{
  return WGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hGALTexture[0]);
}

void WRenderToTexture2DResource::AddRenderView(WViewHandle hView)
{
  m_RenderViews.PushBack(hView);
}

void WRenderToTexture2DResource::RemoveRenderView(WViewHandle hView)
{
  m_RenderViews.RemoveAndSwap(hView);
}

const WDynamicArray<WViewHandle>& WRenderToTexture2DResource::GetAllRenderViews() const
{
  return m_RenderViews;
}

static WUInt16 GetNextBestResolution(float fRes)
{
  fRes = WMath::Clamp(fRes, 8.0f, 4096.0f);

  int mulEight = (int)WMath::Floor((fRes + 7.9f) / 8.0f);

  return static_cast<WUInt16>(mulEight * 8);
}

WResourceLoadDesc WRenderToTexture2DResource::UpdateContent(WStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    WResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = WResourceState::LoadedResourceMissing;

    return res;
  }

  WRenderToTexture2DResourceDescriptor td;
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

  W_ASSERT_DEV(bIsRenderTarget, "Trying to create a RenderToTexture resource from data that is not set up as a render-target");

  {
    W_ASSERT_DEV(m_uiLoadedTextures == 0, "not implemented");

    if (texFormat.m_iRenderTargetResolutionX == -1)
    {
      if (texFormat.m_iRenderTargetResolutionY == 1)
      {
        texFormat.m_iRenderTargetResolutionX = GetNextBestResolution(cvar_RenderingOffscreenTargetResolution1 * texFormat.m_fResolutionScale);
        texFormat.m_iRenderTargetResolutionY = texFormat.m_iRenderTargetResolutionX;
      }
      else if (texFormat.m_iRenderTargetResolutionY == 2)
      {
        texFormat.m_iRenderTargetResolutionX = GetNextBestResolution(cvar_RenderingOffscreenTargetResolution2 * texFormat.m_fResolutionScale);
        texFormat.m_iRenderTargetResolutionY = texFormat.m_iRenderTargetResolutionX;
      }
      else
      {
        W_REPORT_FAILURE(
          "Invalid render target configuration: {0} x {1}", texFormat.m_iRenderTargetResolutionX, texFormat.m_iRenderTargetResolutionY);
      }
    }

    td.m_Format = static_cast<WGALResourceFormat::Enum>(texFormat.m_GalRenderTargetFormat);
    td.m_uiWidth = texFormat.m_iRenderTargetResolutionX;
    td.m_uiHeight = texFormat.m_iRenderTargetResolutionY;

    WTextureUtils::ConfigureSampler(static_cast<WTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

    m_uiLoadedTextures = 0;

    CreateResource(std::move(td));
  }

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;
  return res;
}

void WRenderToTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WRenderToTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

W_STATICLINK_FILE(RendererCore, RendererCore_Textures_Texture2DResource);
