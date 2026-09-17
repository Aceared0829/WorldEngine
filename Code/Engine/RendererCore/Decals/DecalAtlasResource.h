#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/RendererCoreDLL.h>
#include <Texture/Utils/TextureAtlasDesc.h>

using WDecalAtlasResourceHandle = WTypedResourceHandle<class WDecalAtlasResource>;
using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;

class WImage;

/// Descriptor for creating a decal atlas resource.
///
/// Currently empty as decal atlases are typically loaded from asset files rather than
/// created from descriptors at runtime.
struct WDecalAtlasResourceDescriptor
{
};

/// Resource that stores texture atlases for decals.
///
/// Contains three texture layers (base color, normal, ORM) combined into texture atlases.
/// Each decal references a region within these atlases. ORM stands for Occlusion, Roughness,
/// Metallic packed into RGB channels.
class W_RENDERERCORE_DLL WDecalAtlasResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WDecalAtlasResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WDecalAtlasResource);
  W_RESOURCE_DECLARE_CREATEABLE(WDecalAtlasResource, WDecalAtlasResourceDescriptor);

public:
  WDecalAtlasResource();

  const WTexture2DResourceHandle& GetBaseColorTexture() const { return m_hBaseColor; }
  const WTexture2DResourceHandle& GetNormalTexture() const { return m_hNormal; }
  const WTexture2DResourceHandle& GetORMTexture() const { return m_hORM; }
  const WVec2U32& GetBaseColorTextureSize() const { return m_vBaseColorSize; }
  const WVec2U32& GetNormalTextureSize() const { return m_vNormalSize; }
  const WVec2U32& GetORMTextureSize() const { return m_vORMSize; }
  const WTextureAtlasRuntimeDesc& GetAtlas() const { return m_Atlas; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void ReportResourceIsMissing() override;

  void ReadDecalInfo(WStreamReader* Stream);

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void CreateLayerTexture(const WImage& img, bool bSRGB, WTexture2DResourceHandle& out_hTexture);

  WTextureAtlasRuntimeDesc m_Atlas;
  static WUInt32 s_uiDecalAtlasResources;
  WTexture2DResourceHandle m_hBaseColor;
  WTexture2DResourceHandle m_hNormal;
  WTexture2DResourceHandle m_hORM;
  WVec2U32 m_vBaseColorSize;
  WVec2U32 m_vNormalSize;
  WVec2U32 m_vORMSize;
};
