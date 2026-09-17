#pragma once

template <typename T>
struct WImageSizeofHelper
{
  static constexpr size_t Size = sizeof(T);
};

template <>
struct WImageSizeofHelper<void>
{
  static constexpr size_t Size = 1;
};

template <>
struct WImageSizeofHelper<const void>
{
  static constexpr size_t Size = 1;
};

template <typename T>
WBlobPtr<const T> WImageView::GetBlobPtr() const
{
  for (WUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
  {
    ValidateDataTypeAccessor<T>(uiPlaneIndex);
  }
  return WBlobPtr<const T>(reinterpret_cast<T*>(static_cast<WUInt8*>(m_DataPtr.GetPtr())), m_DataPtr.GetCount() / WImageSizeofHelper<T>::Size);
}

inline WConstByteBlobPtr WImageView::GetByteBlobPtr() const
{
  for (WUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
  {
    ValidateDataTypeAccessor<WUInt8>(uiPlaneIndex);
  }
  return WConstByteBlobPtr(static_cast<WUInt8*>(m_DataPtr.GetPtr()), m_DataPtr.GetCount());
}

template <typename T>
WBlobPtr<T> WImage::GetBlobPtr()
{
  WBlobPtr<const T> constPtr = WImageView::GetBlobPtr<T>();

  return WBlobPtr<T>(const_cast<T*>(static_cast<const T*>(constPtr.GetPtr())), constPtr.GetCount());
}

inline WByteBlobPtr WImage::GetByteBlobPtr()
{
  WConstByteBlobPtr constPtr = WImageView::GetByteBlobPtr();

  return WByteBlobPtr(const_cast<WUInt8*>(constPtr.GetPtr()), constPtr.GetCount());
}

template <typename T>
const T* WImageView::GetPixelPointer(WUInt32 uiMipLevel /*= 0*/, WUInt32 uiFace /*= 0*/, WUInt32 uiArrayIndex /*= 0*/, WUInt32 x /*= 0*/,
  WUInt32 y /*= 0*/, WUInt32 z /*= 0*/, WUInt32 uiPlaneIndex /*= 0*/) const
{
  ValidateDataTypeAccessor<T>(uiPlaneIndex);
  W_ASSERT_DEV(x < GetNumBlocksX(uiMipLevel, uiPlaneIndex), "Invalid x coordinate");
  W_ASSERT_DEV(y < GetNumBlocksY(uiMipLevel, uiPlaneIndex), "Invalid y coordinate");
  W_ASSERT_DEV(z < GetNumBlocksZ(uiMipLevel, uiPlaneIndex), "Invalid z coordinate");

  WUInt64 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex) +
                    z * GetDepthPitch(uiMipLevel, uiPlaneIndex) +
                    y * GetRowPitch(uiMipLevel, uiPlaneIndex) +
                    x * WImageFormat::GetBitsPerBlock(m_Format, uiPlaneIndex) / 8;
  return reinterpret_cast<const T*>(&m_DataPtr[offset]);
}

template <typename T>
T* WImage::GetPixelPointer(
  WUInt32 uiMipLevel /*= 0*/, WUInt32 uiFace /*= 0*/, WUInt32 uiArrayIndex /*= 0*/, WUInt32 x /*= 0*/, WUInt32 y /*= 0*/, WUInt32 z /*= 0*/, WUInt32 uiPlaneIndex /*= 0*/)
{
  return const_cast<T*>(WImageView::GetPixelPointer<T>(uiMipLevel, uiFace, uiArrayIndex, x, y, z, uiPlaneIndex));
}


template <typename T>
void WImageView::ValidateDataTypeAccessor(WUInt32 uiPlaneIndex) const
{
  WUInt32 bytesPerBlock = WImageFormat::GetBitsPerBlock(GetImageFormat(), uiPlaneIndex) / 8;
  W_IGNORE_UNUSED(bytesPerBlock);
  W_ASSERT_DEV(bytesPerBlock % WImageSizeofHelper<T>::Size == 0, "Accessor type is not suitable for interpreting contained data");
}
