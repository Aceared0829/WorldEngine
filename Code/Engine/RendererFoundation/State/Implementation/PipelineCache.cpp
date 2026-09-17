#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/State/PipelineCache.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, PipelineCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WGALPipelineCache);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WGALPipelineCache* pDummy = WGALPipelineCache::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

W_IMPLEMENT_SINGLETON(WGALPipelineCache);

WGALPipelineCache::WGALPipelineCache()
  : m_SingletonRegistrar(this)
{
  WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WGALPipelineCache::GALDeviceEventHandler, this));
}

WGALPipelineCache::~WGALPipelineCache()
{
  WGALDevice::s_Events.RemoveEventHandler(WMakeDelegate(&WGALPipelineCache::GALDeviceEventHandler, this));
}

void WGALPipelineCache::GALDeviceEventHandler(const WGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case WGALDeviceEvent::AfterInit:
      m_pDevice = e.m_pDevice;
      break;
    case WGALDeviceEvent::BeforeShutdown:
      Clear();
      break;
    default:
      break;
  }
}

void WGALPipelineCache::Clear()
{
  W_LOCK(m_Mutex);

  for (auto it = m_GraphicsPipelines.GetIterator(); it.IsValid(); ++it)
  {
    m_pDevice->DestroyGraphicsPipeline(it.Value());
  }
  m_GraphicsPipelines.Clear();

  for (auto it = m_ComputePipelines.GetIterator(); it.IsValid(); ++it)
  {
    m_pDevice->DestroyComputePipeline(it.Value());
  }
  m_ComputePipelines.Clear();
}

WGALGraphicsPipelineHandle WGALPipelineCache::GetPipeline(const WGALGraphicsPipelineCreationDescription& description)
{
  WGALPipelineCache* pCache = WGALPipelineCache::GetSingleton();

  WGALGraphicsPipelineHandle hGraphicsPipeline = pCache->TryGetPipeline<WGALGraphicsPipelineHandle>(description, pCache->m_GraphicsPipelines);

  if (hGraphicsPipeline.IsInvalidated())
  {
    hGraphicsPipeline = pCache->m_pDevice->CreateGraphicsPipeline(description);
    if (hGraphicsPipeline.IsInvalidated())
    {
      return {};
    }

    if (pCache->TryInsertPipeline<WGALGraphicsPipelineHandle>(description, hGraphicsPipeline, pCache->m_GraphicsPipelines).Failed())
    {
      // Already created and inserted, reduce ref count again.
      pCache->m_pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);
    }
  }

  return hGraphicsPipeline;
}

WGALComputePipelineHandle WGALPipelineCache::GetPipeline(const WGALComputePipelineCreationDescription& description)
{
  WGALPipelineCache* pCache = WGALPipelineCache::GetSingleton();

  WGALComputePipelineHandle hComputePipeline = pCache->TryGetPipeline<WGALComputePipelineHandle>(description, pCache->m_ComputePipelines);

  if (hComputePipeline.IsInvalidated())
  {
    hComputePipeline = pCache->m_pDevice->CreateComputePipeline(description);
    if (hComputePipeline.IsInvalidated())
    {
      return {};
    }

    if (pCache->TryInsertPipeline<WGALComputePipelineHandle>(description, hComputePipeline, pCache->m_ComputePipelines).Failed())
    {
      // Already created and inserted, reduce ref count again.
      pCache->m_pDevice->DestroyComputePipeline(hComputePipeline);
    }
  }

  return hComputePipeline;
}

WUInt32 WGALPipelineCache::CacheKeyHasher::Hash(const WGALPipelineCache::GraphicsPipelineCacheKey& a)
{
  return a.m_uiHash;
}

bool WGALPipelineCache::CacheKeyHasher::Equal(const WGALPipelineCache::GraphicsPipelineCacheKey& a, const WGALPipelineCache::GraphicsPipelineCacheKey& b)
{
  return a.m_uiHash == b.m_uiHash && a.m_Desc == b.m_Desc;
}

WUInt32 WGALPipelineCache::CacheKeyHasher::Hash(const WGALPipelineCache::ComputePipelineCacheKey& a)
{
  return a.m_uiHash;
}

bool WGALPipelineCache::CacheKeyHasher::Equal(const WGALPipelineCache::ComputePipelineCacheKey& a, const WGALPipelineCache::ComputePipelineCacheKey& b)
{
  return a.m_uiHash == b.m_uiHash && a.m_Desc == b.m_Desc;
}


W_STATICLINK_FILE(RendererFoundation, RendererFoundation_State_Implementation_PipelineCache);
