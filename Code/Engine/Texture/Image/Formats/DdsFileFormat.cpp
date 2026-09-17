#include <Texture/TexturePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/Image.h>

W_STATICLINK_FORCE static WImageFileFormatRegistrator<WDdsFileFormat> g_ddsFormat;

struct WDdsPixelFormat
{
  WUInt32 m_uiSize;
  WUInt32 m_uiFlags;
  WUInt32 m_uiFourCC;
  WUInt32 m_uiRGBBitCount;
  WUInt32 m_uiRBitMask;
  WUInt32 m_uiGBitMask;
  WUInt32 m_uiBBitMask;
  WUInt32 m_uiABitMask;
};

struct WDdsHeader
{
  WUInt32 m_uiMagic;
  WUInt32 m_uiSize;
  WUInt32 m_uiFlags;
  WUInt32 m_uiHeight;
  WUInt32 m_uiWidth;
  WUInt32 m_uiPitchOrLinearSize;
  WUInt32 m_uiDepth;
  WUInt32 m_uiMipMapCount;
  WUInt32 m_uiReserved1[11];
  WDdsPixelFormat m_ddspf;
  WUInt32 m_uiCaps;
  WUInt32 m_uiCaps2;
  WUInt32 m_uiCaps3;
  WUInt32 m_uiCaps4;
  WUInt32 m_uiReserved2;
};

struct WDdsResourceDimension
{
  enum Enum
  {
    TEXTURE1D = 2,
    TEXTURE2D = 3,
    TEXTURE3D = 4,
  };
};

struct WDdsResourceMiscFlags
{
  enum Enum
  {
    TEXTURECUBE = 0x4,
  };
};

struct WDdsHeaderDxt10
{
  WUInt32 m_uiDxgiFormat;
  WUInt32 m_uiResourceDimension;
  WUInt32 m_uiMiscFlag;
  WUInt32 m_uiArraySize;
  WUInt32 m_uiMiscFlags2;
};

struct WDdsdFlags
{
  enum Enum
  {
    CAPS = 0x000001,
    HEIGHT = 0x000002,
    WIDTH = 0x000004,
    PITCH = 0x000008,
    PIXELFORMAT = 0x001000,
    MIPMAPCOUNT = 0x020000,
    LINEARSIZE = 0x080000,
    DEPTH = 0x800000,
  };
};

struct WDdpfFlags
{
  enum Enum
  {
    ALPHAPIXELS = 0x00001,
    ALPHA = 0x00002,
    FOURCC = 0x00004,
    RGB = 0x00040,
    YUV = 0x00200,
    LUMINANCE = 0x20000,
  };
};

struct WDdsCaps
{
  enum Enum
  {
    COMPLEX = 0x000008,
    MIPMAP = 0x400000,
    TEXTURE = 0x001000,
  };
};

struct WDdsCaps2
{
  enum Enum
  {
    CUBEMAP = 0x000200,
    CUBEMAP_POSITIVEX = 0x000400,
    CUBEMAP_NEGATIVEX = 0x000800,
    CUBEMAP_POSITIVEY = 0x001000,
    CUBEMAP_NEGATIVEY = 0x002000,
    CUBEMAP_POSITIVEZ = 0x004000,
    CUBEMAP_NEGATIVEZ = 0x008000,
    VOLUME = 0x200000,
  };
};

static const WUInt32 WDdsMagic = 0x20534444;
static const WUInt32 WDdsDxt10FourCc = 0x30315844;

