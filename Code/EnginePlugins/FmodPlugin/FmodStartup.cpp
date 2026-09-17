#include <FmodPlugin/FmodPluginPCH.h>

#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GameEngine/GameApplication/GameApplication.h>

static WFmodSoundBankResourceLoader s_SoundBankResourceLoader;
static WFmodSoundEventResourceLoader s_SoundEventResourceLoader;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(FMOD, FmodPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WResourceManager::SetResourceTypeLoader<WFmodSoundBankResource>(&s_SoundBankResourceLoader);
    WResourceManager::SetResourceTypeLoader<WFmodSoundEventResource>(&s_SoundEventResourceLoader);

    WResourceManager::RegisterResourceForAssetType("Sound Bank", WGetStaticRTTI<WFmodSoundBankResource>());
    WResourceManager::RegisterResourceForAssetType("Sound Event", WGetStaticRTTI<WFmodSoundEventResource>());

    {
      WFmodSoundEventResourceDescriptor desc;
      WFmodSoundEventResourceHandle hResource = WResourceManager::CreateResource<WFmodSoundEventResource>("FmodEventMissing", std::move(desc), "Fallback for missing sound event");
      WResourceManager::SetResourceTypeMissingFallback<WFmodSoundEventResource>(hResource);
    }

    {
      WFmodSoundBankResourceDescriptor desc;
      WFmodSoundBankResourceHandle hResource = WResourceManager::CreateResource<WFmodSoundBankResource>("FmodBankMissing", std::move(desc), "Fallback for missing sound bank");
      WResourceManager::SetResourceTypeMissingFallback<WFmodSoundBankResource>(hResource);
    }

    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(&WFmod::GameApplicationEventHandler);

    WFmod::GetSingleton()->Startup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(&WFmod::GameApplicationEventHandler);

    WFmod::GetSingleton()->Shutdown();
    WResourceManager::SetResourceTypeLoader<WFmodSoundBankResource>(nullptr);
    WResourceManager::SetResourceTypeLoader<WFmodSoundEventResource>(nullptr);

    WFmodSoundEventResource::CleanupDynamicPluginReferences();
    WFmodSoundBankResource::CleanupDynamicPluginReferences();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

void WFmodConfiguration::Save(WOpenDdlWriter& ref_ddl) const
{
  WOpenDdlUtils::StoreString(ref_ddl, m_sMasterSoundBank, "MasterBank");
  WOpenDdlUtils::StoreUInt16(ref_ddl, m_uiVirtualChannels, "VirtualChannels");
  WOpenDdlUtils::StoreUInt32(ref_ddl, m_uiSamplerRate, "SamplerRate");

  switch (m_SpeakerMode)
  {
    case WFmodSpeakerMode::ModeStereo:
      WOpenDdlUtils::StoreString(ref_ddl, "Stereo", "Mode");
      break;
    case WFmodSpeakerMode::Mode5Point1:
      WOpenDdlUtils::StoreString(ref_ddl, "5.1", "Mode");
      break;
    case WFmodSpeakerMode::Mode7Point1:
      WOpenDdlUtils::StoreString(ref_ddl, "7.1", "Mode");
      break;
  }
}

void WFmodConfiguration::Load(const WOpenDdlReaderElement& ddl)
{
  if (const WOpenDdlReaderElement* pElement = ddl.FindChildOfType(WOpenDdlPrimitiveType::String, "MasterBank"))
  {
    m_sMasterSoundBank = pElement->GetPrimitivesString()[0];
  }

  if (const WOpenDdlReaderElement* pElement = ddl.FindChildOfType(WOpenDdlPrimitiveType::UInt16, "VirtualChannels"))
  {
    m_uiVirtualChannels = pElement->GetPrimitivesUInt16()[0];
  }

  if (const WOpenDdlReaderElement* pElement = ddl.FindChildOfType(WOpenDdlPrimitiveType::UInt32, "SamplerRate"))
  {
    m_uiSamplerRate = pElement->GetPrimitivesUInt32()[0];
  }

  if (const WOpenDdlReaderElement* pElement = ddl.FindChildOfType(WOpenDdlPrimitiveType::String, "Mode"))
  {
    auto mode = pElement->GetPrimitivesString()[0];

    if (mode == "Stereo")
      m_SpeakerMode = WFmodSpeakerMode::ModeStereo;
    else if (mode == "7.1")
      m_SpeakerMode = WFmodSpeakerMode::Mode7Point1;
    else
      m_SpeakerMode = WFmodSpeakerMode::Mode5Point1;
  }
}

bool WFmodConfiguration::operator==(const WFmodConfiguration& rhs) const
{
  if (m_sMasterSoundBank != rhs.m_sMasterSoundBank)
    return false;
  if (m_uiVirtualChannels != rhs.m_uiVirtualChannels)
    return false;
  if (m_uiSamplerRate != rhs.m_uiSamplerRate)
    return false;
  if (m_SpeakerMode != rhs.m_SpeakerMode)
    return false;

  return true;
}

WResult WFmodAssetProfiles::Save(WStringView sFile) const
{
  WFileWriter file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  WOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  for (auto it = m_AssetProfiles.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Key().IsEmpty())
    {
      ddl.BeginObject("Platform", it.Key());

      it.Value().Save(ddl);

      ddl.EndObject();
    }
  }

  return W_SUCCESS;
}

WResult WFmodAssetProfiles::Load(WStringView sFile)
{
  m_AssetProfiles.Clear();

  WFileReader file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  WOpenDdlReader ddl;
  W_SUCCEED_OR_RETURN(ddl.ParseDocument(file));

  const WOpenDdlReaderElement* pRoot = ddl.GetRootElement();
  const WOpenDdlReaderElement* pChild = pRoot->GetFirstChild();

  while (pChild)
  {
    if (pChild->IsCustomType("Platform") && pChild->HasName())
    {
      auto& cfg = m_AssetProfiles[pChild->GetName()];

      cfg.Load(*pChild);
    }

    pChild = pChild->GetSibling();
  }

  return W_SUCCESS;
}

W_STATICLINK_FILE(FmodPlugin, FmodPlugin_FmodStartup);
