#pragma once

#include <Texture/Image/ImageFormat.h>

/// Helper class containing methods to convert between WImageFormat::Enum and platform-specific image formats.
class W_TEXTURE_DLL WImageFormatMappings
{
public:
  /// Maps an WImageFormat::Enum to an equivalent Direct3D DXGI_FORMAT.
  static WUInt32 ToDxgiFormat(WImageFormat::Enum format);

  /// Maps a Direct3D DXGI_FORMAT to an equivalent WImageFormat::Enum.
  static WImageFormat::Enum FromDxgiFormat(WUInt32 uiDxgiFormat);

  /// Maps an WImageFormat::Enum to an equivalent FourCC code.
  static WUInt32 ToFourCc(WImageFormat::Enum format);

  /// Maps a FourCC code to an equivalent WImageFormat::Enum.
  static WImageFormat::Enum FromFourCc(WUInt32 uiFourCc);
};
