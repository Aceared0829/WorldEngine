#include <Texture/TexturePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/StaticArray.h>
#include <Texture/Image/ImageFormat.h>

namespace
{
  struct WImageFormatMetaData
  {
    WImageFormatMetaData()
    {
      WMemoryUtils::ZeroFillArray(m_uiBitsPerChannel);
      WMemoryUtils::ZeroFillArray(m_uiChannelMasks);

      m_planeData.SetCount(1);
    }

    const char* m_szName = nullptr;

    struct PlaneData
    {
      WUInt8 m_uiBitsPerBlock{0}; ///< Bits per block for compressed formats; for uncompressed formats (which always have a block size of 1x1x1), this is equal to bits per pixel.
      WUInt8 m_uiBlockWidth{1};
      WUInt8 m_uiBlockHeight{1};
      WUInt8 m_uiBlockDepth{1};
      WImageFormat::Enum m_subFormat{WImageFormat::UNKNOWN}; ///< Subformats when viewing only a subslice of the data.
    };

    WStaticArray<PlaneData, 2> m_planeData;

    WUInt8 m_uiNumChannels{0};

    WUInt8 m_uiBitsPerChannel[WImageFormatChannel::COUNT];
    WUInt32 m_uiChannelMasks[WImageFormatChannel::COUNT];


    bool m_requireFirstLevelBlockAligned{false}; ///< Only for compressed formats: If true, the first level's dimensions must be a multiple of the
                                                 ///< block size; if false, padding can be applied for compressing the first mip level, too.
    bool m_isDepth{false};
    bool m_isStencil{false};

    WImageFormatDataType::Enum m_dataType{WImageFormatDataType::NONE};
    WImageFormatType::Enum m_formatType{WImageFormatType::UNKNOWN};

    WImageFormat::Enum m_asLinear{WImageFormat::UNKNOWN};
    WImageFormat::Enum m_asSrgb{WImageFormat::UNKNOWN};

    WUInt32 getNumBlocksX(WUInt32 uiWidth, WUInt32 uiPlaneIndex) const
    {
      return (uiWidth - 1) / m_planeData[uiPlaneIndex].m_uiBlockWidth + 1;
    }

    WUInt32 getNumBlocksY(WUInt32 uiHeight, WUInt32 uiPlaneIndex) const
    {
      return (uiHeight - 1) / m_planeData[uiPlaneIndex].m_uiBlockHeight + 1;
    }

    WUInt32 getNumBlocksZ(WUInt32 uiDepth, WUInt32 uiPlaneIndex) const
    {
      return (uiDepth - 1) / m_planeData[uiPlaneIndex].m_uiBlockDepth + 1;
    }

    WUInt32 getRowPitch(WUInt32 uiWidth, WUInt32 uiPlaneIndex) const
    {
      return getNumBlocksX(uiWidth, uiPlaneIndex) * m_planeData[uiPlaneIndex].m_uiBitsPerBlock / 8;
    }
  };

  WStaticArray<WImageFormatMetaData, WImageFormat::NUM_FORMATS> s_formatMetaData;

  void InitFormatLinear(WImageFormat::Enum format, const char* szName, WImageFormatDataType::Enum dataType, WUInt8 uiBitsPerPixel, WUInt8 uiBitsR,
    WUInt8 uiBitsG, WUInt8 uiBitsB, WUInt8 uiBitsA, WUInt8 uiNumChannels)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerPixel;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = WImageFormatType::LINEAR;

    s_formatMetaData[format].m_uiNumChannels = uiNumChannels;

