#pragma once

#include <Core/ResourceManager/Resource.h>
#include <FmodPlugin/FmodPluginDLL.h>

using WFmodSoundEventResourceHandle = WTypedResourceHandle<class WFmodSoundEventResource>;
using WFmodSoundBankResourceHandle = WTypedResourceHandle<class WFmodSoundBankResource>;

struct W_FMODPLUGIN_DLL WFmodSoundEventResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class W_FMODPLUGIN_DLL WFmodSoundEventResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WFmodSoundEventResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WFmodSoundEventResource);
  W_RESOURCE_DECLARE_CREATEABLE(WFmodSoundEventResource, WFmodSoundEventResourceDescriptor);

public:
  WFmodSoundEventResource();
  ~WFmodSoundEventResource();

  /// Creates an instance of this sound event and plays it.
  ///
  /// This is only allowed for events that are not looped, otherwise W_FAILURE is returned.
  WResult PlayOnce(const WTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f) const;

  /// Creates a new sound event instance of this FMOD sound event. May return nullptr, if the event data could not be loaded.
  FMOD::Studio::EventInstance* CreateInstance() const;

  /// Returns the FMOD sound event descriptor. May be nullptr, if the sound bank could not be loaded or the event GUID was invalid.
  FMOD::Studio::EventDescription* GetDescriptor() const { return m_pEventDescription; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  WFmodSoundBankResourceHandle m_hSoundBank;
  FMOD::Studio::EventDescription* m_pEventDescription = nullptr;
};

class W_FMODPLUGIN_DLL WFmodSoundEventResourceLoader : public WResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)

    {
    }

    WDefaultMemoryStreamStorage m_Storage;
    WMemoryStreamReader m_Reader;
    WFmodSoundBankResourceHandle m_hSoundBank;
    FMOD::Studio::EventDescription* m_pEventDescription = nullptr;
  };

  virtual WResourceLoadData OpenDataStream(const WResource* pResource) override;
  virtual void CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const WResource* pResource) const override;
};
