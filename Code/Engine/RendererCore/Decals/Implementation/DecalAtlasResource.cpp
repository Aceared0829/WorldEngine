#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalAtlasResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "TextureResource"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WDecalAtlasResourceDescriptor desc;
    WDecalAtlasResourceHandle hFallback = WResourceManager::CreateResource<WDecalAtlasResource>("Fallback Decal Atlas", std::move(desc), "Empty Decal Atlas for loading and missing decals");

    WResourceManager::SetResourceTypeLoadingFallback<WDecalAtlasResource>(hFallback);
    WResourceManager::SetResourceTypeMissingFallback<WDecalAtlasResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WResourceManager::SetResourceTypeLoadingFallback<WDecalAtlasResource>(WDecalAtlasResourceHandle());
    WResourceManager::SetResourceTypeMissingFallback<WDecalAtlasResource>(WDecalAtlasResourceHandle());
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on


//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDecalAtlasResource, 1, WRTTIDefaultAllocator<WDecalAtlasResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WDecalAtlasResource);
// clang-format on

WUInt32 WDecalAtlasResource::s_uiDecalAtlasResources = 0;

WDecalAtlasResource::WDecalAtlasResource()
  : WResource(DoUpdate::OnAnyThread, 1)
  , m_vBaseColorSize(WVec2U32::MakeZero())
  , m_vNormalSize(WVec2U32::MakeZero())
{
}

WResourceLoadDesc WDecalAtlasResource::UnloadData(Unload WhatToUnload)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WDecalAtlasResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WDecalAtlasResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::LoadedResourceMissing;

  if (Stream == nullptr)
    return res;

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // skip the asset header
  {
    WAssetFileHeader header;
    header.Read(*Stream).IgnoreResult();
  }

  {
    WUInt8 uiVersion = 0;
    *Stream >> uiVersion;
    W_ASSERT_DEV(uiVersion <= 4, "Invalid decal atlas version {0}", uiVersion);

    // this version is now incompatible
    if (uiVersion < 4)
      return res;
  }

  // read the textures
  {
    WDdsFileFormat dds;
    WImage baseColor, normal, orm;

    if (dds.ReadImage(*Stream, baseColor, "dds").Failed())
    {
      WLog::Error("Failed to load baseColor image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, normal, "dds").Failed())
    {
      WLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, orm, "dds").Failed())
    {
      WLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    CreateLayerTexture(baseColor, true, m_hBaseColor);
    CreateLayerTexture(normal, false, m_hNormal);
    CreateLayerTexture(orm, false, m_hORM);

    m_vBaseColorSize = WVec2U32(baseColor.GetWidth(), baseColor.GetHeight());
    m_vNormalSize = WVec2U32(normal.GetWidth(), normal.GetHeight());
    m_vORMSize = WVec2U32(orm.GetWidth(), orm.GetHeight());
  }

  ReadDecalInfo(Stream);

  res.m_State = WResourceState::Loaded;
  return res;
}

void WDecalAtlasResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WDecalAtlasResource) + (WUInt32)m_Atlas.m_Items.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WDecalAtlasResource, WDecalAtlasResourceDescriptor)
{
  WResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = WResourceState::Loaded;

  m_Atlas.Clear();

  return ret;
}

void WDecalAtlasResource::CreateLayerTexture(const WImage& img, bool bSRGB, WTexture2DResourceHandle& out_hTexture)
{
  WTexture2DResourceDescriptor td;
  td.m_SamplerDesc.m_AddressU = WImageAddressMode::Clamp;
  td.m_SamplerDesc.m_AddressV = WImageAddressMode::Clamp;
  td.m_SamplerDesc.m_AddressW = WImageAddressMode::Clamp;

  WUInt32 uiMemory;
  WTempHybridArray<WGALSystemMemoryDescription, 32> initData;
  WTexture2DResource::FillOutDescriptor(td, &img, bSRGB, img.GetNumMipLevels(), uiMemory, initData);
  WTextureUtils::ConfigureSampler(WTextureFilterSetting::HighQuality, td.m_SamplerDesc);

  WStringBuilder sTexId;
  sTexId.SetFormat("{0}_Tex{1}", GetResourceID(), s_uiDecalAtlasResources);
  ++s_uiDecalAtlasResources;

  out_hTexture = WResourceManager::CreateResource<WTexture2DResource>(sTexId, std::move(td));
}

void WDecalAtlasResource::ReadDecalInfo(WStreamReader* Stream)
{
  m_Atlas.Deserialize(*Stream).IgnoreResult();
}

void WDecalAtlasResource::ReportResourceIsMissing()
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  // normal during development, don't care much
  WLog::Debug("Decal Atlas Resource is missing: '{0}' ('{1}')", GetResourceID(), GetResourceDescription());
#else
  // should probably exist for shipped applications, report this
  WLog::Warning("Decal Atlas Resource is missing: '{0}' ('{1}')", GetResourceID(), GetResourceDescription());
#endif
}



W_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalAtlasResource);
