#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraphContext.h>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphResourceAllocator.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

// --- Transient Resource Creation ---

WRenderGraph::WRenderGraph(WGALDevice* pDevice, WStringView sName, WEnum<WRenderGraphPhase> phase)
  : m_pDevice(pDevice)
  , m_sGraphName(sName)
  , m_Phase(phase)
{
  m_pAllocator = W_DEFAULT_NEW(WRenderGraphResourceAllocator, WRenderGraphManager::GetResourcePool());
}

WRenderGraph::~WRenderGraph()
{
  Reset();
  WRenderGraphManager::OnGraphDestroyed(this);
}

WRenderGraphTextureHandle WRenderGraph::CreateTexture(const WGALTextureCreationDescription& desc)
{
  auto id = WRenderGraphTextureHandle();
  id.m_InternalId.m_Generation = 0;
  id.m_InternalId.m_InstanceIndex = m_TextureCreationDescriptions.GetCount();

  W_ASSERT_DEBUG(desc.Validate(m_pDevice).Succeeded(), "Invalid texture desc");

  m_TextureCreationDescriptions.PushBack(desc);
  return id;
}

WRenderGraphBufferHandle WRenderGraph::CreateBuffer(const WGALBufferCreationDescription& desc)
{
  auto id = WRenderGraphBufferHandle();
  id.m_InternalId.m_Generation = 0;
  id.m_InternalId.m_InstanceIndex = m_BufferCreationDescriptions.GetCount();

  m_BufferCreationDescriptions.PushBack(desc);
  m_BufferCreationDescriptions.PeekBack().m_BufferFlags |= WGALBufferUsageFlags::Transient;
  return id;
}

// --- Resource Queries ---

const WGALTextureCreationDescription& WRenderGraph::GetTextureDesc(WRenderGraphTextureHandle hTexture) const
{
  W_ASSERT_DEBUG(hTexture.m_InternalId.m_InstanceIndex < m_TextureCreationDescriptions.GetCount(), "Invalid texture index");
  return m_TextureCreationDescriptions[hTexture.m_InternalId.m_InstanceIndex];
}

const WGALBufferCreationDescription& WRenderGraph::GetBufferDesc(WRenderGraphBufferHandle hBuffer) const
{
  W_ASSERT_DEBUG(hBuffer.m_InternalId.m_InstanceIndex < m_BufferCreationDescriptions.GetCount(), "Invalid Buffer index");
  return m_BufferCreationDescriptions[hBuffer.m_InternalId.m_InstanceIndex];
}

// --- Import External Resources ---

WRenderGraphTextureHandle WRenderGraph::ImportTexture(WGALTextureHandle hTexture, WBitflags<WGALResourceState> access, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Operation only allowed between Reset and EnqueueRenderGraph");
  if (WRenderGraphTextureHandle* hGraphTexture = m_ImportTextureToHandle.GetValue(hTexture))
  {
    // Replace the access state and stage flags on re-import.
    if (ImportedTexture* pImported = m_HandleToImportTexture.GetValue(*hGraphTexture))
    {
      pImported->m_access = access;
      pImported->m_stage = stage;
    }
    return *hGraphTexture;
  }

  auto id = WRenderGraphTextureHandle();
  id.m_InternalId.m_Generation = 0;
  id.m_InternalId.m_InstanceIndex = m_TextureCreationDescriptions.GetCount();
  const WGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
  if (pTexture)
  {
    m_TextureCreationDescriptions.PushBack(pTexture->GetDescription());
    m_HandleToImportTexture.Insert(id, ImportedTexture{hTexture, access, stage});
    m_ImportTextureToHandle.Insert(hTexture, id);
  }
  else
  {
    WLog::Error("Failed to import texture into render graph: invalid handle");
    id.Invalidate();
  }

  return id;
}

WRenderGraphBufferHandle WRenderGraph::ImportBuffer(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> access, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Operation only allowed between Reset and EnqueueRenderGraph");
  if (WRenderGraphBufferHandle* hGraphBuffer = m_ImportBufferToHandle.GetValue(hBuffer))
  {
    // Replace the access state and stage flags on re-import.
    if (ImportedBuffer* pImported = m_HandleToImportBuffer.GetValue(*hGraphBuffer))
    {
      pImported->m_access = access;
      pImported->m_stage = stage;
    }
    return *hGraphBuffer;
  }

  auto id = WRenderGraphBufferHandle();
  id.m_InternalId.m_Generation = 0;
  id.m_InternalId.m_InstanceIndex = m_BufferCreationDescriptions.GetCount();
  const WGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
  if (pBuffer)
  {
    m_BufferCreationDescriptions.PushBack(pBuffer->GetDescription());
    m_HandleToImportBuffer.Insert(id, ImportedBuffer{hBuffer, access, stage});
    m_ImportBufferToHandle.Insert(hBuffer, id);
  }
  else
  {
    WLog::Error("Failed to import Buffer into render graph: invalid handle");
    id.Invalidate();
  }

  return id;
}

WStatus WRenderGraph::ReplaceImportedTexture(WRenderGraphTextureHandle hGraphTexture, WGALTextureHandle hNewTexture)
{
  W_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Operation only allowed between Reset and EnqueueRenderGraph");

  auto it = m_HandleToImportTexture.Find(hGraphTexture);
  // New texture must not be already registered.
  if (m_ImportTextureToHandle.Find(hNewTexture).IsValid())
  {
    if (it.IsValid() && it.Value().m_hTextureHandle != hNewTexture)
      return WStatus(WFmt("Replacement texture handle already registered"));
  }
  WGALTextureCreationDescription& oldDesc = m_TextureCreationDescriptions[hGraphTexture.m_InternalId.m_InstanceIndex];
  const WGALTexture* pTexture = m_pDevice->GetTexture(hNewTexture);
  if (!pTexture)
    return WStatus(WFmt("Replacement texture is invalid"));

  WGALTextureCreationDescription newDesc = pTexture->GetDescription();
  if (oldDesc.m_TextureFlags.IsSet(WGALTextureUsageFlags::Presentable))
  {
    if (oldDesc.m_uiWidth != newDesc.m_uiWidth)
      return WStatus(W_FAILURE);
    if (oldDesc.m_uiHeight != newDesc.m_uiHeight)
      return WStatus(W_FAILURE);
  }

  if (oldDesc.m_uiWidth > newDesc.m_uiWidth)
    return WStatus(WFmt("Replacement texture width {} is less than the original texture width {}", newDesc.m_uiWidth, oldDesc.m_uiWidth));

  if (oldDesc.m_uiHeight > newDesc.m_uiHeight)
    return WStatus(WFmt("Replacement texture height {} is less than the original texture height {}", newDesc.m_uiHeight, oldDesc.m_uiHeight));

  if (oldDesc.m_uiDepth > newDesc.m_uiDepth)
    return WStatus(WFmt("Replacement texture depth {} is less than the original texture depth {}", newDesc.m_uiDepth, oldDesc.m_uiDepth));

  if (oldDesc.m_uiArraySize > newDesc.m_uiArraySize)
    return WStatus(WFmt("Replacement texture array size {} is less than the original texture array size {}", newDesc.m_uiArraySize, oldDesc.m_uiArraySize));

  if (oldDesc.m_uiMipLevelCount > newDesc.m_uiMipLevelCount)
    return WStatus(WFmt("Replacement texture mip levels {} is less than the original texture mip levels {}", newDesc.m_uiMipLevelCount, oldDesc.m_uiMipLevelCount));

  if (oldDesc.m_Format != newDesc.m_Format)
    return WStatus(WFmt("Replacement texture format {} is different from original texture format {}", WArgEnum(newDesc.m_Format), WArgEnum(oldDesc.m_Format)));

  if (oldDesc.m_SampleCount != newDesc.m_SampleCount)
    return WStatus(WFmt("Replacement sample count {} is different from original sample count {}", newDesc.m_SampleCount, oldDesc.m_SampleCount));

  // if (oldDesc.m_Type != newDesc.m_Type)
  //   return WStatus(WFmt("Replacement texture type {} is different from original texture type {}", WArgEnum(newDesc.m_Type), WArgEnum(oldDesc.m_Type)));

  if (!newDesc.m_TextureFlags.AreAllSet(oldDesc.m_TextureFlags))
    return WStatus(WFmt("Replacement texture flags {} does not contain some of the original flags {}", WArgEnum(newDesc.m_TextureFlags), WArgEnum(oldDesc.m_TextureFlags)));

  // Is the target graph texture not imported yet?
  if (!it.IsValid())
  {
    oldDesc = newDesc;
    m_HandleToImportTexture.Insert(hGraphTexture, ImportedTexture{hNewTexture, newDesc.GetDefaultState(), WGALShaderStageFlags::Auto});
    m_ImportTextureToHandle.Insert(hNewTexture, hGraphTexture);
    return W_SUCCESS;
  }

  if (it.Value().m_hTextureHandle == hNewTexture)
    return W_SUCCESS;

  m_ImportTextureToHandle.Remove(it.Value().m_hTextureHandle);
  it.Value().m_hTextureHandle = hNewTexture;
  m_ImportTextureToHandle[hNewTexture] = hGraphTexture;

  return W_SUCCESS;
}

