#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <RendererCore/RendererCoreDLL.h>

using WDecalResourceHandle = WTypedResourceHandle<class WDecalResource>;

/// Descriptor for creating a decal resource.
///
/// Currently empty as decals are typically loaded from asset files.
struct WDecalResourceDescriptor
{
};

/// Resource representing a single decal that references regions in a decal atlas.
///
/// Decal resources are lightweight and reference textures stored in WDecalAtlasResource.
/// The actual texture data and UV mapping is managed by the atlas.
class W_RENDERERCORE_DLL WDecalResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WDecalResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WDecalResource);
  W_RESOURCE_DECLARE_CREATEABLE(WDecalResource, WDecalResourceDescriptor);

public:
  WDecalResource();

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
};

/// Resource loader for decal resources.
///
/// Loads decal metadata from asset files. The actual texture data is loaded separately
/// through the decal atlas system.
class W_RENDERERCORE_DLL WDecalResourceLoader : public WResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    WContiguousMemoryStreamStorage m_Storage;
    WMemoryStreamReader m_Reader;
  };

  virtual WResourceLoadData OpenDataStream(const WResource* pResource) override;
  virtual void CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const WResource* pResource) const override;
};