static WResult ReadImageData(WStreamReader& inout_stream, WImageHeader& ref_imageHeader, WDdsHeader& ref_ddsHeader)
{
  if (inout_stream.ReadBytes(&ref_ddsHeader, sizeof(WDdsHeader)) != sizeof(WDdsHeader))
  {
    WLog::Error("Failed to read file header.");
    return W_FAILURE;
  }

  if (ref_ddsHeader.m_uiMagic != WDdsMagic)
  {
    WLog::Error("The file is not a recognized DDS file.");
    return W_FAILURE;
  }

  if (ref_ddsHeader.m_uiSize != 124)
  {
    WLog::Error("The file header size {0} doesn't match the expected size of 124.", ref_ddsHeader.m_uiSize);
    return W_FAILURE;
  }

  // Required in every .dds file. According to the spec, CAPS and PIXELFORMAT are also required, but D3DX outputs
  // files not conforming to this.
  if ((ref_ddsHeader.m_uiFlags & WDdsdFlags::WIDTH) == 0 || (ref_ddsHeader.m_uiFlags & WDdsdFlags::HEIGHT) == 0)
  {
    WLog::Error("The file header doesn't specify the mandatory WIDTH or HEIGHT flag.");
    return W_FAILURE;
  }

  if ((ref_ddsHeader.m_uiCaps & WDdsCaps::TEXTURE) == 0)
  {
    WLog::Error("The file header doesn't specify the mandatory TEXTURE flag.");
    return W_FAILURE;
  }

  ref_imageHeader.SetWidth(ref_ddsHeader.m_uiWidth);
  ref_imageHeader.SetHeight(ref_ddsHeader.m_uiHeight);

  if (ref_ddsHeader.m_ddspf.m_uiSize != 32)
  {
    WLog::Error("The pixel format size {0} doesn't match the expected value of 32.", ref_ddsHeader.m_ddspf.m_uiSize);
    return W_FAILURE;
  }

  WDdsHeaderDxt10 headerDxt10{};

  WImageFormat::Enum format = WImageFormat::UNKNOWN;

  // Data format specified in RGBA masks
  if ((ref_ddsHeader.m_ddspf.m_uiFlags & WDdpfFlags::ALPHAPIXELS) != 0 || (ref_ddsHeader.m_ddspf.m_uiFlags & WDdpfFlags::RGB) != 0 ||
      (ref_ddsHeader.m_ddspf.m_uiFlags & WDdpfFlags::ALPHA) != 0)
  {
    format = WImageFormat::FromPixelMask(ref_ddsHeader.m_ddspf.m_uiRBitMask, ref_ddsHeader.m_ddspf.m_uiGBitMask, ref_ddsHeader.m_ddspf.m_uiBBitMask,
      ref_ddsHeader.m_ddspf.m_uiABitMask, ref_ddsHeader.m_ddspf.m_uiRGBBitCount);

    if (format == WImageFormat::UNKNOWN)
    {
      WLog::Error("The pixel mask specified was not recognized (R: {0}, G: {1}, B: {2}, A: {3}, Bpp: {4}).",
        WArgU(ref_ddsHeader.m_ddspf.m_uiRBitMask, 1, false, 16), WArgU(ref_ddsHeader.m_ddspf.m_uiGBitMask, 1, false, 16),
        WArgU(ref_ddsHeader.m_ddspf.m_uiBBitMask, 1, false, 16), WArgU(ref_ddsHeader.m_ddspf.m_uiABitMask, 1, false, 16),
        ref_ddsHeader.m_ddspf.m_uiRGBBitCount);
      return W_FAILURE;
    }

    // Verify that the format we found is correct
    if (WImageFormat::GetBitsPerPixel(format) != ref_ddsHeader.m_ddspf.m_uiRGBBitCount)
    {
      WLog::Error("The number of bits per pixel specified in the file ({0}) does not match the expected value of {1} for the format '{2}'.",
        ref_ddsHeader.m_ddspf.m_uiRGBBitCount, WImageFormat::GetBitsPerPixel(format), WImageFormat::GetName(format));
      return W_FAILURE;
    }
  }
  else if ((ref_ddsHeader.m_ddspf.m_uiFlags & WDdpfFlags::FOURCC) != 0)
  {
    if (ref_ddsHeader.m_ddspf.m_uiFourCC == WDdsDxt10FourCc)
    {
      if (inout_stream.ReadBytes(&headerDxt10, sizeof(WDdsHeaderDxt10)) != sizeof(WDdsHeaderDxt10))
      {
        WLog::Error("Failed to read file header.");
        return W_FAILURE;
      }

      format = WImageFormatMappings::FromDxgiFormat(headerDxt10.m_uiDxgiFormat);

      if (format == WImageFormat::UNKNOWN)
      {
        WLog::Error("The DXGI format {0} has no equivalent image format.", headerDxt10.m_uiDxgiFormat);
        return W_FAILURE;
      }
    }
    else
    {
      format = WImageFormatMappings::FromFourCc(ref_ddsHeader.m_ddspf.m_uiFourCC);

      if (format == WImageFormat::UNKNOWN)
      {
        WLog::Error("The FourCC code '{0}{1}{2}{3}' was not recognized.", WArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 0)),
          WArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 8)), WArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 16)),
          WArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 24)));
        return W_FAILURE;
      }
    }
  }
  else
  {
    WLog::Error("The image format is neither specified as a pixel mask nor as a FourCC code.");
    return W_FAILURE;
  }

  ref_imageHeader.SetImageFormat(format);

  const bool bHasMipMaps = (ref_ddsHeader.m_uiCaps & WDdsCaps::MIPMAP) != 0;
  const bool bDxt10 = (ref_ddsHeader.m_ddspf.m_uiFourCC == WDdsDxt10FourCc);
  const bool bCubeMap = (ref_ddsHeader.m_uiCaps2 & WDdsCaps2::CUBEMAP) != 0 ||
                        (bDxt10 && (headerDxt10.m_uiMiscFlag & WDdsResourceMiscFlags::TEXTURECUBE) != 0);
  const bool bVolume = (ref_ddsHeader.m_uiCaps2 & WDdsCaps2::VOLUME) != 0;


  if (bHasMipMaps)
  {
    ref_imageHeader.SetNumMipLevels(ref_ddsHeader.m_uiMipMapCount);
  }

  // Cubemap and volume texture are mutually exclusive
  if (bVolume && bCubeMap)
  {
    WLog::Error("The header specifies both the VOLUME and CUBEMAP flags.");
    return W_FAILURE;
  }

  if (bCubeMap)
  {
    ref_imageHeader.SetNumFaces(6);
  }
  else if (bVolume)
  {
    ref_imageHeader.SetDepth(ref_ddsHeader.m_uiDepth);
  }

  if (bDxt10 && headerDxt10.m_uiArraySize > 1)
  {
    ref_imageHeader.SetNumArrayIndices(headerDxt10.m_uiArraySize);
  }

  return W_SUCCESS;
}

