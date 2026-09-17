
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

struct W_RENDERERFOUNDATION_DLL WGALResourceFormat
{
  using StorageType = WUInt8;

  enum Enum : WUInt8
  {
    Invalid = 0,

    RGBAFloat,
    XYZWFloat = RGBAFloat,
    RGBAUInt,
    RGBAInt,

    RGBFloat,
    XYZFloat = RGBFloat,
    UVWFloat = RGBFloat,
    RGBUInt,
    RGBInt,

    B5G6R5UNormalized,
    BGRAUByteNormalized,
    BGRAUByteNormalizedsRGB,

    RGBAHalf,
    XYZWHalf = RGBAHalf,
    RGBAUShort,
    RGBAUShortNormalized,
    RGBAShort,
    RGBAShortNormalized,

    RGFloat,
    XYFloat = RGFloat,
    UVFloat = RGFloat,
    RGUInt,
    RGInt,

    RGB10A2UInt,
    RGB10A2UIntNormalized,
    RG11B10Float,

    RGBAUByteNormalized,
    RGBAUByteNormalizedsRGB,
    RGBAUByte,
    RGBAByteNormalized,
    RGBAByte,

    RGHalf,
    XYHalf = RGHalf,
    UVHalf = RGHalf,
    RGUShort,
    RGUShortNormalized,
    RGShort,
    RGShortNormalized,
    RGUByte,
    RGUByteNormalized,
    RGByte,
    RGByteNormalized,

    DFloat,

    RFloat,
    RUInt,
    RInt,
    RHalf,
    RUShort,
    RUShortNormalized,
    RShort,
    RShortNormalized,
    RUByte,
    RUByteNormalized,
    RByte,
    RByteNormalized,

    AUByteNormalized,

    D16,
    D24S8,

    BC1,
    BC1sRGB,
    BC2,
    BC2sRGB,
    BC3,
    BC3sRGB,
    BC4UNormalized,
    BC4Normalized,
    BC5UNormalized,
    BC5Normalized,
    BC6UFloat,
    BC6Float,
    BC7UNormalized,
    BC7UNormalizedsRGB,

    // careful, if anything gets added here, make sure to update IsBlockCompressed()

    ENUM_COUNT,

    Default = Invalid
  };


  // General format Meta-Informations:

  /// The size in bits per element (usually pixels, except for mesh stream elements) of a single element of the given resource format.
  static WUInt32 GetBitsPerElement(WGALResourceFormat::Enum format);

  /// The number of color channels this format contains.
  static WUInt8 GetChannelCount(WGALResourceFormat::Enum format);

  /// \todo A combination of propertyflags, something like srgb, normalized, ...
  // Would be very useful for some GL stuff and Testing.

  /// Returns whether the given resource format is a depth format
  static bool IsDepthFormat(WGALResourceFormat::Enum format);
  static bool IsStencilFormat(WGALResourceFormat::Enum format);

  static bool IsSrgb(WGALResourceFormat::Enum format);

  static bool IsBlockCompressed(WGALResourceFormat::Enum format);

  /// Returns whether the given resource format returns integer values when sampled (e.g. RUShort). Note that normalized formats like RGUShortNormalized are not considered integer formats as they return float values in the [0..1] range when sampled.
  static bool IsIntegerFormat(WGALResourceFormat::Enum format);

  /// Returns whether the given resource format can store negative values.
  static bool IsSignedFormat(WGALResourceFormat::Enum format);

  static bool IsFloatFormat(WGALResourceFormat::Enum format);

private:
  static const WUInt8 s_BitsPerElement[WGALResourceFormat::ENUM_COUNT];

  static const WUInt8 s_ChannelCount[WGALResourceFormat::ENUM_COUNT];
};

template <typename NativeFormatType, NativeFormatType InvalidFormat>
class WGALFormatLookupEntry
{
public:
  inline WGALFormatLookupEntry();

  inline WGALFormatLookupEntry(NativeFormatType storage);

  inline WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RT(NativeFormatType renderTargetType);

  inline WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& D(NativeFormatType depthOnlyType);

  inline WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& S(NativeFormatType stencilOnlyType);

  inline WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& DS(NativeFormatType depthStencilType);

  inline WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& VA(NativeFormatType vertexAttributeType);

  inline WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RV(NativeFormatType resourceViewType);

  NativeFormatType m_eStorage;
  NativeFormatType m_eRenderTarget;
  NativeFormatType m_eDepthOnlyType;
  NativeFormatType m_eStencilOnlyType;
  NativeFormatType m_eDepthStencilType;
  NativeFormatType m_eVertexAttributeType;
  NativeFormatType m_eResourceViewType;
};

// Reusable table class to store lookup information (from WGALResourceFormat to the various formats for texture/buffer storage, views)
template <typename FormatClass>
class WGALFormatLookupTable
{
public:
  WGALFormatLookupTable();

  W_ALWAYS_INLINE const FormatClass& GetFormatInfo(WGALResourceFormat::Enum format) const;

  W_ALWAYS_INLINE void SetFormatInfo(WGALResourceFormat::Enum format, const FormatClass& newFormatInfo);

private:
  FormatClass m_Formats[WGALResourceFormat::ENUM_COUNT];
};

#include <RendererFoundation/Resources/Implementation/ResourceFormats_inl.h>
