#include <Texture/TexturePCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StreamUtils.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

W_STATICLINK_FORCE static WImageFileFormatRegistrator<WStbImageFileFormats> g_StbImageFormats;

// stb_image callbacks would be better than loading the entire file into memory.
// However, it turned out that it does not map well to WStreamReader

// namespace
//{
//  // fill 'data' with 'size' bytes.  return number of bytes actually read
//  int read(void *user, char *data, int size)
//  {
//    WStreamReader* pStream = static_cast<WStreamReader*>(user);
//    return static_cast<int>(pStream->ReadBytes(data, size));
//  }
//  // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
//  void skip(void *user, int n)
//  {
//    WStreamReader* pStream = static_cast<WStreamReader*>(user);
//    if(n > 0)
//      pStream->SkipBytes(n);
//    else
//      // ?? We cannot reverse skip.
//
//  }
//  // returns nonzero if we are at end of file/data
//  int eof(void *user)
//  {
//    WStreamReader* pStream = static_cast<WStreamReader*>(user);
//    // ?
//  }
//}

namespace
{

  void write_func(void* pContext, void* pData, int iSize)
  {
    WStreamWriter* writer = static_cast<WStreamWriter*>(pContext);
    writer->WriteBytes(pData, iSize).IgnoreResult();
  }

  void* ReadImageData(WStreamReader& inout_stream, WDynamicArray<WUInt8>& ref_fileBuffer, WImageHeader& ref_imageHeader, bool& ref_bIsHDR)
  {
    WStreamUtils::ReadAllAndAppend(inout_stream, ref_fileBuffer);

    int width, height, numComp;

    ref_bIsHDR = !!stbi_is_hdr_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount());

    void* sourceImageData = nullptr;
    if (ref_bIsHDR)
    {
      sourceImageData = stbi_loadf_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    else
    {
      sourceImageData = stbi_load_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    if (!sourceImageData)
    {
      WLog::Error("stb_image failed to load: {0}", stbi_failure_reason());
      return nullptr;
    }
    ref_fileBuffer.Clear();

    WImageFormat::Enum format = WImageFormat::UNKNOWN;
    switch (numComp)
    {
      case 1:
        format = (ref_bIsHDR) ? WImageFormat::R32_FLOAT : WImageFormat::R8_UNORM;
        break;
      case 2:
        format = (ref_bIsHDR) ? WImageFormat::R32G32_FLOAT : WImageFormat::R8G8_UNORM;
        break;
      case 3:
        format = (ref_bIsHDR) ? WImageFormat::R32G32B32_FLOAT : WImageFormat::R8G8B8_UNORM;
        break;
      case 4:
        format = (ref_bIsHDR) ? WImageFormat::R32G32B32A32_FLOAT : WImageFormat::R8G8B8A8_UNORM;
        break;
    }

    // Set properties and allocate.
    ref_imageHeader.SetImageFormat(format);
    ref_imageHeader.SetNumMipLevels(1);
    ref_imageHeader.SetNumArrayIndices(1);
    ref_imageHeader.SetNumFaces(1);

    ref_imageHeader.SetWidth(width);
    ref_imageHeader.SetHeight(height);
    ref_imageHeader.SetDepth(1);

    return sourceImageData;
  }

} // namespace

WResult WStbImageFileFormats::ReadImageHeader(WStreamReader& inout_stream, WImageHeader& ref_header, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  W_PROFILE_SCOPE("WStbImageFileFormats::ReadImageHeader");

  bool isHDR = false;
  WDynamicArray<WUInt8> fileBuffer;
  void* sourceImageData = ReadImageData(inout_stream, fileBuffer, ref_header, isHDR);

  if (sourceImageData == nullptr)
    return W_FAILURE;

  stbi_image_free(sourceImageData);

  // Expand grayscale to RGB so that imported textures don't turn red. See end of ReadImage below.
  if (ref_header.GetImageFormat() == WImageFormat::R8_UNORM)
    ref_header.SetImageFormat(WImageFormat::R8G8B8_UNORM);
  // Expand grayscale+alpha to RGBA so that imported textures don't turn yellow. See end of ReadImage below.
  if (ref_header.GetImageFormat() == WImageFormat::R8G8_UNORM)
    ref_header.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);

  return W_SUCCESS;
}