// --- Pass Creation ---

WRenderGraphPassBuilder WRenderGraph::AddGraphicsPass(WStringView sName)
{
  W_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Reset graph first before recording passes again");
  W_ASSERT_DEBUG(m_pCurrentPass == nullptr, "An Add*Pass scope is already open");
  StartPassBuilder(WGALQueueType::Graphics, sName);
  return WRenderGraphPassBuilder(this);
}

WRenderGraphPassBuilder WRenderGraph::AddComputePass(WStringView sName)
{
  W_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Reset graph first before recording passes again");
  W_ASSERT_DEBUG(m_pCurrentPass == nullptr, "An Add*Pass scope is already open");
  StartPassBuilder(WGALQueueType::Compute, sName);
  return WRenderGraphPassBuilder(this);
}

WRenderGraphPassBuilder WRenderGraph::AddTransferPass(WStringView sName)
{
  W_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Reset graph first before recording passes again");
  W_ASSERT_DEBUG(m_pCurrentPass == nullptr, "An Add*Pass scope is already open");
  StartPassBuilder(WGALQueueType::Transfer, sName);
  return WRenderGraphPassBuilder(this);
}

// --- Compilation & Execution ---

void WRenderGraph::PushMarker(WStringView sMarker)
{
  W_ASSERT_DEV(m_RenderGraphState == RenderGraphState::Recording, "PushMarker can only be called during recording.");
  auto& evt = m_MarkerEvents.ExpandAndGetRef();
  evt.m_uiPassIndex = static_cast<WUInt16>(m_Passes.GetCount());
  evt.m_bPush = true;
  evt.m_sName = sMarker;
}

void WRenderGraph::PopMarker()
{
  W_ASSERT_DEV(m_RenderGraphState == RenderGraphState::Recording, "PopMarker can only be called during recording.");
  auto& evt = m_MarkerEvents.ExpandAndGetRef();
  evt.m_uiPassIndex = static_cast<WUInt16>(m_Passes.GetCount());
  evt.m_bPush = false;
}

void WRenderGraph::Reset()
{
  ResetInternal(RenderGraphState::Recording);
}

WResult WRenderGraph::Compile()
{
  W_PROFILE_SCOPE("Compile");

  if (m_RenderGraphState < RenderGraphState::Compiled)
  {
    W_SUCCEED_OR_RETURN(ValidateGraph());
    BuildDependencyGraph();
    CullDeadPasses();
    BuildSortedPassList();
    ComputeResourceLifetimes();
    AllocateTransientResources();
    BuildRenderingSetups();
    m_RenderGraphState = RenderGraphState::Compiled;
  }
  return W_SUCCESS;
}

WResult WRenderGraph::GetInspectionInfo(WRenderGraphInspectionInfo& out_info) const
{
  if (m_RenderGraphState < RenderGraphState::Compiled)
    return W_FAILURE;

  // All passes in declaration order, with alive flag.
  const WUInt32 uiNumPasses = m_Passes.GetCount();
  out_info.m_Passes.SetCount(uiNumPasses);
  for (WUInt32 i = 0; i < uiNumPasses; ++i)
  {
    auto& pi = out_info.m_Passes[i];
    pi.m_sName = m_PassNames[i];
    pi.m_QueueType = m_Passes[i].m_QueueType;
    pi.m_bHasSideEffects = m_Passes[i].m_bHasSideEffects;
    pi.m_bAlive = m_Alive[i];
  }

  // Textures
  const WUInt32 uiNumTextures = m_TextureCreationDescriptions.GetCount();
  out_info.m_Textures.SetCount(uiNumTextures);
  for (WUInt32 i = 0; i < uiNumTextures; ++i)
  {
    auto& ti = out_info.m_Textures[i];
    ti.m_Desc = m_TextureCreationDescriptions[i];

    WRenderGraphTextureHandle hTex;
    hTex.m_InternalId.m_InstanceIndex = i;
    hTex.m_InternalId.m_Generation = 0;
    ti.m_bImported = m_HandleToImportTexture.Contains(hTex);
    ti.m_uiFirstUsePassIndex = m_TextureFirstUse[i];
    ti.m_uiLastUsePassIndex = m_TextureLastUse[i];
    ti.m_uiResolvedIndex = m_TextureToResolvedTexture[i];
  }

  // Buffers
  const WUInt32 uiNumBuffers = m_BufferCreationDescriptions.GetCount();
  out_info.m_Buffers.SetCount(uiNumBuffers);
  for (WUInt32 i = 0; i < uiNumBuffers; ++i)
  {
    auto& bi = out_info.m_Buffers[i];
    bi.m_Desc = m_BufferCreationDescriptions[i];

    WRenderGraphBufferHandle hBuf;
    hBuf.m_InternalId.m_InstanceIndex = i;
    hBuf.m_InternalId.m_Generation = 0;
    bi.m_bImported = m_HandleToImportBuffer.Contains(hBuf);
    bi.m_uiFirstUsePassIndex = m_BufferFirstUse[i];
    bi.m_uiLastUsePassIndex = m_BufferLastUse[i];
    bi.m_uiResolvedIndex = m_BufferToResolvedBuffer[i];
  }

  // Accesses - flatten all pass resource accesses (all passes, not just alive)
  out_info.m_Accesses.Clear();
  out_info.m_Accesses.Reserve(m_ReadBuffers.GetCount() + m_WriteBuffers.GetCount() + m_ReadTextures.GetCount() + m_WriteTextures.GetCount());
  for (WUInt32 passIdx = 0; passIdx < uiNumPasses; ++passIdx)
  {
    const Pass& pass = m_Passes[passIdx];
    WUInt16 uiAccessIndexInPass = 0;
    for (const TextureInfo& info : pass.GetReadTextures(this))
    {
      auto& a = out_info.m_Accesses.ExpandAndGetRef();
      a.m_uiPassIndex = static_cast<WUInt16>(passIdx);
      a.m_uiResourceIndex = info.m_hTexture.m_InternalId.m_InstanceIndex;
      a.m_uiAccessIndex = uiAccessIndexInPass++;
      a.m_bIsTexture = true;
      a.m_Access = info.m_access;
      a.m_TextureRange = info.m_range;
    }
    for (const TextureInfo& info : pass.GetWriteTextures(this))
    {
      auto& a = out_info.m_Accesses.ExpandAndGetRef();
      a.m_uiPassIndex = static_cast<WUInt16>(passIdx);
      a.m_uiResourceIndex = info.m_hTexture.m_InternalId.m_InstanceIndex;
      a.m_uiAccessIndex = uiAccessIndexInPass++;
      a.m_bIsTexture = true;
      a.m_Access = info.m_access;
      a.m_TextureRange = info.m_range;
    }
    for (const BufferInfo& info : pass.GetReadBuffers(this))
    {
      auto& a = out_info.m_Accesses.ExpandAndGetRef();
      a.m_uiPassIndex = static_cast<WUInt16>(passIdx);
      a.m_uiResourceIndex = info.m_hBuffer.m_InternalId.m_InstanceIndex;
      a.m_uiAccessIndex = uiAccessIndexInPass++;
      a.m_bIsTexture = false;
      a.m_Access = info.m_access;
    }
    for (const BufferInfo& info : pass.GetWriteBuffers(this))
    {
      auto& a = out_info.m_Accesses.ExpandAndGetRef();
      a.m_uiPassIndex = static_cast<WUInt16>(passIdx);
      a.m_uiResourceIndex = info.m_hBuffer.m_InternalId.m_InstanceIndex;
      a.m_uiAccessIndex = uiAccessIndexInPass++;
      a.m_bIsTexture = false;
      a.m_Access = info.m_access;
    }
  }

  return W_SUCCESS;
}

