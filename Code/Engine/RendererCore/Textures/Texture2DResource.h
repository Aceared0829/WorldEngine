#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WImage;

using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;

/// Use this descriptor in calls to WResourceManager::CreateResource<WTexture2DResource> to create textures from data in memory.
struct W_RENDERERCORE_DLL WTexture2DResourceDescriptor
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

class W_RENDERERCORE_DLL WTexture2DResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WTexture2DResource, WResource);

  W_RESOURCE_DECLARE_COMMON_CODE(WTexture2DResource);
  W_RESOURCE_DECLARE_CREATEABLE(WTexture2DResource, WTexture2DResourceDescriptor);

public:
  WTexture2DResource();

  W_ALWAYS_INLINE WGALResourceFormat::Enum GetFormat() const { return m_Format; }
  W_ALWAYS_INLINE WUInt32 GetWidth() const { return m_uiWidth; }
  W_ALWAYS_INLINE WUInt32 GetHeight() const { return m_uiHeight; }
  W_ALWAYS_INLINE WGALTextureType::Enum GetType() const { return m_Type; }

  static void FillOutDescriptor(WTexture2DResourceDescriptor& ref_td, const WImage* pImage, bool bSRGB, WUInt32 uiNumMipLevels,
    WUInt32& out_uiMemoryUsed, WHybridArray<WGALSystemMemoryDescription, 32>& ref_initData);

  const WGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const WGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WTexture2DResource(DoUpdate ResourceUpdateThread);

  WUInt8 m_uiLoadedTextures = 0;
  WGALTextureHandle m_hGALTexture[2];
  WUInt32 m_uiMemoryGPU[2] = {0, 0};

  WGALTextureType::Enum m_Type = WGALTextureType::Invalid;
  WGALResourceFormat::Enum m_Format = WGALResourceFormat::Invalid;
  WUInt32 m_uiWidth = 0;
  WUInt32 m_uiHeight = 0;

  WGALSamplerStateHandle m_hSamplerState;
};

//////////////////////////////////////////////////////////////////////////

using WRenderToTexture2DResourceHandle = WTypedResourceHandle<class WRenderToTexture2DResource>;

struct W_RENDERERCORE_DLL WRenderToTexture2DResourceDescriptor
{
  WUInt32 m_uiWidth = 0;
  WUInt32 m_uiHeight = 0;
  WEnum<WGALMSAASampleCount> m_SampleCount;
  WEnum<WGALResourceFormat> m_Format = WGALResourceFormat::RGBAUByteNormalizedsRGB;
  WGALSamplerStateCreationDescription m_SamplerDesc;
  WArrayPtr<WGALSystemMemoryDescription> m_InitialContent;
};

class W_RENDERERCORE_DLL WRenderToTexture2DResource : public WTexture2DResource
{
  W_ADD_DYNAMIC_REFLECTION(WRenderToTexture2DResource, WTexture2DResource);

  W_RESOURCE_DECLARE_COMMON_CODE(WRenderToTexture2DResource);
  W_RESOURCE_DECLARE_CREATEABLE(WRenderToTexture2DResource, WRenderToTexture2DResourceDescriptor);

public:
  WGALRenderTargetViewHandle GetRenderTargetView() const;
  void AddRenderView(WViewHandle hView);
  void RemoveRenderView(WViewHandle hView);
  const WDynamicArray<WViewHandle>& GetAllRenderViews() const;

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

protected:
  // other views that use this texture as their target
  WDynamicArray<WViewHandle> m_RenderViews;
};
