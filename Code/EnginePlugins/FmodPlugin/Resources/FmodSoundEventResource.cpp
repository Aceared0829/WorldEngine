#include <FmodPlugin/FmodPluginPCH.h>

#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFmodSoundEventResource, 1, WRTTIDefaultAllocator<WFmodSoundEventResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WFmodSoundEventResource);

WFmodSoundEventResource::WFmodSoundEventResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(WFmodSoundEventResource);
}

WFmodSoundEventResource::~WFmodSoundEventResource()
{
  W_ASSERT_DEV(m_pEventDescription == nullptr, "SoundEvent has not been freed correctly");
}

WResult WFmodSoundEventResource::PlayOnce(const WTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/) const
{
  bool bIsOneShot = false;
  m_pEventDescription->isOneshot(&bIsOneShot);

  if (!bIsOneShot)
  {
    WLog::Warning("WFmodSoundEventResource::PlayOnce: '{}' is not a one-shot event.", GetResourceIdOrDescription());
    return W_FAILURE;
  }

  auto pInstance = CreateInstance();
  if (pInstance == nullptr)
  {
    WLog::Warning("WFmodSoundEventResource::PlayOnce: Instance of '{}' could not be created.", GetResourceIdOrDescription());
    return W_FAILURE;
  }
  const auto fwd = globalPosition.m_qRotation * WVec3(1, 0, 0);
  const auto up = globalPosition.m_qRotation * WVec3(0, 0, 1);

  FMOD_3D_ATTRIBUTES attr;
  attr.position.x = globalPosition.m_vPosition.x;
  attr.position.y = globalPosition.m_vPosition.y;
  attr.position.z = globalPosition.m_vPosition.z;
  attr.forward.x = fwd.x;
  attr.forward.y = fwd.y;
  attr.forward.z = fwd.z;
  attr.up.x = up.x;
  attr.up.y = up.y;
  attr.up.z = up.z;
  attr.velocity.x = 0;
  attr.velocity.y = 0;
  attr.velocity.z = 0;

  W_FMOD_ASSERT(pInstance->setPitch(fPitch));
  W_FMOD_ASSERT(pInstance->setVolume(fVolume));
  W_FMOD_ASSERT(pInstance->set3DAttributes(&attr));
  W_FMOD_ASSERT(pInstance->setPaused(false));
  W_FMOD_ASSERT(pInstance->start());

  pInstance->release();

  return W_SUCCESS;
}

FMOD::Studio::EventInstance* WFmodSoundEventResource::CreateInstance() const
{
  if (m_pEventDescription)
  {
    FMOD::Studio::EventInstance* pInstance = nullptr;
    W_FMOD_ASSERT(m_pEventDescription->createInstance(&pInstance));
    return pInstance;
  }

  return nullptr;
}

WResourceLoadDesc WFmodSoundEventResource::UnloadData(Unload WhatToUnload)
{
  if (m_pEventDescription)
  {
    // this will kill all event pointers in the components
    // which is why the components actually listen for unload events on these resources
    m_pEventDescription->releaseAllInstances();
    m_pEventDescription->unloadSampleData();
    m_pEventDescription = nullptr;
  }

  m_hSoundBank.Invalidate();

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(WFmodSoundEventResource);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WFmodSoundEventResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WFmodSoundEventResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  WFmodSoundBankResourceHandle* pBankHandle = nullptr;
  Stream->ReadBytes(&pBankHandle, sizeof(WFmodSoundBankResourceHandle*));
  W_ASSERT_DEV(pBankHandle != nullptr, "Invalid Sound Bank Handle pointer in stream");

  Stream->ReadBytes(&m_pEventDescription, sizeof(FMOD::Studio::EventDescription*));
  W_ASSERT_DEV(m_pEventDescription != nullptr, "Invalid Sound Event Descriptor pointer in stream");

  m_hSoundBank = *pBankHandle;

  res.m_State = WResourceState::Loaded;
  return res;
}

void WFmodSoundEventResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = ModifyMemoryUsage().m_uiMemoryCPU;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WFmodSoundEventResource, WFmodSoundEventResourceDescriptor)
{
  // one missing resource is created this way
  // W_REPORT_FAILURE("This resource type does not support creating data.");

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}



W_STATICLINK_FILE(FmodPlugin, FmodPlugin_Resources_FmodSoundEventResource);
