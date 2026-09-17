#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/TgaFileFormat.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>

W_STATICLINK_FORCE static WImageFileFormatRegistrator<WTgaFileFormat> g_TgaFormat;

struct TgaImageDescriptor
{
  WUInt8 m_iAlphaBits : 4;
  WUInt8 m_bFlipH : 1;
  WUInt8 m_bFlipV : 1;
  WUInt8 m_Ignored : 2;
};

// see Wikipedia for details:
// http://de.wikipedia.org/wiki/Targa_Image_File
struct TgaHeader
{
  WInt8 m_iImageIDLength;
  WInt8 m_Ignored1;
  WInt8 m_ImageType;
  WInt8 m_Ignored2[9];
  WInt16 m_iImageWidth;
  WInt16 m_iImageHeight;
  WInt8 m_iBitsPerPixel;
  TgaImageDescriptor m_ImageDescriptor;
};

static_assert(sizeof(TgaHeader) == 18);


static inline WColorLinearUB GetPixelColor(const WImageView& image, WUInt32 x, WUInt32 y, const WUInt32 uiHeight)
{
  WColorLinearUB c(255, 255, 255, 255);

  const WUInt8* pPixel = image.GetPixelPointer<WUInt8>(0, 0, 0, x, uiHeight - y - 1, 0);

  switch (image.GetImageFormat())
  {
    case WImageFormat::R8G8B8A8_UNORM:
      c.r = pPixel[0];
      c.g = pPixel[1];
      c.b = pPixel[2];
      c.a = pPixel[3];
      break;
    case WImageFormat::B8G8R8A8_UNORM:
      c.a = pPixel[3];
      // fall through
    case WImageFormat::B8G8R8_UNORM:
    case WImageFormat::B8G8R8X8_UNORM:
      c.r = pPixel[2];
      c.g = pPixel[1];
      c.b = pPixel[0];
      break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return c;
}


WResult WTgaFileFormat::WriteImage(WStreamWriter& inout_stream, const WImageView& image, WStringView sFileExtension) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  WImageFormat::Enum compatibleFormats[] = {
    WImageFormat::R8G8B8A8_UNORM,
    WImageFormat::B8G8R8A8_UNORM,
    WImageFormat::B8G8R8X8_UNORM,
    WImageFormat::B8G8R8_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  WImageFormat::Enum format = WImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == WImageFormat::UNKNOWN)
  {
    WLog::Error("No conversion from format '{0}' to a format suitable for TGA files known.", WImageFormat::GetName(image.GetImageFormat()));
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

  const bool bCompress = true;

  // Write the header
  {
    WUInt8 uiHeader[18];
    WMemoryUtils::ZeroFill(uiHeader, 18);

    if (!bCompress)
    {
      // uncompressed TGA
      uiHeader[2] = 2;
    }
    else
    {
      // compressed TGA
      uiHeader[2] = 10;
    }

    uiHeader[13] = static_cast<WUInt8>(image.GetWidth(0) / 256);
    uiHeader[15] = static_cast<WUInt8>(image.GetHeight(0) / 256);
    uiHeader[12] = static_cast<WUInt8>(image.GetWidth(0) % 256);
    uiHeader[14] = static_cast<WUInt8>(image.GetHeight(0) % 256);
    uiHeader[16] = static_cast<WUInt8>(WImageFormat::GetBitsPerPixel(image.GetImageFormat()));

    inout_stream.WriteBytes(uiHeader, 18).IgnoreResult();
  }

  const bool bAlpha = image.GetImageFormat() != WImageFormat::B8G8R8_UNORM;

  const WUInt32 uiWidth = image.GetWidth(0);
  const WUInt32 uiHeight = image.GetHeight(0);

  if (!bCompress)
  {
    // Write image uncompressed

    for (WUInt32 y = 0; y < uiWidth; ++y)
    {
      for (WUInt32 x = 0; x < uiHeight; ++x)
      {
        const WColorLinearUB c = GetPixelColor(image, x, y, uiHeight);

        inout_stream << c.b;
        inout_stream << c.g;
        inout_stream << c.r;

        if (bAlpha)
          inout_stream << c.a;
      }
    }
  }
  else
  {
    // write image RLE compressed

    WInt32 iRLE = 0;

    WColorLinearUB pc = {};
    WStaticArray<WColorLinearUB, 129> unequal;
    WInt32 iEqual = 0;

    for (WUInt32 y = 0; y < uiHeight; ++y)
    {
      for (WUInt32 x = 0; x < uiWidth; ++x)
      {
        const WColorLinearUB c = GetPixelColor(image, x, y, uiHeight);

        if (iRLE == 0) // no comparison possible yet
        {
          pc = c;
          iRLE = 1;
          unequal.PushBack(c);
        }
        else if (iRLE == 1) // has one value gathered for comparison
        {
          if (c == pc)
          {
            iRLE = 2;   // two values were equal
            iEqual = 2; // go into equal-mode
          }
          else
          {
            iRLE = 3; // two values were unequal
            pc = c;   // go into unequal-mode
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 2) // equal values
        {
          if ((c == pc) && (iEqual < 128))
            ++iEqual;
          else
          {
            WUInt8 uiRepeat = static_cast<WUInt8>(iEqual + 127);

            inout_stream << uiRepeat;
            inout_stream << pc.b;
            inout_stream << pc.g;
            inout_stream << pc.r;

            if (bAlpha)
              inout_stream << pc.a;

            pc = c;
            iRLE = 1;
            unequal.Clear();
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 3)
        {
          if ((c != pc) && (unequal.GetCount() < 128))
          {
            unequal.PushBack(c);
            pc = c;
          }
          else
          {
            WUInt8 uiRepeat = (unsigned char)(unequal.GetCount()) - 1;
            inout_stream << uiRepeat;

            for (WUInt32 i = 0; i < unequal.GetCount(); ++i)
            {
              inout_stream << unequal[i].b;
              inout_stream << unequal[i].g;
              inout_stream << unequal[i].r;

              if (bAlpha)
                inout_stream << unequal[i].a;
            }

            pc = c;
            iRLE = 1;
            unequal.Clear();
            unequal.PushBack(c);
          }
        }
      }
    }


    if (iRLE == 1) // has one value gathered for comparison
    {
      WUInt8 uiRepeat = 0;

      inout_stream << uiRepeat;
      inout_stream << pc.b;
      inout_stream << pc.g;
      inout_stream << pc.r;

      if (bAlpha)
        inout_stream << pc.a;
    }
    else if (iRLE == 2) // equal values
    {
      WUInt8 uiRepeat = static_cast<WUInt8>(iEqual + 127);

      inout_stream << uiRepeat;
      inout_stream << pc.b;
      inout_stream << pc.g;
      inout_stream << pc.r;

      if (bAlpha)
        inout_stream << pc.a;
    }
    else if (iRLE == 3)
    {
      WUInt8 uiRepeat = (WUInt8)(unequal.GetCount()) - 1;
      inout_stream << uiRepeat;

      for (WUInt32 i = 0; i < unequal.GetCount(); ++i)
      {
        inout_stream << unequal[i].b;
        inout_stream << unequal[i].g;
        inout_stream << unequal[i].r;

        if (bAlpha)
          inout_stream << unequal[i].a;
      }
    }
  }

  return W_SUCCESS;
}


static WResult ReadBytesChecked(WStreamReader& inout_stream, void* pDest, WUInt32 uiNumBytes)
{
  if (inout_stream.ReadBytes(pDest, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}

template <typename TYPE>
static WResult ReadBytesChecked(WStreamReader& inout_stream, TYPE& ref_dest)
{
  return ReadBytesChecked(inout_stream, &ref_dest, sizeof(TYPE));
}

static WResult ReadImageHeaderImpl(WStreamReader& inout_stream, WImageHeader& ref_header, TgaHeader& ref_tgaHeader)
{
  W_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iImageIDLength));
  W_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_Ignored1));
  W_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_ImageType));
  W_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, &ref_tgaHeader.m_Ignored2, 9));
  W_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iImageWidth));
  W_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iImageHeight));
  W_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iBitsPerPixel));
  W_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_ImageDescriptor));

  // ignore optional data
  if (inout_stream.SkipBytes(ref_tgaHeader.m_iImageIDLength) != ref_tgaHeader.m_iImageIDLength)
    return W_FAILURE;

  const WUInt32 uiBytesPerPixel = ref_tgaHeader.m_iBitsPerPixel / 8;

  // check whether width, height an BitsPerPixel are valid
  if ((ref_tgaHeader.m_iImageWidth <= 0) || (ref_tgaHeader.m_iImageHeight <= 0) || ((uiBytesPerPixel != 1) && (uiBytesPerPixel != 3) && (uiBytesPerPixel != 4)) || (ref_tgaHeader.m_ImageType != 2 && ref_tgaHeader.m_ImageType != 3 && ref_tgaHeader.m_ImageType != 10 && ref_tgaHeader.m_ImageType != 11))
  {
    WLog::Error("TGA has an invalid header: Width = {0}, Height = {1}, BPP = {2}, ImageType = {3}", ref_tgaHeader.m_iImageWidth, ref_tgaHeader.m_iImageHeight, ref_tgaHeader.m_iBitsPerPixel, ref_tgaHeader.m_ImageType);
    return W_FAILURE;
  }

  // Set image data

  if (uiBytesPerPixel == 1)
    ref_header.SetImageFormat(WImageFormat::R8_UNORM);
  else if (uiBytesPerPixel == 3)
    ref_header.SetImageFormat(WImageFormat::B8G8R8_UNORM);
  else
    ref_header.SetImageFormat(WImageFormat::B8G8R8A8_UNORM);

  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(1);
  ref_header.SetNumFaces(1);

  ref_header.SetWidth(ref_tgaHeader.m_iImageWidth);
  ref_header.SetHeight(ref_tgaHeader.m_iImageHeight);
  ref_header.SetDepth(1);

  return W_SUCCESS;
}

