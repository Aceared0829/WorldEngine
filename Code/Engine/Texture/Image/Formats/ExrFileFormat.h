#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#ifdef BUILDSYSTEM_ENABLE_TINYEXR_SUPPORT

/// EXR file format support using TinyEXR.
class W_TEXTURE_DLL WExrFileFormat : public WImageFileFormat
{
public:
  WResult ReadImageHeader(WStreamReader& ref_stream, WImageHeader& ref_header, WStringView sFileExtension) const override;
  WResult ReadImage(WStreamReader& ref_stream, WImage& ref_image, WStringView sFileExtension) const override;
  WResult WriteImage(WStreamWriter& ref_stream, const WImageView& image, WStringView sFileExtension) const override;

  bool CanReadFileType(WStringView sExtension) const override;
  bool CanWriteFileType(WStringView sExtension) const override;
};

#endif
