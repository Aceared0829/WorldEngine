#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/IO/DependencyFile.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

struct W_RMLUIPLUGIN_DLL WRmlUiScaleMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Fixed,
    WithScreenSize,

    Default = WithScreenSize
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RMLUIPLUGIN_DLL, WRmlUiScaleMode);

struct W_RMLUIPLUGIN_DLL WRmlUiResourceDescriptor
{
  WResult Save(WStreamWriter& inout_stream);
  WResult Load(WStreamReader& inout_stream);

  WDependencyFile m_DependencyFile;

  WString m_sRmlFile;
  WEnum<WRmlUiScaleMode> m_ScaleMode;
  WVec2U32 m_ReferenceResolution;
};

using WRmlUiResourceHandle = WTypedResourceHandle<class WRmlUiResource>;

class W_RMLUIPLUGIN_DLL WRmlUiResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WRmlUiResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WRmlUiResource);
  W_RESOURCE_DECLARE_CREATEABLE(WRmlUiResource, WRmlUiResourceDescriptor);

public:
  WRmlUiResource();

  const WString& GetRmlFile() const { return m_sRmlFile; }
  const WEnum<WRmlUiScaleMode>& GetScaleMode() const { return m_ScaleMode; }
  const WVec2U32& GetReferenceResolution() const { return m_vReferenceResolution; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WString m_sRmlFile;
  WEnum<WRmlUiScaleMode> m_ScaleMode;
  WVec2U32 m_vReferenceResolution = WVec2U32::MakeZero();
};

class WRmlUiResourceLoader : public WResourceLoaderFromFile
{
public:
  virtual bool IsResourceOutdated(const WResource* pResource) const override;
};
