#include <FmodPlugin/FmodPluginPCH.h>

#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFmodSoundBankResource, 1, WRTTIDefaultAllocator<WFmodSoundBankResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WFmodSoundBankResource);

WFmodSoundBankResource::WFmodSoundBankResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(WFmodSoundBankResource);
}

WFmodSoundBankResource::~WFmodSoundBankResource()
{
  W_ASSERT_DEV(m_pSoundBank == nullptr, "Soundbank has not been freed correctly");
}

WResourceLoadDesc WFmodSoundBankResource::UnloadData(Unload WhatToUnload)
{
  if (m_pSoundBank)
  {
    m_pSoundBank->unload();
    m_pSoundBank = nullptr;
  }

  if (m_pSoundBankData != nullptr)
  {
    WFmod::GetSingleton()->QueueSoundBankDataForDeletion(m_pSoundBankData);
    m_pSoundBankData = nullptr;
  }

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(WFmodSoundBankResource);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WFmodSoundBankResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WFmodSoundBankResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  Stream->ReadBytes(&m_pSoundBank, sizeof(FMOD::Studio::Bank*));
  Stream->ReadBytes(&m_pSoundBankData, sizeof(WDataBuffer*));

  W_ASSERT_DEV(m_pSoundBank != nullptr, "Invalid Sound Bank pointer in stream");
  W_ASSERT_DEV(m_pSoundBankData != nullptr, "Invalid Sound Bank Data pointer in stream");

  res.m_State = WResourceState::Loaded;

  // the newly loaded sound bank might contain VCAs that had not been loaded yet
  WFmod::GetSingleton()->UpdateSoundGroupVolumes();

  return res;
}

void WFmodSoundBankResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WFmodSoundBankResource);

  if (m_pSoundBankData)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += (WUInt32)m_pSoundBankData->GetHeapMemoryUsage() + sizeof(*m_pSoundBankData);
  }

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WFmodSoundBankResource, WFmodSoundBankResourceDescriptor)
{
  // have to create one 'missing' resource
  // W_REPORT_FAILURE("This resource type does not support creating data.");

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}



W_STATICLINK_FILE(FmodPlugin, FmodPlugin_Resources_FmodSoundBankResource);
