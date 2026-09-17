#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderGraph/RenderGraphManager.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Utils/ResourceStateTracker.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderGraphManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WRenderGraphManager::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WRenderGraphManager::OnEngineShutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WEvent<const WRenderGraphRenderEvent&, WMutex> WRenderGraphManager::s_RenderEvent;
WMutex WRenderGraphManager::s_Mutex;
WDynamicArray<WSharedPtr<WRenderGraph>> WRenderGraphManager::s_EnqueuedRenderGraphs[3];
WDynamicArray<WRenderGraph*> WRenderGraphManager::s_AllRenderGraphs[3];
WUniquePtr<WRenderGraphResourcePool> WRenderGraphManager::s_pPool;
WUniquePtr<WGALResourceStateTracker> WRenderGraphManager::s_pStateTracker;
WDynamicArray<WSharedPtr<WRenderGraphPassObserver>> WRenderGraphManager::s_Observers;
WSharedPtr<WRenderGraph> WRenderGraphManager::s_pObserverGraph;
WDynamicArray<WRenderGraph*> WRenderGraphManager::s_ExecutingGraphs;
WDynamicArray<WRenderGraphPassObserver*> WRenderGraphManager::s_ExecutingObservers;
WUInt32 WRenderGraphManager::s_uiCurrentGraphIndex = 0;
WUInt32 WRenderGraphManager::s_uiCurrentPassIndex = 0;


void WRenderGraphManager::OnEngineStartup()
{
  WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WRenderGraphManager::GALDeviceEventHandler));
  WGALCommandEncoder::s_TextureBarrierValidationFailed.AddEventHandler(WMakeDelegate(&WRenderGraphManager::PrintTextureResourceHistory));
  WGALCommandEncoder::s_BufferBarrierValidationFailed.AddEventHandler(WMakeDelegate(&WRenderGraphManager::PrintBufferResourceHistory));
  InitPool(WGALDevice::GetDefaultDevice());
}

void WRenderGraphManager::OnEngineShutdown()
{
  s_pObserverGraph = nullptr;

  DeinitPool(WGALDevice::GetDefaultDevice());
  WGALDevice::s_Events.RemoveEventHandler(WMakeDelegate(&WRenderGraphManager::GALDeviceEventHandler));
  WGALCommandEncoder::s_TextureBarrierValidationFailed.RemoveEventHandler(WMakeDelegate(&WRenderGraphManager::PrintTextureResourceHistory));
  WGALCommandEncoder::s_BufferBarrierValidationFailed.RemoveEventHandler(WMakeDelegate(&WRenderGraphManager::PrintBufferResourceHistory));
  s_pPool.Clear();
  for (auto& bucket : s_EnqueuedRenderGraphs)
    bucket.Clear();
  W_ASSERT_DEV(s_AllRenderGraphs[WRenderGraphPhase::PreRender].IsEmpty(), "Not all PreRender-phase render graphs were destroyed before shutdown.");
  W_ASSERT_DEV(s_AllRenderGraphs[WRenderGraphPhase::Render].IsEmpty(), "Not all render-phase render graphs were destroyed before shutdown.");
  W_ASSERT_DEV(s_AllRenderGraphs[WRenderGraphPhase::PostRender].IsEmpty(), "Not all post-render-phase render graphs were destroyed before shutdown.");

  for (WUInt32 i = s_Observers.GetCount(); i > 0; --i)
  {
    if (s_Observers[i - 1]->GetRefCount() == 1)
    {
      s_Observers.RemoveAtAndSwap(i - 1);
      continue;
    }
  }

  W_ASSERT_DEV(s_Observers.IsEmpty(), "Not render graph observers were destroyed before shutdown.");
}

void WRenderGraphManager::GALDeviceEventHandler(const WGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case WGALDeviceEvent::BeforeBeginFrame:
      BeginFrame();
      break;
    case WGALDeviceEvent::AfterEndFrame:
    {
      W_ASSERT_DEV(s_EnqueuedRenderGraphs[0].IsEmpty() && s_EnqueuedRenderGraphs[1].IsEmpty() && s_EnqueuedRenderGraphs[2].IsEmpty(),
        "RenderAllGraphs must be called before end frame or a graph was registered after rendering finished.");
    }
    break;
    default:
      break;
  }
}

