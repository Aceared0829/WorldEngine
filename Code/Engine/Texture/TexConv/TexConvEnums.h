#pragma once

#include <Texture/TextureDLL.h>

#include <Foundation/Reflection/Reflection.h>

struct WTexConvOutputType
{
  enum Enum
  {
    None,
    Texture2D,
    Volume,
    Cubemap,
    Atlas,
    Texture2DArray,

    Default = Texture2D
  };

  using StorageType = WUInt8;
};

struct WTexConvCompressionMode
{
  enum Enum
  {
    // note: order of enum values matters
    None = 0,   // uncompressed
    Medium = 1, // compressed with high quality, if possible
    High = 2,   // strongest compression, if possible

    Default = Medium,
  };

  using StorageType = WUInt8;
};

W_DECLARE_REFLECTABLE_TYPE(W_TEXTURE_DLL, WTexConvCompressionMode);

struct WTexConvUsage
{
  enum Enum
  {
    Auto, ///< Target format will be detected from heuristics (filename, content)

    // Exact format will be decided together with WTexConvCompressionMode

    Color,
    Linear,
    Hdr,

    NormalMap,          // DirectX convention
    NormalMap_Inverted, // OpenGL convention

    BumpMap,

    Default = Auto
  };

  using StorageType = WUInt8;
};

W_DECLARE_REFLECTABLE_TYPE(W_TEXTURE_DLL, WTexConvUsage);

struct WTexConvMipmapMode
{
  enum Enum
  {
    None, ///< Mipmap generation is disabled, output will have no mipmaps
    Linear,
    Kaiser,

    Default = Kaiser
  };

  using StorageType = WUInt8;
};

W_DECLARE_REFLECTABLE_TYPE(W_TEXTURE_DLL, WTexConvMipmapMode);

struct WTexConvTargetPlatform
{
  enum Enum
  {
    PC,
    Android,

    Default = PC
  };

  using StorageType = WUInt8;
};

/// Defines which channel of another texture to read to get a value
struct WTexConvChannelValue
{
  enum Enum
  {
    Red,   ///< read the RED channel
    Green, ///< read the GREEN channel
    Blue,  ///< read the BLUE channel
    Alpha, ///< read the ALPHA channel

    Black, ///< don't read any channel, just take the constant value 0
    White, ///< don't read any channel, just take the constant value 0xFF / 1.0f
  };
};

/// Defines which filter kernel is used to approximate the x/y bump map gradients
struct WTexConvBumpMapFilter
{
  enum Enum
  {
    Finite, ///< Simple finite differences in a 4-Neighborhood
    Sobel,  ///< Sobel kernel (8-Neighborhood)
    Scharr, ///< Scharr kernel (8-Neighborhood)

    Default = Finite
  };

  using StorageType = WUInt8;
};
