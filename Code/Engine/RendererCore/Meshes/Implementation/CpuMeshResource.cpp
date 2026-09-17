#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Meshes/CpuMeshResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCpuMeshResource, 1, WRTTIDefaultAllocator<WCpuMeshResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WCpuMeshResource);
// clang-format on

WCpuMeshResource::WCpuMeshResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WResourceLoadDesc WCpuMeshResource::UnloadData(Unload WhatToUnload)
{
  WResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire mesh
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_Descriptor.Clear();

    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = WResourceState::Unloaded;
  }

  return res;
}

WResourceLoadDesc WCpuMeshResource::UpdateContent(WStreamReader* Stream)
{
  WMeshResourceDescriptor desc;
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  if (m_Descriptor.Load(*Stream).Failed())
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  res.m_State = WResourceState::Loaded;
  return res;
}

void WCpuMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WCpuMeshResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WCpuMeshResource, WMeshResourceDescriptor)
{
  m_Descriptor = descriptor;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}



W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_CpuMeshResource);
