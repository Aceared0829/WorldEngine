#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

class W_RENDERERFOUNDATION_DLL WReadbackBufferLock
{
public:
  W_DISALLOW_COPY_AND_ASSIGN(WReadbackBufferLock);

  WReadbackBufferLock() = default;
  WReadbackBufferLock(WGALDevice* pDevice, const WGALReadbackBuffer* pBuffer, WArrayPtr<const WUInt8>& out_memory);
  W_ALWAYS_INLINE WReadbackBufferLock(WReadbackBufferLock&& rhs) { *this = std::move(rhs); }
  ~WReadbackBufferLock();

  void operator=(WReadbackBufferLock&& rhs);

  W_ALWAYS_INLINE bool IsValid() const { return m_pDevice != nullptr; }
  W_ALWAYS_INLINE bool operator!() const { return m_pDevice == nullptr; }
  W_ALWAYS_INLINE operator bool() const { return m_pDevice != nullptr; }

private:
  const WGALDevice* m_pDevice = nullptr;
  const WGALReadbackBuffer* m_pBuffer = nullptr;
};

class W_RENDERERFOUNDATION_DLL WReadbackTextureLock
{
public:
  W_DISALLOW_COPY_AND_ASSIGN(WReadbackTextureLock);

  WReadbackTextureLock() = default;
  WReadbackTextureLock(WGALDevice* pDevice, const WGALReadbackTexture* pTexture, const WArrayPtr<const WGALTextureSubresource>& subResources, WDynamicArray<WGALSystemMemoryDescription>& out_memory);
  W_ALWAYS_INLINE WReadbackTextureLock(WReadbackTextureLock&& rhs) { *this = std::move(rhs); }
  ~WReadbackTextureLock();

  void operator=(WReadbackTextureLock&& rhs);

  W_ALWAYS_INLINE bool IsValid() const { return m_pDevice != nullptr; }
  W_ALWAYS_INLINE bool operator!() const { return m_pDevice == nullptr; }
  W_ALWAYS_INLINE operator bool() const { return m_pDevice != nullptr; }

private:
  const WGALDevice* m_pDevice = nullptr;
  const WGALReadbackTexture* m_pTexture = nullptr;
  WArrayPtr<const WGALTextureSubresource> m_SubResources;
};
