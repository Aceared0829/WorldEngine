#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <Texture/Image/Image.h>

using WTextureCubeResourceHandle = WTypedResourceHandle<class WTextureCubeResource>;

/// Use this descriptor in calls to WResourceManager::CreateResource<WTextureCubeResource> to create textures from data in memory.
struct WTextureCubeResourceDescriptor
{
  WTextureCubeResourceDescriptor()
  {
    m_uiQualityLevelsDiscardable = 0;
    m_uiQualityLevelsLoadable = 0;
  }

  /// Describes the texture format, etc.
  WGALTextureCreationDescription m_DescGAL;
  WGALSamplerStateCreationDescription m_SamplerDesc;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  WUInt8 m_uiQualityLevelsDiscardable;

  /// How many additional quality levels can be loaded (typically from file).
  WUInt8 m_uiQualityLevelsLoadable;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not
  /// initialize data.
  WArrayPtr<WGALSystemMemoryDescription> m_InitialContent;
};

/// Resource for cube map textures.
///
/// Cube maps consist of 6 square texture faces representing the sides of a cube.
/// They are sampled using 3D direction vectors and are commonly used for skyboxes,
/// environment reflections, and irradiance maps.
class W_RENDERERCORE_DLL WTextureCubeResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WTextureCubeResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WTextureCubeResource);
  W_RESOURCE_DECLARE_CREATEABLE(WTextureCubeResource, WTextureCubeResourceDescriptor);

public:
  WTextureCubeResource();

  W_ALWAYS_INLINE WGALResourceFormat::Enum GetFormat() const { return m_Format; }

  /// Returns the size of each cube face (width and height are always equal).
  W_ALWAYS_INLINE WUInt32 GetWidthAndHeight() const { return m_uiWidthAndHeight; }

  const WGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const WGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WUInt8 m_uiLoadedTextures;
  WGALTextureHandle m_hGALTexture[2];
  WUInt32 m_uiMemoryGPU[2];

  WGALResourceFormat::Enum m_Format;
  WUInt32 m_uiWidthAndHeight;

  WGALSamplerStateHandle m_hSamplerState;
};
