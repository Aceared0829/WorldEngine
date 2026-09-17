#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationClipMapping, 1, WRTTIDefaultAllocator<WAnimationClipMapping>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("ClipName", GetClipName, SetClipName)->AddAttributes(new WDynamicStringEnumAttribute("AnimationClipMappingEnum")),
    W_RESOURCE_MEMBER_PROPERTY("Clip", m_hClip)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation"), new WRequiredAttribute()),
  }
    W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimGraphResource, 1, WRTTIDefaultAllocator<WAnimGraphResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WAnimGraphResource);
// clang-format on

WAnimGraphResource::WAnimGraphResource()
  : WResource(WResource::DoUpdate::OnAnyThread, 0)
{
}

WAnimGraphResource::~WAnimGraphResource() = default;

WResourceLoadDesc WAnimGraphResource::UnloadData(Unload WhatToUnload)
{
  WResourceLoadDesc d;
  d.m_State = WResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  return d;
}

WResourceLoadDesc WAnimGraphResource::UpdateContent(WStreamReader* Stream)
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
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).AssertSuccess();

  {
    const auto uiVersion = Stream->ReadVersion(2);
    Stream->ReadArray(m_IncludeGraphs).AssertSuccess();

    if (uiVersion >= 2)
    {
      WUInt32 uiNum = 0;
      *Stream >> uiNum;

      m_AnimationClipMapping.SetCount(uiNum);
      for (WUInt32 i = 0; i < uiNum; ++i)
      {
        *Stream >> m_AnimationClipMapping[i].m_sClipName;
        *Stream >> m_AnimationClipMapping[i].m_hClip;
      }
    }
  }

  if (m_AnimGraph.Deserialize(*Stream).Failed())
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  m_AnimGraph.PrepareForUse();

  res.m_State = WResourceState::Loaded;

  return res;
}

void WAnimGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphResource);
