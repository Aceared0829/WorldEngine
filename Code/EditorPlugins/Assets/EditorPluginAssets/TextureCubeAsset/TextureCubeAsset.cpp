#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureCubeAssetDocument, 3, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("ChannelMode", WTextureChannelMode, m_ChannelMode),
    W_MEMBER_PROPERTY("TextureLod", m_iTextureLod),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const char* ToFilterMode(WTextureFilterSetting::Enum mode);
const char* ToUsageMode(WTexConvUsage::Enum mode);
const char* ToCompressionMode(WTexConvCompressionMode::Enum mode);
const char* ToMipmapMode(WTexConvMipmapMode::Enum mode);

WTextureCubeAssetDocument::WTextureCubeAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WTextureCubeAssetProperties>(sDocumentPath, WAssetDocEngineConnection::Simple)
{
}

WStatus WTextureCubeAssetDocument::RunTexConv(const char* szTargetFile, const WAssetFileHeader& AssetHeader, bool bUpdateThumbnail)
{
  const WTextureCubeAssetProperties* pProp = GetProperties();

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

  if (pProp->m_TextureUsage == WTexConvUsage::Hdr)
  {
    arguments << "-hdrExposure";
    temp.SetFormat("{0}", WArgF(pProp->m_fHdrExposureBias, 2));
    arguments << temp.GetData();
  }

  // TODO: downscale steps and min/max resolution

  arguments << "-mipmaps";
  arguments << ToMipmapMode(pProp->m_MipmapMode);

  arguments << "-compression";
  arguments << ToCompressionMode(pProp->m_CompressionMode);

  arguments << "-usage";
  arguments << ToUsageMode(pProp->m_TextureUsage);

  arguments << "-filter" << ToFilterMode(pProp->m_TextureFilter);

  arguments << "-type";
  arguments << "Cubemap";

  switch (pProp->m_ChannelMapping)
  {
    case WTextureCubeChannelMappingEnum::RGB1:
      arguments << "-rgb"
                << "in0";
      break;

    case WTextureCubeChannelMappingEnum::RGB1TO6:
      arguments << "-rgb0"
                << "in0";
      arguments << "-rgb1"
                << "in1";
      arguments << "-rgb2"
                << "in2";
      arguments << "-rgb3"
                << "in3";
      arguments << "-rgb4"
                << "in4";
      arguments << "-rgb5"
                << "in5";
      break;


    case WTextureCubeChannelMappingEnum::RGBA1:
      arguments << "-rgba"
                << "in0";
      break;

    case WTextureCubeChannelMappingEnum::RGBA1TO6:
      arguments << "-rgba0"
                << "in0";
      arguments << "-rgba1"
                << "in1";
      arguments << "-rgba2"
                << "in2";
      arguments << "-rgba3"
                << "in3";
      arguments << "-rgba4"
                << "in4";
      arguments << "-rgba5"
                << "in5";
      break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  const WInt32 iNumInputFiles = pProp->GetNumInputFiles();
  for (WInt32 i = 0; i < iNumInputFiles; ++i)
  {
    if (WStringUtils::IsNullOrEmpty(pProp->GetInputFile(i)))
      break;

    temp.SetFormat("-in{0}", i);
    arguments << temp.GetData();
    arguments << QString(pProp->GetAbsoluteInputFilePath(i).GetData());
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

void WTextureCubeAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  switch (GetProperties()->m_ChannelMapping)
  {
    case WTextureCubeChannelMappingEnum::RGB1:
    case WTextureCubeChannelMappingEnum::RGBA1:
    {
      // remove file dependencies, that aren't used
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile1());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile2());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile3());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile4());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile5());
      break;
    }

    case WTextureCubeChannelMappingEnum::RGB1TO6:
    case WTextureCubeChannelMappingEnum::RGBA1TO6:
      break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }
}

WTransformStatus WTextureCubeAssetDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  const bool bUpdateThumbnail = pAssetProfile == WAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

  WTransformStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail);

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

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureCubeAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WTextureCubeAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

WTextureCubeAssetDocumentGenerator::WTextureCubeAssetDocumentGenerator()
{
  AddSupportedFileType("dds");
  AddSupportedFileType("hdr");
  AddSupportedFileType("exr");

  // these formats would need to use 6 files for the faces
  // more elaborate detection and mapping would need to be implemented
  // AddSupportedFileType("tga");
  // AddSupportedFileType("jpg");
  // AddSupportedFileType("jpeg");
  // AddSupportedFileType("png");
}

WTextureCubeAssetDocumentGenerator::~WTextureCubeAssetDocumentGenerator() = default;

void WTextureCubeAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  if (sAbsInputFile.IsEmpty())
  {
    // called with an empty string to populate the "Import As" menu
    WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_sName = "CubemapImport.SkyboxAuto";
    info.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
    return;
  }

  const WStringBuilder baseFilename = sAbsInputFile.GetFileName();
  const bool isHDR = sAbsInputFile.HasExtension("hdr") || sAbsInputFile.HasExtension("exr");

  const bool isCubemap = ((baseFilename.FindSubString_NoCase("cubemap") != nullptr) || (baseFilename.FindSubString_NoCase("skybox") != nullptr));

  if (isHDR)
  {
    {
      WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
      info.m_Priority = isCubemap ? WAssetDocGeneratorPriority::HighPriority : WAssetDocGeneratorPriority::Undecided;
      info.m_sName = "CubemapImport.SkyboxHDR";
      info.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
    }
  }
  else
  {
    {
      WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
      info.m_Priority = isCubemap ? WAssetDocGeneratorPriority::HighPriority : WAssetDocGeneratorPriority::Undecided;
      info.m_sName = "CubemapImport.Skybox";
      info.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
    }
  }
}

WStatus WTextureCubeAssetDocumentGenerator::Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  const WStringBuilder sOutFile = GetImportTargetPath(sInputFileAbs);

  auto pApp = WQtEditorApp::GetSingleton();

  WStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  WDocument* pDoc = pApp->CreateDocument(sOutFile, WDocumentFlags::None);
  if (pDoc == nullptr)
    return WStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  WTextureCubeAssetDocument* pAssetDoc = WDynamicCast<WTextureCubeAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", sInputFileRel.GetView());
  accessor.SetValue("ChannelMapping", (int)WTextureCubeChannelMappingEnum::RGB1);

  if (sMode == "CubemapImport.SkyboxAuto")
  {
    const bool isHDR = sInputFileAbs.HasExtension("hdr") || sInputFileAbs.HasExtension("exr");
    if (isHDR)
    {
      sMode = "CubemapImport.SkyboxHDR";
    }
    else
    {
      sMode = "CubemapImport.Skybox";
    }
  }

  if (sMode == "CubemapImport.SkyboxHDR")
  {
    accessor.SetValue("Usage", (int)WTexConvUsage::Hdr);
  }
  else if (sMode == "CubemapImport.Skybox")
  {
    accessor.SetValue("Usage", (int)WTexConvUsage::Color);
  }

  WLog::Success("Imported cubemap: '{}'", sOutFile);

  return WStatus(W_SUCCESS);
}
