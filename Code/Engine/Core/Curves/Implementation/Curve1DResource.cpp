#include <Core/CorePCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCurve1DResource, 1, WRTTIDefaultAllocator<WCurve1DResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WCurve1DResource);

WCurve1DResource::WCurve1DResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WCurve1DResource, WCurve1DResourceDescriptor)
{
  m_Descriptor = descriptor;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

WResourceLoadDesc WCurve1DResource::UnloadData(Unload WhatToUnload)
{
  W_IGNORE_UNUSED(WhatToUnload);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  m_Descriptor.m_Curves.Clear();

  return res;
}

WResourceLoadDesc WCurve1DResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WCurve1DResource::UpdateContent", GetResourceIdOrDescription());

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

void WCurve1DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<WUInt32>(m_Descriptor.m_Curves.GetHeapMemoryUsage()) + static_cast<WUInt32>(sizeof(m_Descriptor));

  for (const auto& curve : m_Descriptor.m_Curves)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += curve.GetHeapMemoryUsage();
  }
}

void WCurve1DResourceDescriptor::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  const WUInt8 uiCurves = static_cast<WUInt8>(m_Curves.GetCount());
  inout_stream << uiCurves;

  for (WUInt32 i = 0; i < uiCurves; ++i)
  {
    m_Curves[i].Save(inout_stream);
  }
}

void WCurve1DResourceDescriptor::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  WUInt8 uiCurves = 0;
  inout_stream >> uiCurves;

  m_Curves.SetCount(uiCurves);

  for (WUInt32 i = 0; i < uiCurves; ++i)
  {
    m_Curves[i].Load(inout_stream);

    /// \todo We can do this on load, or somehow ensure this is always already correctly saved
    m_Curves[i].SortControlPoints();
    m_Curves[i].CreateLinearApproximation();
  }
}



W_STATICLINK_FILE(Core, Core_Curves_Implementation_Curve1DResource);
