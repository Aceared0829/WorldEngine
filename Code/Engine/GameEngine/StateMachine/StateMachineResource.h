#pragma once

#include <Core/ResourceManager/Resource.h>
#include <GameEngine/StateMachine/StateMachine.h>

using WStateMachineResourceHandle = WTypedResourceHandle<class WStateMachineResource>;

class W_GAMEENGINE_DLL WStateMachineResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WStateMachineResource);

public:
  WStateMachineResource();
  ~WStateMachineResource();

  const WSharedPtr<const WStateMachineDescription>& GetDescription() const { return m_pDescription; }

  WUniquePtr<WStateMachineInstance> CreateInstance(WReflectedClass& ref_owner);

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  WSharedPtr<const WStateMachineDescription> m_pDescription;
};
