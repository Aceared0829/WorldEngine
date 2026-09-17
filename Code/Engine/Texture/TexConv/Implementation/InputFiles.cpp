#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

namespace
{
  // Replaces the image with a single cell of the grid that it is subdivided into.
  //
  // Fails for block compressed formats. Mipmaps, faces and array slices other than the first one are discarded,
  // TexConv regenerates those from the cropped image.
  WResult CropToGridCell(WImage& inout_image, WStringView sImageName, WUInt32 uiGridX, WUInt32 uiGridY, WUInt32 uiCell)
  {
    const WEnum<WImageFormat> format = inout_image.GetImageFormat();

    if (WImageFormat::IsCompressed(format))
    {
      WLog::Error("Cannot extract a grid cell from '{}', the image format is block compressed.", sImageName);
      return W_FAILURE;
    }

    const WUInt32 uiCellWidth = inout_image.GetWidth() / uiGridX;
    const WUInt32 uiCellHeight = inout_image.GetHeight() / uiGridY;

    if (uiCellWidth == 0 || uiCellHeight == 0)
    {
      WLog::Error("Cannot extract a {}x{} grid from '{}', the image is only {}x{} pixels.", uiGridX, uiGridY, sImageName, inout_image.GetWidth(), inout_image.GetHeight());
      return W_FAILURE;
    }

    const WUInt32 uiCellIdx = uiCell % (uiGridX * uiGridY);
    const WUInt32 uiOffsetX = (uiCellIdx % uiGridX) * uiCellWidth;
    const WUInt32 uiOffsetY = (uiCellIdx / uiGridX) * uiCellHeight;

    WImageHeader header;
    header.SetWidth(uiCellWidth);
    header.SetHeight(uiCellHeight);
    header.SetImageFormat(format);

    WImage cropped;
    cropped.ResetAndAlloc(header);

    const WRectU32 srcRect(uiOffsetX, uiOffsetY, uiCellWidth, uiCellHeight);
    W_SUCCEED_OR_RETURN(WImageUtils::Copy(inout_image, srcRect, cropped, WVec3U32::MakeZero()));

    inout_image.ResetAndMove(std::move(cropped));
    return W_SUCCESS;
  }
} // namespace

WResult WTexConvProcessor::LoadInputImages()
{
  W_PROFILE_SCOPE("Load Images");

  if (m_Descriptor.m_InputImages.IsEmpty() && m_Descriptor.m_InputFiles.IsEmpty())
  {
    WLog::Error("No input images have been specified.");
    return W_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty() && !m_Descriptor.m_InputFiles.IsEmpty())
  {
    WLog::Error("Both input files and input images have been specified. You need to either specify files or images.");
    return W_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty())
  {
    // make sure the two arrays have the same size
    m_Descriptor.m_InputFiles.SetCount(m_Descriptor.m_InputImages.GetCount());

    WStringBuilder tmp;
    for (WUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
    {
      tmp.SetFormat("InputImage{}", WArgI(i, 2, true));
      m_Descriptor.m_InputFiles[i] = tmp;
    }
  }
  else
  {
    m_Descriptor.m_InputImages.Reserve(m_Descriptor.m_InputFiles.GetCount());

    for (const auto& file : m_Descriptor.m_InputFiles)
    {
      auto& img = m_Descriptor.m_InputImages.ExpandAndGetRef();
      if (img.LoadFrom(file).Failed())
      {
        WLog::Error("Could not load input file '{0}'.", WArgSensitive(file, "File"));
        return W_FAILURE;
      }
    }
  }

  for (WUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
  {
    const auto& img = m_Descriptor.m_InputImages[i];

    if (img.GetImageFormat() == WImageFormat::UNKNOWN)
    {
      WLog::Error("Unknown image format for '{}'", WArgSensitive(m_Descriptor.m_InputFiles[i], "File"));
      return W_FAILURE;
    }
  }

  const WUInt32 uiGridX = WMath::Max<WUInt32>(1, m_Descriptor.m_uiSourceGridX);
  const WUInt32 uiGridY = WMath::Max<WUInt32>(1, m_Descriptor.m_uiSourceGridY);

  if (uiGridX > 1 || uiGridY > 1)
  {
    for (WUInt32 i = 0; i < m_Descriptor.m_InputImages.GetCount(); ++i)
    {
      W_SUCCEED_OR_RETURN(CropToGridCell(m_Descriptor.m_InputImages[i], m_Descriptor.m_InputFiles[i], uiGridX, uiGridY, m_Descriptor.m_uiSourceGridCell));
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::ConvertAndScaleImage(WStringView sImageName, WImage& inout_Image, WUInt32 uiResolutionX, WUInt32 uiResolutionY, WEnum<WTexConvUsage> usage)
{
  const bool bSingleChannel = WImageFormat::GetNumChannels(inout_Image.GetImageFormat()) == 1;

  if (inout_Image.Convert(WImageFormat::R32G32B32A32_FLOAT).Failed())
  {
    WLog::Error("Could not convert '{}' to RGBA 32-Bit Float format.", sImageName);
    return W_FAILURE;
  }

  // some scale operations fail when they are done in place, so use a scratch image as destination for now
  WImage scratch;
  if (WImageUtils::Scale(inout_Image, scratch, uiResolutionX, uiResolutionY, nullptr, WImageAddressMode::Clamp, WImageAddressMode::Clamp).Failed())
  {
    WLog::Error("Could not resize '{}' to {}x{}", sImageName, uiResolutionX, uiResolutionY);
    return W_FAILURE;
  }

  inout_Image.ResetAndMove(std::move(scratch));

  if (usage == WTexConvUsage::Color && bSingleChannel)
  {
    // replicate single channel ("red" textures) into the other channels
    W_SUCCEED_OR_RETURN(WImageUtils::CopyChannel(inout_Image, 1, inout_Image, 0));
    W_SUCCEED_OR_RETURN(WImageUtils::CopyChannel(inout_Image, 2, inout_Image, 0));
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::ConvertAndScaleInputImages(WUInt32 uiResolutionX, WUInt32 uiResolutionY, WEnum<WTexConvUsage> usage)
{
  W_PROFILE_SCOPE("ConvertAndScaleInputImages");

  for (WUInt32 idx = 0; idx < m_Descriptor.m_InputImages.GetCount(); ++idx)
  {
    auto& img = m_Descriptor.m_InputImages[idx];
    WStringView sName = m_Descriptor.m_InputFiles[idx];

    W_SUCCEED_OR_RETURN(ConvertAndScaleImage(sName, img, uiResolutionX, uiResolutionY, usage));
  }

  return W_SUCCESS;
}
