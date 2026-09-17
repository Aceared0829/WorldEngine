#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

const WImageFileFormat* WImageFileFormat::GetReaderFormat(WStringView sExtension)
{
  for (auto format = WRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
  {
    if (format->GetFormatType().CanReadFileType(sExtension))
    {
      return &format->GetFormatType();
    }
  }

  return nullptr;
}

const WImageFileFormat* WImageFileFormat::GetWriterFormat(WStringView sExtension)
{
  for (auto format = WRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
  {
    if (format->GetFormatType().CanWriteFileType(sExtension))
    {
      return &format->GetFormatType();
    }
  }

  return nullptr;
}

WResult WImageFileFormat::ReadImageHeader(WStringView sFileName, WImageHeader& ref_header)
{
  W_LOG_BLOCK("Read Image Header", sFileName);

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
    if (pFormat->ReadImageHeader(reader, ref_header, it) != W_SUCCESS)
    {
      WLog::Warning("Failed to read image file '{0}'", WArgSensitive(sFileName, "File"));
      return W_FAILURE;
    }

    return W_SUCCESS;
  }

  WLog::Warning("No known image file format for extension '{0}'", it);
  return W_FAILURE;
}

//////////////////////////////////////////////////////////////////////////

W_ENUMERABLE_CLASS_IMPLEMENTATION(WRegisteredImageFileFormat);

WRegisteredImageFileFormat::WRegisteredImageFileFormat() = default;
WRegisteredImageFileFormat::~WRegisteredImageFileFormat() = default;
