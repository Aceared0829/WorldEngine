#include <MiniAudioPlugin/MiniAudioPluginPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <MiniAudioPlugin/MiniAudioSingleton.h>
#include <MiniAudioPlugin/Resources/MiniAudioSoundResource.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMiniAudioSoundResource, 1, WRTTIDefaultAllocator<WMiniAudioSoundResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WMiniAudioSoundResource);

WMiniAudioSoundResource::WMiniAudioSoundResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(WMiniAudioSoundResource);
}

WMiniAudioSoundResource::~WMiniAudioSoundResource() = default;

const WDataBuffer& WMiniAudioSoundResource::GetAudioData() const
{
  return m_AudioData[0];
}

const WDataBuffer& WMiniAudioSoundResource::GetAudioData(WRandom& ref_rng) const
{
  return m_AudioData[ref_rng.UInt32Index(m_AudioData.GetCount())];
}

float WMiniAudioSoundResource::GetVolume(WRandom& ref_rng) const
{
  if (m_fMinVolume < m_fMaxVolume)
    return ref_rng.FloatMinMax(m_fMinVolume, m_fMaxVolume);

  return m_fMinVolume;
}

float WMiniAudioSoundResource::GetPitch(WRandom& ref_rng) const
{
  if (m_fMinPitch < m_fMaxPitch)
    return ref_rng.FloatMinMax(m_fMinPitch, m_fMaxPitch);

  return m_fMinPitch;
}

WMiniAudioSoundInstance* WMiniAudioSoundResource::InstantiateSound(WRandom* pRng, WWorld* pWorld, const WComponentHandle& hComponent)
{
  WMiniAudioSingleton* pMA = WMiniAudioSingleton::GetSingleton();

  WMiniAudioSoundInstance* pInstance = nullptr;

  ma_sound_group* pGroup = nullptr;

  if (!m_sSoundGroup.IsEmpty())
  {
    pGroup = pMA->GetSoundGroup(m_sSoundGroup).m_pGroup.Borrow();
  }

  if (pRng)
  {
    pInstance = pMA->AllocateSoundInstance(GetAudioData(*pRng), pWorld, hComponent, pGroup);
  }
  else
  {
    pInstance = pMA->AllocateSoundInstance(GetAudioData(), pWorld, hComponent, pGroup);
  }



  W_MA_CHECK(ma_data_source_set_looping(&pInstance->m_Decoder, GetLoop()));

  ma_sound_set_min_distance(&pInstance->m_Sound, GetMinDistance());
  // ma_sound_set_max_distance(&pInstance->m_Sound, GetMaxDistance());
  // ma_sound_set_attenuation_model(&pInstance->m_Sound, ma_attenuation_model_exponential);
  ma_sound_set_rolloff(&pInstance->m_Sound, GetRolloff());
  ma_sound_set_doppler_factor(&pInstance->m_Sound, GetDopplerFactor());
  ma_sound_set_spatialization_enabled(&pInstance->m_Sound, GetSpatialize());

  return pInstance;
}

WResourceLoadDesc WMiniAudioSoundResource::UnloadData(Unload WhatToUnload)
{
  m_AudioData.Clear();
  m_AudioData.Compact();

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(WMiniAudioSoundResource);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WMiniAudioSoundResource::UpdateContent(WStreamReader* pStream)
{
  W_LOG_BLOCK("WMiniAudioSoundResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (pStream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WString sAbsFilePath;
  (*pStream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  WAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  WUInt8 uiVersion = 0;
  *pStream >> uiVersion;

  *pStream >> m_bLoop;
  *pStream >> m_fMinVolume;
  *pStream >> m_fMaxVolume;
  *pStream >> m_fMinPitch;
  *pStream >> m_fMaxPitch;
  *pStream >> m_bSpatialize;
  *pStream >> m_fMinDistance;
  *pStream >> m_fMaxDistance;
  *pStream >> m_fRolloff;
  *pStream >> m_fDopplerFactor;

  if (m_fMinVolume > m_fMaxVolume)
    WMath::Swap(m_fMinVolume, m_fMaxVolume);

  if (m_fMinDistance > m_fMaxDistance)
    WMath::Swap(m_fMinDistance, m_fMaxDistance);

  if (m_fMinPitch > m_fMaxPitch)
    WMath::Swap(m_fMinPitch, m_fMaxPitch);

  WUInt32 uiNumFiles = 0;
  *pStream >> uiNumFiles;

  m_AudioData.SetCount(uiNumFiles);

  for (WUInt32 i = 0; i < uiNumFiles; ++i)
  {
    WUInt32 uiFileSize = 0;
    *pStream >> uiFileSize;

    m_AudioData[i].SetCountUninitialized(uiFileSize);
    if (pStream->ReadBytes(m_AudioData[i].GetData(), uiFileSize) != uiFileSize)
    {
      res.m_State = WResourceState::LoadedResourceMissing;
      return res;
    }
  }

  if (uiVersion >= 2)
  {
    *pStream >> m_sSoundGroup;
  }

  res.m_State = WResourceState::Loaded;

  return res;
}

void WMiniAudioSoundResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WMiniAudioSoundResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_AudioData.GetHeapMemoryUsage();

  for (const auto& data : m_AudioData)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += data.GetHeapMemoryUsage();
  }

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WMiniAudioSoundResource, WMiniAudioSoundResourceDescriptor)
{
  // have to create one 'missing' resource
  // W_REPORT_FAILURE("This resource type does not support creating data.");

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}
