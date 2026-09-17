#include <Texture/TexturePCH.h>

#ifdef BUILDSYSTEM_ENABLE_LUNASVG_SUPPORT

#  include <Texture/Image/Formats/SvgFileFormat.h>
#  include <Texture/Image/Image.h>

#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>

#  include <lunasvg.h>

W_STATICLINK_FORCE static WImageFileFormatRegistrator<WSvgFileFormat> g_SvgFileFormat;

WResult WSvgFileFormat::ReadImageHeader(WStreamReader& inout_stream, WImageHeader& ref_header, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(inout_stream);
  W_IGNORE_UNUSED(sFileExtension);

  ref_header.SetWidth(m_uiResolutionX);
  ref_header.SetHeight(m_uiResolutionY);
  ref_header.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);
  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(1);
  ref_header.SetNumFaces(1);
  ref_header.SetDepth(1);

  return W_SUCCESS;
}

WResult WSvgFileFormat::ReadImage(WStreamReader& inout_stream, WImage& ref_image, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);
  W_PROFILE_SCOPE("WSvgFileFormat::ReadImage");

  // Read entire SVG file into memory
  WDynamicArray<WUInt8> fileBuffer;
  WStreamUtils::ReadAllAndAppend(inout_stream, fileBuffer);

  if (fileBuffer.IsEmpty())
  {
    WLog::Error("SVG file is empty");
    return W_FAILURE;
  }

  // Parse SVG document
  std::unique_ptr<lunasvg::Document> document = lunasvg::Document::loadFromData(
    reinterpret_cast<const char*>(fileBuffer.GetData()),
    fileBuffer.GetCount());

  if (!document)
  {
    WLog::Error("Failed to parse SVG file");
    return W_FAILURE;
  }

  // Render SVG to bitmap at default resolution
  lunasvg::Bitmap bitmap = document->renderToBitmap(
    static_cast<int>(m_uiResolutionX),
    static_cast<int>(m_uiResolutionY),
    m_Background.ToRGBA8());

  if (bitmap.isNull() || bitmap.data() == nullptr)
  {
    WLog::Error("Failed to render SVG to bitmap");
    return W_FAILURE;
  }

  // Convert from ARGB32 premultiplied to RGBA
  bitmap.convertToRGBA();

  // Set up image header
  WImageHeader header;
  header.SetWidth(m_uiResolutionX);
  header.SetHeight(m_uiResolutionY);
  header.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);
  header.SetNumMipLevels(1);
  header.SetNumArrayIndices(1);
  header.SetNumFaces(1);
  header.SetDepth(1);

  ref_image.ResetAndAlloc(header);

  // Copy pixel data
  const WUInt32 pixelCount = m_uiResolutionX * m_uiResolutionY;
  WMemoryUtils::Copy(ref_image.GetBlobPtr<WUInt8>().GetPtr(), bitmap.data(), pixelCount * 4);

  return W_SUCCESS;
}

WResult WSvgFileFormat::WriteImage(WStreamWriter& inout_stream, const WImageView& image, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(inout_stream);
  W_IGNORE_UNUSED(image);
  W_IGNORE_UNUSED(sFileExtension);

  WLog::Error("Writing SVG files is not supported");
  return W_FAILURE;
}

bool WSvgFileFormat::CanReadFileType(WStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("svg");
}

bool WSvgFileFormat::CanWriteFileType(WStringView sExtension) const
{
  W_IGNORE_UNUSED(sExtension);
  return false;
}

#endif

W_STATICLINK_FILE(Texture, Texture_Image_Formats_SvgFileFormat);
