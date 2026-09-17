#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Utilities/AssetFileHeader.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAssetDocumentManager, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WAssetDocumentManager::WAssetDocumentManager() = default;
WAssetDocumentManager::~WAssetDocumentManager() = default;

WStatus WAssetDocumentManager::CloneDocument(WStringView sPath, WStringView sClonePath, WUuid& inout_cloneGuid)
{
  WStatus res = SUPER::CloneDocument(sPath, sClonePath, inout_cloneGuid);
  if (res.Succeeded())
  {
    // Cloned documents are usually opened right after cloning. To make sure this does not fail we need to inform the asset curator of the newly added asset document.
    WAssetCurator::GetSingleton()->NotifyOfFileChange(sClonePath);
  }
  return res;
}

void WAssetDocumentManager::ComputeAssetProfileHash(const WPlatformProfile* pAssetProfile)
{
  m_uiAssetProfileHash = ComputeAssetProfileHashImpl(DetermineFinalTargetProfile(pAssetProfile));

  if (GeneratesProfileSpecificAssets())
  {
    W_ASSERT_DEBUG(m_uiAssetProfileHash != 0, "Assets that generate a profile-specific output must compute a hash for the profile settings.");
  }
  else
  {
    W_ASSERT_DEBUG(m_uiAssetProfileHash == 0, "Only assets that generate per-profile outputs may specify an asset profile hash.");
    m_uiAssetProfileHash = 0;
  }
}

WUInt64 WAssetDocumentManager::ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const
{
  return 0;
}

WStatus WAssetDocumentManager::ReadAssetDocumentInfo(WUniquePtr<WAssetDocumentInfo>& out_pInfo, WStreamReader& inout_stream) const
{
  WAbstractObjectGraph graph;

  if (WAbstractGraphDdlSerializer::ReadHeader(inout_stream, &graph).Failed())
    return WStatus("Failed to read asset document");

  WRttiConverterContext context;
  WRttiConverterReader rttiConverter(&graph, &context);

  auto* pHeaderNode = graph.GetNodeByName("Header");

  if (pHeaderNode == nullptr)
    return WStatus("Document does not contain a 'Header'");

  WAssetDocumentInfo* pEntry = rttiConverter.CreateObjectFromNode(pHeaderNode).Cast<WAssetDocumentInfo>();
  W_ASSERT_DEBUG(pEntry != nullptr, "Failed to deserialize WAssetDocumentInfo!");
  out_pInfo = WUniquePtr<WAssetDocumentInfo>(pEntry, WFoundation::GetDefaultAllocator());
  return WStatus(W_SUCCESS);
}

