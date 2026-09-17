#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WTextureCubeChannelMappingEnum, 1)
  W_ENUM_CONSTANTS(WTextureCubeChannelMappingEnum::RGB1, WTextureCubeChannelMappingEnum::RGBA1, WTextureCubeChannelMappingEnum::RGB1TO6, WTextureCubeChannelMappingEnum::RGBA1TO6)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureCubeAssetProperties, 3, WRTTIDefaultAllocator<WTextureCubeAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Usage", WTexConvUsage, m_TextureUsage),

    W_ENUM_MEMBER_PROPERTY("MipmapMode", WTexConvMipmapMode, m_MipmapMode),
    W_ENUM_MEMBER_PROPERTY("CompressionMode", WTexConvCompressionMode, m_CompressionMode),

    W_MEMBER_PROPERTY("HdrExposureBias", m_fHdrExposureBias)->AddAttributes(new WClampValueAttribute(-20.0f, 20.0f)),

    W_ENUM_MEMBER_PROPERTY("TextureFilter", WTextureFilterSetting, m_TextureFilter),

    W_ENUM_MEMBER_PROPERTY("ChannelMapping", WTextureCubeChannelMappingEnum, m_ChannelMapping),

    W_ACCESSOR_PROPERTY("Input1", GetInputFile0, SetInputFile0)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("Input2", GetInputFile1, SetInputFile1)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr)),
    W_ACCESSOR_PROPERTY("Input3", GetInputFile2, SetInputFile2)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr)),
    W_ACCESSOR_PROPERTY("Input4", GetInputFile3, SetInputFile3)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr)),
    W_ACCESSOR_PROPERTY("Input5", GetInputFile4, SetInputFile4)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr)),
    W_ACCESSOR_PROPERTY("Input6", GetInputFile5, SetInputFile5)->AddAttributes(new WFileBrowserAttribute("Select Texture", WFileBrowserAttribute::ImagesLdrAndHdr)),

  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WTextureCubeAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WTextureCubeAssetProperties>())
  {
    const WInt64 mapping = e.m_pObject->GetTypeAccessor().GetValue("ChannelMapping").ConvertTo<WInt64>();
    const bool isHDR = e.m_pObject->GetTypeAccessor().GetValue("Usage").ConvertTo<WInt32>() == WTexConvUsage::Hdr;

    auto& props = *e.m_pPropertyStates;

    props["Usage"].m_Visibility = WPropertyUiState::Default;
    props["Input1"].m_Visibility = WPropertyUiState::Default;
    props["Input2"].m_Visibility = WPropertyUiState::Invisible;
    props["Input3"].m_Visibility = WPropertyUiState::Invisible;
    props["Input4"].m_Visibility = WPropertyUiState::Invisible;
    props["Input5"].m_Visibility = WPropertyUiState::Invisible;
    props["Input6"].m_Visibility = WPropertyUiState::Invisible;
    props["HdrExposureBias"].m_Visibility = WPropertyUiState::Disabled;

    if (isHDR)
    {
      props["HdrExposureBias"].m_Visibility = WPropertyUiState::Default;
    }

    if (mapping == WTextureCubeChannelMappingEnum::RGB1TO6 || mapping == WTextureCubeChannelMappingEnum::RGBA1TO6)
    {
      props["Input1"].m_sNewLabelText = "TextureAsset::CM_Right";
      props["Input2"].m_sNewLabelText = "TextureAsset::CM_Left";
      props["Input3"].m_sNewLabelText = "TextureAsset::CM_Top";
      props["Input4"].m_sNewLabelText = "TextureAsset::CM_Bottom";
      props["Input5"].m_sNewLabelText = "TextureAsset::CM_Front";
      props["Input6"].m_sNewLabelText = "TextureAsset::CM_Back";
    }
    else
    {
      props["Input1"].m_sNewLabelText = "TextureAsset::Input1";
      props["Input2"].m_sNewLabelText = "TextureAsset::Input2";
      props["Input3"].m_sNewLabelText = "TextureAsset::Input3";
      props["Input4"].m_sNewLabelText = "TextureAsset::Input4";
      props["Input5"].m_sNewLabelText = "TextureAsset::Input5";
      props["Input6"].m_sNewLabelText = "TextureAsset::Input6";
    }

    switch (mapping)
    {
      case WTextureCubeChannelMappingEnum::RGB1TO6:
      case WTextureCubeChannelMappingEnum::RGBA1TO6:
        props["Input6"].m_Visibility = WPropertyUiState::Default;
        props["Input5"].m_Visibility = WPropertyUiState::Default;
        props["Input4"].m_Visibility = WPropertyUiState::Default;
        props["Input3"].m_Visibility = WPropertyUiState::Default;
        props["Input2"].m_Visibility = WPropertyUiState::Default;
        break;
    }
  }
}

WString WTextureCubeAssetProperties::GetAbsoluteInputFilePath(WInt32 iInput) const
{
  WStringBuilder sPath = m_Input[iInput];
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}

WInt32 WTextureCubeAssetProperties::GetNumInputFiles() const
{
  switch (m_ChannelMapping)
  {
    case WTextureCubeChannelMappingEnum::RGB1:
    case WTextureCubeChannelMappingEnum::RGBA1:
      return 1;

    case WTextureCubeChannelMappingEnum::RGB1TO6:
    case WTextureCubeChannelMappingEnum::RGBA1TO6:
      return 6;
  }

  W_REPORT_FAILURE("Invalid Code Path");
  return 1;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WTextureCubeAssetProperties_2_3 : public WGraphPatch
{
public:
  WTextureCubeAssetProperties_2_3()
    : WGraphPatch("WTextureCubeAssetProperties", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pUsage = pNode->FindProperty("Usage");
    if (pUsage && pUsage->m_Value.IsA<WString>())
    {
      if (pUsage->m_Value.Get<WString>() == "WTextureCubeUsageEnum::Unknown")
      {
        pNode->ChangeProperty("Usage", (WInt32)WTexConvUsage::Auto);
      }
      else if (pUsage->m_Value.Get<WString>() == "WTextureCubeUsageEnum::Other_sRGB" ||
               pUsage->m_Value.Get<WString>() == "WTextureCubeUsageEnum::Skybox")
      {
        pNode->ChangeProperty("Usage", (WInt32)WTexConvUsage::Color);
      }
      else if (pUsage->m_Value.Get<WString>() == "WTextureCubeUsageEnum::Other_Linear" ||
               pUsage->m_Value.Get<WString>() == "WTextureCubeUsageEnum::LookupTable")
      {
        pNode->ChangeProperty("Usage", (WInt32)WTexConvUsage::Linear);
      }
      else if (pUsage->m_Value.Get<WString>() == "WTextureCubeUsageEnum::SkyboxHDR")
      {
        pNode->ChangeProperty("Usage", (WInt32)WTexConvUsage::Hdr);
      }
    }

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
        pNode->AddProperty("CompressionMode", (WInt32)WTexConvCompressionMode::Medium);
      else
        pNode->AddProperty("CompressionMode", (WInt32)WTexConvCompressionMode::None);
    }
  }
};

WTextureCubeAssetProperties_2_3 g_WTextureCubeAssetProperties_2_3;
