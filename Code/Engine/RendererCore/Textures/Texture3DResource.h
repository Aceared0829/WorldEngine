#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/IO/MemoryStream.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>

class WImage;

using WTexture3DResourceHandle = WTypedResourceHandle<class WTexture3DResource>;

/// Use this descriptor in calls to WResourceManager::CreateResource<WTexture3DResource> to create textures from data in memory.
struct W_RENDERERCORE_DLL WTexture3DResourceDescriptor
{
  /// Describes the texture format, etc.
  WGALTextureCreationDescription m_DescGAL;
  WGALSamplerStateCreationDescription m_SamplerDesc;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  WUInt8 m_uiQualityLevelsDiscardable = 0;

  /// How many additional quality levels can be loaded (typically from file).
  WUInt8 m_uiQualityLevelsLoadable = 0;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not
  /// initialize data.
  WArrayPtr<WGALSystemMemoryDescription> m_InitialContent;
};

/// Resource for 3D volume textures.
///
/// 3D textures have width, height, and depth dimensions. They are sampled using 3D coordinates
/// and are commonly used for volumetric effects like fog, clouds, or 3D lookup tables.
class W_RENDERERCORE_DLL WTexture3DResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WTexture3DResource, WResource);

  W_RESOURCE_DECLARE_COMMON_CODE(WTexture3DResource);
  W_RESOURCE_DECLARE_CREATEABLE(WTexture3DResource, WTexture3DResourceDescriptor);

public:
  WTexture3DResource();

  W_ALWAYS_INLINE WGALResourceFormat::Enum GetFormat() const { return m_Format; }
  W_ALWAYS_INLINE WUInt32 GetWidth() const { return m_uiWidth; }
  W_ALWAYS_INLINE WUInt32 GetHeight() const { return m_uiHeight; }
  W_ALWAYS_INLINE WUInt32 GetDepth() const { return m_uiDepth; }
  W_ALWAYS_INLINE WGALTextureType::Enum GetType() const { return m_Type; }

  /// Fills a descriptor from image data, preparing it for texture creation.
  ///
  /// Configures format, dimensions, mipmaps, and sets up initial content from the image.
  static void FillOutDescriptor(WTexture3DResourceDescriptor& ref_td, const WImage* pImage, bool bSRGB, WUInt32 uiNumMipLevels,
    WUInt32& out_uiMemoryUsed, WHybridArray<WGALSystemMemoryDescription, 32>& ref_initData);

  const WGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const WGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WTexture3DResource(DoUpdate ResourceUpdateThread);

  WUInt8 m_uiLoadedTextures = 0;
  WGALTextureHandle m_hGALTexture[2];
  WUInt32 m_uiMemoryGPU[2] = {0, 0};

  WGALTextureType::Enum m_Type = WGALTextureType::Invalid;
  WGALResourceFormat::Enum m_Format = WGALResourceFormat::Invalid;
  WUInt32 m_uiWidth = 0;
  WUInt32 m_uiHeight = 0;
  WUInt32 m_uiDepth = 0;

  WGALSamplerStateHandle m_hSamplerState;
};
