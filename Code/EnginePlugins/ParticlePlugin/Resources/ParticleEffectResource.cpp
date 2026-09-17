#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEffectResource, 1, WRTTIDefaultAllocator<WParticleEffectResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WParticleEffectResource);
// clang-format on

WParticleEffectResource::WParticleEffectResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WParticleEffectResource::~WParticleEffectResource() = default;

WResourceLoadDesc WParticleEffectResource::UnloadData(Unload WhatToUnload)
{
  /// \todo Clear something
  // m_Desc.m_System1

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WParticleEffectResource::UpdateContent(WStreamReader* Stream)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Desc.Load(*Stream);

  return res;
}

void WParticleEffectResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  /// \todo Better statistics
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WParticleEffectResource) + sizeof(WParticleEffectResourceDescriptor);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WParticleEffectResource, WParticleEffectResourceDescriptor)
{
  m_Desc = descriptor;

  WResourceLoadDesc res;
  res.m_State = WResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}

void WParticleEffectResourceDescriptor::Save(WStreamWriter& inout_stream) const
{
  m_Effect.Save(inout_stream);
}

void WParticleEffectResourceDescriptor::Load(WStreamReader& inout_stream)
{
  m_Effect.Load(inout_stream);
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Resources_ParticleEffectResource);