void WRenderGraphManager::InitPool(WGALDevice* pDevice)
{
  W_ASSERT_DEBUG(s_pPool == nullptr, "Render graph resource pool already initialized");
  s_pPool = W_DEFAULT_NEW(WRenderGraphResourcePool, pDevice);
  s_pStateTracker = W_DEFAULT_NEW(WGALResourceStateTracker, pDevice);
}

void WRenderGraphManager::DeinitPool(WGALDevice* pDevice)
{
  s_pStateTracker.Clear();
  for (auto& bucket : s_EnqueuedRenderGraphs)
    bucket.Clear();
  s_pPool.Clear();
}

void WRenderGraphManager::EnqueueRenderGraph(const WSharedPtr<WRenderGraph>& pRenderGraph)
{
  W_LOCK(s_Mutex);
  W_ASSERT_DEV(pRenderGraph->m_pCurrentPass == nullptr, "Can't enqueue a render graph that still has a pass open for recording");
  W_ASSERT_DEV(pRenderGraph->m_RenderGraphState == WRenderGraph::RenderGraphState::Recording, "Only graphs in the recording state can be enqueued");
  pRenderGraph->m_RenderGraphState = WRenderGraph::RenderGraphState::Enqueued;
  s_EnqueuedRenderGraphs[pRenderGraph->m_Phase.GetValue()].PushBack(pRenderGraph);
}

WSharedPtr<WRenderGraph> WRenderGraphManager::CreateRenderGraph(WStringView sName, WEnum<WRenderGraphPhase> phase)
{
  W_LOCK(s_Mutex);
  WSharedPtr<WRenderGraph> pGraph = W_DEFAULT_NEW(WRenderGraph, WGALDevice::GetDefaultDevice(), sName, phase);
  s_AllRenderGraphs[phase.GetValue()].PushBack(pGraph.Borrow());
  return pGraph;
}

void WRenderGraphManager::BeginFrame()
{
  W_LOCK(s_Mutex);
  W_PROFILE_SCOPE("WRenderGraphManager::BeginFrame");
  for (auto& bucket : s_EnqueuedRenderGraphs)
    bucket.Clear();
  s_pStateTracker->Clear();
}

