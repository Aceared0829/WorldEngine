#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureAssetDocument, 7, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("ChannelMode", WTextureChannelMode, m_ChannelMode),
    W_MEMBER_PROPERTY("TextureLod", m_iTextureLod),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WTextureChannelMode, 1)
  W_ENUM_CONSTANT(WTextureChannelMode::RGBA)->AddAttributes(new WGroupAttribute("Multi", 0.0f)),
  W_ENUM_CONSTANT(WTextureChannelMode::RGB)->AddAttributes(new WGroupAttribute("Multi", 1.0f)),
  W_ENUM_CONSTANT(WTextureChannelMode::Red)->AddAttributes(new WGroupAttribute("Single", 0.0f)),
  W_ENUM_CONSTANT(WTextureChannelMode::Green)->AddAttributes(new WGroupAttribute("Single", 1.0f)),
  W_ENUM_CONSTANT(WTextureChannelMode::Blue)->AddAttributes(new WGroupAttribute("Single", 2.0f)),
  W_ENUM_CONSTANT(WTextureChannelMode::Alpha)->AddAttributes(new WGroupAttribute("Single", 3.0f)),
  W_ENUM_CONSTANT(WTextureChannelMode::CoverageRed)->AddAttributes(new WGroupAttribute("Coverage", 0.0f)),
  W_ENUM_CONSTANT(WTextureChannelMode::CoverageAlpha)->AddAttributes(new WGroupAttribute("Coverage", 1.0f)),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WTextureAssetDocument::WTextureAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WTextureAssetProperties>(sDocumentPath, WAssetDocEngineConnection::Simple)
{
}

