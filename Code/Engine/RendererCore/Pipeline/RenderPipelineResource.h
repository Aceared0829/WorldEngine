#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/HashTable.h>
#include <RendererCore/RendererCoreDLL.h>

using WRenderPipelineResourceHandle = WTypedResourceHandle<class WRenderPipelineResource>;
class WRenderPipeline;

/// Descriptor for creating a render pipeline resource.
///
/// Contains the serialized pipeline configuration including passes, extractors, and connections.
struct WRenderPipelineResourceDescriptor
{
  void Clear() {}

  WDynamicArray<WUInt8> m_SerializedPipeline;
  WString m_sPath;
};

/// Runtime resource representing a render pipeline configuration.
///
/// Stores a serialized render pipeline that can be instantiated to create runtime WRenderPipeline objects.
/// Multiple views can share the same pipeline resource but each creates its own pipeline instance.
class W_RENDERERCORE_DLL WRenderPipelineResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WRenderPipelineResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WRenderPipelineResource);
  W_RESOURCE_DECLARE_CREATEABLE(WRenderPipelineResource, WRenderPipelineResourceDescriptor);

public:
  WRenderPipelineResource();

  W_ALWAYS_INLINE const WRenderPipelineResourceDescriptor& GetDescriptor() { return m_Desc; }

  /// Creates a new runtime render pipeline instance from this resource.
  WInternal::NewInstance<WRenderPipeline> CreateRenderPipeline() const;

public:
  /// Returns a fallback pipeline resource used when the requested pipeline cannot be loaded.
  static WRenderPipelineResourceHandle CreateMissingPipeline();

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  WRenderPipelineResourceDescriptor m_Desc;
};