void WRenderGraph::Execute(WRenderGraphContext& ref_ctx, WArrayPtr<WRenderGraphPassObserver*> observers)
{
  W_ASSERT_DEV(m_RenderGraphState == RenderGraphState::BarriersCreated, "Graph must be compiled before execution");

  ref_ctx.m_pTextureToResolvedTexture = &m_TextureToResolvedTexture;
  ref_ctx.m_pBufferToResolvedBuffer = &m_BufferToResolvedBuffer;
  ref_ctx.m_pResolvedTextures = &m_ResolvedTextures;
  ref_ctx.m_pResolvedBuffers = &m_ResolvedBuffers;
  ref_ctx.m_pUserData = GetUserData();

  // We need to purge bind groups between graphs as otherwise unintended resources get bound that we can't reason about and thus are unable to barrier correctly. E.g. a depth pre-pass might bind the deferred decal atlas from a previous pipeline run.
  ref_ctx.GetRenderContext()->ResetBindGroups();

  {
    WRenderGraphRenderEvent ev;
    ev.m_Type = WRenderGraphRenderEvent::Type::BeforeGraphExecution;
    ev.m_pGraph = this;
    ev.m_pContext = &ref_ctx;
    WRenderGraphManager::s_RenderEvent.Broadcast(ev);
  }

  WStringBuilder sName = m_sGraphName;
  if (!m_sUserName.IsEmpty())
    sName.AppendFormat("-{}", m_sUserName);

  W_PROFILE_AND_MARKER(ref_ctx.GetCommandEncoder(), sName.GetData());

  WHybridArray<GPUTimingScope*, 4> scopes;
  auto ExecuteMarker = [&](const MarkerEvent& evt, WGALCommandEncoder* pEncoder)
  {
#if W_ENABLED(W_USE_PROFILING)
    if (evt.m_bPush)
    {
      scopes.PushBack(WProfilingScopeAndMarker::Start(pEncoder, evt.m_sName.GetData()));
    }
    else
    {
      if (!scopes.IsEmpty())
        WProfilingScopeAndMarker::Stop(pEncoder, scopes.PeekBack());
      scopes.PopBack();
    }
#endif
  };

  WUInt32 uiNextMarker = 0;
  const WUInt32 uiMarkerCount = m_MarkerEvents.GetCount();
  const WUInt32 uiPasses = m_CompiledPasses.GetCount();
  for (WUInt32 i = 0; i < uiPasses; ++i)
  {
    WRenderGraphManager::s_uiCurrentPassIndex = i;
    const CompiledPass& compiled = m_CompiledPasses[i];
    const Pass& pass = m_Passes[compiled.m_uiOriginalPassIndex];
    const char* szName = m_PassNames[compiled.m_uiOriginalPassIndex].GetData();

    // Emit any markers scheduled before this pass.
    while (uiNextMarker < uiMarkerCount && m_MarkerEvents[uiNextMarker].m_uiPassIndex <= compiled.m_uiOriginalPassIndex)
    {
      ExecuteMarker(m_MarkerEvents[uiNextMarker], ref_ctx.GetCommandEncoder());
      ++uiNextMarker;
    }

    // Issue barriers before the pass.
    auto textureBarriers = compiled.GetTextureBarriers(this);
    if (!textureBarriers.IsEmpty())
    {
      ref_ctx.m_pCommandEncoder->TextureBarrier(textureBarriers);
    }
    auto bufferBarriers = compiled.GetBufferBarriers(this);
    if (!bufferBarriers.IsEmpty())
    {
      ref_ctx.m_pCommandEncoder->BufferBarrier(bufferBarriers);
    }

    ref_ctx.GetRenderContext()->ResetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
    ref_ctx.GetRenderContext()->ResetBindGroup(W_GAL_BIND_GROUP_MATERIAL);
    ref_ctx.GetRenderContext()->ResetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);

    // Begin the appropriate scope and invoke the callback.
    switch (pass.m_QueueType)
    {
      case WGALQueueType::Graphics:
      {
        WSizeU32 size = compiled.m_RenderingSetup.GetFrameBuffer().m_Size;
        WRectFloat viewport{0, 0, (float)size.width, (float)size.height};
        if (size.HasNonZeroArea())
        {
          ref_ctx.m_pRenderContext->BeginRendering(compiled.m_RenderingSetup, viewport, szName, pass.m_bStereoscopic);
          if (pass.m_ExecuteFunction.IsValid())
            pass.m_ExecuteFunction(ref_ctx);
          ref_ctx.m_pRenderContext->EndRendering();
        }
        break;
      }
      case WGALQueueType::Compute:
      {
        ref_ctx.m_pRenderContext->BeginCompute(szName);
        if (pass.m_ExecuteFunction.IsValid())
          pass.m_ExecuteFunction(ref_ctx);
        ref_ctx.m_pRenderContext->EndCompute();
        break;
      }
      case WGALQueueType::Transfer:
      {
        // Transfer pass - no scope required.
        ref_ctx.m_pCommandEncoder->PushMarker(szName);
        if (pass.m_ExecuteFunction.IsValid())
          pass.m_ExecuteFunction(ref_ctx);
        ref_ctx.m_pCommandEncoder->PopMarker();
        break;
      }
      default:
        W_REPORT_FAILURE("Invalid queue type");
        break;
    }

    // Execute observer copies after this pass.
    for (auto* pObserver : observers)
    {
      if (!pObserver->m_bValid || pObserver->m_uiSortedPassIndex != (WUInt32)i)
        continue;

      if (!pObserver->m_PreCopyBarriers.IsEmpty())
        ref_ctx.m_pCommandEncoder->TextureBarrier(pObserver->m_PreCopyBarriers);

      ref_ctx.m_pCommandEncoder->CopyTexture(pObserver->GetCopyTexture(), pObserver->m_hResolvedSourceTexture);

      if (!pObserver->m_PostCopyBarriers.IsEmpty())
        ref_ctx.m_pCommandEncoder->TextureBarrier(pObserver->m_PostCopyBarriers);
    }
  }

  // Emit any trailing markers (after the last pass).
  while (uiNextMarker < uiMarkerCount)
  {
    ExecuteMarker(m_MarkerEvents[uiNextMarker], ref_ctx.GetCommandEncoder());
    ++uiNextMarker;
  }

  {
    WRenderGraphRenderEvent ev;
    ev.m_Type = WRenderGraphRenderEvent::Type::AfterGraphExecution;
    ev.m_pGraph = this;
    ev.m_pContext = &ref_ctx;
    WRenderGraphManager::s_RenderEvent.Broadcast(ev);
  }
}

