#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetManager.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetWindow.moc.h>
#include <FmodPlugin/FmodIncludes.h>
#include <Foundation/IO/OSFile.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSoundBankAssetDocumentManager, 1, WRTTIDefaultAllocator<WSoundBankAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class WSimpleFmod
{
public:
  WSimpleFmod() = default;
  ~WSimpleFmod() { W_ASSERT_DEV(m_pSystem == nullptr, "FMod is not shut down"); }

  void Startup()
  {
    W_ASSERT_DEV(m_pSystem == nullptr, "FMod is not shut down");

    W_FMOD_ASSERT(FMOD::Studio::System::create(&m_pSystem));

    void* extraDriverData = nullptr;
    W_FMOD_ASSERT(m_pSystem->initialize(32, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extraDriverData));
  }

  void Shutdown()
  {
    if (m_pSystem == nullptr)
      return;

    W_FMOD_ASSERT(m_pSystem->unloadAll());
    W_FMOD_ASSERT(m_pSystem->release());

    m_pSystem = nullptr;
  }

  FMOD::Studio::System* GetSystem()
  {
    if (m_pSystem == nullptr)
      Startup();

    return m_pSystem;
  }

private:
  FMOD::Studio::System* m_pSystem = nullptr;
};


WSoundBankAssetDocumentManager::WSoundBankAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WSoundBankAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Sound Bank";
  m_DocTypeDesc.m_sFileExtension = "WSoundBankAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Sound_Bank.svg";
  m_DocTypeDesc.m_sAssetCategory = "Sound";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WSoundBankAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Fmod_Bank");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinFmodSoundBank";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::None;

  WQtImageCache::GetSingleton()->RegisterTypeImage("Sound Bank", QPixmap(":/AssetIcons/Sound_Bank.svg"));

  m_pFmod = W_DEFAULT_NEW(WSimpleFmod);
}

WSoundBankAssetDocumentManager::~WSoundBankAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WSoundBankAssetDocumentManager::OnDocumentManagerEvent, this));

  m_pFmod->Shutdown();
  m_pFmod.Clear();
}

void WSoundBankAssetDocumentManager::FillOutSubAssetList(const WAssetDocumentInfo& assetInfo, WDynamicArray<WSubAssetData>& out_subAssets) const
{
  W_PROFILE_SCOPE("GetSoundBankSubAssets");

  SoundBankCache& cache = m_Cache[assetInfo.m_DocumentID];
  const WTimestamp lastTS = cache.m_LastModification;
  bool bCanEarlyOut = true;

  for (const WString& dep : assetInfo.m_TransformDependencies)
  {
    if (!WPathUtils::HasExtension(dep, "bank"))
      continue;

    {
      WString sAssetFile = dep;
      if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
        continue;

      WFileStats stat;
      if (WOSFile::GetFileStats(sAssetFile, stat).Succeeded())
      {
        if (stat.m_LastModificationTime.Compare(cache.m_LastModification, WTimestamp::CompareMode::Newer))
        {
          cache.m_LastModification = stat.m_LastModificationTime;
        }

        if (stat.m_LastModificationTime.Compare(lastTS, WTimestamp::CompareMode::Newer))
        {
          bCanEarlyOut = false;
        }
      }
    }
  }

  if (bCanEarlyOut)
  {
    out_subAssets = cache.m_CachedSubAssets;
    return;
  }

  cache.m_CachedSubAssets.Clear();

  WHashedString sAssetsDocumentTypeName;
  sAssetsDocumentTypeName.Assign("Sound Event");

  auto* pSystem = m_pFmod->GetSystem();
  WTempHybridArray<FMOD::Studio::Bank*, 16> loadedBanks;


  // TODO: it is unclear whether the code below can produce deadlocks, because of locked soundbank files on disk
  // in theory, since m_TransformDependencies is alphabetically sorted, the access order should always be the same, and thus
  // no deadlock should be possible,

  for (const WString& dep : assetInfo.m_TransformDependencies)
  {
    if (!WPathUtils::HasExtension(dep, "bank"))
      continue;

    WString sAssetFile = dep;
    if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
      continue;

    FMOD::Studio::Bank* pBank = nullptr;
    auto res = pSystem->loadBankFile(sAssetFile, FMOD_STUDIO_LOAD_BANK_NORMAL, &pBank);
    if (res != FMOD_OK || pBank == nullptr)
      continue;

    loadedBanks.PushBack(pBank);

    WStringBuilder sStringsBank = sAssetFile;
    sStringsBank.PathParentDirectory();
    sStringsBank.AppendPath("*.strings.bank");

    // honestly we have no idea what the strings bank name should be
    // and if there are multiple, which one is the correct one
    // so we just load everything that we can find
    WFileSystemIterator fsIt;
    for (fsIt.StartSearch(sStringsBank, WFileSystemIteratorFlags::ReportFiles); fsIt.IsValid(); fsIt.Next())
    {
      sStringsBank = fsIt.GetCurrentPath();
      sStringsBank.AppendPath(fsIt.GetStats().m_sName);

      FMOD::Studio::Bank* pStringsBank = nullptr;
      if (pSystem->loadBankFile(sStringsBank, FMOD_STUDIO_LOAD_BANK_NORMAL, &pStringsBank) == FMOD_OK && pStringsBank != nullptr)
      {
        loadedBanks.PushBack(pStringsBank);
      }
    }

    int iEvents = 0;
    W_FMOD_ASSERT(pBank->getEventCount(&iEvents));

    if (iEvents > 0)
    {
      WDynamicArray<FMOD::Studio::EventDescription*> events;
      events.SetCountUninitialized(iEvents);

      pBank->getEventList(events.GetData(), iEvents, &iEvents);

      char szPath[256];
      int iLen;

      FMOD_GUID guid;

      WStringBuilder sGuid, sGuidNoSpace, sEventName;

      for (WUInt32 i = 0; i < events.GetCount(); ++i)
      {
        iLen = 0;
        W_FMOD_ASSERT(events[i]->getPath(szPath, 255, &iLen));
        szPath[iLen] = '\0';

        sEventName = szPath;

        if (sEventName.StartsWith_NoCase("snapshot:/"))
          continue;

        if (sEventName.StartsWith_NoCase("event:/"))
          sEventName.Shrink(7, 0);
        else
        {
          WLog::Warning("Skipping unknown FMOD event type: '{0}", sEventName);
          continue;
        }

        events[i]->getID(&guid);

        WUuid* WGuid = reinterpret_cast<WUuid*>(&guid);
        WConversionUtils::ToString(*WGuid, sGuid);
        sGuidNoSpace = sGuid;
        sGuidNoSpace.ReplaceAll(" ", "");

        auto& sub = cache.m_CachedSubAssets.ExpandAndGetRef();
        sub.m_Guid = *WGuid;
        sub.m_sName = sEventName;
        sub.m_sSubAssetsDocumentTypeName = sAssetsDocumentTypeName;
      }
    }

    for (FMOD::Studio::Bank* pBank : loadedBanks)
    {
      W_FMOD_ASSERT(pBank->unload());
    }

    loadedBanks.Clear();
  }

  W_ASSERT_DEV(loadedBanks.IsEmpty(), "A soundbank wasn't unloaded.");

  out_subAssets = cache.m_CachedSubAssets;
}

