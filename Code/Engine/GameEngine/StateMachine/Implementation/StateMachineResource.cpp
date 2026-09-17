#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineResource, 1, WRTTIDefaultAllocator<WStateMachineResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WStateMachineResource);
// clang-format on

WStateMachineResource::WStateMachineResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WStateMachineResource::~WStateMachineResource() = default;

WUniquePtr<WStateMachineInstance> WStateMachineResource::CreateInstance(WReflectedClass& ref_owner)
{
  if (m_pDescription != nullptr)
  {
    return W_DEFAULT_NEW(WStateMachineInstance, ref_owner, m_pDescription);
  }

  return nullptr;
}

WResourceLoadDesc WStateMachineResource::UnloadData(Unload WhatToUnload)
{
  m_pDescription = nullptr;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WStateMachineResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WStateMachineResource::UpdateContent", GetResourceIdOrDescription());

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

  WUniquePtr<WStateMachineDescription> pDescription = W_DEFAULT_NEW(WStateMachineDescription);
  if (pDescription->Deserialize(*Stream).Failed())
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  m_pDescription = std::move(pDescription);

  res.m_State = WResourceState::Loaded;
  return res;
}

void WStateMachineResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}


W_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineResource);