WResult WDdsFileFormat::ReadImageHeader(WStreamReader& inout_stream, WImageHeader& ref_header, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  W_PROFILE_SCOPE("WDdsFileFormat::ReadImageHeader");

  WDdsHeader ddsHeader;
  return ReadImageData(inout_stream, ref_header, ddsHeader);
}

WResult WDdsFileFormat::ReadImage(WStreamReader& inout_stream, WImage& ref_image, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  W_PROFILE_SCOPE("WDdsFileFormat::ReadImage");

  WImageHeader imageHeader;
  WDdsHeader ddsHeader;
  W_SUCCEED_OR_RETURN(ReadImageData(inout_stream, imageHeader, ddsHeader));

  ref_image.ResetAndAlloc(imageHeader);

  const bool bPitch = (ddsHeader.m_uiFlags & WDdsdFlags::PITCH) != 0;

  // If pitch is specified, it must match the computed value
  if (bPitch && ref_image.GetRowPitch(0) != ddsHeader.m_uiPitchOrLinearSize)
  {
    WLog::Error("The row pitch specified in the header doesn't match the expected pitch.");
    return W_FAILURE;
  }

  WUInt64 uiDataSize = ref_image.GetByteBlobPtr().GetCount();

  if (inout_stream.ReadBytes(ref_image.GetByteBlobPtr().GetPtr(), uiDataSize) != uiDataSize)
  {
    WLog::Error("Failed to read image data.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WDdsFileFormat::WriteImage(WStreamWriter& inout_stream, const WImageView& image, WStringView sFileExtension) const
{
  W_IGNORE_UNUSED(sFileExtension);

  const WImageFormat::Enum format = image.GetImageFormat();
  const WUInt32 uiBpp = WImageFormat::GetBitsPerPixel(format);

  const WUInt32 uiNumFaces = image.GetNumFaces();
  const WUInt32 uiNumMipLevels = image.GetNumMipLevels();
  const WUInt32 uiNumArrayIndices = image.GetNumArrayIndices();

  const WUInt32 uiWidth = image.GetWidth(0);
  const WUInt32 uiHeight = image.GetHeight(0);
  const WUInt32 uiDepth = image.GetDepth(0);

  bool bHasMipMaps = uiNumMipLevels > 1;
  bool bVolume = uiDepth > 1;
  bool bCubeMap = uiNumFaces > 1;
  bool bArray = uiNumArrayIndices > 1;

  bool bDxt10 = false;

  WDdsHeader fileHeader;
  WDdsHeaderDxt10 headerDxt10;

  WMemoryUtils::ZeroFill(&fileHeader, 1);
  WMemoryUtils::ZeroFill(&headerDxt10, 1);

  fileHeader.m_uiMagic = WDdsMagic;
  fileHeader.m_uiSize = 124;
  fileHeader.m_uiWidth = uiWidth;
  fileHeader.m_uiHeight = uiHeight;

  // Required in every .dds file.
  fileHeader.m_uiFlags = WDdsdFlags::WIDTH | WDdsdFlags::HEIGHT | WDdsdFlags::CAPS | WDdsdFlags::PIXELFORMAT;

  if (bHasMipMaps)
  {
    fileHeader.m_uiFlags |= WDdsdFlags::MIPMAPCOUNT;
    fileHeader.m_uiMipMapCount = uiNumMipLevels;
  }

  if (bVolume)
  {
    // Volume and array are incompatible
    if (bArray)
    {
      WLog::Error("The image is both an array and volume texture. This is not supported.");
      return W_FAILURE;
    }

    fileHeader.m_uiFlags |= WDdsdFlags::DEPTH;
    fileHeader.m_uiDepth = uiDepth;
  }

  switch (WImageFormat::GetType(image.GetImageFormat()))
  {
    case WImageFormatType::LINEAR:
      [[fallthrough]];
    case WImageFormatType::PLANAR:
      fileHeader.m_uiFlags |= WDdsdFlags::PITCH;
      fileHeader.m_uiPitchOrLinearSize = static_cast<WUInt32>(image.GetRowPitch(0));
      break;

    case WImageFormatType::BLOCK_COMPRESSED:
      fileHeader.m_uiFlags |= WDdsdFlags::LINEARSIZE;
      fileHeader.m_uiPitchOrLinearSize = 0; /// \todo sub-image size
      break;

    default:
      WLog::Error("Unknown image format type.");
      return W_FAILURE;
  }

  fileHeader.m_uiCaps = WDdsCaps::TEXTURE;

  if (bCubeMap)
  {
    if (uiNumFaces != 6)
    {
      WLog::Error("The image is a cubemap, but has {0} faces instead of the expected 6.", uiNumFaces);
      return W_FAILURE;
    }

    if (bVolume)
    {
      WLog::Error("The image is both a cubemap and volume texture. This is not supported.");
      return W_FAILURE;
    }

    fileHeader.m_uiCaps |= WDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= WDdsCaps2::CUBEMAP | WDdsCaps2::CUBEMAP_POSITIVEX | WDdsCaps2::CUBEMAP_NEGATIVEX | WDdsCaps2::CUBEMAP_POSITIVEY |
                            WDdsCaps2::CUBEMAP_NEGATIVEY | WDdsCaps2::CUBEMAP_POSITIVEZ | WDdsCaps2::CUBEMAP_NEGATIVEZ;
  }

  if (bArray)
  {
    fileHeader.m_uiCaps |= WDdsCaps::COMPLEX;

    // Must be written as DXT10
    bDxt10 = true;
  }

  if (bVolume)
  {
    fileHeader.m_uiCaps |= WDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= WDdsCaps2::VOLUME;
  }

  if (bHasMipMaps)
  {
    fileHeader.m_uiCaps |= WDdsCaps::MIPMAP | WDdsCaps::COMPLEX;
  }

  fileHeader.m_ddspf.m_uiSize = 32;

  WUInt32 uiRedMask = WImageFormat::GetRedMask(format);
  WUInt32 uiGreenMask = WImageFormat::GetGreenMask(format);
  WUInt32 uiBlueMask = WImageFormat::GetBlueMask(format);
  WUInt32 uiAlphaMask = WImageFormat::GetAlphaMask(format);

  WUInt32 uiFourCc = WImageFormatMappings::ToFourCc(format);
  WUInt32 uiDxgiFormat = WImageFormatMappings::ToDxgiFormat(format);

  // When not required to use a DXT10 texture, try to write a legacy DDS by specifying FourCC or pixel masks
  if (!bDxt10)
  {
    // The format has a known mask and we would also recognize it as the same when reading back in, since multiple formats may have the same pixel
    // masks
    if ((uiRedMask | uiGreenMask | uiBlueMask | uiAlphaMask) &&
        format == WImageFormat::FromPixelMask(uiRedMask, uiGreenMask, uiBlueMask, uiAlphaMask, uiBpp))
    {
      fileHeader.m_ddspf.m_uiFlags = WDdpfFlags::ALPHAPIXELS | WDdpfFlags::RGB;
      fileHeader.m_ddspf.m_uiRBitMask = uiRedMask;
      fileHeader.m_ddspf.m_uiGBitMask = uiGreenMask;
      fileHeader.m_ddspf.m_uiBBitMask = uiBlueMask;
      fileHeader.m_ddspf.m_uiABitMask = uiAlphaMask;
      fileHeader.m_ddspf.m_uiRGBBitCount = WImageFormat::GetBitsPerPixel(format);
    }
    // The format has a known FourCC
    else if (uiFourCc != 0)
    {
      fileHeader.m_ddspf.m_uiFlags = WDdpfFlags::FOURCC;
      fileHeader.m_ddspf.m_uiFourCC = uiFourCc;
    }
    else
    {
      // Fallback to DXT10 path
      bDxt10 = true;
    }
  }

  if (bDxt10)
  {
    // We must write a DXT10 file, but there is no matching DXGI_FORMAT - we could also try converting, but that is rarely intended when writing .dds
    if (uiDxgiFormat == 0)
    {
      WLog::Error("The image needs to be written as a DXT10 file, but no matching DXGI format was found for '{0}'.", WImageFormat::GetName(format));
      return W_FAILURE;
    }

    fileHeader.m_ddspf.m_uiFlags = WDdpfFlags::FOURCC;
    fileHeader.m_ddspf.m_uiFourCC = WDdsDxt10FourCc;

    headerDxt10.m_uiDxgiFormat = uiDxgiFormat;

    if (bVolume)
    {
      headerDxt10.m_uiResourceDimension = WDdsResourceDimension::TEXTURE3D;
    }
    else if (uiHeight > 1)
    {
      headerDxt10.m_uiResourceDimension = WDdsResourceDimension::TEXTURE2D;
    }
    else
    {
      headerDxt10.m_uiResourceDimension = WDdsResourceDimension::TEXTURE1D;
    }

    if (bCubeMap)
    {
      headerDxt10.m_uiMiscFlag = WDdsResourceMiscFlags::TEXTURECUBE;
    }

    // NOT multiplied by number of cubemap faces
    headerDxt10.m_uiArraySize = uiNumArrayIndices;

    // Can be used to describe the alpha channel usage, but automatically makes it incompatible with the D3DX libraries if not 0.
    headerDxt10.m_uiMiscFlags2 = 0;
  }

  if (inout_stream.WriteBytes(&fileHeader, sizeof(fileHeader)) != W_SUCCESS)
  {
    WLog::Error("Failed to write image header.");
    return W_FAILURE;
  }

  if (bDxt10)
  {
    if (inout_stream.WriteBytes(&headerDxt10, sizeof(headerDxt10)) != W_SUCCESS)
    {
      WLog::Error("Failed to write image DX10 header.");
      return W_FAILURE;
    }
  }

  if (inout_stream.WriteBytes(image.GetByteBlobPtr().GetPtr(), image.GetByteBlobPtr().GetCount()) != W_SUCCESS)
  {
    WLog::Error("Failed to write image data.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

bool WDdsFileFormat::CanReadFileType(WStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("dds");
}

bool WDdsFileFormat::CanWriteFileType(WStringView sExtension) const
{
  return CanReadFileType(sExtension);
}



W_STATICLINK_FILE(Texture, Texture_Image_Formats_DdsFileFormat);
