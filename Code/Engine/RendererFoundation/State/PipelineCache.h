#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WGALDevice;

/// A cache from pipeline descriptor to handle which holds a reference to each pipeline that is never freed until shutdown. This is just a stopgap solution until the high level interface changes and mostly used by `WRenderContext` to provide the old interface until further refactoring.
class W_RENDERERFOUNDATION_DLL WGALPipelineCache
{
  W_DECLARE_SINGLETON(WGALPipelineCache);

public:
  /// Creates a pipeline or retrieves it from the cache. Ownership remains with the cache so do not call DestroyGraphicsPipeline on the handle.
  static WGALGraphicsPipelineHandle GetPipeline(const WGALGraphicsPipelineCreationDescription& description);
  /// Creates a pipeline or retrieves it from the cache. Ownership remains with the cache so do not call DestroyComputePipeline on the handle.
  static WGALComputePipelineHandle GetPipeline(const WGALComputePipelineCreationDescription& description);

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, PipelineCache);
  friend class WMemoryUtils;

  struct GraphicsPipelineCacheKey
  {
    W_DECLARE_POD_TYPE();
    WUInt32 m_uiHash = 0;
    WGALGraphicsPipelineCreationDescription m_Desc;
  };

  struct ComputePipelineCacheKey
  {
    W_DECLARE_POD_TYPE();
    WUInt32 m_uiHash = 0;
    WGALComputePipelineCreationDescription m_Desc;
  };

  struct CacheKeyHasher
  {
    static WUInt32 Hash(const GraphicsPipelineCacheKey& a);
    static bool Equal(const GraphicsPipelineCacheKey& a, const GraphicsPipelineCacheKey& b);

    static WUInt32 Hash(const ComputePipelineCacheKey& a);
    static bool Equal(const ComputePipelineCacheKey& a, const ComputePipelineCacheKey& b);
  };

private:
  WGALPipelineCache();
  ~WGALPipelineCache();
  void GALDeviceEventHandler(const WGALDeviceEvent& e);
  void Clear();

  template <typename HandleType, typename DescType, typename KeyType>
  W_ALWAYS_INLINE HandleType TryGetPipeline(const DescType& description, WHashTable<KeyType, HandleType, CacheKeyHasher>& table);

  template <typename HandleType, typename DescType, typename KeyType>
  W_ALWAYS_INLINE WResult TryInsertPipeline(const DescType& description, HandleType hNewPipeline, WHashTable<KeyType, HandleType, CacheKeyHasher>& table);

private:
  WMutex m_Mutex;
  WGALDevice* m_pDevice = nullptr;
  WHashTable<GraphicsPipelineCacheKey, WGALGraphicsPipelineHandle, CacheKeyHasher> m_GraphicsPipelines;
  WHashTable<ComputePipelineCacheKey, WGALComputePipelineHandle, CacheKeyHasher> m_ComputePipelines;
};

#include <RendererFoundation/State/Implementation/PipelineCache_inl.h>
