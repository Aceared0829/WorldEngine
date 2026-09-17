#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

WImageView::WImageView()
{
  Clear();
}

WImageView::WImageView(const WImageHeader& header, WConstByteBlobPtr imageData)
{
  ResetAndViewExternalStorage(header, imageData);
}

void WImageView::Clear()
{
  WImageHeader::Clear();
  m_SubImageOffsets.Clear();
  m_DataPtr.Clear();
}

bool WImageView::IsValid() const
{
  return !m_DataPtr.IsEmpty();
}

void WImageView::ResetAndViewExternalStorage(const WImageHeader& header, WConstByteBlobPtr imageData)
{
  static_cast<WImageHeader&>(*this) = header;

  WUInt64 dataSize = ComputeLayout();

  W_IGNORE_UNUSED(dataSize);
  W_ASSERT_DEV(imageData.GetCount() == dataSize, "Provided image storage ({} bytes) doesn't match required data size ({} bytes)",
    imageData.GetCount(), dataSize);

  // Const cast is safe here as we will only perform non-const access if this is an WImage which owns mutable access to the storage
  m_DataPtr = WBlobPtr<WUInt8>(const_cast<WUInt8*>(static_cast<const WUInt8*>(imageData.GetPtr())), imageData.GetCount());
}

WResult WImageView::SaveTo(WStringView sFileName) const
{
  W_LOG_BLOCK("Writing Image", sFileName);

  if (m_Format == WImageFormat::UNKNOWN)
  {
    WLog::Error("Cannot write image '{0}' - image data is invalid or empty", sFileName);
    return W_FAILURE;
  }

  WFileWriter writer;
  if (writer.Open(sFileName) == W_FAILURE)
  {
    WLog::Error("Failed to open image file '{0}'", sFileName);
    return W_FAILURE;
  }

  WStringView it = WPathUtils::GetFileExtension(sFileName);

  if (const WImageFileFormat* pFormat = WImageFileFormat::GetWriterFormat(it))
  {
    if (pFormat->WriteImage(writer, *this, it) != W_SUCCESS)
    {
      WLog::Error("Failed to write image file '{0}'", sFileName);
      return W_FAILURE;
    }

    return W_SUCCESS;
  }

  WLog::Error("No known image file format for extension '{0}'", it);
  return W_FAILURE;
}

const WImageHeader& WImageView::GetHeader() const
{
  return *this;
}

WImageView WImageView::GetRowView(
  WUInt32 uiMipLevel /*= 0*/, WUInt32 uiFace /*= 0*/, WUInt32 uiArrayIndex /*= 0*/, WUInt32 y /*= 0*/, WUInt32 z /*= 0*/, WUInt32 uiPlaneIndex /*= 0*/) const
{
  WImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the subformat
  WImageFormat::Enum subFormat = WImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * WImageFormat::GetBlockWidth(subFormat) / WImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(WImageFormat::GetBlockHeight(m_Format, 0) * WImageFormat::GetBlockHeight(subFormat) / WImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(WImageFormat::GetBlockDepth(subFormat) / WImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(WImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex));

  WUInt64 offset = 0;

  offset += GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  offset += z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  offset += y * GetRowPitch(uiMipLevel, uiPlaneIndex);

  WBlobPtr<const WUInt8> dataSlice = m_DataPtr.GetSubArray(offset, GetRowPitch(uiMipLevel, uiPlaneIndex));
  return WImageView(header, WConstByteBlobPtr(dataSlice.GetPtr(), dataSlice.GetCount()));
}

void WImageView::ReinterpretAs(WImageFormat::Enum format)
{
  W_ASSERT_DEBUG(
    WImageFormat::IsCompressed(format) == WImageFormat::IsCompressed(GetImageFormat()), "Cannot reinterpret compressed and non-compressed formats");

  W_ASSERT_DEBUG(WImageFormat::GetBitsPerPixel(GetImageFormat()) == WImageFormat::GetBitsPerPixel(format),
    "Cannot reinterpret between formats of different sizes");

  SetImageFormat(format);
}

WUInt64 WImageView::ComputeLayout()
{
  m_SubImageOffsets.Clear();
  m_SubImageOffsets.Reserve(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices * GetPlaneCount());

  WUInt64 uiDataSize = 0;

  for (WUInt32 uiArrayIndex = 0; uiArrayIndex < m_uiNumArrayIndices; uiArrayIndex++)
  {
    for (WUInt32 uiFace = 0; uiFace < m_uiNumFaces; uiFace++)
    {
      for (WUInt32 uiMipLevel = 0; uiMipLevel < m_uiNumMipLevels; uiMipLevel++)
      {
        for (WUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); uiPlaneIndex++)
        {
          m_SubImageOffsets.PushBack(uiDataSize);

          uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * GetDepth(uiMipLevel);
        }
      }
    }
  }

  // Push back total size as a marker
  m_SubImageOffsets.PushBack(uiDataSize);

  return uiDataSize;
}

