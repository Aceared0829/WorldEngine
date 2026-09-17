#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WTexture2DChannelMappingEnum, 1)
  W_ENUM_CONSTANTS(WTexture2DChannelMappingEnum::R1, WTexture2DChannelMappingEnum::R1_ALPHA)
  W_ENUM_CONSTANTS(WTexture2DChannelMappingEnum::RG1, WTexture2DChannelMappingEnum::R1_G2)
  W_ENUM_CONSTANTS(WTexture2DChannelMappingEnum::RGB1, WTexture2DChannelMappingEnum::RGB1_ABLACK, WTexture2DChannelMappingEnum::R1_G2_B3)
  W_ENUM_CONSTANTS(WTexture2DChannelMappingEnum::RGBA1, WTexture2DChannelMappingEnum::RGB1_A2, WTexture2DChannelMappingEnum::R1_G2_B3_A4)
  W_ENUM_CONSTANTS(WTexture2DChannelMappingEnum::RGBWHITE_A1, WTexture2DChannelMappingEnum::RGBWHITE_R1)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WTextureArrayChannelMappingEnum, 1)
  W_ENUM_CONSTANTS(WTextureArrayChannelMappingEnum::RGBA, WTextureArrayChannelMappingEnum::RGB, WTextureArrayChannelMappingEnum::RG)
  W_ENUM_CONSTANTS(WTextureArrayChannelMappingEnum::R_Red, WTextureArrayChannelMappingEnum::R_Green, WTextureArrayChannelMappingEnum::R_Blue, WTextureArrayChannelMappingEnum::R_Alpha)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WTexture2DResolution, 1)
  W_ENUM_CONSTANTS(WTexture2DResolution::Fixed64x64, WTexture2DResolution::Fixed128x128, WTexture2DResolution::Fixed256x256, WTexture2DResolution::Fixed512x512, WTexture2DResolution::Fixed1024x1024, WTexture2DResolution::Fixed2048x2048)
  W_ENUM_CONSTANTS(WTexture2DResolution::CVarRtResolution1, WTexture2DResolution::CVarRtResolution2)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WRenderTargetFormat, 1)
  W_ENUM_CONSTANTS(WRenderTargetFormat::RGBA8sRgb, WRenderTargetFormat::RGBA8, WRenderTargetFormat::RGB10, WRenderTargetFormat::RGBA16)
  W_ENUM_CONSTANTS(WRenderTargetFormat::R8, WRenderTargetFormat::R16, WRenderTargetFormat::R32)
  W_ENUM_CONSTANTS(WRenderTargetFormat::RG8, WRenderTargetFormat::RG16, WRenderTargetFormat::RG32)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureAssetProperties, 6, WRTTIDefaultAllocator<WTextureAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("IsRenderTarget", m_bIsRenderTarget)->AddAttributes(new WHiddenAttribute),
    W_ENUM_MEMBER_PROPERTY("Usage", WTexConvUsage, m_TextureUsage),
    W_ENUM_MEMBER_PROPERTY("CompressionMode", WTexConvCompressionMode, m_CompressionMode),

    W_ENUM_MEMBER_PROPERTY("Format", WRenderTargetFormat, m_RtFormat),
    W_ENUM_MEMBER_PROPERTY("Resolution", WTexture2DResolution, m_Resolution),
    W_MEMBER_PROPERTY("CVarResScale", m_fCVarResolutionScale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),

    W_ENUM_MEMBER_PROPERTY("TextureFilter", WTextureFilterSetting, m_TextureFilter),
    W_ENUM_MEMBER_PROPERTY("AddressModeU", WImageAddressMode, m_AddressModeU),
    W_ENUM_MEMBER_PROPERTY("AddressModeV", WImageAddressMode, m_AddressModeV),
    W_ENUM_MEMBER_PROPERTY("AddressModeW", WImageAddressMode, m_AddressModeW),

    W_MEMBER_PROPERTY("IsArrayTexture", m_bIsArrayTexture),

    W_ENUM_MEMBER_PROPERTY("MipmapMode", WTexConvMipmapMode, m_MipmapMode),
    W_MEMBER_PROPERTY("PreserveAlphaCoverage", m_bPreserveAlphaCoverage),
    W_MEMBER_PROPERTY("AlphaThreshold", m_fAlphaThreshold)->AddAttributes(new WDefaultValueAttribute(0.25f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("PremultipliedAlpha", m_bPremultipliedAlpha),

    W_MEMBER_PROPERTY("DilateColor", m_bDilateColor)->AddAttributes(new WDefaultValueAttribute(false)),
    W_MEMBER_PROPERTY("FlipHorizontal", m_bFlipHorizontal),
    W_MEMBER_PROPERTY("HdrExposureBias", m_fHdrExposureBias)->AddAttributes(new WClampValueAttribute(-20.0f, 20.0f)),

    W_ENUM_MEMBER_PROPERTY("ChannelMapping", WTexture2DChannelMappingEnum, m_ChannelMapping),
    W_ENUM_MEMBER_PROPERTY("ArrayChannelMapping", WTextureArrayChannelMappingEnum, m_ArrayChannelMapping),

    W_ACCESSOR_PROPERTY("Input1", GetInputFile0, SetInputFile0)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr)),
    W_ACCESSOR_PROPERTY("Input2", GetInputFile1, SetInputFile1)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr)),
    W_ACCESSOR_PROPERTY("Input3", GetInputFile2, SetInputFile2)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr)),
    W_ACCESSOR_PROPERTY("Input4", GetInputFile3, SetInputFile3)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr)),

    W_ARRAY_MEMBER_PROPERTY("ArraySlices", m_ArraySlices)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr)),

  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WTextureAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WTextureAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool isRenderTarget = e.m_pObject->GetTypeAccessor().GetValue("IsRenderTarget").ConvertTo<bool>();
    const bool isTextureArray = e.m_pObject->GetTypeAccessor().GetValue("IsArrayTexture").ConvertTo<bool>();

    props["AddressModeW"].m_Visibility = WPropertyUiState::Invisible;

    if (isRenderTarget)
    {
      const WInt32 resMode = e.m_pObject->GetTypeAccessor().GetValue("Resolution").ConvertTo<WInt32>();
      const bool resIsCVar = resMode == WTexture2DResolution::CVarRtResolution1 || resMode == WTexture2DResolution::CVarRtResolution2;

      props["CVarResScale"].m_Visibility = resIsCVar ? WPropertyUiState::Default : WPropertyUiState::Disabled;
      props["Usage"].m_Visibility = WPropertyUiState::Invisible;
      props["MipmapMode"].m_Visibility = WPropertyUiState::Invisible;
      props["CompressionMode"].m_Visibility = WPropertyUiState::Invisible;
      props["PremultipliedAlpha"].m_Visibility = WPropertyUiState::Invisible;
      props["FlipHorizontal"].m_Visibility = WPropertyUiState::Invisible;
      props["ChannelMapping"].m_Visibility = WPropertyUiState::Invisible;
      props["ArrayChannelMapping"].m_Visibility = WPropertyUiState::Invisible;
      props["PreserveAlphaCoverage"].m_Visibility = WPropertyUiState::Invisible;
      props["AlphaThreshold"].m_Visibility = WPropertyUiState::Invisible;
      props["PremultipliedAlpha"].m_Visibility = WPropertyUiState::Invisible;
      props["HdrExposureBias"].m_Visibility = WPropertyUiState::Invisible;
      props["DilateColor"].m_Visibility = WPropertyUiState::Invisible;

      props["Input1"].m_Visibility = WPropertyUiState::Invisible;
      props["Input2"].m_Visibility = WPropertyUiState::Invisible;
      props["Input3"].m_Visibility = WPropertyUiState::Invisible;
      props["Input4"].m_Visibility = WPropertyUiState::Invisible;
      props["ArraySlices"].m_Visibility = WPropertyUiState::Invisible;
      props["IsArrayTexture"].m_Visibility = WPropertyUiState::Invisible;

      props["Format"].m_Visibility = WPropertyUiState::Default;
      props["Resolution"].m_Visibility = WPropertyUiState::Default;
    }
    else if (isTextureArray)
    {
      props["CVarResScale"].m_Visibility = WPropertyUiState::Invisible;
      props["Format"].m_Visibility = WPropertyUiState::Invisible;
      props["Resolution"].m_Visibility = WPropertyUiState::Invisible;

      props["ChannelMapping"].m_Visibility = WPropertyUiState::Invisible;
      props["ArrayChannelMapping"].m_Visibility = WPropertyUiState::Default;
      props["Input1"].m_Visibility = WPropertyUiState::Invisible;
      props["Input2"].m_Visibility = WPropertyUiState::Invisible;
      props["Input3"].m_Visibility = WPropertyUiState::Invisible;
      props["Input4"].m_Visibility = WPropertyUiState::Invisible;
      props["FlipHorizontal"].m_Visibility = WPropertyUiState::Invisible;
      props["DilateColor"].m_Visibility = WPropertyUiState::Invisible;
      props["PremultipliedAlpha"].m_Visibility = WPropertyUiState::Invisible;
      props["PreserveAlphaCoverage"].m_Visibility = WPropertyUiState::Invisible;
      props["AlphaThreshold"].m_Visibility = WPropertyUiState::Invisible;
      props["HdrExposureBias"].m_Visibility = WPropertyUiState::Invisible;

      props["Usage"].m_Visibility = WPropertyUiState::Default;
      props["MipmapMode"].m_Visibility = WPropertyUiState::Default;
      props["CompressionMode"].m_Visibility = WPropertyUiState::Default;
      props["TextureFilter"].m_Visibility = WPropertyUiState::Default;
      props["AddressModeU"].m_Visibility = WPropertyUiState::Default;
      props["AddressModeV"].m_Visibility = WPropertyUiState::Default;
      props["ArraySlices"].m_Visibility = WPropertyUiState::Default;
      props["IsArrayTexture"].m_Visibility = WPropertyUiState::Default;

      const WInt64 arrayMapping = e.m_pObject->GetTypeAccessor().GetValue("ArrayChannelMapping").ConvertTo<WInt64>();
      const bool hasMips = e.m_pObject->GetTypeAccessor().GetValue("MipmapMode").ConvertTo<WInt32>() != WTexConvMipmapMode::None;

      if (arrayMapping == WTextureArrayChannelMappingEnum::RGBA)
      {
        props["DilateColor"].m_Visibility = WPropertyUiState::Default;

        if (hasMips)
        {
          props["PreserveAlphaCoverage"].m_Visibility = WPropertyUiState::Default;
          props["AlphaThreshold"].m_Visibility = WPropertyUiState::Default;
        }
      }

      if (e.m_pObject->GetTypeAccessor().GetValue("Usage").ConvertTo<WInt32>() == WTexConvUsage::Hdr)
      {
        props["HdrExposureBias"].m_Visibility = WPropertyUiState::Default;
      }
    }
    else
    {
      const bool hasMips = e.m_pObject->GetTypeAccessor().GetValue("MipmapMode").ConvertTo<WInt32>() != WTexConvMipmapMode::None;
      const bool isHDR = e.m_pObject->GetTypeAccessor().GetValue("Usage").ConvertTo<WInt32>() == WTexConvUsage::Hdr;

      props["CVarResScale"].m_Visibility = WPropertyUiState::Invisible;
      props["Usage"].m_Visibility = WPropertyUiState::Default;
      props["Mipmaps"].m_Visibility = WPropertyUiState::Default;
      props["Compression"].m_Visibility = WPropertyUiState::Default;
      props["PremultipliedAlpha"].m_Visibility = WPropertyUiState::Disabled;
      props["FlipHorizontal"].m_Visibility = WPropertyUiState::Default;
      props["ChannelMapping"].m_Visibility = WPropertyUiState::Default;
      props["ArrayChannelMapping"].m_Visibility = WPropertyUiState::Invisible;
      props["Format"].m_Visibility = WPropertyUiState::Invisible;
      props["Resolution"].m_Visibility = WPropertyUiState::Invisible;
      props["PreserveAlphaCoverage"].m_Visibility = WPropertyUiState::Disabled;
      props["AlphaThreshold"].m_Visibility = WPropertyUiState::Disabled;
      props["HdrExposureBias"].m_Visibility = WPropertyUiState::Disabled;
      props["DilateColor"].m_Visibility = WPropertyUiState::Disabled;
      props["ArraySlices"].m_Visibility = WPropertyUiState::Invisible;
      props["IsArrayTexture"].m_Visibility = WPropertyUiState::Default;

      const WInt64 mapping = e.m_pObject->GetTypeAccessor().GetValue("ChannelMapping").ConvertTo<WInt64>();

      props["Usage"].m_Visibility = WPropertyUiState::Default;
      props["Input1"].m_Visibility = WPropertyUiState::Default;
      props["Input2"].m_Visibility = WPropertyUiState::Invisible;
      props["Input3"].m_Visibility = WPropertyUiState::Invisible;
      props["Input4"].m_Visibility = WPropertyUiState::Invisible;

      {
        props["Input1"].m_sNewLabelText = "TextureAsset::Input1";
        props["Input2"].m_sNewLabelText = "TextureAsset::Input2";
        props["Input3"].m_sNewLabelText = "TextureAsset::Input3";
        props["Input4"].m_sNewLabelText = "TextureAsset::Input4";
      }

      switch (mapping)
      {
        case WTexture2DChannelMappingEnum::R1_G2_B3_A4:
          props["Input4"].m_Visibility = WPropertyUiState::Default;
          // fall through

        case WTexture2DChannelMappingEnum::R1_G2_B3:
          props["Input3"].m_Visibility = WPropertyUiState::Default;
          // fall through

        case WTexture2DChannelMappingEnum::RGB1_A2:
        case WTexture2DChannelMappingEnum::R1_G2:
          props["Input2"].m_Visibility = WPropertyUiState::Default;
          break;
      }

      if (mapping == WTexture2DChannelMappingEnum::R1 || mapping == WTexture2DChannelMappingEnum::R1_ALPHA || mapping == WTexture2DChannelMappingEnum::RGBA1 ||
          mapping == WTexture2DChannelMappingEnum::R1_G2_B3_A4 || mapping == WTexture2DChannelMappingEnum::RGB1_A2 ||
          mapping == WTexture2DChannelMappingEnum::R1_G2_B3_A4)
      {
        if (mapping != WTexture2DChannelMappingEnum::R1)
        {
          props["PremultipliedAlpha"].m_Visibility = WPropertyUiState::Default;
          props["DilateColor"].m_Visibility = WPropertyUiState::Default;
        }

        if (hasMips)
        {
          props["PreserveAlphaCoverage"].m_Visibility = WPropertyUiState::Default;
          props["AlphaThreshold"].m_Visibility = WPropertyUiState::Default;
        }
      }

      if (mapping == WTexture2DChannelMappingEnum::RGBWHITE_A1 || mapping == WTexture2DChannelMappingEnum::RGBWHITE_R1)
      {
        // RGB is a constant white, so dilating the color into transparent areas is pointless,
        // but keeping the mask's coverage across mips is not.
        if (hasMips)
        {
          props["PreserveAlphaCoverage"].m_Visibility = WPropertyUiState::Default;
          props["AlphaThreshold"].m_Visibility = WPropertyUiState::Default;
        }
      }

      if (isHDR)
      {
        props["HdrExposureBias"].m_Visibility = WPropertyUiState::Default;
      }
    }

    // always hide this, feature may be removed at some point
    props["PremultipliedAlpha"].m_Visibility = WPropertyUiState::Invisible;
  }
}

