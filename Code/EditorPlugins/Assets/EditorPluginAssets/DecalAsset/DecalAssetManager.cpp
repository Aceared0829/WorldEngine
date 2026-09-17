#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetWindow.moc.h>
#include <Texture/Utils/TextureAtlasDesc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

const char* ToCompressionMode(WTexConvCompressionMode::Enum mode);
const char* ToMipmapMode(WTexConvMipmapMode::Enum mode);

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDecalAssetDocumentManager, 1, WRTTIDefaultAllocator<WDecalAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WDecalAssetDocumentManager::WDecalAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WDecalAssetDocumentManager::OnDocumentManagerEvent, this));

  // texture asset source files
  WAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  WAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");

  m_DocTypeDesc.m_sDocumentTypeName = "Decal";
  m_DocTypeDesc.m_sFileExtension = "WDecalAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Decal.svg";
  m_DocTypeDesc.m_sAssetCategory = "Effects";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WDecalAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Decal");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinDecal";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::SupportsThumbnail;
}

WDecalAssetDocumentManager::~WDecalAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WDecalAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WDecalAssetDocumentManager::AddEntriesToAssetTable(WStringView sDataDirectory, const WPlatformProfile* pAssetProfile, WDelegate<void(WStringView sGuid, WStringView sPath, WStringView sType)> addEntry) const
{
  WStringBuilder projectDir = WToolsProject::GetSingleton()->GetProjectDirectory();
  projectDir.MakeCleanPath();
  projectDir.Append("/");

  if (projectDir.StartsWith_NoCase(sDataDirectory))
  {
    addEntry("{ ProjectDecalAtlas }", "Default/Decals.WBinTextureAtlas", "Decal Atlas");
  }
}

WString WDecalAssetDocumentManager::GetAssetTableEntry(const WSubAsset* pSubAsset, WStringView sDataDirectory, const WPlatformProfile* pAssetProfile) const
{
  // means NO table entry will be written, because for decals we don't need a redirection
  return WString();
}

void WDecalAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WDecalAssetDocument>())
      {
        new WQtDecalAssetDocumentWindow(static_cast<WDecalAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WDecalAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WDecalAssetDocument(sPath);
}

void WDecalAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

WUInt64 WDecalAssetDocumentManager::ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}

