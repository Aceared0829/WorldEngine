#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSoundBankAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSoundBankAssetProperties, 1, WRTTIDefaultAllocator<WSoundBankAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SoundBankFile", m_sSoundBank)->AddAttributes(new WFileBrowserAttribute("Select SoundBank", "*.bank"), new WRequiredAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSoundBankAssetDocument::WSoundBankAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WSoundBankAssetProperties>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

void WSoundBankAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const WSoundBankAssetProperties* pProp = GetProperties();

  pInfo->m_TransformDependencies.Insert(pProp->m_sSoundBank);
}

WTransformStatus WSoundBankAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  const WSoundBankAssetProperties* pProp = GetProperties();

  if (pProp->m_sSoundBank.IsEmpty())
    return WStatus("No sound-bank file has been specified.");

  if (!WPathUtils::HasExtension(pProp->m_sSoundBank, "bank"))
    return WStatus(WFmt("Specified sound-bank file should have 'bank' extension: '{0}'", pProp->m_sSoundBank));

  /// \todo For platform specific sound banks, adjust the path to point to the correct file

  WStringBuilder sAssetFile = pProp->m_sSoundBank;
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
    return WStatus(WFmt("Failed to make sound-bank path absolute: '{0}'", pProp->m_sSoundBank));

  WFileReader SoundBankFile;
  if (SoundBankFile.Open(sAssetFile).Failed())
    return WStatus(WFmt("Could not open sound-bank for reading: '{0}'", sAssetFile));

  // we copy the entire sound bank into our transformed asset
  // however, at least during development, we typically do not load the data from there,
  // but from the FMOD sound bank files directly, so that we do not need to wait for an asset transform

  WDefaultMemoryStreamStorage storage;

  // copy the file from disk into memory
  {
    WMemoryStreamWriter writer(&storage);

    WUInt8 Temp[4 * 1024];

    while (true)
    {
      WUInt64 uiRead = SoundBankFile.ReadBytes(Temp, W_ARRAY_SIZE(Temp));

      if (uiRead == 0)
        break;

      writer.WriteBytes(Temp, uiRead).IgnoreResult();
    }
  }

  const WUInt8 uiVersion = 1;
  stream << uiVersion;

  // now store the entire file in our asset output
  stream << storage.GetStorageSize32();
  return storage.CopyToStream(stream);
}