void WImageView::ValidateSubImageIndices(WUInt32 uiMipLevel, WUInt32 uiFace, WUInt32 uiArrayIndex, WUInt32 uiPlaneIndex) const
{
  W_IGNORE_UNUSED(uiMipLevel);
  W_IGNORE_UNUSED(uiFace);
  W_IGNORE_UNUSED(uiArrayIndex);
  W_IGNORE_UNUSED(uiPlaneIndex);

  W_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
  W_ASSERT_DEV(uiFace < m_uiNumFaces, "Invalid uiFace");
  W_ASSERT_DEV(uiArrayIndex < m_uiNumArrayIndices, "Invalid array slice");
  W_ASSERT_DEV(uiPlaneIndex < GetPlaneCount(), "Invalid plane index");
}

const WUInt64& WImageView::GetSubImageOffset(WUInt32 uiMipLevel, WUInt32 uiFace, WUInt32 uiArrayIndex, WUInt32 uiPlaneIndex) const
{
  ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  return m_SubImageOffsets[uiPlaneIndex + GetPlaneCount() * (uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex))];
}

WImage::WImage()
{
  Clear();
}

WImage::WImage(const WImageHeader& header)
{
  ResetAndAlloc(header);
}

WImage::WImage(const WImageHeader& header, WByteBlobPtr externalData)
{
  ResetAndUseExternalStorage(header, externalData);
}

WImage::WImage(WImage&& other)
{
  ResetAndMove(std::move(other));
}

WImage::WImage(const WImageView& other)
{
  ResetAndCopy(other);
}

void WImage::operator=(WImage&& rhs)
{
  ResetAndMove(std::move(rhs));
}

void WImage::Clear()
{
  m_InternalStorage.Clear();

  WImageView::Clear();
}

void WImage::ResetAndAlloc(const WImageHeader& header)
{
  const WUInt64 requiredSize = header.ComputeDataSize();

  // it is debatable whether this function should reuse external storage, at all
  // however, it is especially dangerous to rely on the external storage being big enough, since many functions just take an WImage as a
  // destination parameter and expect it to behave correctly when any of the Reset functions is called on it; it is not intuitive, that
  // Reset may fail due to how the image was previously reset

  // therefore, if external storage is insufficient, fall back to internal storage

  if (!UsesExternalStorage() || m_DataPtr.GetCount() < requiredSize)
  {
    m_InternalStorage.SetCountUninitialized(requiredSize);
    m_DataPtr = m_InternalStorage.GetBlobPtr<WUInt8>();
  }

  WImageView::ResetAndViewExternalStorage(header, WConstByteBlobPtr(m_DataPtr.GetPtr(), m_DataPtr.GetCount()));
}

void WImage::ResetAndUseExternalStorage(const WImageHeader& header, WByteBlobPtr externalData)
{
  m_InternalStorage.Clear();

  WImageView::ResetAndViewExternalStorage(header, externalData);
}

void WImage::ResetAndMove(WImage&& other)
{
  static_cast<WImageHeader&>(*this) = other.GetHeader();

  if (other.UsesExternalStorage())
  {
    m_InternalStorage.Clear();
    m_SubImageOffsets = std::move(other.m_SubImageOffsets);
    m_DataPtr = other.m_DataPtr;
    other.Clear();
  }
  else
  {
    m_InternalStorage = std::move(other.m_InternalStorage);
    m_SubImageOffsets = std::move(other.m_SubImageOffsets);
    m_DataPtr = m_InternalStorage.GetBlobPtr<WUInt8>();
    other.Clear();
  }
}

void WImage::ResetAndCopy(const WImageView& other)
{
  ResetAndAlloc(other.GetHeader());

  memcpy(GetBlobPtr<WUInt8>().GetPtr(), other.GetBlobPtr<WUInt8>().GetPtr(), static_cast<size_t>(other.GetBlobPtr<WUInt8>().GetCount()));
}

WResult WImage::LoadFrom(WStringView sFileName)
{
  W_LOG_BLOCK("Loading Image", sFileName);
  W_PROFILE_SCOPE(WPathUtils::GetFileNameAndExtension(sFileName));

  WFileReader reader;
  if (reader.Open(sFileName) == W_FAILURE)
  {
    WLog::Warning("Failed to open image file '{0}'", WArgSensitive(sFileName, "File"));
    return W_FAILURE;
  }

  WStringView it = WPathUtils::GetFileExtension(sFileName);

  if (const WImageFileFormat* pFormat = WImageFileFormat::GetReaderFormat(it))
  {
    if (pFormat->ReadImage(reader, *this, it) != W_SUCCESS)
    {
      WLog::Warning("Failed to read image file '{0}'", WArgSensitive(sFileName, "File"));
      return W_FAILURE;
    }

    return W_SUCCESS;
  }

  WLog::Warning("No known image file format for extension '{0}'", it);

  return W_FAILURE;
}

