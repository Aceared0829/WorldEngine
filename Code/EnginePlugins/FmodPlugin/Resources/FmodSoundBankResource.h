#pragma once

#include <Core/ResourceManager/Resource.h>
#include <FmodPlugin/FmodPluginDLL.h>

using WFmodSoundBankResourceHandle = WTypedResourceHandle<class WFmodSoundBankResource>;

struct W_FMODPLUGIN_DLL WFmodSoundBankResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class W_FMODPLUGIN_DLL WFmodSoundBankResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WFmodSoundBankResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WFmodSoundBankResource);
  W_RESOURCE_DECLARE_CREATEABLE(WFmodSoundBankResource, WFmodSoundBankResourceDescriptor);

public:
  WFmodSoundBankResource();
  ~WFmodSoundBankResource();

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  FMOD::Studio::Bank* m_pSoundBank = nullptr;
  WDataBuffer* m_pSoundBankData = nullptr;
};

class W_FMODPLUGIN_DLL WFmodSoundBankResourceLoader : public WResourceTypeLoader
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
    FMOD::Studio::Bank* m_pSoundBank = nullptr;
    WDataBuffer* m_pSoundbankData = nullptr;
  };

  virtual WResourceLoadData OpenDataStream(const WResource* pResource) override;
  virtual void CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const WResource* pResource) const override;
};
