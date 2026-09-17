#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WGALDevice;

/// Creates fallback resources in case the high-level renderer did not map a resource to a binding slot.
class W_RENDERERFOUNDATION_DLL WGALRendererFallbackResources
{
public:
  static const WGALBufferHandle GetFallbackBuffer(WEnum<WGALShaderResourceType> resourceType);
  static const WGALTextureHandle GetFallbackTexture(WEnum<WGALShaderResourceType> resourceType, WEnum<WGALShaderTextureType> textureType, bool bDepth);

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, FallbackResources)
  static void GALDeviceEventHandler(const WGALDeviceEvent& e);
  static void Initialize();
  static void DeInitialize();

  static WGALDevice* s_pDevice;
  static WEventSubscriptionID s_EventID;

  struct Key
  {
    W_DECLARE_POD_TYPE();
    WEnum<WGALShaderResourceType> m_ResourceType;
    WEnum<WGALShaderTextureType> m_WType;
    bool m_bDepth = false;
  };

  struct KeyHash
  {
    static WUInt32 Hash(const Key& a);
    static bool Equal(const Key& a, const Key& b);

    static WUInt32 Hash(const WEnum<WGALShaderResourceType>& a);
    static bool Equal(const WEnum<WGALShaderResourceType>& a, const WEnum<WGALShaderResourceType>& b);
  };

  static WHashTable<Key, WGALTextureHandle, KeyHash> s_TextureResourceViews;
  static WHashTable<WEnum<WGALShaderResourceType>, WGALBufferHandle, KeyHash> s_BufferResourceViews;

  static WDynamicArray<WGALBufferHandle> s_Buffers;
  static WDynamicArray<WGALTextureHandle> s_Textures;
};
