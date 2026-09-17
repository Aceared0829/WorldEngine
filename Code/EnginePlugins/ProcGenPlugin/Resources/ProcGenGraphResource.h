#pragma once

#include <Core/ResourceManager/Resource.h>
#include <ProcGenPlugin/Declarations.h>

using WProcGenGraphResourceHandle = WTypedResourceHandle<class WProcGenGraphResource>;

struct W_PROCGENPLUGIN_DLL WProcGenGraphResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class W_PROCGENPLUGIN_DLL WProcGenGraphResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WProcGenGraphResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WProcGenGraphResource);
  W_RESOURCE_DECLARE_CREATEABLE(WProcGenGraphResource, WProcGenGraphResourceDescriptor);

public:
  WProcGenGraphResource();
  ~WProcGenGraphResource();

  const WDynamicArray<WSharedPtr<const WProcGenInternal::PlacementOutput>>& GetPlacementOutputs() const;
  const WDynamicArray<WSharedPtr<const WProcGenInternal::VertexColorOutput>>& GetVertexColorOutputs() const;

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  WDynamicArray<WSharedPtr<const WProcGenInternal::PlacementOutput>> m_PlacementOutputs;
  WDynamicArray<WSharedPtr<const WProcGenInternal::VertexColorOutput>> m_VertexColorOutputs;

  WSharedPtr<WProcGenInternal::GraphSharedDataBase> m_pSharedData;
};
