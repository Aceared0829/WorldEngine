#pragma once

#include <RendererFoundation/Device/ReadbackLock.h>

class WGALDevice;

/// Base-class for WGALReadbackBufferHelper and WGALReadbackTextureHelper.
class W_RENDERERFOUNDATION_DLL WGALReadbackHelper
{
public:
  /// Returns the fence of the last readback operation.
  W_FORCE_INLINE WGALFenceHandle GetCurrentFence() const { return m_hFence; }
  /// Returns the current status of the readback.
  [[nodiscard]] WEnum<WGALAsyncResult> GetReadbackResult(WTime timeout) const;

protected:
  WGALDevice* m_pDevice = nullptr;
  WGALFenceHandle m_hFence = 0;
};

/// Helper class that automatically creates a readback buffer and controls it's lifetime.
class W_RENDERERFOUNDATION_DLL WGALReadbackBufferHelper : public WGALReadbackHelper
{
public:
  WGALReadbackBufferHelper() = default;
  ~WGALReadbackBufferHelper();

  /// Free the memory of the readback buffer.
  void Reset();
  /// Starts a readback of a buffer. A new readback buffer will be created if the current one does not match the new buffer.
  WGALFenceHandle ReadbackBuffer(WGALCommandEncoder& ref_encoder, WGALBufferHandle hBuffer);
  /// Same as WGALDevice::LockBuffer, but checks that the fence has been reached. If not, returns an invalid lock object.
  WReadbackBufferLock LockBuffer(WArrayPtr<const WUInt8>& out_memory);

private:
  W_DISALLOW_COPY_AND_ASSIGN(WGALReadbackBufferHelper);
  WGALReadbackBufferHandle m_hReadbackBuffer;
};

/// Helper class that automatically creates a readback texture and controls it's lifetime.
class W_RENDERERFOUNDATION_DLL WGALReadbackTextureHelper : public WGALReadbackHelper
{
public:
  WGALReadbackTextureHelper() = default;
  ~WGALReadbackTextureHelper();

  /// Free the memory of the readback texture.
  void Reset();
  /// Starts a readback of a texture. A new readback texture will be created if the current one does not match the new texture.
  WGALFenceHandle ReadbackTexture(WGALCommandEncoder& ref_encoder, WGALTextureHandle hTexture);
  /// Same as WGALDevice::LockTexture, but checks that the fence has been reached. If not, returns an invalid lock object.
  WReadbackTextureLock LockTexture(const WArrayPtr<const WGALTextureSubresource>& subResources, WDynamicArray<WGALSystemMemoryDescription>& out_memory);

private:
  W_DISALLOW_COPY_AND_ASSIGN(WGALReadbackTextureHelper);
  WGALReadbackTextureHandle m_hReadbackTexture;
};
