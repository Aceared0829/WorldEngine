#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>
#include <Texture/Utils/TextureAtlasDesc.h>
#include <Texture/Utils/TexturePacker.h>

WResult WTexConvProcessor::GenerateTextureAtlas(WMemoryStreamWriter& stream)
{
  if (m_Descriptor.m_OutputType != WTexConvOutputType::Atlas)
    return W_SUCCESS;


  if (m_Descriptor.m_sTextureAtlasDescFile.IsEmpty())
  {
    WLog::Error("Texture atlas description file is not specified.");
    return W_FAILURE;
  }

  WTextureAtlasCreationDesc atlasDesc;
  WDynamicArray<TextureAtlasItem> atlasItems;

  if (atlasDesc.Load(m_Descriptor.m_sTextureAtlasDescFile).Failed())
  {
    WLog::Error("Failed to load texture atlas description '{0}'", WArgSensitive(m_Descriptor.m_sTextureAtlasDescFile, "File"));
    return W_FAILURE;
  }

  m_Descriptor.m_uiMinResolution = WMath::Max(32u, m_Descriptor.m_uiMinResolution);

  W_SUCCEED_OR_RETURN(LoadAtlasInputs(atlasDesc, atlasItems));

  const WUInt8 uiVersion = 4;
  stream << uiVersion;

  WDdsFileFormat ddsWriter;
  WImage atlasImg;

  for (WUInt32 layerIdx = 0; layerIdx < atlasDesc.m_Layers.GetCount(); ++layerIdx)
  {
    W_SUCCEED_OR_RETURN(CreateAtlasLayerTexture(atlasDesc, atlasItems, layerIdx, atlasImg));

    if (ddsWriter.WriteImage(stream, atlasImg, "dds").Failed())
    {
      WLog::Error("Failed to write DDS image to texture atlas file.");
      return W_FAILURE;
    }

    // debug: write out atlas slices as pure DDS
    if (false)
    {
      WStringBuilder sOut;
      sOut.SetFormat("D:/atlas_{}.dds", layerIdx);

      WFileWriter fOut;
      if (fOut.Open(sOut).Succeeded())
      {
        W_SUCCEED_OR_RETURN(ddsWriter.WriteImage(fOut, atlasImg, "dds"));
      }
    }
  }

  W_SUCCEED_OR_RETURN(WriteTextureAtlasInfo(atlasItems, atlasDesc.m_Layers.GetCount(), stream));

  return W_SUCCESS;
}

WResult WTexConvProcessor::LoadAtlasInputs(const WTextureAtlasCreationDesc& atlasDesc, WDynamicArray<TextureAtlasItem>& items) const
{
  items.Clear();

  for (const auto& srcItem : atlasDesc.m_Items)
  {
    auto& item = items.ExpandAndGetRef();
    item.m_uiUniqueID = srcItem.m_uiUniqueID;
    item.m_uiFlags = srcItem.m_uiFlags;
    item.m_uiNumVariationsX = WMath::Max<WUInt8>(1, srcItem.m_uiNumVariationsX);
    item.m_uiNumVariationsY = WMath::Max<WUInt8>(1, srcItem.m_uiNumVariationsY);

    for (WUInt32 layer = 0; layer < atlasDesc.m_Layers.GetCount(); ++layer)
    {
      if (!srcItem.m_sLayerInput[layer].IsEmpty())
      {
        if (item.m_InputImage[layer].LoadFrom(srcItem.m_sLayerInput[layer]).Failed())
        {
          WLog::Error("Failed to load texture atlas texture '{0}'", WArgSensitive(srcItem.m_sLayerInput[layer], "File"));
          return W_FAILURE;
        }

        if (atlasDesc.m_Layers[layer].m_Usage == WTexConvUsage::Color)
        {
          // enforce sRGB format for all color textures
          item.m_InputImage[layer].ReinterpretAs(WImageFormat::AsSrgb(item.m_InputImage[layer].GetImageFormat()));
        }

        WUInt32 uiResX = 0, uiResY = 0;
        W_SUCCEED_OR_RETURN(DetermineTargetResolution(item.m_InputImage[layer], WImageFormat::UNKNOWN, uiResX, uiResY));

        W_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY, atlasDesc.m_Layers[layer].m_Usage));
      }
    }


    if (!srcItem.m_sAlphaInput.IsEmpty())
    {
      WImage alphaImg;

      if (alphaImg.LoadFrom(srcItem.m_sAlphaInput).Failed())
      {
        WLog::Error("Failed to load texture atlas alpha mask '{0}'", srcItem.m_sAlphaInput);
        return W_FAILURE;
      }

      WUInt32 uiResX = 0, uiResY = 0;
      W_SUCCEED_OR_RETURN(DetermineTargetResolution(alphaImg, WImageFormat::UNKNOWN, uiResX, uiResY));

      W_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sAlphaInput, alphaImg, uiResX, uiResY, WTexConvUsage::Linear));


      if (srcItem.m_sLayerInput[0].IsEmpty())
      {
        // no base color was given, generate an opaque white one, so that the alpha mask alone defines the decal
        WImageHeader header;
        header.SetWidth(uiResX);
        header.SetHeight(uiResY);
        header.SetImageFormat(WImageFormat::R32G32B32A32_FLOAT);

        item.m_InputImage[0].ResetAndAlloc(header);

        for (WColor& pixel : item.m_InputImage[0].GetBlobPtr<WColor>())
        {
          pixel = WColor::White;
        }
      }
      else
      {
        // layer 0 must have the exact same size as the alpha texture
        W_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[0], item.m_InputImage[0], uiResX, uiResY, WTexConvUsage::Linear));
      }

      // copy alpha channel into layer 0
      W_SUCCEED_OR_RETURN(WImageUtils::CopyChannel(item.m_InputImage[0], 3, alphaImg, 0));

      // rescale all layers to be no larger than the alpha mask texture
      for (WUInt32 layer = 1; layer < atlasDesc.m_Layers.GetCount(); ++layer)
      {
        if (item.m_InputImage[layer].GetWidth() <= uiResX && item.m_InputImage[layer].GetHeight() <= uiResY)
          continue;

        W_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY, WTexConvUsage::Linear));
      }
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::WriteTextureAtlasInfo(const WDynamicArray<TextureAtlasItem>& atlasItems, WUInt32 uiNumLayers, WStreamWriter& stream)
{
  WTextureAtlasRuntimeDesc runtimeAtlas;
  runtimeAtlas.m_uiNumLayers = uiNumLayers;

  runtimeAtlas.m_Items.Reserve(atlasItems.GetCount());

  for (const auto& item : atlasItems)
  {
    auto& e = runtimeAtlas.m_Items[item.m_uiUniqueID];
    e.m_uiFlags = item.m_uiFlags;
    e.m_uiNumVariationsX = item.m_uiNumVariationsX;
    e.m_uiNumVariationsY = item.m_uiNumVariationsY;

    for (WUInt32 l = 0; l < uiNumLayers; ++l)
    {
      e.m_LayerRects[l] = item.m_AtlasRect[l];
    }
  }

  return runtimeAtlas.Serialize(stream);
}

