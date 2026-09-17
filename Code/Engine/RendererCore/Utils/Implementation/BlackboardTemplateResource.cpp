#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Utils/BlackboardTemplateResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBlackboardTemplateResource, 1, WRTTIDefaultAllocator<WBlackboardTemplateResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WBlackboardTemplateResource);
// clang-format on

WBlackboardTemplateResource::WBlackboardTemplateResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WBlackboardTemplateResource::~WBlackboardTemplateResource() = default;

WResourceLoadDesc WBlackboardTemplateResource::UnloadData(Unload WhatToUnload)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WBlackboardTemplateResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WBlackboardTemplateResource::UpdateContent", GetResourceIdOrDescription());

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

  // skip the asset file header at the start of the file
  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  WBlackboardTemplateResourceDescriptor desc;
  if (desc.Deserialize(*Stream).Failed())
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  CreateResource(std::move(desc));

  res.m_State = WResourceState::Loaded;
  return res;
}

void WBlackboardTemplateResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WBlackboardTemplateResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Descriptor.m_Entries.GetHeapMemoryUsage();
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WBlackboardTemplateResource, WBlackboardTemplateResourceDescriptor)
{
  m_Descriptor = std::move(descriptor);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

WResult WBlackboardTemplateResourceDescriptor::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Entries));
  return W_SUCCESS;
}

WResult WBlackboardTemplateResourceDescriptor::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Entries));
  return W_SUCCESS;
}


W_STATICLINK_FILE(RendererCore, RendererCore_Utils_Implementation_BlackboardTemplateResource);