    s_formatMetaData[format].m_uiBitsPerChannel[WImageFormatChannel::R] = uiBitsR;
    s_formatMetaData[format].m_uiBitsPerChannel[WImageFormatChannel::G] = uiBitsG;
    s_formatMetaData[format].m_uiBitsPerChannel[WImageFormatChannel::B] = uiBitsB;
    s_formatMetaData[format].m_uiBitsPerChannel[WImageFormatChannel::A] = uiBitsA;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_LINEAR(format, dataType, uiBitsPerPixel, uiBitsR, uiBitsG, uiBitsB, uiBitsA, uiNumChannels) \
  InitFormatLinear(WImageFormat::format, #format, WImageFormatDataType::dataType, uiBitsPerPixel, uiBitsR, uiBitsG, uiBitsB, uiBitsA, uiNumChannels)

  void InitFormatCompressed(WImageFormat::Enum format, const char* szName, WImageFormatDataType::Enum dataType, WUInt8 uiBitsPerBlock,
    WUInt8 uiBlockWidth, WUInt8 uiBlockHeight, WUInt8 uiBlockDepth, bool bRequireFirstLevelBlockAligned, WUInt8 uiNumChannels)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerBlock;
    s_formatMetaData[format].m_planeData[0].m_uiBlockWidth = uiBlockWidth;
    s_formatMetaData[format].m_planeData[0].m_uiBlockHeight = uiBlockHeight;
    s_formatMetaData[format].m_planeData[0].m_uiBlockDepth = uiBlockDepth;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = WImageFormatType::BLOCK_COMPRESSED;

    s_formatMetaData[format].m_uiNumChannels = uiNumChannels;

    s_formatMetaData[format].m_requireFirstLevelBlockAligned = bRequireFirstLevelBlockAligned;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_COMPRESSED(                                                                                                                    \
  format, dataType, uiBitsPerBlock, uiBlockWidth, uiBlockHeight, uiBlockDepth, requireFirstLevelBlockAligned, uiNumChannels)                       \
  InitFormatCompressed(WImageFormat::format, #format, WImageFormatDataType::dataType, uiBitsPerBlock, uiBlockWidth, uiBlockHeight, uiBlockDepth, \
    requireFirstLevelBlockAligned, uiNumChannels)

  void InitFormatDepth(WImageFormat::Enum format, const char* szName, WImageFormatDataType::Enum dataType, WUInt8 uiBitsPerPixel, bool bIsStencil,
    WUInt8 uiBitsD, WUInt8 uiBitsS)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerPixel;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = WImageFormatType::LINEAR;

    s_formatMetaData[format].m_isDepth = true;
    s_formatMetaData[format].m_isStencil = bIsStencil;

    s_formatMetaData[format].m_uiNumChannels = bIsStencil ? 2 : 1;

    s_formatMetaData[format].m_uiBitsPerChannel[WImageFormatChannel::D] = uiBitsD;
    s_formatMetaData[format].m_uiBitsPerChannel[WImageFormatChannel::S] = uiBitsS;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_DEPTH(format, dataType, uiBitsPerPixel, isStencil, uiBitsD, uiBitsS) \
  InitFormatDepth(WImageFormat::format, #format, WImageFormatDataType::dataType, uiBitsPerPixel, isStencil, uiBitsD, uiBitsS);

  void SetupSrgbPair(WImageFormat::Enum linearFormat, WImageFormat::Enum srgbFormat)
  {
    s_formatMetaData[linearFormat].m_asLinear = linearFormat;
    s_formatMetaData[linearFormat].m_asSrgb = srgbFormat;

    s_formatMetaData[srgbFormat].m_asLinear = linearFormat;
    s_formatMetaData[srgbFormat].m_asSrgb = srgbFormat;
  }

} // namespace

static void SetupImageFormatTable()
{
  if (!s_formatMetaData.IsEmpty())
    return;

  s_formatMetaData.SetCount(WImageFormat::NUM_FORMATS);

  s_formatMetaData[WImageFormat::UNKNOWN].m_szName = "UNKNOWN";

  INIT_FORMAT_LINEAR(R32G32B32A32_FLOAT, FLOAT, 128, 32, 32, 32, 32, 4);
  INIT_FORMAT_LINEAR(R32G32B32A32_UINT, UINT, 128, 32, 32, 32, 32, 4);
  INIT_FORMAT_LINEAR(R32G32B32A32_SINT, SINT, 128, 32, 32, 32, 32, 4);

  INIT_FORMAT_LINEAR(R32G32B32_FLOAT, FLOAT, 96, 32, 32, 32, 0, 3);
  INIT_FORMAT_LINEAR(R32G32B32_UINT, UINT, 96, 32, 32, 32, 0, 3);
  INIT_FORMAT_LINEAR(R32G32B32_SINT, SINT, 96, 32, 32, 32, 0, 3);

  INIT_FORMAT_LINEAR(R32G32_FLOAT, FLOAT, 64, 32, 32, 0, 0, 2);
  INIT_FORMAT_LINEAR(R32G32_UINT, UINT, 64, 32, 32, 0, 0, 2);
  INIT_FORMAT_LINEAR(R32G32_SINT, SINT, 64, 32, 32, 0, 0, 2);

  INIT_FORMAT_LINEAR(R32_FLOAT, FLOAT, 32, 32, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R32_UINT, UINT, 32, 32, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R32_SINT, SINT, 32, 32, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R16G16B16A16_FLOAT, FLOAT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_UINT, UINT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_SINT, SINT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_UNORM, UNORM, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_SNORM, SNORM, 64, 16, 16, 16, 16, 4);

  INIT_FORMAT_LINEAR(R16G16B16_UNORM, UNORM, 48, 16, 16, 16, 0, 3);

  INIT_FORMAT_LINEAR(R16G16_FLOAT, FLOAT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_UINT, UINT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_SINT, SINT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_UNORM, UNORM, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_SNORM, SNORM, 32, 16, 16, 0, 0, 2);

  INIT_FORMAT_LINEAR(R16_FLOAT, FLOAT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_UINT, UINT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_SINT, SINT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_UNORM, UNORM, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_SNORM, SNORM, 16, 16, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R8G8B8A8_UINT, UINT, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_SINT, SINT, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_SNORM, SNORM, 32, 8, 8, 8, 8, 4);

  INIT_FORMAT_LINEAR(R8G8B8A8_UNORM, UNORM, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 8, 4);
  SetupSrgbPair(WImageFormat::R8G8B8A8_UNORM, WImageFormat::R8G8B8A8_UNORM_SRGB);

  s_formatMetaData[WImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0x000000FF;
  s_formatMetaData[WImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[WImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x00FF0000;
  s_formatMetaData[WImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0xFF000000;

  INIT_FORMAT_LINEAR(R8G8B8_UNORM, UNORM, 24, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(R8G8B8_UNORM_SRGB, UNORM, 24, 8, 8, 8, 0, 3);
  SetupSrgbPair(WImageFormat::R8G8B8_UNORM, WImageFormat::R8G8B8_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B8G8R8A8_UNORM, UNORM, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(B8G8R8A8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 8, 4);
  SetupSrgbPair(WImageFormat::B8G8R8A8_UNORM, WImageFormat::B8G8R8A8_UNORM_SRGB);

  s_formatMetaData[WImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[WImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[WImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[WImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0xFF000000;

  INIT_FORMAT_LINEAR(B8G8R8X8_UNORM, UNORM, 32, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(B8G8R8X8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 0, 3);
  SetupSrgbPair(WImageFormat::B8G8R8X8_UNORM, WImageFormat::B8G8R8X8_UNORM_SRGB);

  s_formatMetaData[WImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[WImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[WImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[WImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0x00000000;

  INIT_FORMAT_LINEAR(B8G8R8_UNORM, UNORM, 24, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(B8G8R8_UNORM_SRGB, UNORM, 24, 8, 8, 8, 0, 3);
  SetupSrgbPair(WImageFormat::B8G8R8_UNORM, WImageFormat::B8G8R8_UNORM_SRGB);

  s_formatMetaData[WImageFormat::B8G8R8_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[WImageFormat::B8G8R8_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[WImageFormat::B8G8R8_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[WImageFormat::B8G8R8_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0x00000000;

  INIT_FORMAT_LINEAR(R8G8_UINT, UINT, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_SINT, SINT, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_UNORM, UNORM, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_SNORM, SNORM, 16, 8, 8, 0, 0, 2);

  INIT_FORMAT_LINEAR(R8_UINT, UINT, 8, 8, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R8_SINT, SINT, 8, 8, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R8_SNORM, SNORM, 8, 8, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R8_UNORM, UNORM, 8, 8, 0, 0, 0, 1);
  s_formatMetaData[WImageFormat::R8_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0xFF;
  s_formatMetaData[WImageFormat::R8_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x00;
  s_formatMetaData[WImageFormat::R8_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x00;
  s_formatMetaData[WImageFormat::R8_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0x00;

  INIT_FORMAT_COMPRESSED(BC1_UNORM, UNORM, 64, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC1_UNORM_SRGB, UNORM, 64, 4, 4, 1, true, 4);
  SetupSrgbPair(WImageFormat::BC1_UNORM, WImageFormat::BC1_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC2_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC2_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(WImageFormat::BC2_UNORM, WImageFormat::BC2_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC3_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC3_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(WImageFormat::BC3_UNORM, WImageFormat::BC3_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC4_UNORM, UNORM, 64, 4, 4, 1, true, 1);
  INIT_FORMAT_COMPRESSED(BC4_SNORM, SNORM, 64, 4, 4, 1, true, 1);

  INIT_FORMAT_COMPRESSED(BC5_UNORM, UNORM, 128, 4, 4, 1, true, 2);
  INIT_FORMAT_COMPRESSED(BC5_SNORM, SNORM, 128, 4, 4, 1, true, 2);

  INIT_FORMAT_COMPRESSED(BC6H_UF16, FLOAT, 128, 4, 4, 1, true, 3);
  INIT_FORMAT_COMPRESSED(BC6H_SF16, FLOAT, 128, 4, 4, 1, true, 3);

  INIT_FORMAT_COMPRESSED(BC7_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC7_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(WImageFormat::BC7_UNORM, WImageFormat::BC7_UNORM_SRGB);



  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM, UNORM, 16, 5, 5, 5, 1, 4);
  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 4);
  SetupSrgbPair(WImageFormat::B5G5R5A1_UNORM, WImageFormat::B5G5R5A1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B4G4R4A4_UNORM, UNORM, 16, 4, 4, 4, 4, 4);
  s_formatMetaData[WImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0x0F00;
  s_formatMetaData[WImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x00F0;
  s_formatMetaData[WImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x000F;
  s_formatMetaData[WImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0xF000;
  INIT_FORMAT_LINEAR(B4G4R4A4_UNORM_SRGB, UNORM, 16, 4, 4, 4, 4, 4);
  SetupSrgbPair(WImageFormat::B4G4R4A4_UNORM, WImageFormat::B4G4R4A4_UNORM_SRGB);

  INIT_FORMAT_LINEAR(A4B4G4R4_UNORM, UNORM, 16, 4, 4, 4, 4, 4);
  s_formatMetaData[WImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0xF000;
  s_formatMetaData[WImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x0F00;
  s_formatMetaData[WImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x00F0;
  s_formatMetaData[WImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0x000F;
  INIT_FORMAT_LINEAR(A4B4G4R4_UNORM_SRGB, UNORM, 16, 4, 4, 4, 4, 4);
  SetupSrgbPair(WImageFormat::A4B4G4R4_UNORM, WImageFormat::A4B4G4R4_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G6R5_UNORM, UNORM, 16, 5, 6, 5, 0, 3);
  s_formatMetaData[WImageFormat::B5G6R5_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0xF800;
  s_formatMetaData[WImageFormat::B5G6R5_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x07E0;
  s_formatMetaData[WImageFormat::B5G6R5_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x001F;
  s_formatMetaData[WImageFormat::B5G6R5_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(B5G6R5_UNORM_SRGB, UNORM, 16, 5, 6, 5, 0, 3);
  SetupSrgbPair(WImageFormat::B5G6R5_UNORM, WImageFormat::B5G6R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[WImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[WImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[WImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x001F;
  s_formatMetaData[WImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0x8000;
  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(WImageFormat::B5G5R5A1_UNORM, WImageFormat::B5G5R5A1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G5R5X1_UNORM, UNORM, 16, 5, 5, 5, 0, 3);
  s_formatMetaData[WImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[WImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[WImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x001F;
  s_formatMetaData[WImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(B5G5R5X1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 0, 3);
  SetupSrgbPair(WImageFormat::B5G5R5X1_UNORM, WImageFormat::B5G5R5X1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(A1B5G5R5_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[WImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[WImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[WImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x001F;
  s_formatMetaData[WImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0x8000;
  INIT_FORMAT_LINEAR(A1B5G5R5_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(WImageFormat::A1B5G5R5_UNORM, WImageFormat::A1B5G5R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(X1B5G5R5_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[WImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[WImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[WImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x001F;
  s_formatMetaData[WImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(X1B5G5R5_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(WImageFormat::X1B5G5R5_UNORM, WImageFormat::X1B5G5R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(R11G11B10_FLOAT, FLOAT, 32, 11, 11, 10, 0, 3);
  INIT_FORMAT_LINEAR(R10G10B10A2_UINT, UINT, 32, 10, 10, 10, 2, 4);
  INIT_FORMAT_LINEAR(R10G10B10A2_UNORM, UNORM, 32, 10, 10, 10, 2, 4);

  // msdn.microsoft.com/library/windows/desktop/bb943991(v=vs.85).aspx documents R10G10B10A2 as having an alpha mask of 0
  s_formatMetaData[WImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[WImageFormatChannel::R] = 0x000003FF;
  s_formatMetaData[WImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[WImageFormatChannel::G] = 0x000FFC00;
  s_formatMetaData[WImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[WImageFormatChannel::B] = 0x3FF00000;
  s_formatMetaData[WImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[WImageFormatChannel::A] = 0;

  INIT_FORMAT_DEPTH(D32_FLOAT, DEPTH_STENCIL, 32, false, 32, 0);
  INIT_FORMAT_DEPTH(D32_FLOAT_S8X24_UINT, DEPTH_STENCIL, 64, true, 32, 8);
  INIT_FORMAT_DEPTH(D24_UNORM_S8_UINT, DEPTH_STENCIL, 32, true, 24, 8);
  INIT_FORMAT_DEPTH(D16_UNORM, DEPTH_STENCIL, 16, false, 16, 0);

  INIT_FORMAT_COMPRESSED(ASTC_4x4_UNORM, UNORM, 128, 4, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x4_UNORM, UNORM, 128, 5, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x5_UNORM, UNORM, 128, 5, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x5_UNORM, UNORM, 128, 6, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x6_UNORM, UNORM, 128, 6, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x5_UNORM, UNORM, 128, 8, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x6_UNORM, UNORM, 128, 8, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x5_UNORM, UNORM, 128, 10, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x6_UNORM, UNORM, 128, 10, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x8_UNORM, UNORM, 128, 8, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x8_UNORM, UNORM, 128, 10, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x10_UNORM, UNORM, 128, 10, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x10_UNORM, UNORM, 128, 12, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x12_UNORM, UNORM, 128, 12, 12, 1, false, 4);

  INIT_FORMAT_COMPRESSED(ASTC_4x4_UNORM_SRGB, UNORM, 128, 4, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x4_UNORM_SRGB, UNORM, 128, 5, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x5_UNORM_SRGB, UNORM, 128, 5, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x5_UNORM_SRGB, UNORM, 128, 6, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x6_UNORM_SRGB, UNORM, 128, 6, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x5_UNORM_SRGB, UNORM, 128, 8, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x6_UNORM_SRGB, UNORM, 128, 8, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x5_UNORM_SRGB, UNORM, 128, 10, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x6_UNORM_SRGB, UNORM, 128, 10, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x8_UNORM_SRGB, UNORM, 128, 8, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x8_UNORM_SRGB, UNORM, 128, 10, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x10_UNORM_SRGB, UNORM, 128, 10, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x10_UNORM_SRGB, UNORM, 128, 12, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x12_UNORM_SRGB, UNORM, 128, 12, 12, 1, false, 4);

  SetupSrgbPair(WImageFormat::ASTC_4x4_UNORM, WImageFormat::ASTC_4x4_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_5x4_UNORM, WImageFormat::ASTC_5x4_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_5x5_UNORM, WImageFormat::ASTC_5x5_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_6x5_UNORM, WImageFormat::ASTC_6x5_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_6x6_UNORM, WImageFormat::ASTC_6x6_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_8x5_UNORM, WImageFormat::ASTC_8x5_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_8x6_UNORM, WImageFormat::ASTC_8x6_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_10x5_UNORM, WImageFormat::ASTC_10x5_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_10x6_UNORM, WImageFormat::ASTC_10x6_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_8x8_UNORM, WImageFormat::ASTC_8x8_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_10x8_UNORM, WImageFormat::ASTC_10x8_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_10x10_UNORM, WImageFormat::ASTC_10x10_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_12x10_UNORM, WImageFormat::ASTC_12x10_UNORM_SRGB);
  SetupSrgbPair(WImageFormat::ASTC_12x12_UNORM, WImageFormat::ASTC_12x12_UNORM_SRGB);

  {
    auto& meta = s_formatMetaData[WImageFormat::NV12];

    meta.m_szName = "NV12";
    meta.m_formatType = WImageFormatType::PLANAR;
    meta.m_uiNumChannels = 3;

    meta.m_planeData.SetCount(2);

    meta.m_planeData[0].m_uiBitsPerBlock = 8;
    meta.m_planeData[0].m_uiBlockWidth = 1;
    meta.m_planeData[0].m_uiBlockHeight = 1;
    meta.m_planeData[0].m_uiBlockDepth = 1;
    meta.m_planeData[0].m_subFormat = WImageFormat::R8_UNORM;

    meta.m_planeData[1].m_uiBitsPerBlock = 16;
    meta.m_planeData[1].m_uiBlockWidth = 2;
    meta.m_planeData[1].m_uiBlockHeight = 2;
    meta.m_planeData[1].m_uiBlockDepth = 1;
    meta.m_planeData[1].m_subFormat = WImageFormat::R8G8_UNORM;
  }

  {
    auto& meta = s_formatMetaData[WImageFormat::P010];

    meta.m_szName = "P010";
    meta.m_formatType = WImageFormatType::PLANAR;
    meta.m_uiNumChannels = 3;

    meta.m_planeData.SetCount(2);

    meta.m_planeData[0].m_uiBitsPerBlock = 10;
    meta.m_planeData[0].m_uiBlockWidth = 1;
    meta.m_planeData[0].m_uiBlockHeight = 1;
    meta.m_planeData[0].m_uiBlockDepth = 1;
    meta.m_planeData[0].m_subFormat = WImageFormat::R16_UNORM;

    meta.m_planeData[1].m_uiBitsPerBlock = 20;
    meta.m_planeData[1].m_uiBlockWidth = 2;
    meta.m_planeData[1].m_uiBlockHeight = 2;
    meta.m_planeData[1].m_uiBlockDepth = 1;
    meta.m_planeData[1].m_subFormat = WImageFormat::R16G16_UNORM;
  }
}

static const W_ALWAYS_INLINE WImageFormatMetaData& GetImageFormatMetaData(WImageFormat::Enum format)
{
  if (s_formatMetaData.IsEmpty())
  {
    SetupImageFormatTable();
  }

  return s_formatMetaData[format];
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Image, ImageFormats)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    SetupImageFormatTable();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WUInt32 WImageFormat::GetBitsPerPixel(Enum format, WUInt32 uiPlaneIndex)
{
  const WImageFormatMetaData& metaData = GetImageFormatMetaData(format);
  auto pixelsPerBlock = metaData.m_planeData[uiPlaneIndex].m_uiBlockWidth * metaData.m_planeData[uiPlaneIndex].m_uiBlockHeight * metaData.m_planeData[uiPlaneIndex].m_uiBlockDepth;
  return (metaData.m_planeData[uiPlaneIndex].m_uiBitsPerBlock + pixelsPerBlock - 1) / pixelsPerBlock; // Return rounded-up value
}


float WImageFormat::GetExactBitsPerPixel(Enum format, WUInt32 uiPlaneIndex)
{
  const WImageFormatMetaData& metaData = GetImageFormatMetaData(format);
  auto pixelsPerBlock = metaData.m_planeData[uiPlaneIndex].m_uiBlockWidth * metaData.m_planeData[uiPlaneIndex].m_uiBlockHeight * metaData.m_planeData[uiPlaneIndex].m_uiBlockDepth;
  return static_cast<float>(metaData.m_planeData[uiPlaneIndex].m_uiBitsPerBlock) / pixelsPerBlock;
}


WUInt32 WImageFormat::GetBitsPerBlock(Enum format, WUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBitsPerBlock;
}


WUInt32 WImageFormat::GetNumChannels(Enum format)
{
  return GetImageFormatMetaData(format).m_uiNumChannels;
}

WImageFormat::Enum WImageFormat::FromPixelMask(
  WUInt32 uiRedMask, WUInt32 uiGreenMask, WUInt32 uiBlueMask, WUInt32 uiAlphaMask, WUInt32 uiBitsPerPixel)
{
  // Some DDS files in the wild are encoded as this
  if (uiBitsPerPixel == 8 && uiRedMask == 0xff && uiGreenMask == 0xff && uiBlueMask == 0xff)
  {
    return R8_UNORM;
  }

  for (WUInt32 index = 0; index < NUM_FORMATS; index++)
  {
    Enum format = static_cast<Enum>(index);
    if (GetChannelMask(format, WImageFormatChannel::R) == uiRedMask && GetChannelMask(format, WImageFormatChannel::G) == uiGreenMask &&
        GetChannelMask(format, WImageFormatChannel::B) == uiBlueMask && GetChannelMask(format, WImageFormatChannel::A) == uiAlphaMask &&
        GetBitsPerPixel(format) == uiBitsPerPixel && GetDataType(format) == WImageFormatDataType::UNORM && !IsCompressed(format))
    {
      return format;
    }
  }

  return UNKNOWN;
}


WImageFormat::Enum WImageFormat::GetPlaneSubFormat(Enum format, WUInt32 uiPlaneIndex)
{
  const auto& metadata = GetImageFormatMetaData(format);

  if (metadata.m_formatType == WImageFormatType::PLANAR)
  {
    return metadata.m_planeData[uiPlaneIndex].m_subFormat;
  }
  else
  {
    W_ASSERT_DEV(uiPlaneIndex == 0, "Invalid plane index {0} for format {0}", uiPlaneIndex, WImageFormat::GetName(format));
    return format;
  }
}

bool WImageFormat::IsCompatible(Enum left, Enum right)
{
  if (left == right)
  {
    return true;
  }
  switch (left)
  {
    case WImageFormat::R32G32B32A32_FLOAT:
    case WImageFormat::R32G32B32A32_UINT:
    case WImageFormat::R32G32B32A32_SINT:
      return (right == WImageFormat::R32G32B32A32_FLOAT || right == WImageFormat::R32G32B32A32_UINT || right == WImageFormat::R32G32B32A32_SINT);
    case WImageFormat::R32G32B32_FLOAT:
    case WImageFormat::R32G32B32_UINT:
    case WImageFormat::R32G32B32_SINT:
      return (right == WImageFormat::R32G32B32_FLOAT || right == WImageFormat::R32G32B32_UINT || right == WImageFormat::R32G32B32_SINT);
    case WImageFormat::R32G32_FLOAT:
    case WImageFormat::R32G32_UINT:
    case WImageFormat::R32G32_SINT:
      return (right == WImageFormat::R32G32_FLOAT || right == WImageFormat::R32G32_UINT || right == WImageFormat::R32G32_SINT);
    case WImageFormat::R32_FLOAT:
    case WImageFormat::R32_UINT:
    case WImageFormat::R32_SINT:
      return (right == WImageFormat::R32_FLOAT || right == WImageFormat::R32_UINT || right == WImageFormat::R32_SINT);
    case WImageFormat::R16G16B16A16_FLOAT:
    case WImageFormat::R16G16B16A16_UINT:
    case WImageFormat::R16G16B16A16_SINT:
    case WImageFormat::R16G16B16A16_UNORM:
    case WImageFormat::R16G16B16A16_SNORM:
      return (right == WImageFormat::R16G16B16A16_FLOAT || right == WImageFormat::R16G16B16A16_UINT || right == WImageFormat::R16G16B16A16_SINT ||
              right == WImageFormat::R16G16B16A16_UNORM || right == WImageFormat::R16G16B16A16_SNORM);
    case WImageFormat::R16G16_FLOAT:
    case WImageFormat::R16G16_UINT:
    case WImageFormat::R16G16_SINT:
    case WImageFormat::R16G16_UNORM:
    case WImageFormat::R16G16_SNORM:
      return (right == WImageFormat::R16G16_FLOAT || right == WImageFormat::R16G16_UINT || right == WImageFormat::R16G16_SINT ||
              right == WImageFormat::R16G16_UNORM || right == WImageFormat::R16G16_SNORM);
    case WImageFormat::R8G8B8A8_UINT:
    case WImageFormat::R8G8B8A8_SINT:
    case WImageFormat::R8G8B8A8_UNORM:
    case WImageFormat::R8G8B8A8_SNORM:
    case WImageFormat::R8G8B8A8_UNORM_SRGB:
      return (right == WImageFormat::R8G8B8A8_UINT || right == WImageFormat::R8G8B8A8_SINT || right == WImageFormat::R8G8B8A8_UNORM ||
              right == WImageFormat::R8G8B8A8_SNORM || right == WImageFormat::R8G8B8A8_UNORM_SRGB);
    case WImageFormat::B8G8R8A8_UNORM:
    case WImageFormat::B8G8R8A8_UNORM_SRGB:
      return (right == WImageFormat::B8G8R8A8_UNORM || right == WImageFormat::B8G8R8A8_UNORM_SRGB);
    case WImageFormat::B8G8R8X8_UNORM:
    case WImageFormat::B8G8R8X8_UNORM_SRGB:
      return (right == WImageFormat::B8G8R8X8_UNORM || right == WImageFormat::B8G8R8X8_UNORM_SRGB);
    case WImageFormat::B8G8R8_UNORM:
    case WImageFormat::B8G8R8_UNORM_SRGB:
      return (right == WImageFormat::B8G8R8_UNORM || right == WImageFormat::B8G8R8_UNORM_SRGB);
    case WImageFormat::R8G8_UINT:
    case WImageFormat::R8G8_SINT:
    case WImageFormat::R8G8_UNORM:
    case WImageFormat::R8G8_SNORM:
      return (right == WImageFormat::R8G8_UINT || right == WImageFormat::R8G8_SINT || right == WImageFormat::R8G8_UNORM ||
              right == WImageFormat::R8G8_SNORM);
    case WImageFormat::R8_UINT:
    case WImageFormat::R8_SINT:
    case WImageFormat::R8_UNORM:
    case WImageFormat::R8_SNORM:
      return (
        right == WImageFormat::R8_UINT || right == WImageFormat::R8_SINT || right == WImageFormat::R8_UNORM || right == WImageFormat::R8_SNORM);
    case WImageFormat::BC1_UNORM:
    case WImageFormat::BC1_UNORM_SRGB:
      return (right == WImageFormat::BC1_UNORM || right == WImageFormat::BC1_UNORM_SRGB);
    case WImageFormat::BC2_UNORM:
    case WImageFormat::BC2_UNORM_SRGB:
      return (right == WImageFormat::BC2_UNORM || right == WImageFormat::BC2_UNORM_SRGB);
    case WImageFormat::BC3_UNORM:
    case WImageFormat::BC3_UNORM_SRGB:
      return (right == WImageFormat::BC3_UNORM || right == WImageFormat::BC3_UNORM_SRGB);
    case WImageFormat::BC4_UNORM:
    case WImageFormat::BC4_SNORM:
      return (right == WImageFormat::BC4_UNORM || right == WImageFormat::BC4_SNORM);
    case WImageFormat::BC5_UNORM:
    case WImageFormat::BC5_SNORM:
      return (right == WImageFormat::BC5_UNORM || right == WImageFormat::BC5_SNORM);
    case WImageFormat::BC6H_UF16:
    case WImageFormat::BC6H_SF16:
      return (right == WImageFormat::BC6H_UF16 || right == WImageFormat::BC6H_SF16);
    case WImageFormat::BC7_UNORM:
    case WImageFormat::BC7_UNORM_SRGB:
      return (right == WImageFormat::BC7_UNORM || right == WImageFormat::BC7_UNORM_SRGB);
    case WImageFormat::R10G10B10A2_UINT:
    case WImageFormat::R10G10B10A2_UNORM:
      return (right == WImageFormat::R10G10B10A2_UINT || right == WImageFormat::R10G10B10A2_UNORM);
    case WImageFormat::ASTC_4x4_UNORM:
    case WImageFormat::ASTC_4x4_UNORM_SRGB:
      return (right == WImageFormat::ASTC_4x4_UNORM || right == WImageFormat::ASTC_4x4_UNORM_SRGB);
    case WImageFormat::ASTC_5x4_UNORM:
    case WImageFormat::ASTC_5x4_UNORM_SRGB:
      return (right == WImageFormat::ASTC_5x4_UNORM || right == WImageFormat::ASTC_5x4_UNORM_SRGB);
    case WImageFormat::ASTC_5x5_UNORM:
    case WImageFormat::ASTC_5x5_UNORM_SRGB:
      return (right == WImageFormat::ASTC_5x5_UNORM || right == WImageFormat::ASTC_5x5_UNORM_SRGB);
    case WImageFormat::ASTC_6x5_UNORM:
    case WImageFormat::ASTC_6x5_UNORM_SRGB:
      return (right == WImageFormat::ASTC_6x5_UNORM || right == WImageFormat::ASTC_6x5_UNORM_SRGB);
    case WImageFormat::ASTC_6x6_UNORM:
    case WImageFormat::ASTC_6x6_UNORM_SRGB:
      return (right == WImageFormat::ASTC_6x6_UNORM || right == WImageFormat::ASTC_6x6_UNORM_SRGB);
    case WImageFormat::ASTC_8x5_UNORM:
    case WImageFormat::ASTC_8x5_UNORM_SRGB:
      return (right == WImageFormat::ASTC_8x5_UNORM || right == WImageFormat::ASTC_8x5_UNORM_SRGB);
    case WImageFormat::ASTC_8x6_UNORM:
    case WImageFormat::ASTC_8x6_UNORM_SRGB:
      return (right == WImageFormat::ASTC_8x6_UNORM || right == WImageFormat::ASTC_8x6_UNORM_SRGB);
    case WImageFormat::ASTC_10x5_UNORM:
    case WImageFormat::ASTC_10x5_UNORM_SRGB:
      return (right == WImageFormat::ASTC_10x5_UNORM || right == WImageFormat::ASTC_10x5_UNORM_SRGB);
    case WImageFormat::ASTC_10x6_UNORM:
    case WImageFormat::ASTC_10x6_UNORM_SRGB:
      return (right == WImageFormat::ASTC_10x6_UNORM || right == WImageFormat::ASTC_10x6_UNORM_SRGB);
    case WImageFormat::ASTC_8x8_UNORM:
    case WImageFormat::ASTC_8x8_UNORM_SRGB:
      return (right == WImageFormat::ASTC_8x8_UNORM || right == WImageFormat::ASTC_8x8_UNORM_SRGB);
    case WImageFormat::ASTC_10x8_UNORM:
    case WImageFormat::ASTC_10x8_UNORM_SRGB:
      return (right == WImageFormat::ASTC_10x8_UNORM || right == WImageFormat::ASTC_10x8_UNORM_SRGB);
    case WImageFormat::ASTC_10x10_UNORM:
    case WImageFormat::ASTC_10x10_UNORM_SRGB:
      return (right == WImageFormat::ASTC_10x10_UNORM || right == WImageFormat::ASTC_10x10_UNORM_SRGB);
    case WImageFormat::ASTC_12x10_UNORM:
    case WImageFormat::ASTC_12x10_UNORM_SRGB:
      return (right == WImageFormat::ASTC_12x10_UNORM || right == WImageFormat::ASTC_12x10_UNORM_SRGB);
    case WImageFormat::ASTC_12x12_UNORM:
    case WImageFormat::ASTC_12x12_UNORM_SRGB:
      return (right == WImageFormat::ASTC_12x12_UNORM || right == WImageFormat::ASTC_12x12_UNORM_SRGB);
    default:
      W_ASSERT_DEV(false, "Encountered unhandled format: {0}", WImageFormat::GetName(left));
      return false;
  }
}


bool WImageFormat::RequiresFirstLevelBlockAlignment(Enum format)
{
  return GetImageFormatMetaData(format).m_requireFirstLevelBlockAligned;
}

const char* WImageFormat::GetName(Enum format)
{
  return GetImageFormatMetaData(format).m_szName;
}

WUInt32 WImageFormat::GetPlaneCount(Enum format)
{
  return GetImageFormatMetaData(format).m_planeData.GetCount();
}

WUInt32 WImageFormat::GetChannelMask(Enum format, WImageFormatChannel::Enum c)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[c];
}

WUInt32 WImageFormat::GetBitsPerChannel(Enum format, WImageFormatChannel::Enum c)
{
  return GetImageFormatMetaData(format).m_uiBitsPerChannel[c];
}

WUInt32 WImageFormat::GetRedMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[WImageFormatChannel::R];
}

WUInt32 WImageFormat::GetGreenMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[WImageFormatChannel::G];
}

WUInt32 WImageFormat::GetBlueMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[WImageFormatChannel::B];
}

WUInt32 WImageFormat::GetAlphaMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[WImageFormatChannel::A];
}

WUInt32 WImageFormat::GetBlockWidth(Enum format, WUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockWidth;
}

WUInt32 WImageFormat::GetBlockHeight(Enum format, WUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockHeight;
}

WUInt32 WImageFormat::GetBlockDepth(Enum format, WUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockDepth;
}

WImageFormatDataType::Enum WImageFormat::GetDataType(Enum format)
{
  return GetImageFormatMetaData(format).m_dataType;
}

bool WImageFormat::IsCompressed(Enum format)
{
  return GetImageFormatMetaData(format).m_formatType == WImageFormatType::BLOCK_COMPRESSED;
}

bool WImageFormat::IsDepth(Enum format)
{
  return GetImageFormatMetaData(format).m_isDepth;
}

bool WImageFormat::IsSrgb(Enum format)
{
  return GetImageFormatMetaData(format).m_asLinear != format;
}

bool WImageFormat::IsStencil(Enum format)
{
  return GetImageFormatMetaData(format).m_isStencil;
}

WImageFormat::Enum WImageFormat::AsSrgb(Enum format)
{
  return GetImageFormatMetaData(format).m_asSrgb;
}

WImageFormat::Enum WImageFormat::AsLinear(Enum format)
{
  return GetImageFormatMetaData(format).m_asLinear;
}

WUInt32 WImageFormat::GetNumBlocksX(Enum format, WUInt32 uiWidth, WUInt32 uiPlaneIndex)
{
  return (uiWidth - 1) / GetBlockWidth(format, uiPlaneIndex) + 1;
}

WUInt32 WImageFormat::GetNumBlocksY(Enum format, WUInt32 uiHeight, WUInt32 uiPlaneIndex)
{
  return (uiHeight - 1) / GetBlockHeight(format, uiPlaneIndex) + 1;
}

WUInt32 WImageFormat::GetNumBlocksZ(Enum format, WUInt32 uiDepth, WUInt32 uiPlaneIndex)
{
  return (uiDepth - 1) / GetBlockDepth(format, uiPlaneIndex) + 1;
}

WUInt64 WImageFormat::GetRowPitch(Enum format, WUInt32 uiWidth, WUInt32 uiPlaneIndex)
{
  return static_cast<WUInt64>(GetNumBlocksX(format, uiWidth, uiPlaneIndex)) * GetBitsPerBlock(format, uiPlaneIndex) / 8;
}

WUInt64 WImageFormat::GetDepthPitch(Enum format, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiPlaneIndex)
{
  return static_cast<WUInt64>(GetNumBlocksY(format, uiHeight, uiPlaneIndex)) * static_cast<WUInt64>(GetRowPitch(format, uiWidth, uiPlaneIndex));
}

WImageFormatType::Enum WImageFormat::GetType(Enum format)
{
  return GetImageFormatMetaData(format).m_formatType;
}

W_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFormat);
