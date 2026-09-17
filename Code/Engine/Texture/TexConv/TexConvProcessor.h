#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Rect.h>
#include <Texture/TexConv/TexConvDesc.h>

struct WTextureAtlasCreationDesc;

class W_TEXTURE_DLL WTexConvProcessor
{
  W_DISALLOW_COPY_AND_ASSIGN(WTexConvProcessor);

public:
  WTexConvProcessor();

  WTexConvDesc m_Descriptor;

  WResult Process();

  WImage m_OutputImage;
  WImage m_LowResOutputImage;
  WImage m_ThumbnailOutputImage;
  WDefaultMemoryStreamStorage m_TextureAtlas;

private:
  //////////////////////////////////////////////////////////////////////////
  // Modifying the Descriptor

  WResult LoadInputImages();
  WResult ForceSRGBFormats();
  WResult ConvertAndScaleInputImages(WUInt32 uiResolutionX, WUInt32 uiResolutionY, WEnum<WTexConvUsage> usage);
  WResult ConvertToNormalMap(WImage& bumpMap) const;
  WResult ConvertToNormalMap(WArrayPtr<WImage> bumpMap) const;
  WResult ClampInputValues(WArrayPtr<WImage> images, float maxValue) const;
  WResult ClampInputValues(WImage& image, float maxValue) const;
  WResult DetectNumChannels(WArrayPtr<const WTexConvSliceChannelMapping> channelMapping, WUInt32& uiNumChannels);
  WResult InvertNormalMap(WImage& img);

  //////////////////////////////////////////////////////////////////////////
  // Reading from the descriptor

  enum class MipmapChannelMode
  {
    AllChannels,
    SingleChannel
  };

  WResult ChooseOutputFormat(WEnum<WImageFormat>& out_Format, WEnum<WTexConvUsage> usage, WUInt32 uiNumChannels) const;
  WResult DetermineTargetResolution(
    const WImage& image, WEnum<WImageFormat> OutputImageFormat, WUInt32& out_uiTargetResolutionX, WUInt32& out_uiTargetResolutionY) const;
  WResult Assemble2DTexture(const WImageHeader& refImg, WImage& dst) const;
  WResult Assemble2DArrayTexture(WImage& dst) const;
  WResult AssembleCubemap(WImage& dst) const;
  WResult Assemble3DTexture(WImage& dst) const;
  WResult AdjustHdrExposure(WImage& img) const;
  WResult PremultiplyAlpha(WImage& image) const;
  WResult DilateColor2D(WImage& img) const;
  WResult Assemble2DSlice(const WTexConvSliceChannelMapping& mapping, WUInt32 uiResolutionX, WUInt32 uiResolutionY, WColor* pPixelOut) const;
  WResult GenerateMipmaps(WImage& img, WUInt32 uiNumMips /* =0 */, MipmapChannelMode channelMode = MipmapChannelMode::AllChannels) const;

  //////////////////////////////////////////////////////////////////////////
  // Purely functional
  static WResult AdjustUsage(WStringView sFilename, const WImage& srcImg, WEnum<WTexConvUsage>& inout_Usage);
  static WResult ConvertAndScaleImage(WStringView sImageName, WImage& inout_Image, WUInt32 uiResolutionX, WUInt32 uiResolutionY, WEnum<WTexConvUsage> usage);

  //////////////////////////////////////////////////////////////////////////
  // Output Generation

  static WResult GenerateOutput(WImage&& src, WImage& dst, WEnum<WImageFormat> format);
  static WResult GenerateThumbnailOutput(const WImage& srcImg, WImage& dstImg, WUInt32 uiTargetRes);
  static WResult GenerateLowResOutput(const WImage& srcImg, WImage& dstImg, WUInt32 uiLowResMip);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  struct TextureAtlasItem
  {
    WUInt32 m_uiUniqueID = 0;
    WUInt32 m_uiFlags = 0;
    WImage m_InputImage[4];
    WRectU32 m_AtlasRect[4];
    WUInt8 m_uiNumVariationsX = 1;
    WUInt8 m_uiNumVariationsY = 1;
  };

  WResult LoadAtlasInputs(const WTextureAtlasCreationDesc& atlasDesc, WDynamicArray<TextureAtlasItem>& items) const;
  WResult CreateAtlasLayerTexture(
    const WTextureAtlasCreationDesc& atlasDesc, WDynamicArray<TextureAtlasItem>& atlasItems, WInt32 layer, WImage& dstImg);

  static WResult WriteTextureAtlasInfo(const WDynamicArray<TextureAtlasItem>& atlasItems, WUInt32 uiNumLayers, WStreamWriter& stream);
  static WResult TrySortItemsIntoAtlas(WDynamicArray<TextureAtlasItem>& items, WUInt32 uiWidth, WUInt32 uiHeight, WInt32 layer);
  static WResult SortItemsIntoAtlas(WDynamicArray<TextureAtlasItem>& items, WUInt32& out_ResX, WUInt32& out_ResY, WInt32 layer);
  static WResult CreateAtlasTexture(WDynamicArray<TextureAtlasItem>& items, WUInt32 uiResX, WUInt32 uiResY, WImage& atlas, WInt32 layer);
  static WResult FillAtlasBorders(WDynamicArray<TextureAtlasItem>& items, WImage& atlas, WInt32 layer);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  WResult GenerateTextureAtlas(WMemoryStreamWriter& stream);
};