constexpr WUInt32 uiAtlasCellSize = 32;

WResult WTexConvProcessor::TrySortItemsIntoAtlas(WDynamicArray<TextureAtlasItem>& items, WUInt32 uiWidth, WUInt32 uiHeight, WInt32 layer)
{
  WTexturePacker packer;

  // TODO: review, currently the texture packer only works on 32 sized cells
  WUInt32 uiPixelAlign = uiAtlasCellSize;

  packer.SetTextureSize(uiWidth, uiHeight, items.GetCount() * 2);

  for (const auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      packer.AddTexture((item.m_InputImage[layer].GetWidth() + (uiPixelAlign - 1)) / uiPixelAlign, (item.m_InputImage[layer].GetHeight() + (uiPixelAlign - 1)) / uiPixelAlign);
    }
  }

  W_SUCCEED_OR_RETURN(packer.PackTextures());

  WUInt32 uiTexIdx = 0;
  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      const auto& tex = packer.GetTextures()[uiTexIdx++];

      item.m_AtlasRect[layer].x = tex.m_Position.x * uiAtlasCellSize;
      item.m_AtlasRect[layer].y = tex.m_Position.y * uiAtlasCellSize;
      item.m_AtlasRect[layer].width = tex.m_Size.x * uiAtlasCellSize;
      item.m_AtlasRect[layer].height = tex.m_Size.y * uiAtlasCellSize;
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::SortItemsIntoAtlas(WDynamicArray<TextureAtlasItem>& items, WUInt32& out_ResX, WUInt32& out_ResY, WInt32 layer)
{
  for (WUInt32 power = 8; power < 14; ++power)
  {
    const WUInt32 halfRes = 1 << (power - 1);
    const WUInt32 resolution = 1 << power;
    const WUInt32 resDivCellSize = resolution / uiAtlasCellSize;
    const WUInt32 halfResDivCellSize = halfRes / uiAtlasCellSize;

    if (TrySortItemsIntoAtlas(items, resDivCellSize, halfResDivCellSize, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = halfRes;
      return W_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, halfResDivCellSize, resDivCellSize, layer).Succeeded())
    {
      out_ResX = halfRes;
      out_ResY = resolution;
      return W_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, resDivCellSize, resDivCellSize, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = resolution;
      return W_SUCCESS;
    }
  }

  WLog::Error("Could not sort items into texture atlas. Too many too large textures.");
  return W_FAILURE;
}