void WRenderGraphManager::ExecuteRenderGraphs(WGALDevice* pDevice)
{
  W_PROFILE_SCOPE("WRenderGraphManager::ExecuteRenderGraphs");
  if (!s_pPool)
    return;

  {
    WRenderGraphRenderEvent ev;
    ev.m_Type = WRenderGraphRenderEvent::Type::BeginRender;
    s_RenderEvent.Broadcast(ev);
  }

  s_ExecutingGraphs.Clear();
  {
    W_PROFILE_SCOPE("CompileRenderGraphs");
    W_LOCK(s_Mutex);
    // Compile all registered graphs in phase order.
    for (auto& bucket : s_EnqueuedRenderGraphs)
    {
      for (auto& pRenderGraph : bucket)
      {
        // EnqueueRenderGraph takes a reference to the graph. If we are holding the last reference, the graph is no longer owned externally and must not be used anymore.
        if (pRenderGraph->GetRefCount() > 1 && pRenderGraph->Compile().Succeeded())
          s_ExecutingGraphs.PushBack(pRenderGraph.Borrow());
      }
    }
    s_ExecutingObservers.Clear();
    for (WUInt32 i = s_Observers.GetCount(); i > 0; --i)
    {
      if (s_Observers[i - 1]->GetRefCount() == 1)
      {
        s_Observers.RemoveAtAndSwap(i - 1);
        continue;
      }
      // Apply any pending requests while we hold the mutex.
      // After this point, m_Request is only accessed on the render thread.
      s_Observers[i - 1]->ApplyPendingRequest();
      s_ExecutingObservers.PushBack(s_Observers[i - 1].Borrow());
    }

    // Create observer render graph
    if (!s_ExecutingObservers.IsEmpty())
    {
      if (s_pObserverGraph == nullptr)
      {
        s_pObserverGraph = CreateRenderGraph("__OBSERVER__", WRenderGraphPhase::PostRender);
      }

      for (auto* pObserver : s_ExecutingObservers)
      {
        pObserver->RecordPreview(*s_pObserverGraph.Borrow());
      }

      if (s_pObserverGraph->Compile().Succeeded())
        s_ExecutingGraphs.PushBack(s_pObserverGraph.Borrow());
    }
  }

  {
    W_PROFILE_SCOPE("ComputeBarriers");
    for (auto pRenderGraph : s_ExecutingGraphs)
    {
      // Collect observers for this graph.
      WHybridArray<WRenderGraphPassObserver*, 4, WTempAllocatorWrapper> graphObservers;
      for (auto* pObserver : s_ExecutingObservers)
      {
        if (pObserver->m_pGraph == pRenderGraph)
        {
          pObserver->Reset();
          graphObservers.PushBack(pObserver);
        }
      }

      pRenderGraph->ComputeBarriers(*s_pStateTracker.Borrow(), graphObservers);
    }
  }



  WHybridArray<WGALTextureBarrier, 8, WTempAllocatorWrapper> textureBarriers;
  s_pStateTracker->RevertTextureState([&](const WGALTextureBarrier& barrier)
    { textureBarriers.PushBack(barrier); });
  WHybridArray<WGALBufferBarrier, 8, WTempAllocatorWrapper> bufferBarriers;
  s_pStateTracker->RevertBufferState([&](const WGALBufferBarrier& barrier)
    { bufferBarriers.PushBack(barrier); });

  // Execute all registered graphs.
  auto pEncoder = pDevice->BeginCommands("RenderGraph");
  WRenderGraphContext ctx(pEncoder, pDevice, WRenderContext::GetDefaultInstance());
  for (s_uiCurrentGraphIndex = 0; s_uiCurrentGraphIndex < s_ExecutingGraphs.GetCount(); ++s_uiCurrentGraphIndex)
  {
    // Collect valid observers for this graph.
    WHybridArray<WRenderGraphPassObserver*, 4, WTempAllocatorWrapper> graphObservers;
    for (auto* pObserver : s_ExecutingObservers)
    {
      if (pObserver->m_bValid && pObserver->m_pGraph == s_ExecutingGraphs[s_uiCurrentGraphIndex])
        graphObservers.PushBack(pObserver);
    }

    s_ExecutingGraphs[s_uiCurrentGraphIndex]->Execute(ctx, graphObservers);
  }
  pEncoder->TextureBarrier(textureBarriers);
  pEncoder->BufferBarrier(bufferBarriers);

  pDevice->EndCommands(pEncoder);
  s_pPool->EndFrame();
  s_pStateTracker->Clear();

  {
    WRenderGraphRenderEvent ev;
    ev.m_Type = WRenderGraphRenderEvent::Type::EndRender;
    s_RenderEvent.Broadcast(ev);
  }

  for (s_uiCurrentGraphIndex = 0; s_uiCurrentGraphIndex < s_ExecutingGraphs.GetCount(); ++s_uiCurrentGraphIndex)
  {
    // Graphs can only be executed once and then need to be re-recorded.
    s_ExecutingGraphs[s_uiCurrentGraphIndex]->ResetInternal(WRenderGraph::RenderGraphState::Recording);
  }
  s_ExecutingGraphs.Clear();

  for (auto& bucket : s_EnqueuedRenderGraphs)
    bucket.Clear();
}