WString WAssetDocumentManager::GenerateResourceThumbnailPath(WStringView sDocumentPath, WStringView sSubAssetName)
{
  WStringBuilder sRelativePath;
  if (sSubAssetName.IsEmpty())
  {
    sRelativePath = sDocumentPath;
  }
  else
  {
    sRelativePath = sDocumentPath.GetFileDirectory();

    WStringBuilder sValidFileName;
    WPathUtils::MakeValidFilename(sSubAssetName, '_', sValidFileName);
    sRelativePath.AppendPath(sValidFileName);
  }

  WString sProjectDir = WAssetCurator::GetSingleton()->FindDataDirectoryForAsset(sRelativePath);

  sRelativePath.MakeRelativeTo(sProjectDir).IgnoreResult();
  sRelativePath.Append(".jpg");

  WStringBuilder sFinalPath(sProjectDir, "/AssetCache/Thumbnails/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

bool WAssetDocumentManager::IsThumbnailUpToDate(WStringView sDocumentPath, WStringView sSubAssetName, WUInt64 uiThumbnailHash, WUInt32 uiTypeVersion)
{
  CURATOR_PROFILE(szDocumentPath);
  WString sThumbPath = GenerateResourceThumbnailPath(sDocumentPath, sSubAssetName);
  WFileReader file;
  if (file.Open(sThumbPath, 256).Failed())
    return false;

  WAssetDocument::ThumbnailInfo thumbnailInfo;

  const WUInt64 uiHeaderSize = thumbnailInfo.GetSerializedSize();
  WUInt64 uiFileSize = file.GetFileSize();

  if (uiFileSize < uiHeaderSize)
    return false;

  file.SkipBytes(uiFileSize - uiHeaderSize);

  if (thumbnailInfo.Deserialize(file).Failed())
  {
    return false;
  }

  return thumbnailInfo.IsThumbnailUpToDate(uiThumbnailHash, uiTypeVersion);
}

void WAssetDocumentManager::AddEntriesToAssetTable(WStringView sDataDirectory, const WPlatformProfile* pAssetProfile, WDelegate<void(WStringView sGuid, WStringView sPath, WStringView sType)> addEntry) const {}

WString WAssetDocumentManager::GetAssetTableEntry(const WSubAsset* pSubAsset, WStringView sDataDirectory, const WPlatformProfile* pAssetProfile) const
{
  return GetRelativeOutputFileName(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor, sDataDirectory, pSubAsset->m_pAssetInfo->m_Path, "", pAssetProfile);
}

WString WAssetDocumentManager::GetAbsoluteOutputFileName(const WAssetDocumentTypeDescriptor* pTypeDesc, WStringView sDocumentPath, WStringView sOutputTag, const WPlatformProfile* pAssetProfile) const
{
  WStringBuilder sProjectDir = WAssetCurator::GetSingleton()->FindDataDirectoryForAsset(sDocumentPath);

  WString sRelativePath = GetRelativeOutputFileName(pTypeDesc, sProjectDir, sDocumentPath, sOutputTag, pAssetProfile);
  WStringBuilder sFinalPath(sProjectDir, "/AssetCache/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

WString WAssetDocumentManager::GetRelativeOutputFileName(const WAssetDocumentTypeDescriptor* pTypeDesc, WStringView sDataDirectory, WStringView sDocumentPath, WStringView sOutputTag, const WPlatformProfile* pAssetProfile) const
{
  const WPlatformProfile* pPlatform = WAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile);
  W_ASSERT_DEBUG(sOutputTag.IsEmpty(), "The output tag '{}' for '{}' is not supported, override GetRelativeOutputFileName", sOutputTag, sDocumentPath);

  WStringBuilder sRelativePath(sDocumentPath);
  sRelativePath.MakeRelativeTo(sDataDirectory).IgnoreResult();
  GenerateOutputFilename(sRelativePath, pPlatform, pTypeDesc->m_sResourceFileExtension, GeneratesProfileSpecificAssets());

  return sRelativePath;
}

WResult WAssetDocumentManager::ReadAssetInfoFile(WAssetInfoFile& out_info, const WAssetDocumentTypeDescriptor* pTypeDesc, WStringView sDocumentPath, WUInt64 uiHash, WStringView sOutputTag, const WPlatformProfile* pAssetProfile) const
{
  const WString sOutputFile = GetAbsoluteOutputFileName(pTypeDesc, sDocumentPath, sOutputTag, pAssetProfile);
  const WStringBuilder sInfoFile = WAssetInfoFile::GetInfoFilePathForOutput(sOutputFile);

  return out_info.ReadFromFile(sInfoFile, uiHash, static_cast<WUInt16>(pTypeDesc->m_pDocumentType->GetTypeVersion()));
}

bool WAssetDocumentManager::IsOutputUpToDate(WStringView sDocumentPath, const WDynamicArray<WString>& outputs, WUInt64 uiHash, const WAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  CURATOR_PROFILE(sDocumentPath);
  if (!IsOutputUpToDate(sDocumentPath, "", uiHash, pTypeDescriptor))
    return false;

  for (const WString& sOutput : outputs)
  {
    if (!IsOutputUpToDate(sDocumentPath, sOutput, uiHash, pTypeDescriptor))
      return false;
  }
  return true;
}

bool WAssetDocumentManager::IsOutputUpToDate(WStringView sDocumentPath, WStringView sOutputTag, WUInt64 uiHash, const WAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  const WString sTargetFile = GetAbsoluteOutputFileName(pTypeDescriptor, sDocumentPath, sOutputTag);
  return WAssetDocumentManager::IsResourceUpToDate(sTargetFile, uiHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion());
}

const WPlatformProfile* WAssetDocumentManager::DetermineFinalTargetProfile(const WPlatformProfile* pAssetProfile)
{
  if (pAssetProfile == nullptr)
  {
    return WAssetCurator::GetSingleton()->GetActiveAssetProfile();
  }

  return pAssetProfile;
}

WResult WAssetDocumentManager::TryOpenAssetDocument(const char* szPathOrGuid)
{
  WAssetCurator::WLockedSubAsset pSubAsset;

  if (WConversionUtils::IsStringUuid(szPathOrGuid))
  {
    WUuid matGuid;
    matGuid = WConversionUtils::ConvertStringToUuid(szPathOrGuid);

    pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(matGuid);
  }
  else
  {
    // I think this is even wrong, either the string is a GUID, or it is not an asset at all, in which case we cannot find it this way
    // either left as an exercise for whoever needs non-asset references
    pSubAsset = WAssetCurator::GetSingleton()->FindSubAsset(szPathOrGuid);
  }

  if (pSubAsset)
  {
    WQtEditorApp::GetSingleton()->OpenDocumentQueued(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
    return W_SUCCESS;
  }

  return W_FAILURE;
}

bool WAssetDocumentManager::IsResourceUpToDate(const char* szResourceFile, WUInt64 uiHash, WUInt16 uiTypeVersion)
{
  CURATOR_PROFILE(szResourceFile);
  WFileReader file;
  if (file.Open(szResourceFile, 256).Failed())
    return false;

  // this might happen if writing to the file failed
  if (file.GetFileSize() == 0)
    return false;

  WAssetFileHeader AssetHeader;
  AssetHeader.Read(file).IgnoreResult();

  return AssetHeader.IsFileUpToDate(uiHash, uiTypeVersion);
}

void WAssetDocumentManager::GenerateOutputFilename(WStringBuilder& inout_sRelativeDocumentPath, const WPlatformProfile* pAssetProfile, const char* szExtension, bool bPlatformSpecific)
{
  inout_sRelativeDocumentPath.ChangeFileExtension(szExtension);
  inout_sRelativeDocumentPath.MakeCleanPath();

  if (bPlatformSpecific)
  {
    const WPlatformProfile* pPlatform = WAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile);
    inout_sRelativeDocumentPath.Prepend(pPlatform->GetConfigName(), "/");
  }
  else
    inout_sRelativeDocumentPath.Prepend("Common/");
}
