#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WRmlUiScaleMode, 1)
  W_ENUM_CONSTANTS(WRmlUiScaleMode::Fixed, WRmlUiScaleMode::WithScreenSize)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

static WTypeVersion s_RmlUiDescVersion = 1;

WResult WRmlUiResourceDescriptor::Save(WStreamWriter& inout_stream)
{
  // write this at the beginning so that the file can be read as an WDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  W_SUCCEED_OR_RETURN(m_DependencyFile.WriteDependencyFile(inout_stream));

  inout_stream.WriteVersion(s_RmlUiDescVersion);

  inout_stream << m_sRmlFile;
  inout_stream << m_ScaleMode;
  inout_stream << m_ReferenceResolution;

  return W_SUCCESS;
}

WResult WRmlUiResourceDescriptor::Load(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(m_DependencyFile.ReadDependencyFile(inout_stream));

  WTypeVersion uiVersion = inout_stream.ReadVersion(s_RmlUiDescVersion);
  W_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sRmlFile;
  inout_stream >> m_ScaleMode;
  inout_stream >> m_ReferenceResolution;

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRmlUiResource, 1, WRTTIDefaultAllocator<WRmlUiResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WRmlUiResource);
// clang-format on

WRmlUiResource::WRmlUiResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WResourceLoadDesc WRmlUiResource::UnloadData(Unload WhatToUnload)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WRmlUiResource::UpdateContent(WStreamReader* Stream)
{
  WRmlUiResourceDescriptor desc;
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // Direct loading of rml file
  if (sAbsFilePath.GetFileExtension() == "rml")
  {
    m_sRmlFile = sAbsFilePath;

    res.m_State = WResourceState::Loaded;
    return res;
  }

  WAssetFileHeader assetHeader;
  assetHeader.Read(*Stream).IgnoreResult();

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void WRmlUiResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WRmlUiResource, WRmlUiResourceDescriptor)
{
  m_sRmlFile = descriptor.m_sRmlFile;
  m_ScaleMode = descriptor.m_ScaleMode;
  m_vReferenceResolution = descriptor.m_ReferenceResolution;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

bool WRmlUiResourceLoader::IsResourceOutdated(const WResource* pResource) const
{
  if (WResourceLoaderFromFile::IsResourceOutdated(pResource))
    return true;

  WStringBuilder sId = pResource->GetResourceID();
  if (sId.GetFileExtension() == "rml")
    return false;

  WFileReader stream;
  if (stream.Open(pResource->GetResourceID()).Failed())
    return false;

  // skip asset header
  WAssetFileHeader assetHeader;
  assetHeader.Read(stream).IgnoreResult();

  WDependencyFile dep;
  if (dep.ReadDependencyFile(stream).Failed())
    return true;

  return dep.HasAnyFileChanged();
}


W_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Resources_RmlUiResource);
