#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Decals/DecalResource.h>

static WDecalResourceLoader s_DecalResourceLoader;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "TextureResource"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WResourceManager::SetResourceTypeLoader<WDecalResource>(&s_DecalResourceLoader);

    WDecalResourceDescriptor desc;
    WDecalResourceHandle hFallback = WResourceManager::CreateResource<WDecalResource>("Fallback Decal", std::move(desc), "Empty Decal for loading and missing decals");

    WResourceManager::SetResourceTypeLoadingFallback<WDecalResource>(hFallback);
    WResourceManager::SetResourceTypeMissingFallback<WDecalResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WResourceManager::SetResourceTypeLoader<WDecalResource>(nullptr);

    WResourceManager::SetResourceTypeLoadingFallback<WDecalResource>(WDecalResourceHandle());
    WResourceManager::SetResourceTypeMissingFallback<WDecalResource>(WDecalResourceHandle());
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
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDecalResource, 1, WRTTIDefaultAllocator<WDecalResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WDecalResource);
// clang-format on

WDecalResource::WDecalResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WResourceLoadDesc WDecalResource::UnloadData(Unload WhatToUnload)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WDecalResource::UpdateContent(WStreamReader* Stream)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

void WDecalResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WDecalResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WDecalResource, WDecalResourceDescriptor)
{
  WResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = WResourceState::Loaded;

  return ret;
}

//////////////////////////////////////////////////////////////////////////

WResourceLoadData WDecalResourceLoader::OpenDataStream(const WResource* pResource)
{
  // nothing to load, decals are solely identified by their id (name)
  // the rest of the information is in the decal atlas resource

  WResourceLoadData res;
  return res;
}

void WDecalResourceLoader::CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData)
{
  // nothing to do
}

bool WDecalResourceLoader::IsResourceOutdated(const WResource* pResource) const
{
  // decals are never outdated
  return false;
}



W_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalResource);
