
// static
W_ALWAYS_INLINE WUInt32 WGALResourceFormat::GetBitsPerElement(WGALResourceFormat::Enum format)
{
  return s_BitsPerElement[format];
}

// static
W_ALWAYS_INLINE WUInt8 WGALResourceFormat::GetChannelCount(WGALResourceFormat::Enum format)
{
  return s_ChannelCount[format];
}

// static
W_FORCE_INLINE bool WGALResourceFormat::IsDepthFormat(WGALResourceFormat::Enum format)
{
  return format == DFloat || format == D16 || format == D24S8;
}

// static
W_FORCE_INLINE bool WGALResourceFormat::IsStencilFormat(Enum format)
{
  return format == D24S8;
}

// static
W_FORCE_INLINE bool WGALResourceFormat::IsSrgb(WGALResourceFormat::Enum format)
{
  return format == BGRAUByteNormalizedsRGB || format == RGBAUByteNormalizedsRGB || format == BC1sRGB || format == BC2sRGB || format == BC3sRGB ||
         format == BC7UNormalizedsRGB;
}

W_FORCE_INLINE bool WGALResourceFormat::IsIntegerFormat(Enum format)
{
  switch (format)
  {
    // D16 is actually a 16 bit unorm format
    case WGALResourceFormat::D16:
    // 32bit, 4 channel
    case WGALResourceFormat::RGBAUInt:
    case WGALResourceFormat::RGBAInt:
    // 32bit, 3 channel
    case WGALResourceFormat::RGBUInt:
    case WGALResourceFormat::RGBInt:
    // 16bit, 4 channel
    case WGALResourceFormat::RGBAUShort:
    case WGALResourceFormat::RGBAShort:
    // 16bit, 2 channel
    case WGALResourceFormat::RGUInt:
    case WGALResourceFormat::RGInt:
    // packed 32bit, 4 channel
    case WGALResourceFormat::RGB10A2UInt:
    // 8bit, 4 channel
    case WGALResourceFormat::RGBAUByte:
    case WGALResourceFormat::RGBAByte:
    // 16bit, 2 channel
    case WGALResourceFormat::RGUShort:
    case WGALResourceFormat::RGShort:
    // 8bit, 2 channel
    case WGALResourceFormat::RGUByte:
    case WGALResourceFormat::RGByte:
    // 32bit, 1 channel
    case WGALResourceFormat::RUInt:
    case WGALResourceFormat::RInt:
    // 16bit, 1 channel
    case WGALResourceFormat::RUShort:
    case WGALResourceFormat::RShort:
    // 8bit, 1 channel
    case WGALResourceFormat::RUByte:
    case WGALResourceFormat::RByte:
      return true;
    default:
      return false;
  }
}

W_FORCE_INLINE bool WGALResourceFormat::IsSignedFormat(Enum format)
{
  switch (format)
  {
    case RGBAFloat:
    case RGBAInt:
    case RGBFloat:
    case RGBInt:
    case RGBAHalf:
    case RGBAShort:
    case RGBAShortNormalized:
    case RGFloat:
    case RGInt:
    case RGB10A2UIntNormalized:
    case RG11B10Float:
    case RGBAByteNormalized:
    case RGBAByte:
    case RGHalf:
    case RGShort:
    case RGShortNormalized:
    case RGByte:
    case RGByteNormalized:
    case RFloat:
    case RInt:
    case RHalf:
    case RShort:
    case RShortNormalized:
    case RByte:
    case RByteNormalized:
    case BC4Normalized:
    case BC5Normalized:
    case BC6Float:
      return true;
    default:
      return false;
  }
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
WGALFormatLookupEntry<NativeFormatType, InvalidFormat>::WGALFormatLookupEntry()
  : m_eStorage(InvalidFormat)
  , m_eRenderTarget(InvalidFormat)
  , m_eDepthOnlyType(InvalidFormat)
  , m_eStencilOnlyType(InvalidFormat)
  , m_eDepthStencilType(InvalidFormat)
  , m_eVertexAttributeType(InvalidFormat)
  , m_eResourceViewType(InvalidFormat)
{
}


template <typename NativeFormatType, NativeFormatType InvalidFormat>
WGALFormatLookupEntry<NativeFormatType, InvalidFormat>::WGALFormatLookupEntry(NativeFormatType storage)
  : m_eStorage(storage)
  , m_eRenderTarget(InvalidFormat)
  , m_eDepthOnlyType(InvalidFormat)
  , m_eStencilOnlyType(InvalidFormat)
  , m_eDepthStencilType(InvalidFormat)
  , m_eVertexAttributeType(InvalidFormat)
  , m_eResourceViewType(InvalidFormat)
{
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& WGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RT(
  NativeFormatType renderTargetType)
{
  m_eRenderTarget = renderTargetType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& WGALFormatLookupEntry<NativeFormatType, InvalidFormat>::D(NativeFormatType depthOnlyType)
{
  m_eDepthOnlyType = depthOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& WGALFormatLookupEntry<NativeFormatType, InvalidFormat>::S(NativeFormatType stencilOnlyType)
{
  m_eStencilOnlyType = stencilOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& WGALFormatLookupEntry<NativeFormatType, InvalidFormat>::DS(
  NativeFormatType depthStencilType)
{
  m_eDepthStencilType = depthStencilType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& WGALFormatLookupEntry<NativeFormatType, InvalidFormat>::VA(
  NativeFormatType vertexAttributeType)
{
  m_eVertexAttributeType = vertexAttributeType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
WGALFormatLookupEntry<NativeFormatType, InvalidFormat>& WGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RV(
  NativeFormatType resourceViewType)
{
  m_eResourceViewType = resourceViewType;
  return *this;
}


template <typename FormatClass>
WGALFormatLookupTable<FormatClass>::WGALFormatLookupTable()
{
  for (WUInt32 i = 0; i < WGALResourceFormat::ENUM_COUNT; i++)
  {
    m_Formats[i] = FormatClass();
  }
}

template <typename FormatClass>
const FormatClass& WGALFormatLookupTable<FormatClass>::GetFormatInfo(WGALResourceFormat::Enum format) const
{
  return m_Formats[format];
}

template <typename FormatClass>
void WGALFormatLookupTable<FormatClass>::SetFormatInfo(WGALResourceFormat::Enum format, const FormatClass& newFormatInfo)
{
  m_Formats[format] = newFormatInfo;
}