WString WTextureAssetProperties::GetAbsoluteInputFilePath(WInt32 iInput) const
{
  WStringBuilder sPath = m_Input[iInput];
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}

WInt32 WTextureAssetProperties::GetNumInputFiles() const
{
  if (m_bIsRenderTarget)
    return 0;

  switch (m_ChannelMapping)
  {
    case WTexture2DChannelMappingEnum::R1:
    case WTexture2DChannelMappingEnum::R1_ALPHA:
    case WTexture2DChannelMappingEnum::RG1:
    case WTexture2DChannelMappingEnum::RGB1:
    case WTexture2DChannelMappingEnum::RGB1_ABLACK:
    case WTexture2DChannelMappingEnum::RGBA1:
    case WTexture2DChannelMappingEnum::RGBWHITE_A1:
    case WTexture2DChannelMappingEnum::RGBWHITE_R1:
      return 1;

    case WTexture2DChannelMappingEnum::R1_G2:
    case WTexture2DChannelMappingEnum::RGB1_A2:
      return 2;

    case WTexture2DChannelMappingEnum::R1_G2_B3:
      return 3;

    case WTexture2DChannelMappingEnum::R1_G2_B3_A4:
      return 4;
  }

  W_REPORT_FAILURE("Invalid Code Path");
  return 1;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WTextureAssetPropertiesPatch_2_3 : public WGraphPatch
{
public:
  WTextureAssetPropertiesPatch_2_3()
    : WGraphPatch("WTextureAssetProperties", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pMipmaps = pNode->FindProperty("Mipmaps");
    if (pMipmaps && pMipmaps->m_Value.IsA<bool>())
    {
      if (pMipmaps->m_Value.Get<bool>())
        pNode->AddProperty("MipmapMode", (WInt32)WTexConvMipmapMode::Kaiser);
      else
        pNode->AddProperty("MipmapMode", (WInt32)WTexConvMipmapMode::None);
    }

    auto* pCompression = pNode->FindProperty("Compression");
    if (pCompression && pCompression->m_Value.IsA<bool>())
    {
      if (pCompression->m_Value.Get<bool>())
        pNode->AddProperty("CompressionMode", (WInt32)WTexConvCompressionMode::High);
      else
        pNode->AddProperty("CompressionMode", (WInt32)WTexConvCompressionMode::None);
    }
  }
};

WTextureAssetPropertiesPatch_2_3 g_WTextureAssetPropertiesPatch_2_3;

//////////////////////////////////////////////////////////////////////////

class WTextureAssetPropertiesPatch_3_4 : public WGraphPatch
{
public:
  WTextureAssetPropertiesPatch_3_4()
    : WGraphPatch("WTextureAssetProperties", 4)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    const char* szAddressModes[] = {"AddressModeU", "AddressModeV", "AddressModeW"};

    for (WUInt32 i = 0; i < 3; ++i)
    {
      auto* pAddress = pNode->FindProperty(szAddressModes[i]);
      if (pAddress && pAddress->m_Value.IsA<WString>())
      {
        if (pAddress->m_Value.Get<WString>() == "WTexture2DAddressMode::Wrap")
        {
          pNode->ChangeProperty(szAddressModes[i], (WInt32)WImageAddressMode::Repeat);
        }
        else if (pAddress->m_Value.Get<WString>() == "WTexture2DAddressMode::Clamp")
        {
          pNode->ChangeProperty(szAddressModes[i], (WInt32)WImageAddressMode::Clamp);
        }
        else if (pAddress->m_Value.Get<WString>() == "WTexture2DAddressMode::Mirror")
        {
          pNode->ChangeProperty(szAddressModes[i], (WInt32)WImageAddressMode::Mirror);
        }
      }
    }
  }
};

