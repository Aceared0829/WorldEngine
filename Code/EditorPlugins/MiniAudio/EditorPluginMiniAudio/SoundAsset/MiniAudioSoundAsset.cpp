#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMiniAudioSoundAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMiniAudioSoundAssetProperties, 1, WRTTIDefaultAllocator<WMiniAudioSoundAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Files", m_SoundFiles)->AddAttributes(new WFileBrowserAttribute("Select Sound", "*.wav;*.mp3"), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("Group", m_sGroup)->AddAttributes(new WDynamicStringEnumAttribute("MiniAudioSoundGroups")),
    W_MEMBER_PROPERTY("Loop", m_bLoop),
    W_MEMBER_PROPERTY("MinRandomVolume", m_fMinVolume)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_MEMBER_PROPERTY("MaxRandomVolume", m_fMaxVolume)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_MEMBER_PROPERTY("MinRandomPitch", m_fMinPitch)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_MEMBER_PROPERTY("MaxRandomPitch", m_fMaxPitch)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_MEMBER_PROPERTY("IsPositional", m_bSpatialize)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("DopplerFactor", m_fDopplerFactor)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 10.0f)),
    W_MEMBER_PROPERTY("SoundSize", m_fMinDistance)->AddAttributes(new WDefaultValueAttribute(0.1f), new WClampValueAttribute(0.01f, 100.0f)),
    // W_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(1.0f, 1000.0f)),
    W_MEMBER_PROPERTY("Rolloff", m_fRolloff)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.001f, 1000.0f)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMiniAudioSoundAssetDocument::WMiniAudioSoundAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WMiniAudioSoundAssetProperties>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

void WMiniAudioSoundAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const WMiniAudioSoundAssetProperties* pProp = GetProperties();

  for (const auto& str : pProp->m_SoundFiles)
  {
    pInfo->m_TransformDependencies.Insert(str);
  }
}

WTransformStatus WMiniAudioSoundAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  const WMiniAudioSoundAssetProperties* pProp = GetProperties();

  if (pProp->m_SoundFiles.IsEmpty())
    return WStatus("No sound files have been specified.");

  const WUInt8 uiVersion = 2;
  stream << uiVersion;

  stream << pProp->m_bLoop;
  stream << pProp->m_fMinVolume;
  stream << pProp->m_fMaxVolume;
  stream << pProp->m_fMinPitch;
  stream << pProp->m_fMaxPitch;
  stream << pProp->m_bSpatialize;
  stream << pProp->m_fMinDistance;
  stream << pProp->m_fMaxDistance;
  stream << pProp->m_fRolloff;
  stream << pProp->m_fDopplerFactor;
  stream << pProp->m_SoundFiles.GetCount();

  for (const auto& sFile : pProp->m_SoundFiles)
  {
    WStringBuilder sAssetFile = sFile;
    if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
      return WStatus(WFmt("Failed to make sound file path absolute: '{0}'", sFile));

    WFileReader SoundFile;
    if (SoundFile.Open(sAssetFile).Failed())
      return WStatus(WFmt("Could not open sound-file for reading: '{0}'", sAssetFile));

    // we copy the entire sound into our transformed asset

    WDefaultMemoryStreamStorage storage;

    // copy the file from disk into memory
    {
      WMemoryStreamWriter writer(&storage);

      WUInt8 Temp[4 * 1024];

      while (true)
      {
        WUInt64 uiRead = SoundFile.ReadBytes(Temp, W_ARRAY_SIZE(Temp));

        if (uiRead == 0)
          break;

        writer.WriteBytes(Temp, uiRead).IgnoreResult();
      }
    }

    // now store the entire file in our asset output
    stream << storage.GetStorageSize32();
    W_SUCCEED_OR_RETURN(storage.CopyToStream(stream));
  }

  // version 2
  stream << pProp->m_sGroup;

  return WStatus(W_SUCCESS);
}

void WMiniAudioSoundAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WMiniAudioSoundAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool bIsPositional = e.m_pObject->GetTypeAccessor().GetValue("IsPositional").ConvertTo<bool>();

    props["DopplerFactor"].m_Visibility = bIsPositional ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SoundSize"].m_Visibility = bIsPositional ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["Rolloff"].m_Visibility = bIsPositional ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
}


//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMiniAudioSoundAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WMiniAudioSoundAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

WMiniAudioSoundAssetDocumentGenerator::WMiniAudioSoundAssetDocumentGenerator()
{
  AddSupportedFileType("wav");
  AddSupportedFileType("mp3");
}

WMiniAudioSoundAssetDocumentGenerator::~WMiniAudioSoundAssetDocumentGenerator() = default;

void WMiniAudioSoundAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = WAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "MiniAudio_Sound";
    info.m_sIcon = ":/AssetIcons/MiniAudioSound.svg";
  }
}

WStatus WMiniAudioSoundAssetDocumentGenerator::Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  const WStringBuilder sOutFile = GetImportTargetPath(sInputFileAbs);

  auto pApp = WQtEditorApp::GetSingleton();

  WStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  WDocument* pDoc = pApp->CreateDocument(sOutFile, WDocumentFlags::None);
  if (pDoc == nullptr)
    return WStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  WMiniAudioSoundAssetDocument* pAssetDoc = WDynamicCast<WMiniAudioSoundAssetDocument*>(pDoc);
  if (pAssetDoc == nullptr)
    return WStatus("Target document is not a valid WMiniAudioSoundAssetDocument");

  auto pPropObj = pAssetDoc->GetPropertyObject();

  WObjectCommandAccessor ca(pAssetDoc->GetCommandHistory());
  ca.StartTransaction("Init Values");

  ca.InsertValueByName(pPropObj, "Files", sInputFileRel.GetView(), 0).AssertSuccess();

  ca.FinishTransaction();

  WLog::Success("Imported sound: '{}'", sOutFile);

  return WStatus(W_SUCCESS);
}
