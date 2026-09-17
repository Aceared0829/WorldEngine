#include <Texture/TexturePCH.h>

#ifdef BUILDSYSTEM_ENABLE_TINYEXR_SUPPORT

#  include <Texture/Image/Formats/ExrFileFormat.h>
#  include <Texture/Image/Image.h>

#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>

#  include <tinyexr/tinyexr.h>

W_STATICLINK_FORCE static WImageFileFormatRegistrator<WExrFileFormat> g_ExrFileFormat;

WResult ReadImageData(WStreamReader& ref_stream, WDynamicArray<WUInt8>& ref_fileBuffer, WImageHeader& ref_header, EXRHeader& ref_exrHeader, EXRImage& ref_exrImage)
{
  // read the entire file to memory
  WStreamUtils::ReadAllAndAppend(ref_stream, ref_fileBuffer);

  // read the EXR version
  EXRVersion exrVersion;

  if (ParseEXRVersionFromMemory(&exrVersion, ref_fileBuffer.GetData(), ref_fileBuffer.GetCount()) != 0)
  {
    WLog::Error("Invalid EXR file: Cannot read version.");
    return W_FAILURE;
  }

  if (exrVersion.multipart)
  {
    WLog::Error("Invalid EXR file: Multi-part formats are not supported.");
    return W_FAILURE;
  }

  // read the EXR header
  const char* err = nullptr;
  if (ParseEXRHeaderFromMemory(&ref_exrHeader, &exrVersion, ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &err) != 0)
  {
    WLog::Error("Invalid EXR file: '{0}'", err);
    FreeEXRErrorMessage(err);
    return W_FAILURE;
  }

  for (int c = 1; c < ref_exrHeader.num_channels; ++c)
  {
    if (ref_exrHeader.pixel_types[c - 1] != ref_exrHeader.pixel_types[c])
    {
      WLog::Error("Unsupported EXR file: all channels should have the same size.");
      break;
    }
  }

  if (LoadEXRImageFromMemory(&ref_exrImage, &ref_exrHeader, ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &err) != 0)
  {
    WLog::Error("Invalid EXR file: '{0}'", err);

    FreeEXRHeader(&ref_exrHeader);
    FreeEXRErrorMessage(err);
    return W_FAILURE;
  }

  WImageFormat::Enum imageFormat = WImageFormat::UNKNOWN;

  switch (ref_exrHeader.num_channels)
  {
    case 1:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = WImageFormat::R32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = WImageFormat::R16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = WImageFormat::R32_UINT;
          break;
      }

      break;
    }

    case 2:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = WImageFormat::R32G32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = WImageFormat::R16G16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = WImageFormat::R32G32_UINT;
          break;
      }

      break;
    }

    case 3:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = WImageFormat::R32G32B32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = WImageFormat::R16G16B16A16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = WImageFormat::R32G32B32_UINT;
          break;
      }

      break;
    }

    case 4:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = WImageFormat::R32G32B32A32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = WImageFormat::R16G16B16A16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = WImageFormat::R32G32B32A32_UINT;
          break;
      }

      break;
    }
  }

  if (imageFormat == WImageFormat::UNKNOWN)
  {
    WLog::Error("Unsupported EXR file: {}-channel files with format '{}' are unsupported.", ref_exrHeader.num_channels, ref_exrHeader.pixel_types[0]);
    return W_FAILURE;
  }

  ref_header.SetWidth(ref_exrImage.width);
  ref_header.SetHeight(ref_exrImage.height);
  ref_header.SetImageFormat(imageFormat);

  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(1);
  ref_header.SetNumFaces(1);
  ref_header.SetDepth(1);

  return W_SUCCESS;
}

WResult WExrFileFormat::ReadImageHeader(WStreamReader& ref_stream, WImageHeader& ref_header, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  W_PROFILE_SCOPE("WExrFileFormat::ReadImageHeader");

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);
  W_SCOPE_EXIT(FreeEXRHeader(&exrHeader));

  EXRImage exrImage;
  InitEXRImage(&exrImage);
  W_SCOPE_EXIT(FreeEXRImage(&exrImage));

  WDynamicArray<WUInt8> fileBuffer;
  return ReadImageData(ref_stream, fileBuffer, ref_header, exrHeader, exrImage);
}

