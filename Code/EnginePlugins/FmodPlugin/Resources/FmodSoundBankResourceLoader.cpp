#include <FmodPlugin/FmodPluginPCH.h>

#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/AssetFileHeader.h>

WResourceLoadData WFmodSoundBankResourceLoader::OpenDataStream(const WResource* pResource)
{
  W_LOG_BLOCK("WFmodSoundBankResourceLoader::OpenDataStream", pResource->GetResourceID());

  LoadedData* pData = W_DEFAULT_NEW(LoadedData);

  WResourceLoadData res;

  {
    WFileReader SoundBankAssetFile;
    if (SoundBankAssetFile.Open(pResource->GetResourceID()).Failed())
      return res;

    res.m_sResourceDescription = SoundBankAssetFile.GetFilePathRelative().GetData();
    WUInt32 uiSoundBankSize = 0;

    if (SoundBankAssetFile.GetFilePathRelative().EndsWith_NoCase("WFmodSoundBank")) // a transformed asset file
    {
      // skip the asset header
      WAssetFileHeader header;
      header.Read(SoundBankAssetFile).IgnoreResult();

      WUInt8 uiVersion = 0;
      SoundBankAssetFile >> uiVersion;

      W_ASSERT_DEV(uiVersion == 1, "Soundbank resource file version '{0}' is invalid", uiVersion);

      SoundBankAssetFile >> uiSoundBankSize;
    }
    else
    {
      // otherwise we assume it is directly an FMOD sound bank file
      uiSoundBankSize = (WUInt32)SoundBankAssetFile.GetFileSize();
    }

    if (uiSoundBankSize > 0)
    {
      pData->m_pSoundbankData = W_DEFAULT_NEW(WDataBuffer);
      pData->m_pSoundbankData->SetCountUninitialized(uiSoundBankSize + FMOD_STUDIO_LOAD_MEMORY_ALIGNMENT);
      WUInt8* pAlignedData = WMemoryUtils::AlignBackwards(pData->m_pSoundbankData->GetData() + FMOD_STUDIO_LOAD_MEMORY_ALIGNMENT, FMOD_STUDIO_LOAD_MEMORY_ALIGNMENT);

      SoundBankAssetFile.ReadBytes(pAlignedData, uiSoundBankSize);

      // The FMOD documentation says it is fully thread-safe, so I assume we can call loadBankMemory at any time
      auto pStudio = WFmod::GetSingleton()->GetStudioSystem();

      // this happens when FMOD is not properly configured
      if (pStudio == nullptr)
        return res;

      auto fmodRes = pStudio->loadBankMemory((const char*)pAlignedData, (int)uiSoundBankSize, FMOD_STUDIO_LOAD_MEMORY_POINT, FMOD_STUDIO_LOAD_BANK_NORMAL, &pData->m_pSoundBank);

      // if this fails with res == FMOD_ERR_NOTREADY, that might be because two processes using FMOD are running and both have the
      // FMOD_STUDIO_INIT_LIVEUPDATE flag set somehow FMOD cannot handle this and bank loading then fails
      if (fmodRes != FMOD_OK)
      {
        // if this fails with FMOD_ERR_EVENT_ALREADY_LOADED we might have attempted to load the bank multiple times in parallel
        // this is not a problem, we just discard the failed second attempt
        if (fmodRes != FMOD_ERR_EVENT_ALREADY_LOADED)
        {
          WLog::Error("Error '{1}' loading FMOD sound bank '{0}'", SoundBankAssetFile.GetFilePathRelative().GetData(), (WInt32)fmodRes);
        }

        W_DEFAULT_DELETE(pData->m_pSoundbankData);
        W_DEFAULT_DELETE(pData);

        res.m_pCustomLoaderData = nullptr;
        res.m_pDataStream = nullptr;
        return res;
      }
    }

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
    {
      WFileStats stat;
      if (WFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
      {
        res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
      }
    }
#endif
  }

  WMemoryStreamWriter w(&pData->m_Storage);

  w.WriteBytes(&pData->m_pSoundBank, sizeof(FMOD::Studio::Bank*)).IgnoreResult();
  w.WriteBytes(&pData->m_pSoundbankData, sizeof(WDataBuffer*)).IgnoreResult();

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void WFmodSoundBankResourceLoader::CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData)
{
  LoadedData* pData = (LoadedData*)loaderData.m_pCustomLoaderData;

  W_DEFAULT_DELETE(pData);
}

bool WFmodSoundBankResourceLoader::IsResourceOutdated(const WResource* pResource) const
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)

  // don't try to reload a file that cannot be found
  WStringBuilder sAbs;
  if (WFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#  if W_ENABLED(W_SUPPORTS_FILE_STATS)
  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    WFileStats stat;
    if (WFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), WTimestamp::CompareMode::FileTimeEqual);
  }

#  endif

  return true;

#else

  return false; // we cannot reload these resources without overhead, so only allow this during development

#endif
}


