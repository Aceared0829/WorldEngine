#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Components/BlackboardComponent.h>

using WBlackboardTemplateResourceHandle = WTypedResourceHandle<class WBlackboardTemplateResource>;

struct W_RENDERERCORE_DLL WBlackboardTemplateResourceDescriptor
{
  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);

  WDynamicArray<WBlackboardEntry> m_Entries;
};

/// Describes the initial state of a blackboard.
///
/// Used by WBlackboardComponent to initialize its blackboard from.
class W_RENDERERCORE_DLL WBlackboardTemplateResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WBlackboardTemplateResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WBlackboardTemplateResource);
  W_RESOURCE_DECLARE_CREATEABLE(WBlackboardTemplateResource, WBlackboardTemplateResourceDescriptor);

public:
  WBlackboardTemplateResource();
  ~WBlackboardTemplateResource();

  const WBlackboardTemplateResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WBlackboardTemplateResourceDescriptor m_Descriptor;
};
