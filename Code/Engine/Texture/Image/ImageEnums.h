#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Texture/TextureDLL.h>

/// Defines how texture coordinates outside [0,1] are handled during sampling.
struct W_TEXTURE_DLL WImageAddressMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Repeat,      ///< Tile the texture by repeating texture coordinates (0.0, 1.0, 2.0 -> 0.0, 0.0, 0.0)
    Clamp,       ///< Clamp coordinates to [0,1] range (coordinates outside become edge pixels)
    ClampBorder, ///< Use a specific border color for coordinates outside [0,1]
    Mirror,      ///< Mirror the texture at boundaries (0.0, 1.0, 2.0 -> 0.0, 1.0, 0.0)

    ENUM_COUNT,

    Default = Repeat
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_TEXTURE_DLL, WImageAddressMode);

//////////////////////////////////////////////////////////////////////////
// WTextureFilterSetting
//////////////////////////////////////////////////////////////////////////

/// Defines texture filtering quality and method for runtime sampling.
///
/// This enum allows both fixed filtering methods and quality-based settings.
/// Fixed methods specify the exact filtering algorithm, while quality settings
/// allow the renderer to choose appropriate filtering based on hardware capabilities
/// and performance considerations.
struct W_TEXTURE_DLL WTextureFilterSetting
{
  using StorageType = WUInt8;

  enum Enum
  {
    FixedNearest,        ///< Always use nearest neighbor filtering (no interpolation)
    FixedBilinear,       ///< Always use bilinear filtering within mip levels
    FixedTrilinear,      ///< Always use trilinear filtering between mip levels
    FixedAnisotropic2x,  ///< Always use 2x anisotropic filtering
    FixedAnisotropic4x,  ///< Always use 4x anisotropic filtering
    FixedAnisotropic8x,  ///< Always use 8x anisotropic filtering
    FixedAnisotropic16x, ///< Always use 16x anisotropic filtering

    LowestQuality,       ///< Let renderer choose lowest quality filtering
    LowQuality,          ///< Let renderer choose low quality filtering
    DefaultQuality,      ///< Let renderer choose default quality filtering
    HighQuality,         ///< Let renderer choose high quality filtering
    HighestQuality,      ///< Let renderer choose highest quality filtering

    Default = DefaultQuality
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_TEXTURE_DLL, WTextureFilterSetting);