WResult WStbImageFileFormats::ReadImage(WStreamReader& inout_stream, WImage& ref_image, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  W_PROFILE_SCOPE("WStbImageFileFormats::ReadImage");

  bool isHDR = false;
  WDynamicArray<WUInt8> fileBuffer;
  WImageHeader imageHeader;
  void* sourceImageData = ReadImageData(inout_stream, fileBuffer, imageHeader, isHDR);

  if (sourceImageData == nullptr)
    return W_FAILURE;

  ref_image.ResetAndAlloc(imageHeader);

  const size_t numComp = WImageFormat::GetNumChannels(imageHeader.GetImageFormat());

  const size_t elementsToCopy = static_cast<size_t>(imageHeader.GetWidth()) * static_cast<size_t>(imageHeader.GetHeight()) * numComp;

  // Set pixels. Different strategies depending on component count.
  if (isHDR)
  {
    float* targetImageData = ref_image.GetBlobPtr<float>().GetPtr();
    WMemoryUtils::Copy(targetImageData, (const float*)sourceImageData, elementsToCopy);
  }
  else
  {
    WUInt8* targetImageData = ref_image.GetBlobPtr<WUInt8>().GetPtr();
    WMemoryUtils::Copy(targetImageData, (const WUInt8*)sourceImageData, elementsToCopy);
  }

  stbi_image_free((void*)sourceImageData);

  // Expand grayscale to RGB so that imported textures don't turn red.
  if (ref_image.GetImageFormat() == WImageFormat::R8_UNORM)
  {
    ref_image.Convert(WImageFormat::R8G8B8_UNORM).AssertSuccess();
    WImageUtils::CopyChannel(ref_image, 1, ref_image, 0).AssertSuccess();
    WImageUtils::CopyChannel(ref_image, 2, ref_image, 0).AssertSuccess();
  }
  // Expand grayscale+alpha to RGBA so that imported textures don't turn yellow.
  if (ref_image.GetImageFormat() == WImageFormat::R8G8_UNORM)
  {
    ref_image.Convert(WImageFormat::R8G8B8A8_UNORM).AssertSuccess();
    WImageUtils::CopyChannel(ref_image, 3, ref_image, 1).AssertSuccess();
    WImageUtils::CopyChannel(ref_image, 1, ref_image, 0).AssertSuccess();
    WImageUtils::CopyChannel(ref_image, 2, ref_image, 0).AssertSuccess();
  }

  return W_SUCCESS;
}

WResult WStbImageFileFormats::WriteImage(WStreamWriter& inout_stream, const WImageView& image, WStringView sFileExtension) const
{
  WImageFormat::Enum compatibleFormats[] = {WImageFormat::R8_UNORM, WImageFormat::R8G8B8_UNORM, WImageFormat::R8G8B8A8_UNORM};

  // Find a compatible format closest to the one the image currently has
  WImageFormat::Enum format = WImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == WImageFormat::UNKNOWN)
  {
    WLog::Error("No conversion from format '{0}' to a format suitable for PNG files known.", WImageFormat::GetName(image.GetImageFormat()));
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

  if (sFileExtension.IsEqual_NoCase("png"))
  {
    if (stbi_write_png_to_func(write_func, &inout_stream, image.GetWidth(), image.GetHeight(), WImageFormat::GetNumChannels(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 0))
    {
      return W_SUCCESS;
    }
  }

  if (sFileExtension.IsEqual_NoCase("jpg") || sFileExtension.IsEqual_NoCase("jpeg"))
  {
    if (stbi_write_jpg_to_func(write_func, &inout_stream, image.GetWidth(), image.GetHeight(), WImageFormat::GetNumChannels(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 95))
    {
      return W_SUCCESS;
    }
  }

  return W_FAILURE;
}

bool WStbImageFileFormats::CanReadFileType(WStringView sExtension) const
{
  if (sExtension.IsEqual_NoCase("hdr"))
    return true;

#if W_DISABLED(W_PLATFORM_WINDOWS_DESKTOP)

  // on Windows Desktop, we prefer to use WIC (WWicFileFormat)
  if (sExtension.IsEqual_NoCase("png") || sExtension.IsEqual_NoCase("jpg") || sExtension.IsEqual_NoCase("jpeg"))
  {
    return true;
  }
#endif

  return false;
}

bool WStbImageFileFormats::CanWriteFileType(WStringView sExtension) const
{
  // even when WIC is available, prefer to write these files through STB, to get consistent output
  if (sExtension.IsEqual_NoCase("png") || sExtension.IsEqual_NoCase("jpg") || sExtension.IsEqual_NoCase("jpeg"))
  {
    return true;
  }

  return false;
}



W_STATICLINK_FILE(Texture, Texture_Image_Formats_StbImageFileFormats);
