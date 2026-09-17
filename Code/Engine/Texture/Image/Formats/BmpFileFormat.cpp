#include <Texture/TexturePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/ImageConversion.h>

W_STATICLINK_FORCE static WImageFileFormatRegistrator<WBmpFileFormat> g_bmpFormat;

enum WBmpCompression
{
  RGB = 0L,
  RLE8 = 1L,
  RLE4 = 2L,
  BITFIELDS = 3L,
  JPEG = 4L,
  PNG = 5L,
};


#pragma pack(push, 1)
struct WBmpFileHeader
{
  WUInt16 m_type = 0;
  WUInt32 m_size = 0;
  WUInt16 m_reserved1 = 0;
  WUInt16 m_reserved2 = 0;
  WUInt32 m_offBits = 0;
};
#pragma pack(pop)

struct WBmpFileInfoHeader
{
  WUInt32 m_size = 0;
  WUInt32 m_width = 0;
  WUInt32 m_height = 0;
  WUInt16 m_planes = 0;
  WUInt16 m_bitCount = 0;
  WBmpCompression m_compression = WBmpCompression::RGB;
  WUInt32 m_sizeImage = 0;
  WUInt32 m_xPelsPerMeter = 0;
  WUInt32 m_yPelsPerMeter = 0;
  WUInt32 m_clrUsed = 0;
  WUInt32 m_clrImportant = 0;
};

struct WCIEXYZ
{
  int ciexyzX = 0;
  int ciexyzY = 0;
  int ciexyzZ = 0;
};

struct WCIEXYZTRIPLE
{
  WCIEXYZ ciexyzRed;
  WCIEXYZ ciexyzGreen;
  WCIEXYZ ciexyzBlue;
};

struct WBmpFileInfoHeaderV4
{
  WUInt32 m_redMask = 0;
  WUInt32 m_greenMask = 0;
  WUInt32 m_blueMask = 0;
  WUInt32 m_alphaMask = 0;
  WUInt32 m_csType = 0;
  WCIEXYZTRIPLE m_endpoints;
  WUInt32 m_gammaRed = 0;
  WUInt32 m_gammaGreen = 0;
  WUInt32 m_gammaBlue = 0;
};

static_assert(sizeof(WCIEXYZTRIPLE) == 3 * 3 * 4);

// just to be on the safe side
#if W_ENABLED(W_PLATFORM_WINDOWS)
static_assert(sizeof(WCIEXYZTRIPLE) == sizeof(CIEXYZTRIPLE));
#endif

struct WBmpFileInfoHeaderV5
{
  WUInt32 m_intent;
  WUInt32 m_profileData;
  WUInt32 m_profileSize;
  WUInt32 m_reserved;
};

static const WUInt16 WBmpFileMagic = 0x4D42u;

struct WBmpBgrxQuad
{
  W_DECLARE_POD_TYPE();

  WBmpBgrxQuad() = default;

  WBmpBgrxQuad(WUInt8 uiRed, WUInt8 uiGreen, WUInt8 uiBlue)
    : m_blue(uiBlue)
    , m_green(uiGreen)
    , m_red(uiRed)
    , m_reserved(0)
  {
  }

  WUInt8 m_blue;
  WUInt8 m_green;
  WUInt8 m_red;
  WUInt8 m_reserved;
};

