#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Math.h>

#include <Texture/Image/ImageFormat.h>
#include <Texture/TextureDLL.h>

/// A class containing image meta data, such as format and dimensions.
///
/// This class has no associated behavior or functionality, and its getters and setters have no effect other than changing
/// the contained value. It is intended as a container to be modified by image utils and loaders.
class W_TEXTURE_DLL WImageHeader
{
public:
  /// Constructs an image using an unknown format and zero size.
  WImageHeader() { Clear(); }

  /// Constructs an image using an unknown format and zero size.
  void Clear()
  {
    m_uiNumMipLevels = 1;
    m_uiNumFaces = 1;
    m_uiNumArrayIndices = 1;
    m_uiWidth = 0;
    m_uiHeight = 0;
    m_uiDepth = 1;
    m_Format = WImageFormat::UNKNOWN;
  }

  /// Sets the image format.
  void SetImageFormat(const WImageFormat::Enum& format) { m_Format = format; }

  /// Returns the image format.
  WImageFormat::Enum GetImageFormat() const { return m_Format; }

  /// Sets the image width.
  void SetWidth(WUInt32 uiWidth) { m_uiWidth = uiWidth; }

  /// Returns the image width for a given mip level, clamped to 1.
  WUInt32 GetWidth(WUInt32 uiMipLevel = 0) const
  {
    W_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return WMath::Max(m_uiWidth >> uiMipLevel, 1U);
  }

  /// Sets the image height.
  void SetHeight(WUInt32 uiHeight) { m_uiHeight = uiHeight; }

  /// Returns the image height for a given mip level, clamped to 1.
  WUInt32 GetHeight(WUInt32 uiMipLevel = 0) const
  {
    W_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return WMath::Max(m_uiHeight >> uiMipLevel, 1U);
  }

  /// Sets the image depth. The default is 1.
  void SetDepth(WUInt32 uiDepth) { m_uiDepth = uiDepth; }

  /// Returns the image depth for a given mip level, clamped to 1.
  WUInt32 GetDepth(WUInt32 uiMipLevel = 0) const
  {
    W_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return WMath::Max(m_uiDepth >> uiMipLevel, 1U);
  }

  /// Sets the number of mip levels, including the full-size image.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumMipLevels(WUInt32 uiNumMipLevels) { m_uiNumMipLevels = uiNumMipLevels; }

  /// Returns the number of mip levels, including the full-size image.
  WUInt32 GetNumMipLevels() const { return m_uiNumMipLevels; }

  /// Sets the number of cubemap faces. Use 1 for a non-cubemap.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumFaces(WUInt32 uiNumFaces) { m_uiNumFaces = uiNumFaces; }

  /// Returns the number of cubemap faces, or 1 for a non-cubemap.
  WUInt32 GetNumFaces() const { return m_uiNumFaces; }

  /// Sets the number of array indices.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumArrayIndices(WUInt32 uiNumArrayIndices) { m_uiNumArrayIndices = uiNumArrayIndices; }

  /// Returns the number of array indices.
  WUInt32 GetNumArrayIndices() const { return m_uiNumArrayIndices; }

  /// Returns the number of image planes.
  WUInt32 GetPlaneCount() const
  {
    return WImageFormat::GetPlaneCount(m_Format);
  }

  /// Returns the number of blocks contained in a given mip level in the horizontal direction.
  WUInt32 GetNumBlocksX(WUInt32 uiMipLevel = 0, WUInt32 uiPlaneIndex = 0) const
  {
    return WImageFormat::GetNumBlocksX(m_Format, GetWidth(uiMipLevel), uiPlaneIndex);
  }

  /// Returns the number of blocks contained in a given mip level in the horizontal direction.
  WUInt32 GetNumBlocksY(WUInt32 uiMipLevel = 0, WUInt32 uiPlaneIndex = 0) const
  {
    return WImageFormat::GetNumBlocksY(m_Format, GetHeight(uiMipLevel), uiPlaneIndex);
  }

  /// Returns the number of blocks contained in a given mip level in the depth direction.
  WUInt32 GetNumBlocksZ(WUInt32 uiMipLevel = 0, WUInt32 uiPlaneIndex = 0) const
  {
    return WImageFormat::GetNumBlocksZ(m_Format, GetDepth(uiMipLevel), uiPlaneIndex);
  }

  /// Returns the offset in bytes between two subsequent rows of the given mip level.
  WUInt64 GetRowPitch(WUInt32 uiMipLevel = 0, WUInt32 uiPlaneIndex = 0) const
  {
    return WImageFormat::GetRowPitch(m_Format, GetWidth(uiMipLevel), uiPlaneIndex);
  }

  /// Returns the offset in bytes between two subsequent depth slices of the given mip level.
  WUInt64 GetDepthPitch(WUInt32 uiMipLevel = 0, WUInt32 uiPlaneIndex = 0) const
  {
    return WImageFormat::GetDepthPitch(m_Format, GetWidth(uiMipLevel), GetHeight(uiMipLevel), uiPlaneIndex);
  }

  /// Computes the data size required for an image with the header's format and dimensions.
  WUInt64 ComputeDataSize() const
  {
    WUInt64 uiDataSize = 0;

    for (WUInt32 uiMipLevel = 0; uiMipLevel < GetNumMipLevels(); uiMipLevel++)
    {
      for (WUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
      {
        uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * static_cast<WUInt64>(GetDepth(uiMipLevel));
      }
    }

    return WMath::SafeMultiply64(uiDataSize, WMath::SafeMultiply32(GetNumArrayIndices(), GetNumFaces()));
  }

  /// Computes the number of mip maps in the full mip chain.
  WUInt32 ComputeNumberOfMipMaps() const
  {
    WUInt32 numMipMaps = 1;
    WUInt32 width = GetWidth();
    WUInt32 height = GetHeight();
    WUInt32 depth = GetDepth();

    while (width > 1 || height > 1 || depth > 1)
    {
      width = WMath::Max(1u, width / 2);
      height = WMath::Max(1u, height / 2);
      depth = WMath::Max(1u, depth / 2);

      numMipMaps++;
    }

    return numMipMaps;
  }

  bool operator==(const WImageHeader& other) const
  {
    return m_uiNumMipLevels == other.m_uiNumMipLevels &&
           m_uiNumFaces == other.m_uiNumFaces &&
           m_uiNumArrayIndices == other.m_uiNumArrayIndices &&
           m_uiWidth == other.m_uiWidth &&
           m_uiHeight == other.m_uiHeight &&
           m_uiDepth == other.m_uiDepth &&
           m_Format == other.m_Format;
  }

  bool operator!=(const WImageHeader& other) const
  {
    return !operator==(other);
  }

protected:
  WUInt32 m_uiNumMipLevels;
  WUInt32 m_uiNumFaces;
  WUInt32 m_uiNumArrayIndices;

  WUInt32 m_uiWidth;
  WUInt32 m_uiHeight;
  WUInt32 m_uiDepth;

  WImageFormat::Enum m_Format;
};
