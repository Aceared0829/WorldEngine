#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Serialization/ReflectionSerializer.h>


const WPlatformProfile* WAssetCurator::GetDevelopmentAssetProfile() const
{
  return m_AssetProfiles[0];
}

const WPlatformProfile* WAssetCurator::GetActiveAssetProfile() const
{
  return m_AssetProfiles[m_uiActiveAssetProfile];
}

WUInt32 WAssetCurator::GetActiveAssetProfileIndex() const
{
  return m_uiActiveAssetProfile;
}

WUInt32 WAssetCurator::FindAssetProfileByName(const char* szPlatform)
{
  W_LOCK(m_CuratorMutex);

  W_ASSERT_DEV(!m_AssetProfiles.IsEmpty(), "Need to have a valid asset platform config");

  for (WUInt32 i = 0; i < m_AssetProfiles.GetCount(); ++i)
  {
    if (m_AssetProfiles[i]->GetConfigName().IsEqual_NoCase(szPlatform))
    {
      return i;
    }
  }

  return WInvalidIndex;
}

WUInt32 WAssetCurator::GetNumAssetProfiles() const
{
  return m_AssetProfiles.GetCount();
}

const WPlatformProfile* WAssetCurator::GetAssetProfile(WUInt32 uiIndex) const
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[uiIndex];
}

WPlatformProfile* WAssetCurator::GetAssetProfile(WUInt32 uiIndex)
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[uiIndex];
}

WPlatformProfile* WAssetCurator::CreateAssetProfile()
{
  WPlatformProfile* pProfile = W_DEFAULT_NEW(WPlatformProfile);
  m_AssetProfiles.PushBack(pProfile);

  return pProfile;
}

WResult WAssetCurator::DeleteAssetProfile(WPlatformProfile* pProfile)
{
  if (m_AssetProfiles.GetCount() <= 1)
    return W_FAILURE;

  // do not allow to delete element 0 !

  for (WUInt32 i = 1; i < m_AssetProfiles.GetCount(); ++i)
  {
    if (m_AssetProfiles[i] == pProfile)
    {
      if (m_uiActiveAssetProfile == i)
        return W_FAILURE;

      if (i < m_uiActiveAssetProfile)
        --m_uiActiveAssetProfile;

      W_DEFAULT_DELETE(pProfile);
      m_AssetProfiles.RemoveAtAndCopy(i);

      return W_SUCCESS;
    }
  }

  return W_FAILURE;
}

void WAssetCurator::SetActiveAssetProfileByIndex(WUInt32 uiIndex, bool bForceReevaluation /*= false*/)
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    uiIndex = 0; // fall back to default platform

  if (!bForceReevaluation && m_uiActiveAssetProfile == uiIndex)
    return;

  W_LOG_BLOCK("Switch Active Asset Platform", m_AssetProfiles[uiIndex]->GetConfigName());

  m_uiActiveAssetProfile = uiIndex;

  CheckFileSystem();

  {
    WAssetCuratorEvent e;
    e.m_Type = WAssetCuratorEvent::Type::ActivePlatformChanged;
    m_Events.Broadcast(e);
  }

  {
    WSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ChangeActivePlatform";
    msg.m_sPayload = GetActiveAssetProfile()->GetConfigName();
    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void WAssetCurator::SaveRuntimeProfiles()
{
  for (WUInt32 i = 0; i < GetNumAssetProfiles(); ++i)
  {
    WStringBuilder sProfileRuntimeDataFile;

    WPlatformProfile* pProfile = GetAssetProfile(i);

    sProfileRuntimeDataFile.Set(":project/RuntimeConfigs/", pProfile->GetConfigName(), ".WProfile");

    pProfile->SaveForRuntime(sProfileRuntimeDataFile).IgnoreResult();
  }
}

WResult WAssetCurator::SaveAssetProfiles()
{
  WDeferredFileWriter file;
  file.SetOutput(":project/Editor/AssetProfiles.ddl");

  WOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("AssetProfiles");

  for (const auto* pCfg : m_AssetProfiles)
  {
    ddl.BeginObject("Config", pCfg->GetConfigName());

    // make sure to create the same GUID every time, otherwise the serialized file changes all the time
    const WUuid guid = WUuid::MakeStableUuidFromString(pCfg->GetConfigName());

    WReflectionSerializer::WriteObjectToDDL(ddl, pCfg->GetDynamicRTTI(), pCfg, guid);

    ddl.EndObject();
  }

  ddl.EndObject();

  return file.Close();
}

WResult WAssetCurator::LoadAssetProfiles()
{
  W_LOG_BLOCK("LoadAssetProfiles", ":project/Editor/PlatformProfiles.ddl");

  WFileReader file;
  if (file.Open(":project/Editor/AssetProfiles.ddl").Failed())
    return W_FAILURE;

  WOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return W_FAILURE;

  const WOpenDdlReaderElement* pRootElement = ddl.GetRootElement()->FindChildOfType("AssetProfiles");

  if (!pRootElement)
    return W_FAILURE;

  if (pRootElement->FindChildOfType("Config") == nullptr)
    return W_FAILURE;

  ClearAssetProfiles();

  for (auto pChild = pRootElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Config"))
    {
      const WRTTI* pRtti = nullptr;
      void* pConfigObj = WReflectionSerializer::ReadObjectFromDDL(pChild, pRtti);

      auto pProfile = static_cast<WPlatformProfile*>(pConfigObj);

      pProfile->AddMissingConfigs();

      m_AssetProfiles.PushBack(pProfile);
    }
  }

  if (m_AssetProfiles.IsEmpty() || m_AssetProfiles[0]->GetConfigName() != "Default")
  {
    WPlatformProfile* pCfg = W_DEFAULT_NEW(WPlatformProfile);
    pCfg->SetConfigName("Default");
    pCfg->SetTargetPlatform("Windows");

    pCfg->AddMissingConfigs();
    m_AssetProfiles.InsertAt(0, pCfg);
  }

  return W_SUCCESS;
}

void WAssetCurator::ClearAssetProfiles()
{
  for (auto pCfg : m_AssetProfiles)
  {
    pCfg->GetDynamicRTTI()->GetAllocator()->Deallocate(pCfg);
  }

  m_AssetProfiles.Clear();
}

void WAssetCurator::SetupDefaultAssetProfiles()
{
  ClearAssetProfiles();

  {
    WPlatformProfile* pCfg = W_DEFAULT_NEW(WPlatformProfile);
    pCfg->SetConfigName("Default");
    pCfg->SetTargetPlatform("Windows");
    pCfg->AddMissingConfigs();
    m_AssetProfiles.PushBack(pCfg);
  }
}

void WAssetCurator::ComputeAllDocumentManagerAssetProfileHashes()
{
  for (auto pMan : WDocumentManager::GetAllDocumentManagers())
  {
    if (auto pAssMan = WDynamicCast<WAssetDocumentManager*>(pMan))
    {
      pAssMan->ComputeAssetProfileHash(GetActiveAssetProfile());
    }
  }
}