void WRenderGraph::StartPassBuilder(WEnum<WGALQueueType> queueType, WStringView sName)
{
  // Enforce unique pass names by appending a counter suffix on collision.
  WString sUniqueName = sName;
  WUInt32& uiSuffix = m_UniquePassNames.FindOrAdd(sUniqueName);
  uiSuffix++;
  if (uiSuffix >= 2)
  {
    WStringBuilder sTemp;
    sTemp.SetFormat("{} #{}", sName, uiSuffix);
    sUniqueName = sTemp;
  }

  m_PassNames.PushBack(std::move(sUniqueName));
  Pass& pass = m_Passes.ExpandAndGetRef();
  pass.m_QueueType = queueType;
  pass.m_uiReadTextureIndex = m_ReadTextures.GetCount();
  pass.m_uiWriteTextureIndex = m_WriteTextures.GetCount();
  pass.m_uiReadBufferIndex = m_ReadBuffers.GetCount();
  pass.m_uiWriteBufferIndex = m_WriteBuffers.GetCount();
  pass.m_uiColorTargetIndex = m_ColorTargets.GetCount();
  pass.m_uiDepthStencilTargetIndex = m_DepthStencilTargets.GetCount();
  pass.m_uiClearColorIndex = m_ClearColors.GetCount();
  pass.m_uiDepthStencilClearIndex = m_ClearDepthStencils.GetCount();
  m_pCurrentPass = &pass;
}

void WRenderGraph::EndPassBuilder()
{
  // Just clear the pointer as the Pass was already filled in.
  m_pCurrentPass = nullptr;
}

WUInt16 WRenderGraph::FindPassByName(WStringView sName) const
{
  for (WUInt32 i = 0; i < m_PassNames.GetCount(); ++i)
  {
    if (m_PassNames[i] == sName)
      return static_cast<WUInt16>(i);
  }
  return s_Unused;
}

WRenderGraphTextureHandle WRenderGraph::FindTextureAccessInPass(WUInt16 uiOriginalPassIndex, WUInt16 uiAccessIndex) const
{
  W_ASSERT_DEBUG(uiOriginalPassIndex < m_Passes.GetCount(), "Invalid pass index");
  const Pass& pass = m_Passes[uiOriginalPassIndex];

  // Iterate all texture accesses in declaration order: reads, writes, color targets, depth targets.
  WUInt16 uiCurrent = 0;

  for (const TextureInfo& info : pass.GetReadTextures(this))
  {
    if (uiCurrent == uiAccessIndex)
      return info.m_hTexture;
    ++uiCurrent;
  }
  for (const TextureInfo& info : pass.GetWriteTextures(this))
  {
    if (uiCurrent == uiAccessIndex)
      return info.m_hTexture;
    ++uiCurrent;
  }

  WRenderGraphTextureHandle invalid;
  invalid.Invalidate();
  return invalid;
}

// --- Private Compile Steps ---

void WRenderGraph::ResetInternal(RenderGraphState renderGraphState)
{
  if (renderGraphState < RenderGraphState::BarriersCreated)
  {
    // Reset barriers
    m_CompiledTextureBarriers.Clear();
    m_CompiledBufferBarriers.Clear();
    if (renderGraphState == RenderGraphState::Compiled)
    {
      for (CompiledPass& pass : m_CompiledPasses)
      {
        pass.m_uiTextureBarrierIndex = 0;
        pass.m_uiBufferBarrierIndex = 0;
        pass.m_uiTextureBarrierCount = 0;
        pass.m_uiBufferBarrierCount = 0;
      }
    }
  }

  if (renderGraphState < RenderGraphState::Compiled)
  {
    // Reset compile state
    m_CompiledPasses.Clear();

    // Reset intermediate state
    m_AdjacencyStorage.Clear();
    m_Alive.Clear();
    m_AlivePasses.Clear();
    m_TextureFirstUse.Clear();
    m_TextureLastUse.Clear();
    m_BufferFirstUse.Clear();
    m_BufferLastUse.Clear();

    m_AcquireTextures.Clear();
    m_ReleaseTextures.Clear();
    m_AcquireBuffers.Clear();
    m_ReleaseBuffers.Clear();
    m_TextureToResolvedTexture.Clear();
    m_BufferToResolvedBuffer.Clear();

    m_ResolvedTextures.Clear();
    m_ResolvedBuffers.Clear();

    m_pAllocator->FreeResources();
  }

  if (renderGraphState < RenderGraphState::Enqueued)
  {
    // Reset recording state
    m_pCurrentPass = nullptr;

    m_PassNames.Clear();
    m_UniquePassNames.Clear();
    m_Passes.Clear();
    m_ReadTextures.Clear();
    m_WriteTextures.Clear();
    m_ReadBuffers.Clear();
    m_WriteBuffers.Clear();
    m_ColorTargets.Clear();
    m_DepthStencilTargets.Clear();
    m_ClearColors.Clear();
    m_ClearDepthStencils.Clear();
    m_MarkerEvents.Clear();

    m_TextureCreationDescriptions.Clear();
    m_BufferCreationDescriptions.Clear();

    m_HandleToImportTexture.Clear();
    m_ImportTextureToHandle.Clear();
    m_HandleToImportBuffer.Clear();
    m_ImportBufferToHandle.Clear();
  }

  m_RenderGraphState = renderGraphState;
}