void WRenderGraphManager::GetExecutionSummary(WRenderGraphInspectionSummary& out_summary)
{
  out_summary.m_RenderGraphs.Clear();
  out_summary.m_AvailableSwapChains.Clear();

  if (WGALDevice* pDevice = WGALDevice::GetDefaultDevice())
  {
    WTempArray<WGALSwapChainHandle> swapChains;
    pDevice->GetAllSwapChains(swapChains);
    out_summary.m_AvailableSwapChains.Reserve(swapChains.GetCount());
    for (WGALSwapChainHandle hSwapChain : swapChains)
    {
      WRenderGraphSwapChainSummary& swapChainSummary = out_summary.m_AvailableSwapChains.ExpandAndGetRef();
      swapChainSummary.m_uiSwapChainId = GetSwapChainId(hSwapChain);

      if (const WGALSwapChain* pSwapChain = pDevice->GetSwapChain(hSwapChain))
      {
        const WSizeU32 size = pSwapChain->GetCurrentSize();
        swapChainSummary.m_uiWidth = size.width;
        swapChainSummary.m_uiHeight = size.height;
      }
    }
  }

  for (WUInt32 i = 0; i < 3; ++i)
  {
    for (WRenderGraph* pGraph : s_AllRenderGraphs[i])
    {
      if (pGraph == s_pObserverGraph.Borrow())
        continue;
      WRenderGraphExecutionSummary& summary = out_summary.m_RenderGraphs.ExpandAndGetRef();
      summary.m_uiRenderGraphId = GetRenderGraphId(pGraph);
      summary.m_sGraphName = pGraph->GetGraphName();
      summary.m_sUserName = pGraph->GetUserName();
      summary.m_Phase = pGraph->m_Phase;

      summary.m_uiExecutionOrder = -1;
      for (WUInt32 j = 0; j < s_EnqueuedRenderGraphs[i].GetCount(); ++j)
      {
        if (s_EnqueuedRenderGraphs[i][j].Borrow() == pGraph)
        {
          summary.m_uiExecutionOrder = j;
          break;
        }
      }
    }
  }
}

WResult WRenderGraphManager::GetRenderGraphInspectionInfo(WUInt64 uiRenderGraphId, WRenderGraphInspectionInfo& out_inspectionInfo)
{
  if (WRenderGraph* pGraph = GetRenderGraphById(uiRenderGraphId))
  {
    return pGraph->GetInspectionInfo(out_inspectionInfo);
  }
  return W_FAILURE;
}

WUInt64 WRenderGraphManager::GetRenderGraphId(WRenderGraph* pGraph)
{
  return reinterpret_cast<WUInt64>(pGraph);
}

WRenderGraph* WRenderGraphManager::GetRenderGraphById(WUInt64 uiRenderGraphId)
{
  WRenderGraph* pGraph = reinterpret_cast<WRenderGraph*>(uiRenderGraphId);
  for (WUInt32 i = 0; i < 3; ++i)
  {
    for (WRenderGraph* pGraph2 : s_AllRenderGraphs[i])
    {
      if (pGraph == pGraph2)
      {
        return pGraph2;
      }
    }
  }
  return nullptr;
}

WUInt32 WRenderGraphManager::GetSwapChainId(WGALSwapChainHandle hSwapChain)
{
  return hSwapChain.GetInternalID().m_Data;
}

WGALSwapChainHandle WRenderGraphManager::GetSwapChainById(WUInt32 uiSwapChainId)
{
  WGALSwapChainHandle hSwapChain{WGALSwapChainHandle::IdType(uiSwapChainId)};
  if (WGALDevice* pDevice = WGALDevice::GetDefaultDevice())
  {
    if (pDevice->GetSwapChain(hSwapChain) != nullptr)
    {
      return hSwapChain;
    }
  }
  return WGALSwapChainHandle();
}

WSharedPtr<WRenderGraphPassObserver> WRenderGraphManager::CreateObserver()
{
  WSharedPtr<WRenderGraphPassObserver> observer = W_DEFAULT_NEW(WRenderGraphPassObserver, WGALDevice::GetDefaultDevice());
  s_Observers.PushBack(observer);
  return observer;
}

void WRenderGraphManager::OnGraphDestroyed(WRenderGraph* pGraph)
{
  s_AllRenderGraphs[pGraph->m_Phase.GetValue()].RemoveAndSwap(pGraph);
}

WRenderGraphResourcePool* WRenderGraphManager::GetResourcePool()
{
  return s_pPool.Borrow();
}