WStatus WDecalAssetDocumentManager::GenerateDecalTexture(const WPlatformProfile* pAssetProfile)
{
  WAssetCurator* pCurator = WAssetCurator::GetSingleton();
  const auto& allAssets = pCurator->GetKnownSubAssets();

  WUInt64 uiAssetHash = 1;

  for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
  {
    const auto& asset = it.Value();

    if (asset.m_pAssetInfo->GetManager() != this)
      continue;

    uiAssetHash += pCurator->GetAssetTransformHash(it.Key());
  }

  // the atlas has to be regenerated when the asset types change, because that may change the layout of the generated file
  WUInt16 uiAssetVersion = WGetStaticRTTI<WDecalAssetDocument>()->GetTypeVersion() & 0xFF;
  uiAssetVersion |= (WGetStaticRTTI<WDecalAssetProperties>()->GetTypeVersion() & 0xFF) << 8;

  WStringBuilder decalFile = WToolsProject::GetSingleton()->GetProjectDirectory();
  decalFile.AppendPath("AssetCache", GetDecalTexturePath(pAssetProfile));

  if (IsDecalTextureUpToDate(decalFile, uiAssetHash, uiAssetVersion))
    return WStatus(W_SUCCESS);

  WTextureAtlasCreationDesc atlasDesc;

  // find all decal assets, extract their file information to pass it along to TexConv
  {
    atlasDesc.m_Layers.SetCount(3);
    atlasDesc.m_Layers[0].m_Usage = WTexConvUsage::Color;
    atlasDesc.m_Layers[1].m_Usage = WTexConvUsage::NormalMap;
    atlasDesc.m_Layers[2].m_Usage = WTexConvUsage::Linear;
    atlasDesc.m_Layers[2].m_uiNumChannels = 3;

    atlasDesc.m_Items.Reserve(64);

    WQtEditorApp* pEditorApp = WQtEditorApp::GetSingleton();
    WStringBuilder sAbsPath;

    for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
    {
      const auto& asset = it.Value();

      if (asset.m_pAssetInfo->GetManager() != this)
        continue;

      W_LOG_BLOCK("Decal", asset.m_pAssetInfo->m_Path.GetDataDirParentRelativePath());

      // does the document already exist and is it open ?
      bool bWasOpen = false;
      WDocument* pDoc = GetDocumentByPath(asset.m_pAssetInfo->m_Path.GetAbsolutePath());
      if (pDoc)
        bWasOpen = true;
      else
        pDoc = pEditorApp->OpenDocument(asset.m_pAssetInfo->m_Path.GetAbsolutePath(), WDocumentFlags::None);

      if (pDoc == nullptr)
        return WStatus(WFmt("Could not open asset document '{0}'", asset.m_pAssetInfo->m_Path.GetDataDirParentRelativePath()));

      WDecalAssetDocument* pDecalAsset = static_cast<WDecalAssetDocument*>(pDoc);

      {
        auto& item = atlasDesc.m_Items.ExpandAndGetRef();

        // store the GUID as the decal identifier
        WConversionUtils::ToString(pDecalAsset->GetGuid(), sAbsPath);
        item.m_uiUniqueID = WHashingUtils::StringHashTo32(WHashingUtils::StringHash(sAbsPath));

        auto pDecalProps = pDecalAsset->GetProperties();
        item.m_uiFlags = 0;
        item.m_uiFlags |= pDecalProps->NeedsNormal() ? DECAL_USE_NORMAL : 0;
        item.m_uiFlags |= pDecalProps->NeedsORM() ? DECAL_USE_ORM : 0;
        item.m_uiFlags |= pDecalProps->NeedsEmissive() ? DECAL_USE_EMISSIVE : 0;
        item.m_uiFlags |= pDecalProps->m_bBlendModeColorize ? DECAL_BLEND_MODE_COLORIZE : 0;

        item.m_uiNumVariationsX = WMath::Max<WUInt8>(1, pDecalProps->m_uiNumVariationsX);
        item.m_uiNumVariationsY = WMath::Max<WUInt8>(1, pDecalProps->m_uiNumVariationsY);

        if (!pDecalProps->NeedsBaseColor() && pDecalProps->m_sAlphaMask.IsEmpty())
        {
          return WStatus("Decal has neither a base color nor an alpha mask texture");
        }

        if (!pDecalProps->m_sAlphaMask.IsEmpty())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sAlphaMask;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return WStatus(WFmt("Invalid alpha mask texture path '{0}'", sAbsPath));
          }

          item.m_sAlphaInput = sAbsPath;
        }

        if (pDecalProps->NeedsBaseColor())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sBaseColor;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return WStatus(WFmt("Invalid base color texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[0] = sAbsPath;
        }

        if (pDecalProps->NeedsNormal())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sNormal;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return WStatus(WFmt("Invalid normal texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[1] = sAbsPath;
        }

        if (pDecalProps->NeedsORM())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sORM;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return WStatus(WFmt("Invalid ORM texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[2] = sAbsPath;
        }

        if (pDecalProps->NeedsEmissive())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sEmissive;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return WStatus(WFmt("Invalid emissive texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[2] = sAbsPath;
        }
      }


      if (!pDoc->HasWindowBeenRequested() && !bWasOpen)
        pDoc->GetDocumentManager()->CloseDocument(pDoc);
    }
  }

  WAssetFileHeader header;
  header.SetFileHashAndVersion(uiAssetHash, uiAssetVersion);

  WStatus result(W_SUCCESS);

  // Send information to TexConv to do all the work
  {
    WStringBuilder texGroupFile = WToolsProject::GetSingleton()->GetProjectDirectory();
    texGroupFile.AppendPath("AssetCache", GetDecalTexturePath(pAssetProfile));
    texGroupFile.ChangeFileExtension("WDecalAtlasDesc");

    if (atlasDesc.Save(texGroupFile).Failed())
      return WStatus(WFmt("Failed to save texture atlas descriptor file '{0}'", texGroupFile));

    result = RunTexConv(decalFile, texGroupFile, header);
  }

  WFileStats stat;
  if (WOSFile::GetFileStats(decalFile, stat).Succeeded() && stat.m_uiFileSize == 0)
  {
    // if the file was touched, but nothing written to it, delete the file
    // might happen if TexConv crashed or had an error
    WOSFile::DeleteFile(decalFile).IgnoreResult();
    result = WStatus(WFmt("File does not exist: '{}'", decalFile));
  }

  return result;
}

bool WDecalAssetDocumentManager::IsDecalTextureUpToDate(const char* szDecalFile, WUInt64 uiAssetHash, WUInt16 uiAssetVersion) const
{
  WFileReader file;
  if (file.Open(szDecalFile).Succeeded())
  {
    WAssetFileHeader header;
    header.Read(file).IgnoreResult();

    return header.IsFileUpToDate(uiAssetHash, uiAssetVersion);
  }

  return false;
}

WString WDecalAssetDocumentManager::GetDecalTexturePath(const WPlatformProfile* pAssetProfile0) const
{
  const WPlatformProfile* pAssetProfile = WAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile0);
  WStringBuilder result = "Decals";
  GenerateOutputFilename(result, pAssetProfile, "WBinTextureAtlas", true);

  return result;
}

WStatus WDecalAssetDocumentManager::RunTexConv(const char* szTargetFile, const char* szInputFile, const WAssetFileHeader& AssetHeader)
{
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

  arguments << "-type";
  arguments << "Atlas";

  arguments << "-compression";
  arguments << ToCompressionMode(WTexConvCompressionMode::High);

  arguments << "-mipmaps";
  arguments << ToMipmapMode(WTexConvMipmapMode::Linear);

  arguments << "-atlasDesc";
  arguments << QString(szInputFile);

  W_SUCCEED_OR_RETURN(WQtEditorApp::GetSingleton()->ExecuteTool("WTexConv", arguments, 180, WLog::GetThreadLocalLogSystem()));

  return WStatus(W_SUCCESS);
}
