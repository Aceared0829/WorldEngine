#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

class W_TEXTURE_DLL WDdsFileFormat : public WImageFileFormat
{
public:
  virtual WResult ReadImageHeader(WStreamReader& inout_stream, WImageHeader& ref_header, WStringView sFileExtension) const override;
  virtual WResult ReadImage(WStreamReader& inout_stream, WImage& ref_image, WStringView sFileExtension) const override;
  virtual WResult WriteImage(WStreamWriter& inout_stream, const WImageView& image, WStringView sFileExtension) const override;

  virtual bool CanReadFileType(WStringView sExtension) const override;
  virtual bool CanWriteFileType(WStringView sExtension) const override;
};
