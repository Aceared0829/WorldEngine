#pragma once

#include <Texture/TexConv/TexConvEnums.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>

struct WTexConvChannelMapping
{
  WInt8 m_iInputImageIndex = -1;
  WTexConvChannelValue::Enum m_ChannelValue;
};

/// Describes from which input file to read which channel and then write it to the R, G, B, or A channel of the
/// output file. The four elements of the array represent the four channels of the output image.
struct WTexConvSliceChannelMapping
{
  WTexConvChannelMapping m_Channel[4] = {
    WTexConvChannelMapping{-1, WTexConvChannelValue::Red},
    WTexConvChannelMapping{-1, WTexConvChannelValue::Green},
    WTexConvChannelMapping{-1, WTexConvChannelValue::Blue},
    WTexConvChannelMapping{-1, WTexConvChannelValue::Alpha},
  };
};

/// Complete texture conversion configuration with all processing options.
///
/// This structure contains all settings needed to convert source images into optimized
/// textures for runtime use. It handles input specification, format conversion, quality
/// settings, mipmap generation, and platform-specific optimizations.
///
/// **Basic Usage Pattern:**
/// ```cpp
/// WTexConvDesc desc;
/// desc.m_InputFiles.PushBack("diffuse.png");
/// desc.m_OutputType = WTexConvOutputType::Texture2D;
/// desc.m_Usage = WTexConvUsage::Color;
/// desc.m_CompressionMode = WTexConvCompressionMode::HighQuality;
/// desc.m_TargetPlatform = WTexConvTargetPlatform::PC;
/// // Process with WTexConvProcessor...
/// ```
class W_TEXTURE_DLL WTexConvDesc
{
  W_DISALLOW_COPY_AND_ASSIGN(WTexConvDesc);

public:
  WTexConvDesc() = default;

  // Input specification
  WHybridArray<WString, 4> m_InputFiles;                          ///< Source image file paths to process
  WDynamicArray<WImage> m_InputImages;                            ///< Pre-loaded source images (alternative to file paths)

  WHybridArray<WTexConvSliceChannelMapping, 6> m_ChannelMappings; ///< Channel routing for multi-input processing

  // If the source images are a grid (sprite sheet) of equally sized cells, only a single cell is processed
  // and the rest of the image is discarded right after loading. Cells are indexed left to right, top to bottom.
  // With a 1x1 grid the images are used as-is.
  WUInt8 m_uiSourceGridX = 1;     ///< Number of columns in the source images
  WUInt8 m_uiSourceGridY = 1;     ///< Number of rows in the source images
  WUInt16 m_uiSourceGridCell = 0; ///< Which cell to extract, wraps around if it exceeds the number of cells

  // Output configuration
  WEnum<WTexConvOutputType> m_OutputType;         ///< Type of texture to generate (2D, Cubemap, 3D, etc.)
  WEnum<WTexConvTargetPlatform> m_TargetPlatform; ///< Target platform for format optimization

  // Multi-resolution output
  WUInt32 m_uiLowResMipmaps = 0;             ///< Number of low-resolution mipmap levels to generate separately
  WUInt32 m_uiThumbnailOutputResolution = 0; ///< Size for thumbnail generation (0 = no thumbnail)

  // Format and compression
  WEnum<WTexConvUsage> m_Usage;                     ///< Intended usage (Color, Normal, Linear, etc.) affects format selection
  WEnum<WTexConvCompressionMode> m_CompressionMode; ///< Quality vs file size trade-off

  // Resolution control
  WUInt32 m_uiMinResolution = 16;       ///< Minimum texture dimension (prevents over-downscaling)
  WUInt32 m_uiMaxResolution = 1024 * 8; ///< Maximum texture dimension (prevents excessive memory usage)
  WUInt32 m_uiDownscaleSteps = 0;       ///< Number of 2x downscaling steps to apply

  // Mipmap generation
  WEnum<WTexConvMipmapMode> m_MipmapMode;    ///< Mipmap generation strategy
  WEnum<WTextureFilterSetting> m_FilterMode; ///< Runtime filtering quality (W formats only)
  WEnum<WImageAddressMode> m_AddressModeU;   ///< Horizontal texture wrapping mode
  WEnum<WImageAddressMode> m_AddressModeV;   ///< Vertical texture wrapping mode
  WEnum<WImageAddressMode> m_AddressModeW;   ///< Depth texture wrapping mode (3D textures)
  bool m_bPreserveMipmapCoverage = false;      ///< Maintain alpha coverage for alpha testing
  float m_fMipmapAlphaThreshold = 0.25f;       ///< Alpha threshold for coverage preservation.

  // Image processing options
  WUInt8 m_uiDilateColor = 0;      ///< Color dilation steps (fills transparent areas)
  bool m_bFlipHorizontal = false;   ///< Mirror image horizontally
  bool m_bPremultiplyAlpha = false; ///< Pre-multiply RGB by alpha for correct blending
  float m_fHdrExposureBias = 0.0f;  ///< HDR exposure adjustment (stops)
  float m_fMaxValue = 64000.f;      ///< HDR value clamping

  // Runtime metadata
  WUInt64 m_uiAssetHash = 0;    ///< Content hash for cache invalidation
  WUInt16 m_uiAssetVersion = 0; ///< Asset version for dependency tracking

  // Advanced features
  WString m_sTextureAtlasDescFile;               ///< Path to texture atlas description file
  WEnum<WTexConvBumpMapFilter> m_BumpMapFilter; ///< Bump map specific filtering
};
