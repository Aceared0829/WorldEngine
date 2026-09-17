#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Wrapper around WGALBufferHandle that automates buffer updates.
///
/// Created via WRenderContext::CreateConstantBufferStorage. Retrieved via WRenderContext::TryGetConstantBufferStorage,
/// updated lazily via WRenderContext::UploadConstants. Uses hashing to avoid redundant uploads when data hasn't changed.
class W_RENDERERCORE_DLL WConstantBufferStorageBase
{
protected:
  friend class WRenderContext;
  friend class WMemoryUtils;

  WConstantBufferStorageBase(WUInt32 uiSizeInBytes);
  ~WConstantBufferStorageBase();

public:
  /// Returns writable access to the buffer data.
  ///
  /// Marks the buffer as modified for the next upload.
  WArrayPtr<WUInt8> GetRawDataForWriting();

  /// Returns read-only access to the buffer data.
  WArrayPtr<const WUInt8> GetRawDataForReading() const;

  /// Called at the beginning of each frame to reset per-frame state.
  void BeforeBeginFrame() { m_bStartOfFrame = true; }

  /// Uploads modified data to the GPU.
  ///
  /// Uses hashing to skip upload if data hasn't changed since last upload.
  void UploadData(WGALCommandEncoder* pCommandEncoder);

  W_ALWAYS_INLINE WGALBufferHandle GetGALBufferHandle() const { return m_hGALConstantBuffer; }

protected:
  bool m_bHasBeenModified = false;
  bool m_bStartOfFrame = true;
  WUInt32 m_uiLastHash = 0;
  WGALBufferHandle m_hGALConstantBuffer;

  WArrayPtr<WUInt8> m_Data;
};

/// Typed wrapper for constant buffer storage.
///
/// Provides type-safe access to constant buffer data of type T.
template <typename T>
class WConstantBufferStorage : public WConstantBufferStorageBase
{
public:
  /// Returns a typed reference for writing to the constant buffer.
  ///
  /// Marks the buffer as modified for upload.
  W_FORCE_INLINE T& GetDataForWriting()
  {
    WArrayPtr<WUInt8> rawData = GetRawDataForWriting();
    W_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<T*>(rawData.GetPtr());
  }

  /// Returns a typed const reference for reading from the constant buffer.
  W_FORCE_INLINE const T& GetDataForReading() const
  {
    WArrayPtr<const WUInt8> rawData = GetRawDataForReading();
    W_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<const T*>(rawData.GetPtr());
  }
};

using WConstantBufferStorageId = WGenericId<24, 8>;

class WConstantBufferStorageHandle
{
  W_DECLARE_HANDLE_TYPE(WConstantBufferStorageHandle, WConstantBufferStorageId);

  friend class WRenderContext;
};