WString WSoundBankAssetDocumentManager::GetSoundBankAssetTableEntry(const WSubAsset* pSubAsset, WStringView sDataDirectory, const WPlatformProfile* pAssetProfile) const
{
  // at the moment we don't reference the actual transformed asset file
  // instead we reference the source FMOD sound bank file
  // this makes development easier, as we don't need to wait for an asset transform before changes are available

  /// \todo For final release we should reference the transformed file, as it's the one that gets packaged etc.
  /// Maybe we should add another platform target for that ?

  // if (pAssetProfile == WAssetCurator::GetSingleton()->GetDevelopmentAssetProfile())
  {
    for (const WString& dep : pSubAsset->m_pAssetInfo->m_Info->m_TransformDependencies)
    {
      if (dep.EndsWith_NoCase(".bank") && !dep.EndsWith_NoCase(".strings.bank"))
      {
        WStringBuilder result;
        result.Set("?", dep); // ? is an option to tell the system to skip the redirection prefix and use the path as is
        return result;
      }
    }
  }
  // else
  //{
  //  SUPER::GetAssetTableEntry(pSubAsset, szDataDirectory, pAssetProfile);
  //}

  return WString();
}

WString WSoundBankAssetDocumentManager::GetAssetTableEntry(const WSubAsset* pSubAsset, WStringView sDataDirectory, const WPlatformProfile* pAssetProfile) const
{
  if (pSubAsset->m_bMainAsset)
  {
    return GetSoundBankAssetTableEntry(pSubAsset, sDataDirectory, pAssetProfile);
  }
  else
  {
    WStringBuilder result = GetSoundBankAssetTableEntry(pSubAsset, sDataDirectory, pAssetProfile);

    WStringBuilder sGuid;
    WConversionUtils::ToString(pSubAsset->m_Data.m_Guid, sGuid);

    result.Append("|", sGuid);

    return result;
  }
}

void WSoundBankAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WSoundBankAssetDocument>())
      {
        new WSoundBankAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;
    default:
      break;
  }
}

void WSoundBankAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WSoundBankAssetDocument(sPath);
}

void WSoundBankAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

WUInt64 WSoundBankAssetDocumentManager::ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
