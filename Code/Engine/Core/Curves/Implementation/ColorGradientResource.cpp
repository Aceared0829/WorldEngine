#include <Core/CorePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WColorGradientResource, 1, WRTTIDefaultAllocator<WColorGradientResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WColorGradientResource);

WColorGradientResource::WColorGradientResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WColorGradientResource, WColorGradientResourceDescriptor)
{
  m_Descriptor = descriptor;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

WResourceLoadDesc WColorGradientResource::UnloadData(Unload WhatToUnload)
{
  W_IGNORE_UNUSED(WhatToUnload);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  m_Descriptor.m_Gradient.Clear();

  return res;
}

WResourceLoadDesc WColorGradientResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WColorGradientResource::UpdateContent", GetResourceIdOrDescription());

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

  m_Descriptor.Load(*Stream);

  res.m_State = WResourceState::Loaded;
  return res;
}

void WColorGradientResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<WUInt32>(m_Descriptor.m_Gradient.GetHeapMemoryUsage()) + static_cast<WUInt32>(sizeof(m_Descriptor));
}

void WColorGradientResourceDescriptor::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  m_Gradient.Save(inout_stream);
}

void WColorGradientResourceDescriptor::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  m_Gradient.Load(inout_stream);
}



W_STATICLINK_FILE(Core, Core_Curves_Implementation_ColorGradientResource);
