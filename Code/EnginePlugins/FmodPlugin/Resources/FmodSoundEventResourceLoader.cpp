#include <FmodPlugin/FmodPluginPCH.h>

#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

WResourceLoadData WFmodSoundEventResourceLoader::OpenDataStream(const WResource* pResource)
{
  W_LOG_BLOCK("WFmodSoundEventResourceLoader::OpenDataStream", pResource->GetResourceID());

  LoadedData* pData = W_DEFAULT_NEW(LoadedData);

  WResourceLoadData res;

  if (WFmod::GetSingleton()->GetStudioSystem() == nullptr)
  {
    WLog::Warning("FMOD not initialized, ignoring sound event.");
    return res;
  }

  WStringBuilder sResID;
  WFileSystem::ResolveAssetRedirection(pResource->GetResourceID(), sResID);

  const char* szSeperator = sResID.FindSubString("|");

  if (szSeperator == nullptr)
  {
    WLog::Error("FMOD event resource ID is invalid or could not be resolved: '{0}'", sResID);
    return res;
  }

  WStringBuilder sBankPath, sSubPath;
  sBankPath.SetSubString_FromTo(sResID, szSeperator);
  sSubPath = szSeperator + 1;

  pData->m_hSoundBank = WResourceManager::LoadResource<WFmodSoundBankResource>(sBankPath);

  // make sure the sound bank is fully loaded before trying to get the event descriptor (even though we go through the FMOD 'system' the
  // bank resource must be loaded first)
  {
    WResourceLock<WFmodSoundBankResource> pBank(pData->m_hSoundBank, WResourceAcquireMode::BlockTillLoaded);

    if (WConversionUtils::IsStringUuid(sSubPath))
    {
      const WUuid guid = WConversionUtils::ConvertStringToUuid(sSubPath);
      const FMOD_GUID* fmodGuid = reinterpret_cast<const FMOD_GUID*>(&guid);

      if (WFmod::GetSingleton()->GetStudioSystem()->getEventByID(fmodGuid, &pData->m_pEventDescription) != FMOD_OK)
      {
        WLog::Error("FMOD event could not be found. GUID: '{0}'", sSubPath);
        return res;
      }
    }
    else
    {
      if (WFmod::GetSingleton()->GetStudioSystem()->getEvent(sSubPath.GetData(), &pData->m_pEventDescription) != FMOD_OK)
      {
        WLog::Error("FMOD event could not be found. Path: '{0}'", sSubPath);
        return res;
      }
    }
  }

  // make sure to load the sample data (on this thread)
  if (pData->m_pEventDescription->loadSampleData() != FMOD_OK)
  {
    WLog::Error("FMOD event sample data could not be loaded. Event: '{0}'", sSubPath);
    return res;
  }

  WFmodSoundBankResourceHandle* pHandle = &pData->m_hSoundBank;

  WMemoryStreamWriter w(&pData->m_Storage);
  w.WriteBytes(&pHandle, sizeof(WFmodSoundBankResourceHandle*)).IgnoreResult();
  w.WriteBytes(&pData->m_pEventDescription, sizeof(FMOD::Studio::EventDescription*)).IgnoreResult();

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void WFmodSoundEventResourceLoader::CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData)
{
  LoadedData* pData = (LoadedData*)loaderData.m_pCustomLoaderData;

  W_DEFAULT_DELETE(pData);
}

bool WFmodSoundEventResourceLoader::IsResourceOutdated(const WResource* pResource) const
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)

  // if the sound bank ever gets reloaded, the sound events may be invalid, so always reload all events
  /// \todo not sure whether this can be reduced to only reloading when the bank is outdated
  return true;

#else

  return false; // we cannot reload these resources without overhead, so only allow this during development

#endif
}


