#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WImageDataAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WImageDataAssetDocument::WImageDataAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WImageDataAssetProperties>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

WTransformStatus WImageDataAssetDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  const bool bUpdateThumbnail = pAssetProfile == WAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

  WStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail);

  WFileStats stat;
  if (WOSFile::GetFileStats(szTargetFile, stat).Succeeded() && stat.m_uiFileSize == 0)
  {
    // if the file was touched, but nothing written to it, delete the file
    // might happen if TexConv crashed or had an error
    WOSFile::DeleteFile(szTargetFile).IgnoreResult();
    result = WStatus(WFmt("File does not exist: {}", szTargetFile));
  }

  if (result.Succeeded())
  {
    WImageDataAssetEvent e;
    e.m_Type = WImageDataAssetEvent::Type::Transformed;
    m_Events.Broadcast(e);
  }

  return result;
}

WStatus WImageDataAssetDocument::RunTexConv(const char* szTargetFile, const WAssetFileHeader& AssetHeader, bool bUpdateThumbnail)
{
  const WImageDataAssetProperties* pProp = GetProperties();

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

  arguments << "-mipmaps";
  arguments << "None";

  arguments << "-type";
  arguments << "2D";

  arguments << "-compression";
  arguments << "None";

  arguments << "-usage";
  arguments << "Linear";

  // arguments << "-maxRes" << QString::number(pAssetConfig->m_uiMaxResolution);


  {
    arguments << "-in0";

    WStringBuilder sPath = pProp->m_sInputFile;
    sPath.MakeCleanPath();

    if (!sPath.IsAbsolutePath())
    {
      WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
    }

    arguments << QString(sPath.GetData());
  }

  arguments << "-rgba";
  arguments << "in0.rgba";

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