WResult WBmpFileFormat::WriteImage(WStreamWriter& inout_stream, const WImageView& image, WStringView sFileExtension) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  WImageFormat::Enum compatibleFormats[] = {
    WImageFormat::B8G8R8X8_UNORM,
    WImageFormat::B8G8R8A8_UNORM,
    WImageFormat::B8G8R8_UNORM,
    WImageFormat::B5G5R5X1_UNORM,
    WImageFormat::B5G6R5_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  WImageFormat::Enum format = WImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == WImageFormat::UNKNOWN)
  {
    WLog::Error("No conversion from format '{0}' to a format suitable for BMP files known.", WImageFormat::GetName(image.GetImageFormat()));
    return W_FAILURE;
  }

  // Convert if not already in a compatible format
  if (format != image.GetImageFormat())
  {
    WImage convertedImage;
    if (WImageConversion::Convert(image, convertedImage, format) != W_SUCCESS)
    {
      // This should never happen
      W_ASSERT_DEV(false, "WImageConversion::Convert failed even though the conversion was to the format returned by FindClosestCompatibleFormat.");
      return W_FAILURE;
    }

    return WriteImage(inout_stream, convertedImage, sFileExtension);
  }

  WUInt64 uiRowPitch = image.GetRowPitch(0);

  WUInt32 uiHeight = image.GetHeight(0);

  WUInt64 dataSize = uiRowPitch * uiHeight;
  if (dataSize >= WMath::MaxValue<WUInt32>())
  {
    W_ASSERT_DEV(false, "Size overflow in BMP file format.");
    return W_FAILURE;
  }

  WBmpFileInfoHeader fileInfoHeader;
  fileInfoHeader.m_width = image.GetWidth(0);
  fileInfoHeader.m_height = uiHeight;
  fileInfoHeader.m_planes = 1;
  fileInfoHeader.m_bitCount = static_cast<WUInt16>(WImageFormat::GetBitsPerPixel(format));

  fileInfoHeader.m_sizeImage = 0; // Can be zero unless we store the data compressed

  fileInfoHeader.m_xPelsPerMeter = 0;
  fileInfoHeader.m_yPelsPerMeter = 0;
  fileInfoHeader.m_clrUsed = 0;
  fileInfoHeader.m_clrImportant = 0;

  bool bWriteColorMask = false;

  // Prefer to write a V3 header
  WUInt32 uiHeaderVersion = 3;

  switch (format)
  {
    case WImageFormat::B8G8R8X8_UNORM:
    case WImageFormat::B5G5R5X1_UNORM:
    case WImageFormat::B8G8R8_UNORM:
      fileInfoHeader.m_compression = RGB;
      break;

    case WImageFormat::B8G8R8A8_UNORM:
      fileInfoHeader.m_compression = BITFIELDS;
      uiHeaderVersion = 4;
      break;

    case WImageFormat::B5G6R5_UNORM:
      fileInfoHeader.m_compression = BITFIELDS;
      bWriteColorMask = true;
      break;

    default:
      return W_FAILURE;
  }

  W_ASSERT_DEV(!bWriteColorMask || uiHeaderVersion <= 3, "Internal bug");

  WUInt32 uiFileInfoHeaderSize = sizeof(WBmpFileInfoHeader);
  WUInt32 uiHeaderSize = sizeof(WBmpFileHeader);

  if (uiHeaderVersion >= 4)
  {
    uiFileInfoHeaderSize += sizeof(WBmpFileInfoHeaderV4);
  }
  else if (bWriteColorMask)
  {
    uiHeaderSize += 3 * sizeof(WUInt32);
  }

  uiHeaderSize += uiFileInfoHeaderSize;

  fileInfoHeader.m_size = uiFileInfoHeaderSize;

  WBmpFileHeader header;
  header.m_type = WBmpFileMagic;
  header.m_size = uiHeaderSize + static_cast<WUInt32>(dataSize);
  header.m_reserved1 = 0;
  header.m_reserved2 = 0;
  header.m_offBits = uiHeaderSize;

  // Write all data
  if (inout_stream.WriteBytes(&header, sizeof(header)) != W_SUCCESS)
  {
    WLog::Error("Failed to write header.");
    return W_FAILURE;
  }

  if (inout_stream.WriteBytes(&fileInfoHeader, sizeof(fileInfoHeader)) != W_SUCCESS)
  {
    WLog::Error("Failed to write fileInfoHeader.");
    return W_FAILURE;
  }

  if (uiHeaderVersion >= 4)
  {
    WBmpFileInfoHeaderV4 fileInfoHeaderV4;
    memset(&fileInfoHeaderV4, 0, sizeof(fileInfoHeaderV4));

    fileInfoHeaderV4.m_redMask = WImageFormat::GetRedMask(format);
    fileInfoHeaderV4.m_greenMask = WImageFormat::GetGreenMask(format);
    fileInfoHeaderV4.m_blueMask = WImageFormat::GetBlueMask(format);
    fileInfoHeaderV4.m_alphaMask = WImageFormat::GetAlphaMask(format);

    if (inout_stream.WriteBytes(&fileInfoHeaderV4, sizeof(fileInfoHeaderV4)) != W_SUCCESS)
    {
      WLog::Error("Failed to write fileInfoHeaderV4.");
      return W_FAILURE;
    }
  }
  else if (bWriteColorMask)
  {
    struct
    {
      WUInt32 m_red;
      WUInt32 m_green;
      WUInt32 m_blue;
    } colorMask;


    colorMask.m_red = WImageFormat::GetRedMask(format);
    colorMask.m_green = WImageFormat::GetGreenMask(format);
    colorMask.m_blue = WImageFormat::GetBlueMask(format);

    if (inout_stream.WriteBytes(&colorMask, sizeof(colorMask)) != W_SUCCESS)
    {
      WLog::Error("Failed to write colorMask.");
      return W_FAILURE;
    }
  }

  const WUInt64 uiPaddedRowPitch = ((uiRowPitch - 1) / 4 + 1) * 4;
  // Write rows in reverse order
  for (WInt32 iRow = uiHeight - 1; iRow >= 0; iRow--)
  {
    if (inout_stream.WriteBytes(image.GetPixelPointer<void>(0, 0, 0, 0, iRow, 0), uiRowPitch) != W_SUCCESS)
    {
      WLog::Error("Failed to write data.");
      return W_FAILURE;
    }

    WUInt8 zeroes[4] = {0, 0, 0, 0};
    if (inout_stream.WriteBytes(zeroes, uiPaddedRowPitch - uiRowPitch) != W_SUCCESS)
    {
      WLog::Error("Failed to write data.");
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

namespace
{
  WUInt32 ExtractBits(const void* pData, WUInt32 uiBitAddress, WUInt32 uiNumBits)
  {
    WUInt32 uiMask = (1U << uiNumBits) - 1;
    WUInt32 uiByteAddress = uiBitAddress / 8;
    WUInt32 uiShiftAmount = 7 - (uiBitAddress % 8 + uiNumBits - 1);

    return (reinterpret_cast<const WUInt8*>(pData)[uiByteAddress] >> uiShiftAmount) & uiMask;
  }

  WResult ReadImageInfo(WStreamReader& inout_stream, WImageHeader& ref_header, WBmpFileHeader& ref_fileHeader, WBmpFileInfoHeader& ref_fileInfoHeader, bool& ref_bIndexed,
    bool& ref_bCompressed, WUInt32& ref_uiBpp, WUInt32& ref_uiDataSize)
  {
    if (inout_stream.ReadBytes(&ref_fileHeader, sizeof(WBmpFileHeader)) != sizeof(WBmpFileHeader))
    {
      WLog::Error("Failed to read header data.");
      return W_FAILURE;
    }

    // Some very old BMP variants may have different magic numbers, but we don't support them.
    if (ref_fileHeader.m_type != WBmpFileMagic)
    {
      WLog::Error("The file is not a recognized BMP file.");
      return W_FAILURE;
    }

    // We expect at least header version 3
    WUInt32 uiHeaderVersion = 3;
    if (inout_stream.ReadBytes(&ref_fileInfoHeader, sizeof(WBmpFileInfoHeader)) != sizeof(WBmpFileInfoHeader))
    {
      WLog::Error("Failed to read header data (V3).");
      return W_FAILURE;
    }

    int remainingHeaderBytes = ref_fileInfoHeader.m_size - sizeof(ref_fileInfoHeader);

    // File header shorter than expected - happens with corrupt files or e.g. with OS/2 BMP files which may have shorter headers
    if (remainingHeaderBytes < 0)
    {
      WLog::Error("The file header was shorter than expected.");
      return W_FAILURE;
    }

    // Newer files may have a header version 4 (required for transparency)
    WBmpFileInfoHeaderV4 fileInfoHeaderV4;
    if (remainingHeaderBytes >= sizeof(WBmpFileInfoHeaderV4))
    {
      uiHeaderVersion = 4;
      if (inout_stream.ReadBytes(&fileInfoHeaderV4, sizeof(WBmpFileInfoHeaderV4)) != sizeof(WBmpFileInfoHeaderV4))
      {
        WLog::Error("Failed to read header data (V4).");
        return W_FAILURE;
      }
      remainingHeaderBytes -= sizeof(WBmpFileInfoHeaderV4);
    }

    // Skip rest of header
    if (inout_stream.SkipBytes(remainingHeaderBytes) != remainingHeaderBytes)
    {
      WLog::Error("Failed to skip remaining header data.");
      return W_FAILURE;
    }

    ref_uiBpp = ref_fileInfoHeader.m_bitCount;

    // Find target format to load the image
    WImageFormat::Enum format = WImageFormat::UNKNOWN;

    switch (ref_fileInfoHeader.m_compression)
    {
        // RGB or indexed data
      case RGB:
        switch (ref_uiBpp)
        {
          case 1:
          case 4:
          case 8:
            ref_bIndexed = true;

            // We always decompress indexed to BGRX, since the palette is specified in this format
            format = WImageFormat::B8G8R8X8_UNORM;
            break;

          case 16:
            format = WImageFormat::B5G5R5X1_UNORM;
            break;

          case 24:
            format = WImageFormat::B8G8R8_UNORM;
            break;

          case 32:
            format = WImageFormat::B8G8R8X8_UNORM;
        }
        break;

        // RGB data, but with the color masks specified in place of the palette
      case BITFIELDS:
        switch (ref_uiBpp)
        {
          case 16:
          case 32:
            // In case of old headers, the color masks appear after the header (and aren't counted as part of it)
            if (uiHeaderVersion < 4)
            {
              // Color masks (w/o alpha channel)
              struct
              {
                WUInt32 m_red;
                WUInt32 m_green;
                WUInt32 m_blue;
              } colorMask;

              if (inout_stream.ReadBytes(&colorMask, sizeof(colorMask)) != sizeof(colorMask))
              {
                return W_FAILURE;
              }

              format = WImageFormat::FromPixelMask(colorMask.m_red, colorMask.m_green, colorMask.m_blue, 0, ref_uiBpp);
            }
            else
            {
              // For header version four and higher, the color masks are part of the header
              format = WImageFormat::FromPixelMask(
                fileInfoHeaderV4.m_redMask, fileInfoHeaderV4.m_greenMask, fileInfoHeaderV4.m_blueMask, fileInfoHeaderV4.m_alphaMask, ref_uiBpp);
            }

            break;
        }
        break;

      case RLE4:
        if (ref_uiBpp == 4)
        {
          ref_bIndexed = true;
          ref_bCompressed = true;
          format = WImageFormat::B8G8R8X8_UNORM;
        }
        break;

      case RLE8:
        if (ref_uiBpp == 8)
        {
          ref_bIndexed = true;
          ref_bCompressed = true;
          format = WImageFormat::B8G8R8X8_UNORM;
        }
        break;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
    }

    if (format == WImageFormat::UNKNOWN)
    {
      WLog::Error("Unknown or unsupported BMP encoding.");
      return W_FAILURE;
    }

    const WUInt32 uiWidth = ref_fileInfoHeader.m_width;

    if (uiWidth > 65536)
    {
      WLog::Error("Image specifies width > 65536. Header corrupted?");
      return W_FAILURE;
    }

    const WUInt32 uiHeight = ref_fileInfoHeader.m_height;

    if (uiHeight > 65536)
    {
      WLog::Error("Image specifies height > 65536. Header corrupted?");
      return W_FAILURE;
    }

    ref_uiDataSize = ref_fileInfoHeader.m_sizeImage;

    if (ref_uiDataSize > 1024 * 1024 * 1024)
    {
      WLog::Error("Image specifies data size > 1GiB. Header corrupted?");
      return W_FAILURE;
    }

    const int uiRowPitchIn = (uiWidth * ref_uiBpp + 31) / 32 * 4;

    if (ref_uiDataSize == 0)
    {
      if (ref_fileInfoHeader.m_compression != RGB)
      {
        WLog::Error("The data size wasn't specified in the header.");
        return W_FAILURE;
      }
      ref_uiDataSize = uiRowPitchIn * uiHeight;
    }

    // Set image data
    ref_header.SetImageFormat(format);
    ref_header.SetNumMipLevels(1);
    ref_header.SetNumArrayIndices(1);
    ref_header.SetNumFaces(1);

    ref_header.SetWidth(uiWidth);
    ref_header.SetHeight(uiHeight);
    ref_header.SetDepth(1);

    return W_SUCCESS;
  }

} // namespace

WResult WBmpFileFormat::ReadImageHeader(WStreamReader& inout_stream, WImageHeader& ref_header, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  W_PROFILE_SCOPE("WBmpFileFormat::ReadImage");

  WBmpFileHeader fileHeader;
  WBmpFileInfoHeader fileInfoHeader;
  bool bIndexed = false, bCompressed = false;
  WUInt32 uiBpp = 0;
  WUInt32 uiDataSize = 0;

  return ReadImageInfo(inout_stream, ref_header, fileHeader, fileInfoHeader, bIndexed, bCompressed, uiBpp, uiDataSize);
}

WResult WBmpFileFormat::ReadImage(WStreamReader& inout_stream, WImage& ref_image, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  W_PROFILE_SCOPE("WBmpFileFormat::ReadImage");

  WBmpFileHeader fileHeader;
  WImageHeader header;
  WBmpFileInfoHeader fileInfoHeader;
  bool bIndexed = false, bCompressed = false;
  WUInt32 uiBpp = 0;
  WUInt32 uiDataSize = 0;

  W_SUCCEED_OR_RETURN(ReadImageInfo(inout_stream, header, fileHeader, fileInfoHeader, bIndexed, bCompressed, uiBpp, uiDataSize));

  ref_image.ResetAndAlloc(header);

  WUInt64 uiRowPitch = ref_image.GetRowPitch(0);

  const int uiRowPitchIn = (header.GetWidth() * uiBpp + 31) / 32 * 4;

  if (bIndexed)
  {
    // If no palette size was specified, the full available palette size will be used
    WUInt32 paletteSize = fileInfoHeader.m_clrUsed;
    if (paletteSize == 0)
    {
      paletteSize = 1U << uiBpp;
    }
    else if (paletteSize > 65536)
    {
      WLog::Error("Palette size > 65536.");
      return W_FAILURE;
    }

    WDynamicArray<WBmpBgrxQuad> palette;
    palette.SetCountUninitialized(paletteSize);
    if (inout_stream.ReadBytes(&palette[0], paletteSize * sizeof(WBmpBgrxQuad)) != paletteSize * sizeof(WBmpBgrxQuad))
    {
      WLog::Error("Failed to read palette data.");
      return W_FAILURE;
    }

    if (bCompressed)
    {
      // Compressed data is always in pairs of bytes
      if (uiDataSize % 2 != 0)
      {
        WLog::Error("The data size is not a multiple of 2 bytes in an RLE-compressed file.");
        return W_FAILURE;
      }

      WDynamicArray<WUInt8> compressedData;
      compressedData.SetCountUninitialized(uiDataSize);

      if (inout_stream.ReadBytes(&compressedData[0], uiDataSize) != uiDataSize)
      {
        WLog::Error("Failed to read data.");
        return W_FAILURE;
      }

      const WUInt8* pIn = &compressedData[0];
      const WUInt8* pInEnd = pIn + uiDataSize;

      // Current output position
      WUInt32 uiRow = fileInfoHeader.m_height - 1;
      WUInt32 uiCol = 0;

      WBmpBgrxQuad* pLine = ref_image.GetPixelPointer<WBmpBgrxQuad>(0, 0, 0, 0, uiRow, 0);

      // Decode RLE data directly to RGBX
      while (pIn < pInEnd)
      {
        WUInt32 uiByte1 = *pIn++;
        WUInt32 uiByte2 = *pIn++;

        // Relative mode - the first byte specified a number of indices to be repeated, the second one the indices
        if (uiByte1 > 0)
        {
          // Clamp number of repetitions to row width.
          // The spec isn't clear on this point, but some files pad the number of encoded indices for some reason.
          uiByte1 = WMath::Min(uiByte1, fileInfoHeader.m_width - uiCol);

          if (uiBpp == 4)
          {
            // Alternate between two indices.
            for (WUInt32 uiRep = 0; uiRep < uiByte1 / 2; uiRep++)
            {
              pLine[uiCol++] = palette[uiByte2 >> 4];
              pLine[uiCol++] = palette[uiByte2 & 0x0F];
            }

            // Repeat the first index for odd numbers of repetitions.
            if (uiByte1 & 1)
            {
              pLine[uiCol++] = palette[uiByte2 >> 4];
            }
          }
          else /* if (uiBpp == 8) */
          {
            // Repeat a single index.
            for (WUInt32 uiRep = 0; uiRep < uiByte1; uiRep++)
            {
              pLine[uiCol++] = palette[uiByte2];
            }
          }
        }
        else
        {
          // Absolute mode - the first byte specifies a number of indices encoded separately, or is a special marker
          switch (uiByte2)
          {
              // End of line marker
            case 0:
            {

              // Fill up with palette entry 0
              while (uiCol < fileInfoHeader.m_width)
              {
                pLine[uiCol++] = palette[0];
              }

              // Begin next line
              uiCol = 0;
              uiRow--;
              pLine -= fileInfoHeader.m_width;
            }

            break;

              // End of image marker
            case 1:
              // Check that we really reached the end of the image.
              if (uiRow != 0 && uiCol != fileInfoHeader.m_height - 1)
              {
                WLog::Error("Unexpected end of image marker found.");
                return W_FAILURE;
              }
              break;

            case 2:
              WLog::Error("Found a RLE compression position delta - this is not supported.");
              return W_FAILURE;

            default:
              // Read uiByte2 number of indices

              // More data than fits into the image or can be read?
              if (uiCol + uiByte2 > fileInfoHeader.m_width || pIn + (uiByte2 + 1) / 2 > pInEnd)
              {
                return W_FAILURE;
              }

              if (uiBpp == 4)
              {
                for (WUInt32 uiRep = 0; uiRep < uiByte2 / 2; uiRep++)
                {
                  WUInt32 uiIndices = *pIn++;
                  pLine[uiCol++] = palette[uiIndices >> 4];
                  pLine[uiCol++] = palette[uiIndices & 0x0f];
                }

                if (uiByte2 & 1)
                {
                  pLine[uiCol++] = palette[*pIn++ >> 4];
                }

                // Pad to word boundary
                pIn += (uiByte2 / 2 + uiByte2) & 1;
              }
              else /* if (uiBpp == 8) */
              {
                for (WUInt32 uiRep = 0; uiRep < uiByte2; uiRep++)
                {
                  pLine[uiCol++] = palette[*pIn++];
                }

                // Pad to word boundary
                pIn += uiByte2 & 1;
              }
          }
        }
      }
    }
    else
    {
      WDynamicArray<WUInt8> indexedData;
      indexedData.SetCountUninitialized(uiDataSize);
      if (inout_stream.ReadBytes(&indexedData[0], uiDataSize) != uiDataSize)
      {
        WLog::Error("Failed to read data.");
        return W_FAILURE;
      }

      // Convert to non-indexed
      for (WUInt32 uiRow = 0; uiRow < fileInfoHeader.m_height; uiRow++)
      {
        WUInt8* pIn = &indexedData[uiRowPitchIn * uiRow];

        // Convert flipped vertically
        WBmpBgrxQuad* pOut = ref_image.GetPixelPointer<WBmpBgrxQuad>(0, 0, 0, 0, fileInfoHeader.m_height - uiRow - 1, 0);
        for (WUInt32 uiCol = 0; uiCol < ref_image.GetWidth(0); uiCol++)
        {
          WUInt32 uiIndex = ExtractBits(pIn, uiCol * uiBpp, uiBpp);
          if (uiIndex >= palette.GetCount())
          {
            WLog::Error("Image contains invalid palette indices.");
            return W_FAILURE;
          }
          pOut[uiCol] = palette[uiIndex];
        }
      }
    }
  }
  else
  {
    // Format must match the number of bits in the file
    if (WImageFormat::GetBitsPerPixel(header.GetImageFormat()) != uiBpp)
    {
      WLog::Error("The number of bits per pixel specified in the file ({0}) does not match the expected value of {1} for the format '{2}'.",
        uiBpp, WImageFormat::GetBitsPerPixel(header.GetImageFormat()), WImageFormat::GetName(header.GetImageFormat()));
      return W_FAILURE;
    }

    // Skip palette data. Having a palette here doesn't make sense, but is not explicitly disallowed by the standard.
    WUInt32 paletteSize = fileInfoHeader.m_clrUsed * sizeof(WBmpBgrxQuad);
    if (inout_stream.SkipBytes(paletteSize) != paletteSize)
    {
      WLog::Error("Failed to skip palette data.");
      return W_FAILURE;
    }

    // Read rows in reverse order
    for (WInt32 iRow = fileInfoHeader.m_height - 1; iRow >= 0; iRow--)
    {
      if (inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, iRow, 0), uiRowPitch) != uiRowPitch)
      {
        WLog::Error("Failed to read row data.");
        return W_FAILURE;
      }
      if (inout_stream.SkipBytes(uiRowPitchIn - uiRowPitch) != uiRowPitchIn - uiRowPitch)
      {
        WLog::Error("Failed to skip row data.");
        return W_FAILURE;
      }
    }
  }

  return W_SUCCESS;
}

bool WBmpFileFormat::CanReadFileType(WStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("bmp") || sExtension.IsEqual_NoCase("dib") || sExtension.IsEqual_NoCase("rle");
}

bool WBmpFileFormat::CanWriteFileType(WStringView sExtension) const
{
  return CanReadFileType(sExtension);
}



W_STATICLINK_FILE(Texture, Texture_Image_Formats_BmpFileFormat);