WResult WImage::Convert(WImageFormat::Enum targetFormat)
{
  return WImageConversion::Convert(*this, *this, targetFormat);
}

WImageView WImageView::GetSubImageView(WUInt32 uiMipLevel /*= 0*/, WUInt32 uiFace /*= 0*/, WUInt32 uiArrayIndex /*= 0*/) const
{
  WImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);
  header.SetWidth(GetWidth(uiMipLevel));
  header.SetHeight(GetHeight(uiMipLevel));
  header.SetDepth(GetDepth(uiMipLevel));
  header.SetImageFormat(m_Format);

  const WUInt64& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, 0);
  WUInt64 size = *(&offset + GetPlaneCount()) - offset;

  WBlobPtr<const WUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return WImageView(header, WConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

WImage WImage::GetSubImageView(WUInt32 uiMipLevel /*= 0*/, WUInt32 uiFace /*= 0*/, WUInt32 uiArrayIndex /*= 0*/)
{
  WImageView constView = WImageView::GetSubImageView(uiMipLevel, uiFace, uiArrayIndex);

  // Create an WImage attached to the view. Const cast is safe here since we own the storage.
  return WImage(
    constView.GetHeader(), WByteBlobPtr(const_cast<WUInt8*>(constView.GetBlobPtr<WUInt8>().GetPtr()), constView.GetBlobPtr<WUInt8>().GetCount()));
}

WImageView WImageView::GetPlaneView(WUInt32 uiMipLevel /*= 0*/, WUInt32 uiFace /*= 0*/, WUInt32 uiArrayIndex /*= 0*/, WUInt32 uiPlaneIndex /*= 0*/) const
{
  WImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the first plane which determines the "nominal" width, height and depth
  WImageFormat::Enum subFormat = WImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * WImageFormat::GetBlockWidth(subFormat) / WImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(GetHeight(uiMipLevel) * WImageFormat::GetBlockHeight(subFormat) / WImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(GetDepth(uiMipLevel) * WImageFormat::GetBlockDepth(subFormat) / WImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(subFormat);

  const WUInt64& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  WUInt64 size = *(&offset + 1) - offset;

  WBlobPtr<const WUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return WImageView(header, WConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

WImage WImage::GetPlaneView(WUInt32 uiMipLevel /* = 0 */, WUInt32 uiFace /* = 0 */, WUInt32 uiArrayIndex /* = 0 */, WUInt32 uiPlaneIndex /* = 0 */)
{
  WImageView constView = WImageView::GetPlaneView(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);

  // Create an WImage attached to the view. Const cast is safe here since we own the storage.
  return WImage(
    constView.GetHeader(), WByteBlobPtr(const_cast<WUInt8*>(constView.GetBlobPtr<WUInt8>().GetPtr()), constView.GetBlobPtr<WUInt8>().GetCount()));
}

WImage WImage::GetSliceView(WUInt32 uiMipLevel /*= 0*/, WUInt32 uiFace /*= 0*/, WUInt32 uiArrayIndex /*= 0*/, WUInt32 z /*= 0*/, WUInt32 uiPlaneIndex /*= 0*/)
{
  WImageView constView = WImageView::GetSliceView(uiMipLevel, uiFace, uiArrayIndex, z, uiPlaneIndex);

  // Create an WImage attached to the view. Const cast is safe here since we own the storage.
  return WImage(
    constView.GetHeader(), WByteBlobPtr(const_cast<WUInt8*>(constView.GetBlobPtr<WUInt8>().GetPtr()), constView.GetBlobPtr<WUInt8>().GetCount()));
}

WImageView WImageView::GetSliceView(WUInt32 uiMipLevel /*= 0*/, WUInt32 uiFace /*= 0*/, WUInt32 uiArrayIndex /*= 0*/, WUInt32 z /*= 0*/, WUInt32 uiPlaneIndex /*= 0*/) const
{
  WImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the first plane which determines the "nominal" width, height and depth
  WImageFormat::Enum subFormat = WImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * WImageFormat::GetBlockWidth(subFormat) / WImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(GetHeight(uiMipLevel) * WImageFormat::GetBlockHeight(subFormat) / WImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(WImageFormat::GetBlockDepth(subFormat) / WImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(subFormat);

  WUInt64 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex) + z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  WUInt64 size = GetDepthPitch(uiMipLevel, uiPlaneIndex);

  WBlobPtr<const WUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return WImageView(header, WConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

bool WImage::UsesExternalStorage() const
{
  return m_InternalStorage.GetBlobPtr<WUInt8>() != m_DataPtr;
}