namespace
{
  bool RangesOverlap(const WGALTextureRange& a, const WGALTextureSubresource& subResource)
  {
    const WUInt32 uiSliceEnd = (WUInt32)a.m_uiBaseArraySlice + (WUInt32)a.m_uiArraySlices - 1;
    if (subResource.m_uiArraySlice < (WUInt32)a.m_uiBaseArraySlice || subResource.m_uiArraySlice > uiSliceEnd)
      return false;

    const WUInt32 uiMipEnd = (WUInt32)a.m_uiBaseMipLevel + (WUInt32)a.m_uiMipLevels - 1;
    if (subResource.m_uiMipLevel < (WUInt32)a.m_uiBaseMipLevel || subResource.m_uiMipLevel > uiMipEnd)
      return false;

    return true;
  }
} // namespace

void WRenderGraphManager::PrintTextureResourceHistory(const WTextureValidationError& error)
{
  WLog::Error("Bind group '{}' binding '{}': texture sub-resource [mip={}, slice={}] state mismatch. Tracked: {} [{}], Expected: {} [{}]",
    error.m_uiBindGroup, error.m_sBinding.GetData(), error.m_failedSubResource.m_uiMipLevel, error.m_failedSubResource.m_uiArraySlice, WArgEnum(error.m_actualState), WArgEnum(error.m_actualStages), WArgEnum(error.m_expectedState), WArgEnum(error.m_expectedStages));


  if (s_ExecutingGraphs.IsEmpty())
    return;

  // First check if the texture is known to any graphs.


  WLog::Error("=== Texture Resource History (handle {}, range=[mip={}, slice={}]) ===", error.m_hTexture.GetInternalID().m_Data, error.m_failedSubResource.m_uiMipLevel, error.m_failedSubResource.m_uiArraySlice);

  for (WUInt32 uiGraph = 0; uiGraph < s_ExecutingGraphs.GetCount(); ++uiGraph)
  {
    const WRenderGraph* pGraph = s_ExecutingGraphs[uiGraph];

    auto it = pGraph->m_ImportTextureToHandle.Find(error.m_hTexture);
    if (!it.IsValid())
    {
      if (uiGraph == s_uiCurrentGraphIndex)
      {
        const auto& compiled = pGraph->m_CompiledPasses[s_uiCurrentPassIndex];
        const auto& pass = pGraph->m_Passes[compiled.m_uiOriginalPassIndex];
        const char* szPassName = pGraph->m_PassNames[compiled.m_uiOriginalPassIndex].GetData();
        WLog::Error("{} Graph {} ({})", uiGraph == s_uiCurrentGraphIndex ? ">" : " ", pGraph->m_sGraphName, pGraph->m_sUserName);
        WLog::Error("  {} Pass[{}] '{}' ", ">", s_uiCurrentPassIndex, szPassName);
      }
      continue;
    }
    WStringBuilder sBlock;
    sBlock.SetFormat("{} Graph {} ({})", uiGraph == s_uiCurrentGraphIndex ? ">" : " ", pGraph->m_sGraphName, pGraph->m_sUserName);

    for (WUInt32 uiPass = 0; uiPass < pGraph->m_CompiledPasses.GetCount(); ++uiPass)
    {
      const auto& compiled = pGraph->m_CompiledPasses[uiPass];
      const auto& pass = pGraph->m_Passes[compiled.m_uiOriginalPassIndex];
      const char* szPassName = pGraph->m_PassNames[compiled.m_uiOriginalPassIndex].GetData();

      bool bHasActivity = false;
      WStringBuilder sOps;

      WHybridArray<WRenderGraph::TextureInfo, 1> overlapped;
      WHybridArray<WGALTextureBarrier, 1> overlappedBarriers;
      // Check read textures
      for (const WRenderGraph::TextureInfo& info : pass.GetReadTextures(pGraph))
      {
        const WUInt16 resolvedIdx = pGraph->m_TextureToResolvedTexture[info.m_hTexture.m_InternalId.m_InstanceIndex];
        if (pGraph->m_ResolvedTextures[resolvedIdx] == error.m_hTexture && RangesOverlap(info.m_range, error.m_failedSubResource))
        {
          overlapped.PushBack(info);
          bHasActivity = true;
        }
      }

      // Check write textures
      for (const WRenderGraph::TextureInfo& info : pass.GetWriteTextures(pGraph))
      {
        const WUInt16 resolvedIdx = pGraph->m_TextureToResolvedTexture[info.m_hTexture.m_InternalId.m_InstanceIndex];
        if (pGraph->m_ResolvedTextures[resolvedIdx] == error.m_hTexture && RangesOverlap(info.m_range, error.m_failedSubResource))
        {
          overlapped.PushBack(info);
          bHasActivity = true;
        }
      }

      // Check barriers for this pass
      for (const WGALTextureBarrier& barrier : compiled.GetTextureBarriers(pGraph))
      {
        if (barrier.m_hTexture != error.m_hTexture)
          continue;

        if (barrier.m_bAllSubresources)
        {
          overlappedBarriers.PushBack(barrier);
          bHasActivity = true;
        }
        else
        {
          const WGALTextureRange barrierRange = {
            static_cast<WUInt16>(barrier.m_Subresource.m_uiArraySlice), 1,
            static_cast<WUInt8>(barrier.m_Subresource.m_uiMipLevel), 1};
          if (RangesOverlap(barrierRange, error.m_failedSubResource))
          {
            overlappedBarriers.PushBack(barrier);
            bHasActivity = true;
          }
        }
      }

      if (bHasActivity || (uiGraph == s_uiCurrentGraphIndex) && s_uiCurrentPassIndex == uiPass)
      {
        if (!sBlock.IsEmpty())
        {
          WLog::Error("{}", sBlock);
          sBlock.Clear();
        }
        // W_ASSERT_DEBUG(overlapped.GetCount() == 1, ""); // overlappedBarriers.GetCount(), "");
        WStringBuilder sText;
        const bool bIsCurrent = (uiGraph == s_uiCurrentGraphIndex && uiPass == s_uiCurrentPassIndex);
        sText.SetFormat("  {} Pass[{}] '{}' ", bIsCurrent ? ">" : " ", uiPass, szPassName);
        for (const WRenderGraph::TextureInfo& info : overlapped)
        {
          sText.AppendFormat(", OP: state={}, stage={}, range=[mip={}+{}, slice={}+{}]",
            WArgEnum(info.m_access), WArgEnum(info.m_stage),
            info.m_range.m_uiBaseMipLevel, info.m_range.m_uiMipLevels,
            info.m_range.m_uiBaseArraySlice, info.m_range.m_uiArraySlices);
        }
        for (const WGALTextureBarrier& barrier : overlappedBarriers)
        {
          sText.AppendFormat(", BARRIER: {} -> {}, stages: {} -> {} (mip={}, slice={})",
            WArgEnum(barrier.m_StateBefore), WArgEnum(barrier.m_StateAfter),
            WArgEnum(barrier.m_StagesBefore), WArgEnum(barrier.m_StagesAfter),
            barrier.m_Subresource.m_uiMipLevel, barrier.m_Subresource.m_uiArraySlice);
        }

        WLog::Error("{}", sText.GetView());
      }
    }


    if (!sBlock.IsEmpty())
    {
      WLog::Error("{} : Imported but no barrier found", sBlock);
    }
  }

  WLog::Error("=== End Texture Resource History ===");
}

