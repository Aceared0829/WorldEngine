#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

/// File format implementation for loading TIFF files using WIC
class W_TEXTURE_DLL WWicFileFormat : public WImageFileFormat
{
public:
  WWicFileFormat();
  virtual ~WWicFileFormat();

  virtual WResult ReadImageHeader(WStreamReader& inout_stream, WImageHeader& ref_header, WStringView sFileExtension) const override;
  virtual WResult ReadImage(WStreamReader& inout_stream, WImage& ref_image, WStringView sFileExtension) const override;
  virtual WResult WriteImage(WStreamWriter& inout_stream, const WImageView& image, WStringView sFileExtension) const override;

  virtual bool CanReadFileType(WStringView sExtension) const override;
  virtual bool CanWriteFileType(WStringView sExtension) const override;

private:
  mutable bool m_bTryCoInit = true; // Helper for keeping track of whether we have tried to init COM exactly once
  mutable bool m_bCoUninitOnShutdown =
    false;                          // Helper for keeping track of whether we have to uninitialize COM (because we were the first to initialize it)

  WResult ReadFileData(WStreamReader& stream, WDynamicArray<WUInt8>& storage) const;
};

#endif