static const char* ToWrapMode(WImageAddressMode::Enum mode)
{
  switch (mode)
  {
    case WImageAddressMode::Repeat:
      return "Repeat";
    case WImageAddressMode::Clamp:
      return "Clamp";
    case WImageAddressMode::ClampBorder:
      return "ClampBorder";
    case WImageAddressMode::Mirror:
      return "Mirror";
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
}

const char* ToFilterMode(WTextureFilterSetting::Enum mode)
{
  switch (mode)
  {
    case WTextureFilterSetting::FixedNearest:
      return "Nearest";
    case WTextureFilterSetting::FixedBilinear:
      return "Bilinear";
    case WTextureFilterSetting::FixedTrilinear:
      return "Trilinear";
    case WTextureFilterSetting::FixedAnisotropic2x:
      return "Aniso2x";
    case WTextureFilterSetting::FixedAnisotropic4x:
      return "Aniso4x";
    case WTextureFilterSetting::FixedAnisotropic8x:
      return "Aniso8x";
    case WTextureFilterSetting::FixedAnisotropic16x:
      return "Aniso16x";
    case WTextureFilterSetting::LowestQuality:
      return "Lowest";
    case WTextureFilterSetting::LowQuality:
      return "Low";
    case WTextureFilterSetting::DefaultQuality:
      return "Default";
    case WTextureFilterSetting::HighQuality:
      return "High";
    case WTextureFilterSetting::HighestQuality:
      return "Highest";
  }

  W_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToUsageMode(WTexConvUsage::Enum mode)
{
  switch (mode)
  {
    case WTexConvUsage::Auto:
      return "Auto";
    case WTexConvUsage::Color:
      return "Color";
    case WTexConvUsage::Linear:
      return "Linear";
    case WTexConvUsage::Hdr:
      return "Hdr";
    case WTexConvUsage::NormalMap:
      return "NormalMap";
    case WTexConvUsage::NormalMap_Inverted:
      return "NormalMap_Inverted";
    case WTexConvUsage::BumpMap:
      return "BumpMap";
  }

  W_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToMipmapMode(WTexConvMipmapMode::Enum mode)
{
  switch (mode)
  {
    case WTexConvMipmapMode::None:
      return "None";
    case WTexConvMipmapMode::Linear:
      return "Linear";
    case WTexConvMipmapMode::Kaiser:
      return "Kaiser";
  }

  W_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToCompressionMode(WTexConvCompressionMode::Enum mode)
{
  switch (mode)
  {
    case WTexConvCompressionMode::None:
      return "None";
    case WTexConvCompressionMode::Medium:
      return "Medium";
    case WTexConvCompressionMode::High:
      return "High";
  }

  W_ASSERT_NOT_IMPLEMENTED;
  return "";
}

WStatus WTextureAssetDocument::RunTexConv(const char* szTargetFile, const WAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const WTextureAssetProfileConfig* pAssetConfig)
{
  const WTextureAssetProperties* pProp = GetProperties();

  QStringList arguments;
  WStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << WConversionUtils::ToString(AssetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const WUInt64 uiHash64 = AssetHeader.GetFileHash();
    const WUInt32 uiHashLow32 = uiHash64 & 0xFFFFFFFF;
    const WUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.SetFormat("{0}", WArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.SetFormat("{0}", WArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }


  arguments << "-out";
  arguments << szTargetFile;

  // TexConv writes this itself, because only it knows the resolution and format it chose.
  {
    const WStringBuilder sInfoFile = WAssetInfoFile::GetInfoFilePathForOutput(szTargetFile);
    arguments << "-assetInfoOut";
    arguments << sInfoFile.GetData();
  }

  const WStringBuilder sThumbnail = GetThumbnailFilePath();
  if (bUpdateThumbnail)
  {
    // Thumbnail
    const WStringBuilder sDir = sThumbnail.GetFileDirectory();
    WOSFile::CreateDirectoryStructure(sDir).IgnoreResult();

    arguments << "-thumbnailRes";
    arguments << "256";
    arguments << "-thumbnailOut";

    arguments << QString::fromUtf8(sThumbnail.GetData());
  }

  // low resolution data
  {
    WStringBuilder lowResPath = szTargetFile;
    WStringBuilder name = lowResPath.GetFileName();
    name.Append("-lowres");
    lowResPath.ChangeFileName(name);

    arguments << "-lowMips";
    arguments << "6";
    arguments << "-lowOut";

    arguments << QString::fromUtf8(lowResPath.GetData());
  }

  arguments << "-mipmaps";
  arguments << ToMipmapMode(pProp->m_MipmapMode);

  arguments << "-compression";
  arguments << ToCompressionMode(pProp->m_CompressionMode);

  arguments << "-usage";
  arguments << ToUsageMode(pProp->m_TextureUsage);

  if (pProp->m_bPremultipliedAlpha)
    arguments << "-premulalpha";

  if (pProp->m_bDilateColor)
  {
    arguments << "-dilate";
    // arguments << "8"; // default value
  }

  if (pProp->m_bFlipHorizontal)
    arguments << "-flip_horz";

  if (pProp->m_bPreserveAlphaCoverage)
  {
    arguments << "-mipsPreserveCoverage";
    arguments << "-mipsAlphaThreshold";
    temp.SetFormat("{0}", WArgF(pProp->m_fAlphaThreshold, 2));
    arguments << temp.GetData();
  }

  if (pProp->m_TextureUsage == WTexConvUsage::Hdr)
  {
    arguments << "-hdrExposure";
    temp.SetFormat("{0}", WArgF(pProp->m_fHdrExposureBias, 2));
    arguments << temp.GetData();
  }

  arguments << "-maxRes" << QString::number(pAssetConfig->m_uiMaxResolution);

  arguments << "-addressU" << ToWrapMode(pProp->m_AddressModeU);
  arguments << "-addressV" << ToWrapMode(pProp->m_AddressModeV);
  arguments << "-addressW" << ToWrapMode(pProp->m_AddressModeW);
  arguments << "-filter" << ToFilterMode(pProp->m_TextureFilter);

  if (pProp->m_bIsArrayTexture)
  {
    arguments << "-type" << "Texture2DArray";

    for (WUInt32 i = 0; i < pProp->m_ArraySlices.GetCount(); ++i)
    {
      if (pProp->m_ArraySlices[i].IsEmpty())
        continue;

      WStringBuilder sAbsPath = pProp->m_ArraySlices[i];
      sAbsPath.MakeCleanPath();
      if (!sAbsPath.IsAbsolutePath())
        WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsPath);

      temp.SetFormat("-in{0}", i);
      arguments << temp.GetData();
      arguments << QString::fromUtf8(sAbsPath.GetData());

      switch (pProp->GetArrayChannelMapping())
      {
        case WTextureArrayChannelMappingEnum::RGBA:
          temp.SetFormat("-rgba{0}", i);
          arguments << temp.GetData();
          temp.SetFormat("in{0}.rgba", i);
          arguments << temp.GetData();
          break;

        case WTextureArrayChannelMappingEnum::RGB:
          temp.SetFormat("-rgb{0}", i);
          arguments << temp.GetData();
          temp.SetFormat("in{0}.rgb", i);
          arguments << temp.GetData();
          break;

        case WTextureArrayChannelMappingEnum::RG:
          temp.SetFormat("-rg{0}", i);
          arguments << temp.GetData();
          temp.SetFormat("in{0}.rg", i);
          arguments << temp.GetData();
          break;

        case WTextureArrayChannelMappingEnum::R_Red:
        case WTextureArrayChannelMappingEnum::R_Green:
        case WTextureArrayChannelMappingEnum::R_Blue:
        case WTextureArrayChannelMappingEnum::R_Alpha:
        {
          const char* szChannel = "r";
          if (pProp->GetArrayChannelMapping() == WTextureArrayChannelMappingEnum::R_Green)
            szChannel = "g";
          else if (pProp->GetArrayChannelMapping() == WTextureArrayChannelMappingEnum::R_Blue)
            szChannel = "b";
          else if (pProp->GetArrayChannelMapping() == WTextureArrayChannelMappingEnum::R_Alpha)
            szChannel = "a";

          temp.SetFormat("-r{0}", i);
          arguments << temp.GetData();
          temp.SetFormat("in{0}.{1}", i, szChannel);
          arguments << temp.GetData();
          break;
        }
      }
    }
  }
  else
  {
    const WInt32 iNumInputFiles = pProp->GetNumInputFiles();
    for (WInt32 i = 0; i < iNumInputFiles; ++i)
    {
      temp.SetFormat("-in{0}", i);

      if (WStringUtils::IsNullOrEmpty(pProp->GetInputFile(i)))
        break;

      arguments << temp.GetData();
      arguments << QString(pProp->GetAbsoluteInputFilePath(i).GetData());
    }

    switch (pProp->GetChannelMapping())
    {
      case WTexture2DChannelMappingEnum::R1:
      {
        arguments << "-r";
        arguments << "in0.r"; // always linear
      }
      break;

      case WTexture2DChannelMappingEnum::R1_ALPHA:
      {
        arguments << "-r";
        arguments << "in0.a"; // always linear
      }
      break;

      case WTexture2DChannelMappingEnum::RG1:
      {
        arguments << "-rg";
        arguments << "in0.rg"; // always linear
      }
      break;

      case WTexture2DChannelMappingEnum::R1_G2:
      {
        arguments << "-r";
        arguments << "in0.r";
        arguments << "-g";
        arguments << "in1.g"; // always linear
      }
      break;

      case WTexture2DChannelMappingEnum::RGB1:
      {
        arguments << "-rgb";
        arguments << "in0.rgb";
      }
      break;

      case WTexture2DChannelMappingEnum::RGB1_ABLACK:
      {
        arguments << "-rgb";
        arguments << "in0.rgb";
        arguments << "-a";
        arguments << "black";
      }
      break;

      case WTexture2DChannelMappingEnum::R1_G2_B3:
      {
        arguments << "-r";
        arguments << "in0.r";
        arguments << "-g";
        arguments << "in1.r";
        arguments << "-b";
        arguments << "in2.r";
      }
      break;

      case WTexture2DChannelMappingEnum::RGBA1:
      {
        arguments << "-rgba";
        arguments << "in0.rgba";
      }
      break;

      case WTexture2DChannelMappingEnum::RGB1_A2:
      {
        arguments << "-rgb";
        arguments << "in0.rgb";
        arguments << "-a";
        arguments << "in1.r";
      }
      break;

      case WTexture2DChannelMappingEnum::R1_G2_B3_A4:
      {
        arguments << "-r";
        arguments << "in0.r";
        arguments << "-g";
        arguments << "in1.r";
        arguments << "-b";
        arguments << "in2.r";
        arguments << "-a";
        arguments << "in3.r";
      }
      break;

      case WTexture2DChannelMappingEnum::RGBWHITE_A1:
      {
        arguments << "-rgb";
        arguments << "white";
        arguments << "-a";
        arguments << "in0.a"; // always linear
      }
      break;

      case WTexture2DChannelMappingEnum::RGBWHITE_R1:
      {
        arguments << "-rgb";
        arguments << "white";
        arguments << "-a";
        arguments << "in0.r"; // always linear
      }
      break;
    }
  }

  W_SUCCEED_OR_RETURN(WQtEditorApp::GetSingleton()->ExecuteTool("WTexConv", arguments, 180, WLog::GetThreadLocalLogSystem()));

  if (bUpdateThumbnail)
  {
    WUInt64 uiThumbnailHash = WAssetCurator::GetSingleton()->GetAssetThumbnailHash(GetGuid());
    W_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
    InvalidateAssetThumbnail();
  }

  return WStatus(W_SUCCESS);
}


void WTextureAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (!m_bIsRenderTarget)
  {
    // every 2D texture also generates a "-lowres" output, which is used to be embedded into materials for quick streaming
    pInfo->m_Outputs.Insert("LOWRES");
  }

  // Always clean up any stale Input1-4 dependencies (may be present if the asset was previously non-array mode).
  for (WUInt32 i = GetProperties()->GetNumInputFiles(); i < 4; ++i)
  {
    pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile(i));
  }

  if (GetProperties()->m_bIsArrayTexture)
  {
    // Register all array slices as transform dependencies (dynamic array, not auto-registered by the base class).
    for (const WString& sSlice : GetProperties()->m_ArraySlices)
    {
      if (!sSlice.IsEmpty())
      {
        pInfo->m_TransformDependencies.Insert(sSlice);
      }
    }
  }
}

void WTextureAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (m_bIsRenderTarget)
  {
    if (GetProperties()->m_bIsRenderTarget == false)
    {
      GetCommandHistory()->StartTransaction("MakeRenderTarget");
      GetObjectAccessor()->SetValueByName(GetPropertyObject(), "IsRenderTarget", true).AssertSuccess();
      GetCommandHistory()->FinishTransaction();
      GetCommandHistory()->ClearUndoHistory();
    }
  }
}

WTransformStatus WTextureAssetDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  if (sOutputTag.IsEqual("LOWRES"))
  {
    // no need to generate this file, it will be generated together with the main output
    return WTransformStatus();
  }

  const auto* pAssetConfig = pAssetProfile->GetTypeConfig<WTextureAssetProfileConfig>();

  const auto props = GetProperties();

  if (m_bIsRenderTarget)
  {
    WDeferredFileWriter file;
    file.SetOutput(szTargetFile);

    W_SUCCEED_OR_RETURN(AssetHeader.Write(file));

    // TODO: move this into a shared location, reuse in WTexConv::WriteTexHeader
    const WUInt8 uiTexFileFormatVersion = 5;
    file << uiTexFileFormatVersion;

    WGALResourceFormat::Enum format = WGALResourceFormat::Invalid;
    bool bIsSRGB = false;

    switch (props->m_RtFormat)
    {
      case WRenderTargetFormat::RGBA8:
        format = WGALResourceFormat::RGBAUByteNormalized;
        break;

      case WRenderTargetFormat::RGBA8sRgb:
        format = WGALResourceFormat::RGBAUByteNormalizedsRGB;
        bIsSRGB = true;
        break;

      case WRenderTargetFormat::RGB10:
        format = WGALResourceFormat::RG11B10Float;
        break;

      case WRenderTargetFormat::RGBA16:
        format = WGALResourceFormat::RGBAHalf;
        break;

      case WRenderTargetFormat::R8:
        format = WGALResourceFormat::RUByteNormalized;
        break;

      case WRenderTargetFormat::R16:
        format = WGALResourceFormat::RHalf;
        break;

      case WRenderTargetFormat::R32:
        format = WGALResourceFormat::RFloat;
        break;

      case WRenderTargetFormat::RG8:
        format = WGALResourceFormat::RGUByteNormalized;
        break;

      case WRenderTargetFormat::RG16:
        format = WGALResourceFormat::RGHalf;
        break;

      case WRenderTargetFormat::RG32:
        format = WGALResourceFormat::RGFloat;
        break;

        W_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    file << bIsSRGB;
    file << (WUInt8)props->m_AddressModeU;
    file << (WUInt8)props->m_AddressModeV;
    file << (WUInt8)props->m_AddressModeW;
    file << (WUInt8)props->m_TextureFilter;

    WInt16 resX = 0, resY = 0;

    switch (props->m_Resolution)
    {
      case WTexture2DResolution::Fixed64x64:
        resX = 64;
        resY = 64;
        break;
      case WTexture2DResolution::Fixed128x128:
        resX = 128;
        resY = 128;
        break;
      case WTexture2DResolution::Fixed256x256:
        resX = 256;
        resY = 256;
        break;
      case WTexture2DResolution::Fixed512x512:
        resX = 512;
        resY = 512;
        break;
      case WTexture2DResolution::Fixed1024x1024:
        resX = 1024;
        resY = 1024;
        break;
      case WTexture2DResolution::Fixed2048x2048:
        resX = 2048;
        resY = 2048;
        break;
      case WTexture2DResolution::CVarRtResolution1:
        resX = -1;
        resY = 1;
        break;
      case WTexture2DResolution::CVarRtResolution2:
        resX = -1;
        resY = 2;
        break;
      default:
        W_ASSERT_NOT_IMPLEMENTED;
    }

    file << resX;
    file << resY;
    file << props->m_fCVarResolutionScale;
    file << (int)format;


    if (file.Close().Failed())
      return WTransformStatus(WFmt("Writing to target file failed: '{0}'", szTargetFile));

    return WTransformStatus();
  }
  else
  {
    const bool bUpdateThumbnail = pAssetProfile == WAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

    WTransformStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail, pAssetConfig);

    WFileStats stat;
    if (WOSFile::GetFileStats(szTargetFile, stat).Succeeded() && stat.m_uiFileSize == 0)
    {
      // if the file was touched, but nothing written to it, delete the file
      // might happen if TexConv crashed or had an error
      WOSFile::DeleteFile(szTargetFile).IgnoreResult();

      if (result.Succeeded())
        result = WTransformStatus("TexConv did not write an output file");
    }

    return result;
  }
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WTextureAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

WTextureAssetDocumentGenerator::WTextureAssetDocumentGenerator()
{
  AddSupportedFileType("tga");
  AddSupportedFileType("dds");
  AddSupportedFileType("jpg");
  AddSupportedFileType("jpeg");
  AddSupportedFileType("png");
  AddSupportedFileType("hdr");
  AddSupportedFileType("exr");
}

WTextureAssetDocumentGenerator::~WTextureAssetDocumentGenerator() = default;

WTextureAssetDocumentGenerator::TextureType WTextureAssetDocumentGenerator::DetermineTextureType(WStringView sFile)
{
  WStringBuilder baseFilename = sFile.GetFileName();

  // gets rid of 1K, 2K, etc,
  while (baseFilename.TrimWordEnd("_") ||
         baseFilename.TrimWordEnd("K") ||
         baseFilename.TrimWordEnd("-") ||
         baseFilename.TrimWordEnd("1") ||
         baseFilename.TrimWordEnd("2") ||
         baseFilename.TrimWordEnd("3") ||
         baseFilename.TrimWordEnd("4") ||
         baseFilename.TrimWordEnd("5") ||
         baseFilename.TrimWordEnd("6") ||
         baseFilename.TrimWordEnd("7") ||
         baseFilename.TrimWordEnd("8") ||
         baseFilename.TrimWordEnd("9") ||
         baseFilename.TrimWordEnd("0"))
  {
  }

  const bool dx = baseFilename.TrimWordEnd("_dx");
  const bool gl = baseFilename.TrimWordEnd("_gl");

  if (sFile.HasExtension("hdr"))
  {
    return TextureType::HDR;
  }
  else if (sFile.HasExtension("exr"))
  {
    return TextureType::HDR;
  }
  else if (baseFilename.EndsWith_NoCase("_d") || baseFilename.EndsWith_NoCase("diffuse") || baseFilename.EndsWith_NoCase("diff") || baseFilename.EndsWith_NoCase("col") || baseFilename.EndsWith_NoCase("color"))
  {
    return TextureType::Diffuse;
  }
  else if (baseFilename.EndsWith_NoCase("_n") || baseFilename.EndsWith_NoCase("normal") || baseFilename.EndsWith_NoCase("normals") || baseFilename.EndsWith_NoCase("nrm") || baseFilename.EndsWith_NoCase("norm") || baseFilename.EndsWith_NoCase("_nor"))
  {
    if (dx)
      return TextureType::NormalDX;
    else if (gl)
      return TextureType::NormalGL;

    return TextureType::NormalDX;
  }
  else if (baseFilename.EndsWith_NoCase("_arm") || baseFilename.EndsWith_NoCase("_orm"))
  {
    return TextureType::ORM;
  }
  else if (baseFilename.EndsWith_NoCase("_rough") || baseFilename.EndsWith_NoCase("roughness") || baseFilename.EndsWith_NoCase("_rgh"))
  {
    return TextureType::Roughness;
  }
  else if (baseFilename.EndsWith_NoCase("_ao"))
  {
    return TextureType::Occlusion;
  }
  else if (baseFilename.EndsWith_NoCase("_height") || baseFilename.EndsWith_NoCase("_disp"))
  {
    return TextureType::Height;
  }
  else if (baseFilename.EndsWith_NoCase("_metal") || baseFilename.EndsWith_NoCase("_met") || baseFilename.EndsWith_NoCase("metallic") || baseFilename.EndsWith_NoCase("metalness"))
  {
    return TextureType::Metalness;
  }
  else if (baseFilename.EndsWith_NoCase("_alpha"))
  {
    return TextureType::Linear;
  }

  return TextureType::Diffuse;
}

void WTextureAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  if (sAbsInputFile.IsEmpty())
  {
    {
      WAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
      info2.m_Priority = WAssetDocGeneratorPriority::LowPriority;
      info2.m_sName = "TextureImport.Auto";
      info2.m_sIcon = ":/AssetIcons/Texture_2D.svg";
    }

    return;
  }

  const TextureType tt = DetermineTextureType(sAbsInputFile);

  WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
  info.m_Priority = WAssetDocGeneratorPriority::DefaultPriority;

  // first add the default option
  switch (tt)
  {
    case TextureType::Diffuse:
    {
      info.m_sName = "TextureImport.Diffuse";
      info.m_sIcon = ":/AssetIcons/Texture_2D.svg";
      break;
    }

    case TextureType::NormalDX:
    {
      info.m_sName = "TextureImport.NormalDX";
      info.m_sIcon = ":/AssetIcons/Texture_Normals.svg";
      break;
    }

    case TextureType::NormalGL:
    {
      info.m_sName = "TextureImport.NormalGL";
      info.m_sIcon = ":/AssetIcons/Texture_Normals.svg";
      break;
    }

    case TextureType::Roughness:
    {
      info.m_sName = "TextureImport.Roughness";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::Occlusion:
    {
      info.m_sName = "TextureImport.Occlusion";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::Metalness:
    {
      info.m_sName = "TextureImport.Metalness";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::ORM:
    {
      info.m_sName = "TextureImport.ORM";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::Height:
    {
      info.m_sName = "TextureImport.Height";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::HDR:
    {
      info.m_sName = "TextureImport.HDR";
      info.m_sIcon = ":/AssetIcons/Texture_2D.svg";
      break;
    }

    case TextureType::Linear:
    {
      info.m_sName = "TextureImport.Linear";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }
  }

  // now add all the other options

  if (tt != TextureType::Diffuse)
  {
    WAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Diffuse";
    info2.m_sIcon = ":/AssetIcons/Texture_2D.svg";
  }

  if (tt != TextureType::Linear)
  {
    WAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Linear";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::NormalDX)
  {
    WAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.NormalDX";
    info2.m_sIcon = ":/AssetIcons/Texture_Normals.svg";
  }

  if (tt != TextureType::NormalGL)
  {
    WAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.NormalGL";
    info2.m_sIcon = ":/AssetIcons/Texture_Normals.svg";
  }

  if (tt != TextureType::Metalness)
  {
    WAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Metalness";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Roughness)
  {
    WAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Roughness";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Occlusion)
  {
    WAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Occlusion";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::ORM)
  {
    WAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.ORM";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Height)
  {
    WAssetDocumentGenerator::ImportMode& info2 = out_modes.ExpandAndGetRef();
    info2.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info2.m_sName = "TextureImport.Height";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }
}

WStatus WTextureAssetDocumentGenerator::Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  if (sMode == "TextureImport.Auto")
  {
    const TextureType tt = DetermineTextureType(sInputFileAbs);

    switch (tt)
    {
      case TextureType::Diffuse:
        sMode = "TextureImport.Diffuse";
        break;
      case TextureType::NormalDX:
        sMode = "TextureImport.NormalDX";
        break;
      case TextureType::NormalGL:
        sMode = "TextureImport.NormalGL";
        break;
      case TextureType::Occlusion:
        sMode = "TextureImport.Occlusion";
        break;
      case TextureType::Roughness:
        sMode = "TextureImport.Roughness";
        break;
      case TextureType::Metalness:
        sMode = "TextureImport.Metalness";
        break;
      case TextureType::ORM:
        sMode = "TextureImport.ORM";
        break;
      case TextureType::Height:
        sMode = "TextureImport.Height";
        break;
      case TextureType::HDR:
        sMode = "TextureImport.HDR";
        break;
      case TextureType::Linear:
        sMode = "TextureImport.Linear";
        break;
    }
  }

  const WStringBuilder sOutFile = GetImportTargetPath(sInputFileAbs);

  auto pApp = WQtEditorApp::GetSingleton();

  WStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  WDocument* pDoc = pApp->CreateDocument(sOutFile, WDocumentFlags::None);
  if (pDoc == nullptr)
    return WStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  WTextureAssetDocument* pAssetDoc = WDynamicCast<WTextureAssetDocument*>(pDoc);
  if (pAssetDoc == nullptr)
    return WStatus("Target document is not a valid WTextureAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", sInputFileRel.GetView());
  accessor.SetValue("ChannelMapping", (int)WTexture2DChannelMappingEnum::RGB1);
  accessor.SetValue("Usage", (int)WTexConvUsage::Linear);

  if (sMode == "TextureImport.Diffuse")
  {
    accessor.SetValue("Usage", (int)WTexConvUsage::Color);
  }
  else if (sMode == "TextureImport.NormalDX")
  {
    accessor.SetValue("Usage", (int)WTexConvUsage::NormalMap);
  }
  else if (sMode == "TextureImport.NormalGL")
  {
    accessor.SetValue("Usage", (int)WTexConvUsage::NormalMap_Inverted);
  }
  else if (sMode == "TextureImport.HDR")
  {
    accessor.SetValue("Usage", (int)WTexConvUsage::Hdr);
  }
  else if (sMode == "TextureImport.Linear")
  {
  }
  else if (sMode == "TextureImport.Occlusion")
  {
    accessor.SetValue("ChannelMapping", (int)WTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)WTextureFilterSetting::LowestQuality);
  }
  else if (sMode == "TextureImport.Height")
  {
    accessor.SetValue("ChannelMapping", (int)WTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)WTextureFilterSetting::LowQuality);
  }
  else if (sMode == "TextureImport.Roughness")
  {
    accessor.SetValue("ChannelMapping", (int)WTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)WTextureFilterSetting::LowQuality);
  }
  else if (sMode == "TextureImport.Metalness")
  {
    accessor.SetValue("ChannelMapping", (int)WTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)WTextureFilterSetting::LowQuality);
  }
  else if (sMode == "TextureImport.ORM")
  {
    accessor.SetValue("ChannelMapping", (int)WTexture2DChannelMappingEnum::RGB1);
    accessor.SetValue("TextureFilter", (int)WTextureFilterSetting::LowQuality);
  }

  WLog::Success("Imported texture: '{}'", sOutFile);

  return WStatus(W_SUCCESS);
}