WResult WTgaFileFormat::ReadImageHeader(WStreamReader& inout_stream, WImageHeader& ref_header, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  W_PROFILE_SCOPE("WTgaFileFormat::ReadImageHeader");

  TgaHeader tgaHeader;
  return ReadImageHeaderImpl(inout_stream, ref_header, tgaHeader);
}

WResult WTgaFileFormat::ReadImage(WStreamReader& inout_stream, WImage& ref_image, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  W_PROFILE_SCOPE("WTgaFileFormat::ReadImage");

  WImageHeader imageHeader;
  TgaHeader tgaHeader;
  W_SUCCEED_OR_RETURN(ReadImageHeaderImpl(inout_stream, imageHeader, tgaHeader));

  const WUInt32 uiBytesPerPixel = tgaHeader.m_iBitsPerPixel / 8;

  ref_image.ResetAndAlloc(imageHeader);

  if (tgaHeader.m_ImageType == 3)
  {
    // uncompressed greyscale

    const WUInt32 uiBytesPerRow = uiBytesPerPixel * tgaHeader.m_iImageWidth;

    if (tgaHeader.m_ImageDescriptor.m_bFlipH)
    {
      // read each row (gets rid of the row pitch
      for (WInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        for (WInt32 x = tgaHeader.m_iImageWidth - 1; x >= 0; --x)
        {
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, x, row, 0), uiBytesPerPixel);
        }
      }
    }
    else
    {
      // read each row (gets rid of the row pitch
      for (WInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, row, 0), uiBytesPerRow);
      }
    }
  }
  else if (tgaHeader.m_ImageType == 2)
  {
    // uncompressed

    const WUInt32 uiBytesPerRow = uiBytesPerPixel * tgaHeader.m_iImageWidth;

    if (tgaHeader.m_ImageDescriptor.m_bFlipH)
    {
      // read each row (gets rid of the row pitch
      for (WInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        for (WInt32 x = tgaHeader.m_iImageWidth - 1; x >= 0; --x)
        {
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, x, row, 0), uiBytesPerPixel);
        }
      }
    }
    else
    {
      // read each row (gets rid of the row pitch
      for (WInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, row, 0), uiBytesPerRow);
      }
    }
  }
  else
  {
    // compressed

    WInt32 iCurrentPixel = 0;
    const int iPixelCount = tgaHeader.m_iImageWidth * tgaHeader.m_iImageHeight;

    do
    {
      WUInt8 uiChunkHeader = 0;

      W_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, uiChunkHeader));

      const WInt32 numToRead = (uiChunkHeader & 127) + 1;

      if (iCurrentPixel + numToRead > iPixelCount)
      {
        WLog::Error("TGA contents are invalid");
        return W_FAILURE;
      }

      if (uiChunkHeader < 128)
      {
        // If the header is < 128, it means it is the number of RAW color packets minus 1
        // that follow the header
        // add 1 to get number of following color values

        // Read RAW color values
        for (WInt32 i = 0; i < numToRead; ++i)
        {
          const WInt32 x = iCurrentPixel % tgaHeader.m_iImageWidth;
          const WInt32 y = iCurrentPixel / tgaHeader.m_iImageWidth;

          const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
          const auto col = tgaHeader.m_ImageDescriptor.m_bFlipH ? tgaHeader.m_iImageWidth - x - 1 : x;
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, col, row, 0), uiBytesPerPixel);

          ++iCurrentPixel;
        }
      }
      else // chunk header > 128 RLE data, next color repeated (chunk header - 127) times
      {
        WUInt8 uiBuffer[4] = {255, 255, 255, 255};

        // read the current color
        inout_stream.ReadBytes(uiBuffer, uiBytesPerPixel);

        // if it is a 24-Bit TGA (3 channels), the fourth channel stays at 255 all the time, since the 4th value in ucBuffer is never overwritten

        // copy the color into the image data as many times as dictated
        for (WInt32 i = 0; i < numToRead; ++i)
        {
          const WInt32 x = iCurrentPixel % tgaHeader.m_iImageWidth;
          const WInt32 y = iCurrentPixel / tgaHeader.m_iImageWidth;

          const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
          const auto col = tgaHeader.m_ImageDescriptor.m_bFlipH ? tgaHeader.m_iImageWidth - x - 1 : x;
          WUInt8* pPixel = ref_image.GetPixelPointer<WUInt8>(0, 0, 0, col, row, 0);

          // BGR
          pPixel[0] = uiBuffer[0];

          if (uiBytesPerPixel > 1)
          {
            pPixel[1] = uiBuffer[1];
            pPixel[2] = uiBuffer[2];

            // Alpha
            if (uiBytesPerPixel == 4)
              pPixel[3] = uiBuffer[3];
          }

          ++iCurrentPixel;
        }
      }
    } while (iCurrentPixel < iPixelCount);
  }

  return W_SUCCESS;
}

bool WTgaFileFormat::CanReadFileType(WStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("tga");
}

bool WTgaFileFormat::CanWriteFileType(WStringView sExtension) const
{
  return CanReadFileType(sExtension);
}



W_STATICLINK_FILE(Texture, Texture_Image_Formats_TgaFileFormat);