void WRenderGraphManager::PrintBufferResourceHistory(const WBufferValidationError& error)
{
  WLog::Error("Bind group '{}' binding '{}': buffer state mismatch. Tracked: {} [{}], Expected: {} [{}]",
    error.m_uiBindGroup, error.m_sBinding.GetData(), WArgEnum(error.m_actualState), WArgEnum(error.m_actualStages), WArgEnum(error.m_expectedState), WArgEnum(error.m_expectedStages));

  if (s_ExecutingGraphs.IsEmpty())
    return;

  WLog::Error("=== Buffer Resource History (handle {}) ===", error.m_hBuffer.GetInternalID().m_Data);

  for (WUInt32 uiGraph = 0; uiGraph < s_ExecutingGraphs.GetCount(); ++uiGraph)
  {
    const WRenderGraph* pGraph = s_ExecutingGraphs[uiGraph];

    auto it = pGraph->m_ImportBufferToHandle.Find(error.m_hBuffer);
    if (!it.IsValid())
    {
      if (uiGraph == s_uiCurrentGraphIndex)
      {
        const auto& compiled = pGraph->m_CompiledPasses[s_uiCurrentPassIndex];
        const char* szPassName = pGraph->m_PassNames[compiled.m_uiOriginalPassIndex].GetData();
        WLog::Error("{} Graph {} ({})", ">", pGraph->m_sGraphName, pGraph->m_sUserName);
        WLog::Error("  {} Pass[{}] '{}' ", ">", s_uiCurrentPassIndex, szPassName);
      }
      continue;
    }

    WStringBuilder sBlock;
    sBlock.SetFormat("{} Graph {} ({})", uiGraph == s_uiCurrentGraphIndex ? ">" : " ", pGraph->m_sGraphName, pGraph->m_sUserName);

    for (WUInt32 uiPass = 0; uiPass < pGraph->m_CompiledPasses.GetCount(); ++uiPass)
    {
      const auto& compiled = pGraph->m_CompiledPasses[uiPass];
      const auto& pass = pGraph->m_Passes[compiled.m_uiOriginalPassIndex];
      const char* szPassName = pGraph->m_PassNames[compiled.m_uiOriginalPassIndex].GetData();

      bool bHasActivity = false;
      WHybridArray<WRenderGraph::BufferInfo, 1> overlapped;
      WHybridArray<WGALBufferBarrier, 1> overlappedBarriers;

      for (const auto& info : pass.GetReadBuffers(pGraph))
      {
        const WUInt16 resolvedIdx = pGraph->m_BufferToResolvedBuffer[info.m_hBuffer.m_InternalId.m_InstanceIndex];
        if (pGraph->m_ResolvedBuffers[resolvedIdx] == error.m_hBuffer)
        {
          overlapped.PushBack(info);
          bHasActivity = true;
        }
      }

      for (const auto& info : pass.GetWriteBuffers(pGraph))
      {
        const WUInt16 resolvedIdx = pGraph->m_BufferToResolvedBuffer[info.m_hBuffer.m_InternalId.m_InstanceIndex];
        if (pGraph->m_ResolvedBuffers[resolvedIdx] == error.m_hBuffer)
        {
          overlapped.PushBack(info);
          bHasActivity = true;
        }
      }

      for (const auto& barrier : compiled.GetBufferBarriers(pGraph))
      {
        if (barrier.m_hBuffer == error.m_hBuffer)
        {
          overlappedBarriers.PushBack(barrier);
          bHasActivity = true;
        }
      }

      if (bHasActivity || ((uiGraph == s_uiCurrentGraphIndex) && s_uiCurrentPassIndex == uiPass))
      {
        if (!sBlock.IsEmpty())
        {
          WLog::Error("{}", sBlock);
          sBlock.Clear();
        }

        WStringBuilder sText;
        const bool bIsCurrent = (uiGraph == s_uiCurrentGraphIndex && uiPass == s_uiCurrentPassIndex);
        sText.SetFormat("  {} Pass[{}] '{}' ", bIsCurrent ? ">" : " ", uiPass, szPassName);
        for (const auto& info : overlapped)
        {
          sText.AppendFormat(", OP: state={}, stage={}",
            WArgEnum(info.m_access), WArgEnum(info.m_stage));
        }
        for (const auto& barrier : overlappedBarriers)
        {
          sText.AppendFormat(", BARRIER: {} -> {}, stages: {} -> {}",
            WArgEnum(barrier.m_StateBefore), WArgEnum(barrier.m_StateAfter),
            WArgEnum(barrier.m_StagesBefore), WArgEnum(barrier.m_StagesAfter));
        }

        WLog::Error("{}", sText.GetView());
      }
    }

    if (!sBlock.IsEmpty())
    {
      WLog::Error("{} : Imported but no barrier found", sBlock);
    }
  }

  WLog::Error("=== End Buffer Resource History ===");
}


W_STATICLINK_FILE(RendererCore, RendererCore_RenderGraph_Implementation_RenderGraphManager);
