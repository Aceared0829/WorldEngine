#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

class WShaderStageBinary;

using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;
using WTexture3DResourceHandle = WTypedResourceHandle<class WTexture3DResource>;
using WRenderToTexture2DResourceHandle = WTypedResourceHandle<class WRenderToTexture2DResource>;
using WTextureCubeResourceHandle = WTypedResourceHandle<class WTextureCubeResource>;
using WMeshBufferResourceHandle = WTypedResourceHandle<class WMeshBufferResource>;
using WDynamicMeshBufferResourceHandle = WTypedResourceHandle<class WDynamicMeshBufferResource>;
using WMeshResourceHandle = WTypedResourceHandle<class WMeshResource>;
using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;
using WShaderResourceHandle = WTypedResourceHandle<class WShaderResource>;
using WShaderPermutationResourceHandle = WTypedResourceHandle<class WShaderPermutationResource>;
using WRenderPipelineResourceHandle = WTypedResourceHandle<class WRenderPipelineResource>;
using WDecalResourceHandle = WTypedResourceHandle<class WDecalResource>;
using WDecalAtlasResourceHandle = WTypedResourceHandle<class WDecalAtlasResource>;
using WDecalId = WGenericId<16, 8>;

struct W_RENDERERCORE_DLL WPermutationVar
{
  W_DECLARE_MEM_RELOCATABLE_TYPE();

  WHashedString m_sName;
  WHashedString m_sValue;

  W_ALWAYS_INLINE bool operator==(const WPermutationVar& other) const { return m_sName == other.m_sName && m_sValue == other.m_sValue; }
};

struct W_RENDERERCORE_DLL WMeshImportTransform
{
  using StorageType = WInt8;

  enum Enum
  {
    Blender_YUp,
    Blender_ZUp,

    Custom = 127,

    Default = Blender_YUp
  };

  static WBasisAxis::Enum GetRightDir(WMeshImportTransform::Enum transform, WBasisAxis::Enum dir);
  static WBasisAxis::Enum GetUpDir(WMeshImportTransform::Enum transform, WBasisAxis::Enum dir);
  static bool GetFlipForward(WMeshImportTransform::Enum transform, bool bFlip);
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WMeshImportTransform);