WTextureAssetPropertiesPatch_3_4 g_WTextureAssetPropertiesPatch_3_4;

//////////////////////////////////////////////////////////////////////////

class WTextureAssetPropertiesPatch_4_5 : public WGraphPatch
{
public:
  WTextureAssetPropertiesPatch_4_5()
    : WGraphPatch("WTextureAssetProperties", 5)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pUsage = pNode->FindProperty("Usage");
    if (pUsage && pUsage->m_Value.IsA<WString>())
    {
      if (pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::Unknown")
      {
        pNode->ChangeProperty("Usage", (WInt32)WTexConvUsage::Auto);
      }
      else if (pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::Other_sRGB" ||
               pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::Diffuse" ||
               pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::EmissiveColor")
      {
        pNode->ChangeProperty("Usage", (WInt32)WTexConvUsage::Color);
      }
      else if (pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::Height" || pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::Mask" ||
               pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::LookupTable" ||
               pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::Other_Linear" ||
               pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::EmissiveMask")
      {
        pNode->ChangeProperty("Usage", (WInt32)WTexConvUsage::Linear);
      }
      else if (pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::NormalMap")
      {
        pNode->ChangeProperty("Usage", (WInt32)WTexConvUsage::NormalMap);
      }
      else if (pUsage->m_Value.Get<WString>() == "WTexture2DUsageEnum::HDR")
      {
        pNode->ChangeProperty("Usage", (WInt32)WTexConvUsage::Hdr);
      }
    }
  }
};

WTextureAssetPropertiesPatch_4_5 g_WTextureAssetPropertiesPatch_4_5;