static void CopyChannel(WUInt8* pDst, const WUInt8* pSrc, WUInt32 uiNumElements, WUInt32 uiElementSize, WUInt32 uiDstStride)
{
  if (uiDstStride == uiElementSize)
  {
    // fast path to copy everything in one operation
    // this only happens for single-channel formats
    WMemoryUtils::RawByteCopy(pDst, pSrc, uiNumElements * uiElementSize);
  }
  else
  {
    for (WUInt32 i = 0; i < uiNumElements; ++i)
    {
      WMemoryUtils::RawByteCopy(pDst, pSrc, uiElementSize);

      pSrc = WMemoryUtils::AddByteOffset(pSrc, uiElementSize);
      pDst = WMemoryUtils::AddByteOffset(pDst, uiDstStride);
    }
  }
}

WResult WExrFileFormat::ReadImage(WStreamReader& ref_stream, WImage& ref_image, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  W_PROFILE_SCOPE("WExrFileFormat::ReadImage");

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);
  W_SCOPE_EXIT(FreeEXRHeader(&exrHeader));

  EXRImage exrImage;
  InitEXRImage(&exrImage);
  W_SCOPE_EXIT(FreeEXRImage(&exrImage));

  WImageHeader header;
  WDynamicArray<WUInt8> fileBuffer;

  W_SUCCEED_OR_RETURN(ReadImageData(ref_stream, fileBuffer, header, exrHeader, exrImage));

  ref_image.ResetAndAlloc(header);

  const WUInt32 uiPixelCount = header.GetWidth() * header.GetHeight();
  const WUInt32 uiNumDstChannels = WImageFormat::GetNumChannels(header.GetImageFormat());
  const WUInt32 uiNumSrcChannels = exrHeader.num_channels;

  WUInt32 uiSrcStride = 0;
  switch (exrHeader.pixel_types[0])
  {
    case TINYEXR_PIXELTYPE_FLOAT:
      uiSrcStride = sizeof(float);
      break;

    case TINYEXR_PIXELTYPE_HALF:
      uiSrcStride = sizeof(float) / 2;
      break;

    case TINYEXR_PIXELTYPE_UINT:
      uiSrcStride = sizeof(WUInt32);
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }


  // src and dst element size is always identical, we only copy from float->float, half->half or uint->uint
  // however data is interleaved in dst, but not interleaved in src

  const WUInt32 uiDstStride = uiSrcStride * uiNumDstChannels;
  WUInt8* pDstBytes = ref_image.GetBlobPtr<WUInt8>().GetPtr();

  if (uiNumDstChannels > uiNumSrcChannels)
  {
    // if we have more dst channels, than in the input data, fill everything with white
    WMemoryUtils::PatternFill(pDstBytes, 0xFF, uiDstStride * uiPixelCount);
  }

  WUInt32 c = 0;

  if (uiNumSrcChannels >= 4)
  {
    const WUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 4)
    {
      // copy to alpha
      CopyChannel(pDstBytes + 3 * uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 3)
  {
    const WUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 3)
    {
      // copy to blue
      CopyChannel(pDstBytes + 2 * uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 2)
  {
    const WUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 2)
    {
      // copy to green
      CopyChannel(pDstBytes + uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 1)
  {
    const WUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 1)
    {
      // copy to red
      CopyChannel(pDstBytes, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  return W_SUCCESS;
}

WResult WExrFileFormat::WriteImage(WStreamWriter& ref_stream, const WImageView& image, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(ref_stream);
  W_IGNORE_UNUSED(image);
  W_IGNORE_UNUSED(sFileExtension);

  W_ASSERT_NOT_IMPLEMENTED;
  return W_FAILURE;
}

bool WExrFileFormat::CanReadFileType(WStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("exr");
}

bool WExrFileFormat::CanWriteFileType(WStringView sExtension) const
{
  W_IGNORE_UNUSED(sExtension);

  return false;
}

#endif



W_STATICLINK_FILE(Texture, Texture_Image_Formats_ExrFileFormat);
