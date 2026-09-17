#pragma once

#ifdef BUILDSYSTEM_ENABLE_LUNASVG_SUPPORT

#  include <Texture/Image/Formats/ImageFileFormat.h>

/// SVG file format support using LunaSVG.
///
/// Rasterizes SVG files to RGBA images.
/// Target resolution and background color can be adjusted on the loader.
///
/// Note: Instantiate the loader directly to override the defaults.
class W_TEXTURE_DLL WSvgFileFormat : public WImageFileFormat
{
public:
  /// The resolution at which to rasterize the image.
  WUInt32 m_uiResolutionX = 512;
  WUInt32 m_uiResolutionY = 512;
  WColorGammaUB m_Background{0, 0, 0, 0}; // transparent

  WResult ReadImageHeader(WStreamReader& inout_stream, WImageHeader& ref_header, WStringView sFileExtension) const override;
  WResult ReadImage(WStreamReader& inout_stream, WImage& ref_image, WStringView sFileExtension) const override;
  WResult WriteImage(WStreamWriter& inout_stream, const WImageView& image, WStringView sFileExtension) const override;

  bool CanReadFileType(WStringView sExtension) const override;
  bool CanWriteFileType(WStringView sExtension) const override;
};

#endif