WResult WRenderGraph::ValidateImportedResources() const
{
  for (auto it = m_HandleToImportTexture.GetIterator(); it.IsValid(); ++it)
  {
    if (m_pDevice->GetTexture(it.Value().m_hTextureHandle) == nullptr)
    {
      WLog::Error("Render graph '{}': imported texture handle is no longer valid.", m_sUserName);
      return W_FAILURE;
    }
  }

  for (auto it = m_HandleToImportBuffer.GetIterator(); it.IsValid(); ++it)
  {
    if (m_pDevice->GetBuffer(it.Value().m_hBufferHandle) == nullptr)
    {
      WLog::Error("Render graph '{}': imported buffer handle is no longer valid.", m_sUserName);
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

WResult WRenderGraph::ValidateGraph()
{
  W_PROFILE_SCOPE("ValidateGraph");
  for (WUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    const Pass& pass = m_Passes[i];

    // Validate texture handle indices.
    auto validateTextureHandle = [&](WRenderGraphTextureHandle h, const char* szContext) -> WResult
    {
      if (h.IsInvalidated() || h.m_InternalId.m_InstanceIndex >= m_TextureCreationDescriptions.GetCount())
      {
        WLog::Error("Pass '{}': invalid texture handle in {}.", m_PassNames[i], szContext);
        return W_FAILURE;
      }
      return W_SUCCESS;
    };

    auto validateBufferHandle = [&](WRenderGraphBufferHandle h, const char* szContext) -> WResult
    {
      if (h.IsInvalidated() || h.m_InternalId.m_InstanceIndex >= m_BufferCreationDescriptions.GetCount())
      {
        WLog::Error("Pass '{}': invalid buffer handle in {}.", m_PassNames[i], szContext);
        return W_FAILURE;
      }
      return W_SUCCESS;
    };

    for (const TextureInfo& info : pass.GetReadTextures(this))
    {
      W_SUCCEED_OR_RETURN(validateTextureHandle(info.m_hTexture, "ReadTexture"));
    }
    for (const TextureInfo& info : pass.GetWriteTextures(this))
    {
      W_SUCCEED_OR_RETURN(validateTextureHandle(info.m_hTexture, "WriteTexture"));
    }
    for (const BufferInfo& info : pass.GetReadBuffers(this))
    {
      W_SUCCEED_OR_RETURN(validateBufferHandle(info.m_hBuffer, "ReadBuffer"));
    }
    for (const BufferInfo& info : pass.GetWriteBuffers(this))
    {
      W_SUCCEED_OR_RETURN(validateBufferHandle(info.m_hBuffer, "WriteBuffer"));
    }
    for (const ColorTargetInfo& info : pass.GetColorTargets(this))
    {
      W_SUCCEED_OR_RETURN(validateTextureHandle(info.m_hTexture, "ColorTarget"));
    }
    for (const DepthStencilTargetInfo& info : pass.GetDepthStencilTargets(this))
    {
      W_SUCCEED_OR_RETURN(validateTextureHandle(info.m_hTexture, "DepthStencilTarget"));
    }
  }

  return W_SUCCESS;
}

void WRenderGraph::BuildDependencyGraph()
{
  W_PROFILE_SCOPE("BuildDependencyGraph");
  const WUInt32 uiNumPasses = m_Passes.GetCount();

  // Compute per-pass adjacency capacity and assign offsets into the flat storage.
  // Upper bound per pass: one dependency per resource access.
  WUInt32 uiTotalCapacity = 0;
  for (WUInt32 i = 0; i < uiNumPasses; ++i)
  {
    Pass& pass = m_Passes[i];
    pass.m_uiAdjacencyIndex = static_cast<WUInt16>(uiTotalCapacity);
    pass.m_uiAdjacencyCount = 0;

    const WUInt32 uiMaxDeps = pass.m_uiReadTextureCount + pass.m_uiReadBufferCount +
                               pass.m_uiWriteTextureCount + pass.m_uiWriteBufferCount;
    uiTotalCapacity += uiMaxDeps;
  }
  m_AdjacencyStorage.SetCount(uiTotalCapacity);

  // Helper: add a dependency edge from pass to writerIdx, deduplicating.
  auto addDependency = [this](Pass& pass, WUInt16 passIdx, WUInt16 writerIdx)
  {
    if (writerIdx == passIdx)
      return;

    // Check for duplicates in the current slice.
    const WUInt16* pBegin = m_AdjacencyStorage.GetData() + pass.m_uiAdjacencyIndex;
    const WUInt16* pEnd = pBegin + pass.m_uiAdjacencyCount;
    for (const WUInt16* p = pBegin; p < pEnd; ++p)
    {
      if (*p == writerIdx)
        return;
    }

    m_AdjacencyStorage[pass.m_uiAdjacencyIndex + pass.m_uiAdjacencyCount] = writerIdx;
    ++pass.m_uiAdjacencyCount;
  };

  // We iterate passes in declaration order and track the latest writer per resource.
  WHashTable<WUInt32, WUInt16> textureLastWriter(WFrameAllocator::GetCurrentAllocator()); // texture instance index -> pass index
  textureLastWriter.Reserve(m_TextureCreationDescriptions.GetCount());
  WHashTable<WUInt32, WUInt16> bufferLastWriter(WFrameAllocator::GetCurrentAllocator());
  bufferLastWriter.Reserve(m_BufferCreationDescriptions.GetCount());

  for (WUInt16 passIdx = 0; passIdx < uiNumPasses; ++passIdx)
  {
    Pass& pass = m_Passes[passIdx];

    // Check reads: if this pass reads a resource that was written by an earlier pass, add a dependency.
    for (const TextureInfo& info : pass.GetReadTextures(this))
    {
      const WUInt32 uiTexIdx = info.m_hTexture.m_InternalId.m_InstanceIndex;
      WUInt16 writerIdx;
      if (textureLastWriter.TryGetValue(uiTexIdx, writerIdx))
      {
        addDependency(pass, passIdx, writerIdx);
      }
    }
    for (const BufferInfo& info : pass.GetReadBuffers(this))
    {
      const WUInt32 uiBufIdx = info.m_hBuffer.m_InternalId.m_InstanceIndex;
      WUInt16 writerIdx;
      if (bufferLastWriter.TryGetValue(uiBufIdx, writerIdx))
      {
        addDependency(pass, passIdx, writerIdx);
      }
    }

    // Also check write-after-write: if this pass writes a resource already written by another pass.
    for (const TextureInfo& info : pass.GetWriteTextures(this))
    {
      const WUInt32 uiTexIdx = info.m_hTexture.m_InternalId.m_InstanceIndex;
      WUInt16 writerIdx;
      if (textureLastWriter.TryGetValue(uiTexIdx, writerIdx))
      {
        addDependency(pass, passIdx, writerIdx);
      }
      textureLastWriter[uiTexIdx] = passIdx;
    }
    for (const BufferInfo& info : pass.GetWriteBuffers(this))
    {
      const WUInt32 uiBufIdx = info.m_hBuffer.m_InternalId.m_InstanceIndex;
      WUInt16 writerIdx;
      if (bufferLastWriter.TryGetValue(uiBufIdx, writerIdx))
      {
        addDependency(pass, passIdx, writerIdx);
      }
      bufferLastWriter[uiBufIdx] = passIdx;
    }

    // Color targets are writes and Depth/stencil targets are already added to the texture read / write arrays during recording.
  }
}

void WRenderGraph::CullDeadPasses()
{
  W_PROFILE_SCOPE("CullDeadPasses");
  const WUInt32 uiNumPasses = m_Passes.GetCount();

  // Start with all passes marked as dead.
  m_Alive.SetCount(uiNumPasses);
  for (WUInt32 i = 0; i < uiNumPasses; ++i)
  {
    m_Alive[i] = false;
  }

  // Seed the alive set with passes that have side effects or write to imported resources.
  WHybridArray<WUInt16, 16, WTempAllocatorWrapper> stack;
  for (WUInt16 i = 0; i < uiNumPasses; ++i)
  {
    const Pass& pass = m_Passes[i];
    bool bIsRoot = pass.m_bHasSideEffects;

    if (!bIsRoot)
    {
      // Check if this pass writes to an imported texture.
      for (const TextureInfo& info : pass.GetWriteTextures(const_cast<WRenderGraph*>(this)))
      {
        if (m_HandleToImportTexture.Contains(info.m_hTexture))
        {
          bIsRoot = true;
          break;
        }
      }
    }
    if (!bIsRoot)
    {
      for (const BufferInfo& info : pass.GetWriteBuffers(const_cast<WRenderGraph*>(this)))
      {
        if (m_HandleToImportBuffer.Contains(info.m_hBuffer))
        {
          bIsRoot = true;
          break;
        }
      }
    }

    if (bIsRoot)
    {
      m_Alive[i] = true;
      stack.PushBack(i);
    }
  }

  // Flood-fill backwards through dependencies.
  while (!stack.IsEmpty())
  {
    const WUInt16 current = stack.PeekBack();
    stack.PopBack();

    for (WUInt16 dep : m_Passes[current].GetAdjacency(this))
    {
      if (!m_Alive[dep])
      {
        m_Alive[dep] = true;
        stack.PushBack(dep);
      }
    }
  }
}

void WRenderGraph::BuildSortedPassList()
{
  W_PROFILE_SCOPE("BuildSortedPassList");
  m_AlivePasses.Clear();

  for (WUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    if (m_Alive[i])
    {
      m_AlivePasses.ExpandAndGetRef().uiOriginalPassIndex = i;
    }
  }
}

void WRenderGraph::ComputeResourceLifetimes()
{
  W_PROFILE_SCOPE("ComputeResourceLifetimes");
  const WUInt32 uiNumTextures = m_TextureCreationDescriptions.GetCount();
  const WUInt32 uiNumBuffers = m_BufferCreationDescriptions.GetCount();


  m_TextureFirstUse.SetCount(uiNumTextures, s_Unused);
  m_TextureLastUse.SetCount(uiNumTextures, s_Unused);
  m_BufferFirstUse.SetCount(uiNumBuffers, s_Unused);
  m_BufferLastUse.SetCount(uiNumBuffers, s_Unused);

  auto touchTexture = [&](const WRenderGraphTextureHandle& hTexture, WUInt16 uiSortedIndex)
  {
    if (m_HandleToImportTexture.Contains(hTexture))
      return;

    const WUInt32 uiTexIndex = hTexture.GetInternalID().m_InstanceIndex;
    if (m_TextureFirstUse[uiTexIndex] == s_Unused)
      m_TextureFirstUse[uiTexIndex] = uiSortedIndex;
    m_TextureLastUse[uiTexIndex] = uiSortedIndex;
  };

  auto touchBuffer = [&](const WRenderGraphBufferHandle& hBuffer, WUInt16 uiSortedIndex)
  {
    if (m_HandleToImportBuffer.Contains(hBuffer))
      return;

    const WUInt32 uiBufIndex = hBuffer.GetInternalID().m_InstanceIndex;
    if (m_BufferFirstUse[uiBufIndex] == s_Unused)
      m_BufferFirstUse[uiBufIndex] = uiSortedIndex;
    m_BufferLastUse[uiBufIndex] = uiSortedIndex;
  };

  const WUInt32 uiSortedPasses = m_AlivePasses.GetCount();
  for (WUInt32 sortedIdx = 0; sortedIdx < uiSortedPasses; ++sortedIdx)
  {
    const WUInt16 passIdx = m_AlivePasses[sortedIdx].uiOriginalPassIndex;
    const Pass& pass = m_Passes[passIdx];

    for (const TextureInfo& info : pass.GetReadTextures(this))
    {
      touchTexture(info.m_hTexture, sortedIdx);
    }
    for (const TextureInfo& info : pass.GetWriteTextures(this))
    {
      touchTexture(info.m_hTexture, sortedIdx);
    }
    for (const BufferInfo& info : pass.GetReadBuffers(this))
    {
      touchBuffer(info.m_hBuffer, sortedIdx);
    }
    for (const BufferInfo& info : pass.GetWriteBuffers(this))
    {
      touchBuffer(info.m_hBuffer, sortedIdx);
    }
  }

  WUInt32 uiAcquireTextureTotal = 0;
  WUInt32 uiAcquireBufferTotal = 0;
  for (WUInt32 i = 0; i < uiNumTextures; ++i)
  {
    const WUInt16 uiAcquireTexturePass = m_TextureFirstUse[i];
    if (uiAcquireTexturePass != s_Unused)
    {
      uiAcquireTextureTotal++;
      m_AlivePasses[uiAcquireTexturePass].m_uiAcquireTextureCount++;
    }
    const WUInt16 uiReleaseTexturePass = m_TextureLastUse[i];
    if (uiReleaseTexturePass != s_Unused)
    {
      m_AlivePasses[uiReleaseTexturePass].m_uiReleaseTextureCount++;
    }
  }
  for (WUInt32 i = 0; i < uiNumBuffers; ++i)
  {
    const WUInt16 uiAcquireBufferPass = m_BufferFirstUse[i];
    if (uiAcquireBufferPass != s_Unused)
    {
      uiAcquireBufferTotal++;
      m_AlivePasses[uiAcquireBufferPass].m_uiAcquireBufferCount++;
    }
    const WUInt16 uiReleaseBufferPass = m_BufferLastUse[i];
    if (uiReleaseBufferPass != s_Unused)
    {
      m_AlivePasses[uiReleaseBufferPass].m_uiReleaseBufferCount++;
    }
  }

  // Prefix sums to get per-pass acquire / release indices.
  WUInt32 acquireTextureCounter = 0;
  WUInt32 releaseTextureCounter = 0;
  WUInt32 acquireBufferCounter = 0;
  WUInt32 releaseBufferCounter = 0;
  for (WUInt32 i = 0; i < uiSortedPasses; ++i)
  {
    m_AlivePasses[i].m_uiAcquireTextureIndex = acquireTextureCounter;
    acquireTextureCounter += m_AlivePasses[i].m_uiAcquireTextureCount;
    m_AlivePasses[i].m_uiAcquireTextureCount = 0;

    m_AlivePasses[i].m_uiReleaseTextureIndex = releaseTextureCounter;
    releaseTextureCounter += m_AlivePasses[i].m_uiReleaseTextureCount;
    m_AlivePasses[i].m_uiReleaseTextureCount = 0;

    m_AlivePasses[i].m_uiAcquireBufferIndex = acquireBufferCounter;
    acquireBufferCounter += m_AlivePasses[i].m_uiAcquireBufferCount;
    m_AlivePasses[i].m_uiAcquireBufferCount = 0;

    m_AlivePasses[i].m_uiReleaseBufferIndex = releaseBufferCounter;
    releaseBufferCounter += m_AlivePasses[i].m_uiReleaseBufferCount;
    m_AlivePasses[i].m_uiReleaseBufferCount = 0;
  }

  // #TODO: Can SetCountUninitialized as we will write to all entries.
  m_AcquireTextures.SetCount(uiAcquireTextureTotal);
  m_ReleaseTextures.SetCount(uiAcquireTextureTotal);
  m_AcquireBuffers.SetCount(uiAcquireBufferTotal);
  m_ReleaseBuffers.SetCount(uiAcquireBufferTotal);

  // We have computed all acquire / release counts and indices. The count was reset as we now scatter write the allocations into the per-pass arrays.
  for (WUInt32 i = 0; i < uiNumTextures; ++i)
  {
    const WUInt16 uiAcquireTexturePass = m_TextureFirstUse[i];
    if (uiAcquireTexturePass != s_Unused)
    {
      const WUInt16 acquireIndex = m_AlivePasses[uiAcquireTexturePass].m_uiAcquireTextureIndex + m_AlivePasses[uiAcquireTexturePass].m_uiAcquireTextureCount;
      m_AcquireTextures[acquireIndex] = i;
      m_AlivePasses[uiAcquireTexturePass].m_uiAcquireTextureCount++;
    }
    const WUInt16 uiReleaseTexturePass = m_TextureLastUse[i];
    if (uiReleaseTexturePass != s_Unused)
    {
      const WUInt16 releaseIndex = m_AlivePasses[uiReleaseTexturePass].m_uiReleaseTextureIndex + m_AlivePasses[uiReleaseTexturePass].m_uiReleaseTextureCount;
      m_ReleaseTextures[releaseIndex] = i;
      m_AlivePasses[uiReleaseTexturePass].m_uiReleaseTextureCount++;
    }
  }

  for (WUInt32 i = 0; i < uiNumBuffers; ++i)
  {
    const WUInt16 uiAcquireBufferPass = m_BufferFirstUse[i];
    if (uiAcquireBufferPass != s_Unused)
    {
      const WUInt16 acquireIndex = m_AlivePasses[uiAcquireBufferPass].m_uiAcquireBufferIndex + m_AlivePasses[uiAcquireBufferPass].m_uiAcquireBufferCount;
      m_AcquireBuffers[acquireIndex] = i;
      m_AlivePasses[uiAcquireBufferPass].m_uiAcquireBufferCount++;
    }
    const WUInt16 uiReleaseBufferPass = m_BufferLastUse[i];
    if (uiReleaseBufferPass != s_Unused)
    {
      const WUInt16 releaseIndex = m_AlivePasses[uiReleaseBufferPass].m_uiReleaseBufferIndex + m_AlivePasses[uiReleaseBufferPass].m_uiReleaseBufferCount;
      m_ReleaseBuffers[releaseIndex] = i;
      m_AlivePasses[uiReleaseBufferPass].m_uiReleaseBufferCount++;
    }
  }
}

void WRenderGraph::AllocateTransientResources()
{
  W_PROFILE_SCOPE("AllocateTransientResources");

  const WUInt32 uiNumTextures = m_TextureCreationDescriptions.GetCount();
  const WUInt32 uiNumBuffers = m_BufferCreationDescriptions.GetCount();

  m_TextureToResolvedTexture.SetCount(uiNumTextures, s_Unused);
  m_BufferToResolvedBuffer.SetCount(uiNumBuffers, s_Unused);
  m_ResolvedTextures.Reserve(uiNumTextures);
  m_ResolvedBuffers.Reserve(uiNumBuffers);

  // Seed resolved mappings with imported resources.
  for (auto it = m_HandleToImportTexture.GetIterator(); it.IsValid(); ++it)
  {
    m_TextureToResolvedTexture[it.Key().m_InternalId.m_InstanceIndex] = m_ResolvedTextures.GetCount();
    m_ResolvedTextures.PushBack(it.Value().m_hTextureHandle);
  }
  for (auto it = m_HandleToImportBuffer.GetIterator(); it.IsValid(); ++it)
  {
    m_BufferToResolvedBuffer[it.Key().m_InternalId.m_InstanceIndex] = m_ResolvedBuffers.GetCount();
    m_ResolvedBuffers.PushBack(it.Value().m_hBufferHandle);
  }

  // Go through all intermediate passes and allocate / release transient resources.
  WHashTable<WGALTextureHandle, WUInt16> TextureToResolvedTextureIndex(WFrameAllocator::GetCurrentAllocator());
  TextureToResolvedTextureIndex.Reserve(uiNumTextures);
  WHashTable<WGALBufferHandle, WUInt16> BufferToResolvedBufferIndex(WFrameAllocator::GetCurrentAllocator());
  BufferToResolvedBufferIndex.Reserve(uiNumBuffers);
  // By calling Acquire / Release in chronological order the WRenderGraphResourceAllocator can alias resources that don't overlap by giving out the same resource multiple times.
  for (WUInt32 sortedIdx = 0; sortedIdx < m_AlivePasses.GetCount(); ++sortedIdx)
  {
    const IntermediatePass& pass = m_AlivePasses[sortedIdx];
    WArrayPtr<const WUInt16> acquireTextures = pass.GetAcquireTextures(this);
    for (WUInt16 uiTextureIndex : acquireTextures)
    {
      WGALTextureHandle hTexture = m_pAllocator->AcquireTexture(m_TextureCreationDescriptions[uiTextureIndex]);
      bool bExisted = false;
      WUInt16& uiResolvedTextureIndex = TextureToResolvedTextureIndex.FindOrAdd(hTexture, &bExisted);
      if (!bExisted)
      {
        uiResolvedTextureIndex = m_ResolvedTextures.GetCount();
        m_ResolvedTextures.PushBack(hTexture);
      }
      m_TextureToResolvedTexture[uiTextureIndex] = uiResolvedTextureIndex;
    }

    WArrayPtr<const WUInt16> releaseTextures = pass.GetReleaseTextures(this);
    for (WUInt16 uiTextureIndex : releaseTextures)
    {
      const WUInt16 uiResolvedTextureIndex = m_TextureToResolvedTexture[uiTextureIndex];
      m_pAllocator->ReleaseTexture(m_ResolvedTextures[uiResolvedTextureIndex]);
    }

    WArrayPtr<const WUInt16> acquireBuffers = pass.GetAcquireBuffers(this);
    for (WUInt16 uiBufferIndex : acquireBuffers)
    {
      WGALBufferHandle hBuffer = m_pAllocator->AcquireBuffer(m_BufferCreationDescriptions[uiBufferIndex]);
      bool bExisted = false;
      WUInt16& uiResolvedBufferIndex = BufferToResolvedBufferIndex.FindOrAdd(hBuffer, &bExisted);
      if (!bExisted)
      {
        uiResolvedBufferIndex = m_ResolvedBuffers.GetCount();
        m_ResolvedBuffers.PushBack(hBuffer);
      }
      m_BufferToResolvedBuffer[uiBufferIndex] = uiResolvedBufferIndex;
    }

    WArrayPtr<const WUInt16> releaseBuffers = pass.GetReleaseBuffers(this);
    for (WUInt16 uiBufferIndex : releaseBuffers)
    {
      const WUInt16 uiResolvedBufferIndex = m_BufferToResolvedBuffer[uiBufferIndex];
      m_pAllocator->ReleaseBuffer(m_ResolvedBuffers[uiResolvedBufferIndex]);
    }
  }
}

void WRenderGraph::BuildRenderingSetups()
{
  W_PROFILE_SCOPE("BuildRenderingSetups");
  m_CompiledPasses.SetCount(m_AlivePasses.GetCount());

  for (WUInt32 sortedIdx = 0; sortedIdx < m_AlivePasses.GetCount(); ++sortedIdx)
  {
    const WUInt16 passIdx = m_AlivePasses[sortedIdx].uiOriginalPassIndex;
    const Pass& pass = m_Passes[passIdx];
    CompiledPass& compiled = m_CompiledPasses[sortedIdx];
    compiled.m_uiOriginalPassIndex = passIdx;

    const auto colorTargets = pass.GetColorTargets(this);
    const auto depthTargets = pass.GetDepthStencilTargets(this);

    if (colorTargets.IsEmpty() && depthTargets.IsEmpty())
      continue;

    WGALRenderingSetup& setup = compiled.m_RenderingSetup;

    // Color targets.
    for (WUInt8 i = 0; i < colorTargets.GetCount(); ++i)
    {
      const ColorTargetInfo& ct = colorTargets[i];
      const WUInt16 uiResolvedTextureIndex = m_TextureToResolvedTexture[ct.m_hTexture.m_InternalId.m_InstanceIndex];
      const WGALTextureHandle hGAL = m_ResolvedTextures[uiResolvedTextureIndex];
      auto pTexture = m_pDevice->GetTexture(hGAL);

      WGALRenderTargetViewCreationDescription rtvDesc;
      rtvDesc.m_hTexture = hGAL;
      rtvDesc.m_uiMipLevel = ct.m_range.m_uiBaseMipLevel;
      rtvDesc.m_uiFirstSlice = ct.m_range.m_uiBaseArraySlice;
      rtvDesc.m_uiSliceCount = ct.m_range.m_uiArraySlices;
      rtvDesc.m_OverrideViewFormat = ct.m_overrideViewFormat;
      rtvDesc.m_OverrideViewType = ct.m_overrideViewType;

      WGALRenderTargetViewHandle hRTV = m_pDevice->GetRenderTargetView(rtvDesc);
      if (pTexture->GetDescription().m_Type == WGALTextureType::Texture2DProxy)
      {
        hRTV = m_pDevice->GetDefaultRenderTargetView(hGAL);
      }


      setup.SetColorTarget(i, hRTV, ct.m_loadOp, ct.m_storeOp);
    }

    // Apply clear colors.
    const auto clearColors = pass.GetClearColors(this);
    for (WUInt8 i = 0; i < clearColors.GetCount(); ++i)
    {
      setup.SetClearColor(i, clearColors[i]);
    }

    // Depth/stencil target (at most one).
    if (!depthTargets.IsEmpty())
    {
      const DepthStencilTargetInfo& ds = depthTargets[0];
      const WUInt16 uiResolvedTextureIndex = m_TextureToResolvedTexture[ds.m_hTexture.m_InternalId.m_InstanceIndex];
      const WGALTextureHandle hGAL = m_ResolvedTextures[uiResolvedTextureIndex];

      WGALRenderTargetViewCreationDescription rtvDesc;
      rtvDesc.m_hTexture = hGAL;
      rtvDesc.m_uiMipLevel = ds.m_range.m_uiBaseMipLevel;
      rtvDesc.m_uiFirstSlice = ds.m_range.m_uiBaseArraySlice;
      rtvDesc.m_uiSliceCount = ds.m_range.m_uiArraySlices;
      rtvDesc.m_bReadOnly = ds.m_bReadOnly;

      WGALRenderTargetViewHandle hDSV = m_pDevice->GetRenderTargetView(rtvDesc);
      setup.SetDepthStencilTarget(hDSV, ds.m_depthLoadOp, ds.m_depthStoreOp, ds.m_stencilLoadOp, ds.m_stencilStoreOp);

      // Apply clear depth/stencil.
      const auto clearDS = pass.GetClearDepthStencils(this);
      if (!clearDS.IsEmpty())
      {
        setup.SetClearDepth(clearDS[0].fDepthClear);
        setup.SetClearStencil(clearDS[0].uiStencilClear);
      }
    }
  }
}

void WRenderGraph::ComputeBarriers(WGALResourceStateTracker& ref_tracker, WArrayPtr<WRenderGraphPassObserver*> observers)
{
  W_PROFILE_SCOPE("ComputeBarriers");
  W_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Compiled, "ComputeBarriers must be called after Compile succeeded");

  for (WUInt32 sortedIdx = 0; sortedIdx < m_AlivePasses.GetCount(); ++sortedIdx)
  {
    const WUInt16 passIdx = m_AlivePasses[sortedIdx].uiOriginalPassIndex;
    const Pass& pass = m_Passes[passIdx];
    CompiledPass& compiled = m_CompiledPasses[sortedIdx];

    compiled.m_uiTextureBarrierIndex = m_CompiledTextureBarriers.GetCount();
    compiled.m_uiBufferBarrierIndex = m_CompiledBufferBarriers.GetCount();
    if (sortedIdx == 0)
    {
      // Add barriers for imported resources that have explicitly set a resource state.
      for (auto it : m_HandleToImportTexture)
      {
        if (it.Value().m_access != WGALResourceState::Unknown)
        {
          ref_tracker.ChangeState(it.Value().m_hTextureHandle, {}, it.Value().m_access, it.Value().m_stage, [&](const WGALTextureBarrier& barrier)
            {
              m_CompiledTextureBarriers.PushBack(barrier); //
            });
        }
      }
      for (auto it : m_HandleToImportBuffer)
      {
        if (it.Value().m_access != WGALResourceState::Unknown)
        {
          ref_tracker.ChangeState(it.Value().m_hBufferHandle, it.Value().m_access, it.Value().m_stage, [&](const WGALBufferBarrier& barrier)
            {
              m_CompiledBufferBarriers.PushBack(barrier); //
            });
        }
      }
    }

    auto readTextures = pass.GetReadTextures(this);
    for (const TextureInfo& info : readTextures)
    {
      const WUInt16 uiResolvedTextureIndex = m_TextureToResolvedTexture[info.m_hTexture.m_InternalId.m_InstanceIndex];
      WGALTextureHandle hTexture = m_ResolvedTextures[uiResolvedTextureIndex];
      ref_tracker.ChangeState(hTexture, info.m_range, info.m_access, info.m_stage, [&](const WGALTextureBarrier& barrier)
        {
          m_CompiledTextureBarriers.PushBack(barrier); //
        });
    }
    auto writeTextures = pass.GetWriteTextures(this);
    for (const TextureInfo& info : writeTextures)
    {
      const WUInt16 uiResolvedTextureIndex = m_TextureToResolvedTexture[info.m_hTexture.m_InternalId.m_InstanceIndex];
      WGALTextureHandle hTexture = m_ResolvedTextures[uiResolvedTextureIndex];
      ref_tracker.ChangeState(hTexture, info.m_range, info.m_access, info.m_stage, [&](const WGALTextureBarrier& barrier)
        {
          m_CompiledTextureBarriers.PushBack(barrier); //
        });
    }
    compiled.m_uiTextureBarrierCount = m_CompiledTextureBarriers.GetCount() - compiled.m_uiTextureBarrierIndex;

    auto readBuffers = pass.GetReadBuffers(this);
    for (const BufferInfo& info : readBuffers)
    {
      const WUInt16 uiResolvedBufferIndex = m_BufferToResolvedBuffer[info.m_hBuffer.m_InternalId.m_InstanceIndex];
      WGALBufferHandle hBuffer = m_ResolvedBuffers[uiResolvedBufferIndex];
      ref_tracker.ChangeState(hBuffer, info.m_access, info.m_stage, [&](const WGALBufferBarrier& barrier)
        {
          m_CompiledBufferBarriers.PushBack(barrier); //
        });
    }
    auto writeBuffers = pass.GetWriteBuffers(this);
    for (const BufferInfo& info : writeBuffers)
    {
      const WUInt16 uiResolvedBufferIndex = m_BufferToResolvedBuffer[info.m_hBuffer.m_InternalId.m_InstanceIndex];
      WGALBufferHandle hBuffer = m_ResolvedBuffers[uiResolvedBufferIndex];
      ref_tracker.ChangeState(hBuffer, info.m_access, info.m_stage, [&](const WGALBufferBarrier& barrier)
        {
          m_CompiledBufferBarriers.PushBack(barrier); //
        });
    }
    compiled.m_uiBufferBarrierCount = m_CompiledBufferBarriers.GetCount() - compiled.m_uiBufferBarrierIndex;

    // Set up observers that target this pass.
    for (auto* pObserver : observers)
    {
      if (pObserver->GetRequest().m_sPassName != m_PassNames[passIdx])
        continue;

      const WRenderGraphTextureHandle hSourceTex = FindTextureAccessInPass(passIdx, pObserver->GetRequest().m_uiAccessIndex);
      if (hSourceTex.IsInvalidated())
        continue;

      const WUInt16 uiResolvedIdx = m_TextureToResolvedTexture[hSourceTex.m_InternalId.m_InstanceIndex];
      if (uiResolvedIdx == s_Unused)
        continue;

      const WGALTextureHandle hResolvedSource = m_ResolvedTextures[uiResolvedIdx];
      const WGALTextureCreationDescription& srcDesc = m_TextureCreationDescriptions[hSourceTex.m_InternalId.m_InstanceIndex];

      pObserver->EnsureCopyTexture(srcDesc);
      if (pObserver->GetCopyTexture().IsInvalidated())
        continue;

      pObserver->m_hResolvedSourceTexture = hResolvedSource;
      pObserver->m_uiSortedPassIndex = sortedIdx;
      pObserver->m_bValid = true;

      // Source → CopySource barrier (tracked by the state tracker so the
      // next pass's barriers will restore the correct state).
      ref_tracker.ChangeState(hResolvedSource, {}, WGALResourceState::CopySource, WGALShaderStageFlags::Auto, [&](const WGALTextureBarrier& barrier)
        { pObserver->m_PreCopyBarriers.PushBack(barrier); });

      const WGALResourceState::Enum destReadState = WGALResourceFormat::IsDepthFormat(srcDesc.m_Format)
                                                       ? WGALResourceState::DepthStencilRead
                                                       : WGALResourceState::ShaderResource;

      // Destination → CopyDestination barrier (not tracked — external resource).
      {
        WGALTextureBarrier destBarrier;
        destBarrier.m_hTexture = pObserver->GetCopyTexture();
        destBarrier.m_StateBefore = destReadState;
        destBarrier.m_StateAfter = WGALResourceState::CopyDestination;
        pObserver->m_PreCopyBarriers.PushBack(destBarrier);
      }

      // Destination → read barrier after copy.
      {
        WGALTextureBarrier destBarrier;
        destBarrier.m_hTexture = pObserver->GetCopyTexture();
        destBarrier.m_StateBefore = WGALResourceState::CopyDestination;
        destBarrier.m_StateAfter = destReadState;
        pObserver->m_PostCopyBarriers.PushBack(destBarrier);
      }
    }
  }
  m_RenderGraphState = RenderGraphState::BarriersCreated;
}
