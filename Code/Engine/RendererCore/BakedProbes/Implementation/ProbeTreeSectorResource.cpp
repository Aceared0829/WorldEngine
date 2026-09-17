#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>

WProbeTreeSectorResourceDescriptor::WProbeTreeSectorResourceDescriptor() = default;
WProbeTreeSectorResourceDescriptor::~WProbeTreeSectorResourceDescriptor() = default;
WProbeTreeSectorResourceDescriptor& WProbeTreeSectorResourceDescriptor::operator=(WProbeTreeSectorResourceDescriptor&& other) = default;

void WProbeTreeSectorResourceDescriptor::Clear()
{
  m_ProbePositions.Clear();
  m_SkyVisibility.Clear();
}

WUInt64 WProbeTreeSectorResourceDescriptor::GetHeapMemoryUsage() const
{
  WUInt64 uiMemUsage = 0;
  uiMemUsage += m_ProbePositions.GetHeapMemoryUsage();
  uiMemUsage += m_SkyVisibility.GetHeapMemoryUsage();
  return uiMemUsage;
}

static WTypeVersion s_ProbeTreeResourceDescriptorVersion = 1;
WResult WProbeTreeSectorResourceDescriptor::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProbeTreeResourceDescriptorVersion);

  inout_stream << m_vGridOrigin;
  inout_stream << m_vProbeSpacing;
  inout_stream << m_vProbeCount;

  W_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_ProbePositions));
  W_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_SkyVisibility));

  return W_SUCCESS;
}

WResult WProbeTreeSectorResourceDescriptor::Deserialize(WStreamReader& inout_stream)
{
  Clear();

  const WTypeVersion version = inout_stream.ReadVersion(s_ProbeTreeResourceDescriptorVersion);
  W_IGNORE_UNUSED(version);

  inout_stream >> m_vGridOrigin;
  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_vProbeCount;

  W_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_ProbePositions));
  W_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_SkyVisibility));

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProbeTreeSectorResource, 1, WRTTIDefaultAllocator<WProbeTreeSectorResource>);
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WProbeTreeSectorResource);
// clang-format on

WProbeTreeSectorResource::WProbeTreeSectorResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WProbeTreeSectorResource::~WProbeTreeSectorResource() = default;

WResourceLoadDesc WProbeTreeSectorResource::UnloadData(Unload WhatToUnload)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  m_Desc.Clear();

  return res;
}

WResourceLoadDesc WProbeTreeSectorResource::UpdateContent(WStreamReader* Stream)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WString sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  WProbeTreeSectorResourceDescriptor descriptor;
  if (descriptor.Deserialize(*Stream).Failed())
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(descriptor));
}

void WProbeTreeSectorResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WProbeTreeSectorResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Desc.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WResourceLoadDesc WProbeTreeSectorResource::CreateResource(WProbeTreeSectorResourceDescriptor&& descriptor)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  m_Desc = std::move(descriptor);

  return res;
}


W_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_ProbeTreeSectorResource);
