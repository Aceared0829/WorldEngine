#pragma once

#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <Texture/Image/Image.h>

/// Utility functions for texture format conversion and manipulation.
struct W_RENDERERCORE_DLL WTextureUtils
{
  /// Converts an image format to the corresponding GPU texture format.
  ///
  /// The bSRGB parameter determines whether to use sRGB or linear variants.
  static WGALResourceFormat::Enum ImageFormatToGalFormat(WImageFormat::Enum format, bool bSRGB);

  /// Converts a GPU texture format to the corresponding image format.
  ///
  /// If bRemoveSRGB is true, sRGB formats are converted to their linear equivalents.
  static WImageFormat::Enum GalFormatToImageFormat(WGALResourceFormat::Enum format, bool bRemoveSRGB);

  /// Converts a GPU texture format to the corresponding image format.
  static WImageFormat::Enum GalFormatToImageFormat(WGALResourceFormat::Enum format);

  /// Configures a sampler state based on a texture filter setting.
  ///
  /// Sets up filtering mode, addressing, and anisotropy based on the filter enum.
  static void ConfigureSampler(WTextureFilterSetting::Enum filter, WGALSamplerStateCreationDescription& out_sampler);

  /// Copies the given texture subresource from `memory` into `out_image` according to the texture description.
  static void CopySubResourceToImage(const WGALTextureCreationDescription& desc, const WGALTextureSubresource& subResource, const WGALSystemMemoryDescription& memory, WImage& out_image, bool bRemoveSRGB);
  /// Returns an image view of the texture subresource in `memory`. If the format allows for it, the memory will be aliased, removing the need to copy the data but the view becomes invalid once the memory does. If this is not possible, the function reverts to calling CopySubResourceToImage with `ref_tempImage` used as the data storage. You can check if `ref_tempImage` is valid to figure out which code path was taken.
  static WImageView MakeImageViewFromSubResource(const WGALTextureCreationDescription& desc, const WGALTextureSubresource& subResource, const WGALSystemMemoryDescription& memory, WImage& ref_tempImage, bool bRemoveSRGB);
  /// Copies a texture subresource memory to a new location with a different row pitch.
  static void CopySubResourceToMemory(const WGALTextureCreationDescription& desc, const WGALTextureSubresource& subResource, const WGALSystemMemoryDescription& sourceMemory, WArrayPtr<WUInt8> targetData, WUInt32 uiTargetRowPitch);

  /// If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;
};