WResult WTexConvProcessor::CreateAtlasTexture(WDynamicArray<TextureAtlasItem>& items, WUInt32 uiResX, WUInt32 uiResY, WImage& atlas, WInt32 layer)
{
  WImageHeader imgHeader;
  imgHeader.SetWidth(uiResX);
  imgHeader.SetHeight(uiResY);
  imgHeader.SetImageFormat(WImageFormat::R32G32B32A32_FLOAT);
  atlas.ResetAndAlloc(imgHeader);

  // make sure the target texture is filled with all black
  {
    auto pixelData = atlas.GetBlobPtr<WUInt8>();
    WMemoryUtils::ZeroFill(pixelData.GetPtr(), static_cast<size_t>(pixelData.GetCount()));
  }

  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      WImage& itemImage = item.m_InputImage[layer];

      WRectU32 r;
      r.x = 0;
      r.y = 0;
      r.width = itemImage.GetWidth();
      r.height = itemImage.GetHeight();

      W_SUCCEED_OR_RETURN(WImageUtils::Copy(itemImage, r, atlas, WVec3U32(item.m_AtlasRect[layer].x, item.m_AtlasRect[layer].y, 0)));
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::FillAtlasBorders(WDynamicArray<TextureAtlasItem>& items, WImage& atlas, WInt32 layer)
{
  const WUInt32 uiBorderPixels = 2;

  const WUInt32 uiNumMipmaps = atlas.GetHeader().GetNumMipLevels();
  for (WUInt32 uiMipLevel = 0; uiMipLevel < uiNumMipmaps; ++uiMipLevel)
  {
    for (auto& item : items)
    {
      if (!item.m_InputImage[layer].IsValid())
        continue;

      WRectU32& itemRect = item.m_AtlasRect[layer];
      const WUInt32 uiRectX = itemRect.x >> uiMipLevel;
      const WUInt32 uiRectY = itemRect.y >> uiMipLevel;
      const WUInt32 uiWidth = WMath::Max(1u, itemRect.width >> uiMipLevel);
      const WUInt32 uiHeight = WMath::Max(1u, itemRect.height >> uiMipLevel);

      // fill the border of the item rect with alpha 0 to prevent bleeding into other decals in the atlas
      if (uiWidth <= 2 * uiBorderPixels || uiHeight <= 2 * uiBorderPixels)
      {
        for (WUInt32 y = 0; y < uiHeight; ++y)
        {
          for (WUInt32 x = 0; x < uiWidth; ++x)
          {
            const WUInt32 xClamped = WMath::Min(uiRectX + x, atlas.GetWidth(uiMipLevel));
            const WUInt32 yClamped = WMath::Min(uiRectY + y, atlas.GetHeight(uiMipLevel));
            atlas.GetPixelPointer<WColor>(uiMipLevel, 0, 0, xClamped, yClamped)->a = 0.0f;
          }
        }
      }
      else
      {
        for (WUInt32 i = 0; i < uiBorderPixels; ++i)
        {
          for (WUInt32 y = 0; y < uiHeight; ++y)
          {
            atlas.GetPixelPointer<WColor>(uiMipLevel, 0, 0, uiRectX + i, uiRectY + y)->a = 0.0f;
            atlas.GetPixelPointer<WColor>(uiMipLevel, 0, 0, uiRectX + uiWidth - 1 - i, uiRectY + y)->a = 0.0f;
          }

          for (WUInt32 x = 0; x < uiWidth; ++x)
          {
            atlas.GetPixelPointer<WColor>(uiMipLevel, 0, 0, uiRectX + x, uiRectY + i)->a = 0.0f;
            atlas.GetPixelPointer<WColor>(uiMipLevel, 0, 0, uiRectX + x, uiRectY + uiHeight - 1 - i)->a = 0.0f;
          }
        }
      }
    }
  }

  return W_SUCCESS;
}

WResult WTexConvProcessor::CreateAtlasLayerTexture(const WTextureAtlasCreationDesc& atlasDesc, WDynamicArray<TextureAtlasItem>& atlasItems, WInt32 layer, WImage& dstImg)
{
  WUInt32 uiTexWidth, uiTexHeight;
  W_SUCCEED_OR_RETURN(SortItemsIntoAtlas(atlasItems, uiTexWidth, uiTexHeight, layer));

  WLog::Success("Required Resolution for Texture Atlas: {0} x {1}", uiTexWidth, uiTexHeight);

  WImage atlasImg;
  W_SUCCEED_OR_RETURN(CreateAtlasTexture(atlasItems, uiTexWidth, uiTexHeight, atlasImg, layer));

  WUInt32 uiNumMipmaps = atlasImg.GetHeader().ComputeNumberOfMipMaps();
  W_SUCCEED_OR_RETURN(GenerateMipmaps(atlasImg, uiNumMipmaps));

  if (atlasDesc.m_Layers[layer].m_uiNumChannels == 4)
  {
    W_SUCCEED_OR_RETURN(FillAtlasBorders(atlasItems, atlasImg, layer));
  }

  WEnum<WImageFormat> OutputImageFormat;

  W_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, atlasDesc.m_Layers[layer].m_Usage, atlasDesc.m_Layers[layer].m_uiNumChannels));

  W_SUCCEED_OR_RETURN(GenerateOutput(std::move(atlasImg), dstImg, OutputImageFormat));

  return W_SUCCESS;
}
