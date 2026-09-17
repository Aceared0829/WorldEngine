#pragma once

#include <Core/ResourceManager/Resource.h>
#include <MiniAudioPlugin/MiniAudioPluginDLL.h>

class WRandom;
class WWorld;
struct WMiniAudioSoundInstance;
struct WComponentHandle;

using WMiniAudioSoundResourceHandle = WTypedResourceHandle<class WMiniAudioSoundResource>;

struct W_MINIAUDIOPLUGIN_DLL WMiniAudioSoundResourceDescriptor{
  // empty, these types of resources must be loaded from file
};

class W_MINIAUDIOPLUGIN_DLL WMiniAudioSoundResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WMiniAudioSoundResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WMiniAudioSoundResource);
  W_RESOURCE_DECLARE_CREATEABLE(WMiniAudioSoundResource, WMiniAudioSoundResourceDescriptor);

public:
  WMiniAudioSoundResource();
  ~WMiniAudioSoundResource();

  const WDataBuffer& GetAudioData() const;
  const WDataBuffer& GetAudioData(WRandom& ref_rng) const;

  bool GetLoop() const { return m_bLoop; }
  float GetVolume(WRandom& ref_rng) const;
  float GetPitch(WRandom& ref_rng) const;
  bool GetSpatialize() const { return m_bSpatialize; }
  float GetDopplerFactor() const { return m_fDopplerFactor; }
  float GetMinDistance() const { return m_fMinDistance; }
  float GetMaxDistance() const { return m_fMaxDistance; }
  float GetRolloff() const { return m_fRolloff; }

  /// Instantiates the sound, all arguments are optional.
  WMiniAudioSoundInstance* InstantiateSound(WRandom* pRng, WWorld* pWorld, const WComponentHandle& hComponent);

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  WHybridArray<WDataBuffer, 1> m_AudioData;

  WString m_sSoundGroup;
  bool m_bLoop = false;
  float m_fMinVolume = 1.0f;
  float m_fMaxVolume = 1.0f;
  float m_fMinDistance = 1.0f;
  float m_fMaxDistance = 10.0f;
  float m_fMinPitch = 1.0f;
  float m_fMaxPitch = 1.0f;
  bool m_bSpatialize = true;
  float m_fDopplerFactor = 1.0f;
  float m_fRolloff = 1.0f;
};
