#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Utils/ResourceStateTracker.h>

#include <Foundation/Memory/FrameAllocator.h>
#include <RendererFoundation/Resources/ProxyTexture.h>

WGALResourceStateTracker::TextureState::TextureState()
  : m_SubResourceStates(WFrameAllocator::GetCurrentAllocator())
{
}

WGALResourceStateTracker::WGALResourceStateTracker(WGALDevice* pDevice)
  : m_pDevice(pDevice)
{
  W_ASSERT_DEBUG(pDevice != nullptr, "Device must not be null");
  m_GALDeviceEventSubscriptionID = WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WGALResourceStateTracker::GALDeviceEventHandler, this));
}

WGALResourceStateTracker::~WGALResourceStateTracker()
{
  WGALDevice::s_Events.RemoveEventHandler(m_GALDeviceEventSubscriptionID);
}

void WGALResourceStateTracker::Clear()
{
  m_TextureStates.Clear();
  m_BufferStates.Clear();
}

void WGALResourceStateTracker::GALDeviceEventHandler(const WGALDeviceEvent& e)
{
  if (e.m_Type == WGALDeviceEvent::AfterEndFrame && e.m_pDevice == m_pDevice)
  {
    Clear();
  }
}

// --- Initial State ---

void WGALResourceStateTracker::SetInitialTextureState(WGALTextureHandle hTexture, WBitflags<WGALResourceState> state, WBitflags<WGALShaderStageFlags> stages)
{
  W_ASSERT_DEBUG(stages.IsAnyFlagSet(), "Invalid shader stage, should be an explicit stage or Auto");
  TextureState& ts = GetOrCreateTextureState(hTexture);
  // Reset to compressed tracking: one entry represents the whole resource.
  ts.m_SubResourceStates.SetCount(1);
  ts.m_SubResourceStates[0] = {state, stages};
}

void WGALResourceStateTracker::SetInitialBufferState(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> state, WBitflags<WGALShaderStageFlags> stages)
{
  W_ASSERT_DEBUG(stages.IsAnyFlagSet(), "Invalid shader stage, should be an explicit stage or Auto");
  m_BufferStates[hBuffer] = {state, stages};
}

// --- Texture ChangeState ---

void WGALResourceStateTracker::ChangeState(WGALTextureHandle hTexture,
  WGALTextureRange range,
  WBitflags<WGALResourceState> newState,
  WBitflags<WGALShaderStageFlags> stage,
  const WDelegate<void(const WGALTextureBarrier&)>& barrierCallback)
{
  ResolveProxyTexture(hTexture, range);

  W_ASSERT_DEBUG(stage.IsAnyFlagSet(), "Invalid shader stage, should be an explicit stage or Auto");
  TextureState& ts = GetOrCreateTextureState(hTexture);

  // Resolve W_GAL_ALL_* sentinels against the full texture range.
  const WGALTextureRange& fullRange = ts.m_FullRange;
  if (range.m_uiArraySlices == W_GAL_ALL_ARRAY_SLICES)
    range.m_uiArraySlices = static_cast<WUInt16>(fullRange.m_uiArraySlices - range.m_uiBaseArraySlice);
  if (range.m_uiMipLevels == W_GAL_ALL_MIP_LEVELS)
    range.m_uiMipLevels = static_cast<WUInt8>(fullRange.m_uiMipLevels - range.m_uiBaseMipLevel);

  const bool bIsFullRange = (range == fullRange);
  const bool bDiscard = newState.IsSet(WGALResourceState::Discard);
  newState.Remove(WGALResourceState::Discard);
  // Fast path: compressed state + full-range transition can stay compressed.
  if (ts.m_SubResourceStates.GetCount() == 1 && bIsFullRange)
  {

    SubResourceState& sr = ts.m_SubResourceStates[0];
    const SubResourceState newSr = {newState, stage};
    if (IsTextureBarrierNeeded(sr, newSr))
    {
      WGALTextureBarrier barrier;
      barrier.m_hTexture = hTexture;
      barrier.m_StateBefore = sr.m_State;
      barrier.m_StateAfter = newState;
      barrier.m_StagesBefore = sr.m_Stages;
      barrier.m_StagesAfter = stage;
      barrier.m_bAllSubresources = true;
      barrier.m_bDiscard = bDiscard;
      barrierCallback(barrier);
      sr.m_Stages = stage;
    }
    else
    {
      // No barrier needed: accumulate stages so subsequent barriers have correct srcStages covering all stages that accessed the resource.
      if (!sr.m_Stages.IsSet(WGALShaderStageFlags::Auto))
        sr.m_Stages.Add(stage);
    }

    sr.m_State = newState;
    return;
  }

  // Partial transition on compressed tracking requires expansion to sub-resources.
  if (ts.m_SubResourceStates.GetCount() == 1 && !bIsFullRange)
  {
    ExpandTextureState(ts);
  }

  // Per-sub-resource transitions.
  for (WUInt16 uiLayer = range.m_uiBaseArraySlice; uiLayer < range.m_uiBaseArraySlice + range.m_uiArraySlices; ++uiLayer)
  {
    for (WUInt8 uiMip = range.m_uiBaseMipLevel; uiMip < range.m_uiBaseMipLevel + range.m_uiMipLevels; ++uiMip)
    {
      const WUInt32 uiIdx = WGALTextureRange::ComputeSubResourceIndex(uiMip, uiLayer, fullRange);
      SubResourceState& sr = ts.m_SubResourceStates[uiIdx];

      const SubResourceState newSr = {newState, stage};
      if (IsTextureBarrierNeeded(sr, newSr))
      {
        WGALTextureBarrier barrier;
        barrier.m_hTexture = hTexture;
        barrier.m_StateBefore = sr.m_State;
        barrier.m_StateAfter = newState;
        barrier.m_StagesBefore = sr.m_Stages;
        barrier.m_StagesAfter = stage;
        barrier.m_bAllSubresources = false;
        barrier.m_Subresource.m_uiMipLevel = uiMip;
        barrier.m_Subresource.m_uiArraySlice = uiLayer;
        barrier.m_bDiscard = bDiscard;
        barrierCallback(barrier);
        sr.m_Stages = stage;
      }
      else
      {
        // No barrier needed: accumulate stages so subsequent barriers have correct srcStages covering all stages that accessed the resource.
        if (!sr.m_Stages.IsSet(WGALShaderStageFlags::Auto))
          sr.m_Stages.Add(stage);
      }

      sr.m_State = newState;
    }
  }
}

// --- RevertTextureState ---

void WGALResourceStateTracker::RevertTextureState(const WDelegate<void(const WGALTextureBarrier&)>& barrierCallback)
{
  for (auto it = m_TextureStates.GetIterator(); it.IsValid(); ++it)
  {
    const WGALTextureHandle hTexture = it.Key();
    TextureState& ts = it.Value();

    const WGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
    W_ASSERT_DEBUG(pTexture != nullptr, "Invalid texture handle in state tracker");
    const WBitflags<WGALResourceState> defaultState = pTexture->GetDescription().GetDefaultState();

    // Compressed path: one entry represents the entire resource.
    if (ts.m_SubResourceStates.GetCount() == 1)
    {
      SubResourceState& sr = ts.m_SubResourceStates[0];
      const SubResourceState newSr = {defaultState, WGALShaderStageFlags::Auto};
      if (IsTextureBarrierNeeded(sr, newSr))
      {
        WGALTextureBarrier barrier;
        barrier.m_hTexture = hTexture;
        barrier.m_StateBefore = sr.m_State;
        barrier.m_StateAfter = defaultState;
        barrier.m_StagesBefore = sr.m_Stages;
        barrier.m_StagesAfter = WGALShaderStageFlags::Auto;
        barrier.m_bAllSubresources = true;
        barrierCallback(barrier);
      }
      sr = newSr;
      continue;
    }

    // Expanded path: evaluate each sub-resource individually.
    for (WUInt16 uiLayer = 0; uiLayer < ts.m_FullRange.m_uiArraySlices; ++uiLayer)
    {
      for (WUInt8 uiMip = 0; uiMip < ts.m_FullRange.m_uiMipLevels; ++uiMip)
      {
        const WUInt32 uiIdx = WGALTextureRange::ComputeSubResourceIndex(uiMip, uiLayer, ts.m_FullRange);
        SubResourceState& sr = ts.m_SubResourceStates[uiIdx];
        const SubResourceState newSr = {defaultState, WGALShaderStageFlags::Auto};
        if (IsTextureBarrierNeeded(sr, newSr))
        {
          WGALTextureBarrier barrier;
          barrier.m_hTexture = hTexture;
          barrier.m_StateBefore = sr.m_State;
          barrier.m_StateAfter = defaultState;
          barrier.m_StagesBefore = sr.m_Stages;
          barrier.m_StagesAfter = WGALShaderStageFlags::Auto;
          barrier.m_bAllSubresources = false;
          barrier.m_Subresource.m_uiMipLevel = uiMip;
          barrier.m_Subresource.m_uiArraySlice = uiLayer;
          barrierCallback(barrier);
        }
        sr = newSr;
      }
    }

    // Re-compress: all sub-resources now share the same default state.
    ts.m_SubResourceStates.SetCount(1);
    ts.m_SubResourceStates[0] = {defaultState, WGALShaderStageFlags::Auto};
  }
}

void WGALResourceStateTracker::RevertBufferState(const WDelegate<void(const WGALBufferBarrier&)>& barrierCallback)
{
  for (auto it = m_BufferStates.GetIterator(); it.IsValid(); ++it)
  {
    const WGALBufferHandle hBuffer = it.Key();
    SubResourceState& bs = it.Value();

    const WGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
    W_ASSERT_DEBUG(pBuffer != nullptr, "Invalid buffer handle in state tracker");
    const WBitflags<WGALResourceState> defaultState = pBuffer->GetDescription().GetDefaultState();

    const SubResourceState newSr = {defaultState, WGALShaderStageFlags::Auto};
    if (IsBufferBarrierNeeded(bs, newSr))
    {
      WGALBufferBarrier barrier;
      barrier.m_hBuffer = hBuffer;
      barrier.m_StateBefore = bs.m_State;
      barrier.m_StateAfter = defaultState;
      barrier.m_StagesBefore = bs.m_Stages;
      barrier.m_StagesAfter = WGALShaderStageFlags::Auto;
      barrierCallback(barrier);
    }

    bs = newSr;
  }
}

// --- Buffer ChangeState ---

void WGALResourceStateTracker::ChangeState(WGALBufferHandle hBuffer,
  WBitflags<WGALResourceState> newState,
  WBitflags<WGALShaderStageFlags> stage,
  const WDelegate<void(const WGALBufferBarrier&)>& barrierCallback)
{
  W_ASSERT_DEBUG(stage.IsAnyFlagSet(), "Invalid shader stage, should be an explicit stage or Auto");
  SubResourceState& bs = GetOrCreateBufferState(hBuffer);

  const SubResourceState newSr = {newState, stage};
  if (IsBufferBarrierNeeded(bs, newSr))
  {
    WGALBufferBarrier barrier;
    barrier.m_hBuffer = hBuffer;
    barrier.m_StateBefore = bs.m_State;
    barrier.m_StateAfter = newState;
    barrier.m_StagesBefore = bs.m_Stages;
    barrier.m_StagesAfter = stage;
    barrierCallback(barrier);
    bs.m_Stages = stage;
  }
  else
  {
    bs.m_Stages.Add(stage);
  }

  bs.m_State = newState;
}

// --- Helpers ---

WGALResourceStateTracker::TextureState& WGALResourceStateTracker::GetOrCreateTextureState(WGALTextureHandle hTexture)
{
  TextureState& ts = m_TextureStates[hTexture];

  // m_uiArraySlices == W_GAL_ALL_ARRAY_SLICES marks an uninitialized range.
  if (ts.m_FullRange.m_uiArraySlices == W_GAL_ALL_ARRAY_SLICES)
  {
    const WGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
    W_ASSERT_DEBUG(pTexture != nullptr, "Invalid texture handle");
    ts.m_FullRange = pTexture->ClampRange({});

    if (ts.m_SubResourceStates.IsEmpty())
    {
      // First observation starts from the resource default state.
      ts.m_SubResourceStates.SetCount(1);
      ts.m_SubResourceStates[0] = {pTexture->GetDescription().GetDefaultState(), WGALShaderStageFlags::Auto};
    }
  }

  return ts;
}

WGALResourceStateTracker::SubResourceState& WGALResourceStateTracker::GetOrCreateBufferState(WGALBufferHandle hBuffer)
{
  bool bExisted = false;
  SubResourceState& bs = m_BufferStates.FindOrAdd(hBuffer, &bExisted);

  if (!bExisted)
  {
    const WGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
    W_ASSERT_DEBUG(pBuffer != nullptr, "Invalid buffer handle");
    bs = {pBuffer->GetDescription().GetDefaultState(), WGALShaderStageFlags::Auto};
  }

  return bs;
}

void WGALResourceStateTracker::ExpandTextureState(TextureState& state)
{
  W_ASSERT_DEBUG(state.m_SubResourceStates.GetCount() == 1, "Already expanded");

  const SubResourceState uniform = state.m_SubResourceStates[0];
  const WUInt32 uiTotal = (WUInt32)state.m_FullRange.m_uiMipLevels * (WUInt32)state.m_FullRange.m_uiArraySlices;

  state.m_SubResourceStates = WHybridArray<SubResourceState, 1>(WFrameAllocator::GetCurrentAllocator());
  state.m_SubResourceStates.SetCount(uiTotal);
  for (WUInt32 i = 0; i < uiTotal; ++i)
  {
    state.m_SubResourceStates[i] = uniform;
  }
}

bool WGALResourceStateTracker::IsTextureBarrierNeeded(const SubResourceState& oldState, const SubResourceState& newState, bool bForceUAVBarrier)
{
  // State changes always require a barrier.
  if (oldState.m_State != newState.m_State)
    return true;

  // UAV write-after-write needs synchronization even when state bits match.
  if (bForceUAVBarrier && oldState.m_State.IsAnySet(WGALResourceState::UnorderedAccess))
    return true;

  // Check whether the new stages are already covered by the old barrier's
  // destination scope. If not, we need an execution dependency to make the
  // resource visible at the new stage(s).
  if (!AreStagesCovered(oldState.m_Stages, newState.m_Stages))
    return true;

  return false;
}

bool WGALResourceStateTracker::IsBufferBarrierNeeded(const SubResourceState& oldState, const SubResourceState& newState, bool bForceUAVBarrier)
{
  // New state must be a sub-set of the current state.
  if (!oldState.m_State.AreAllSet(newState.m_State))
    return true;

  // UAV write-after-write needs synchronization even when state bits match.
  if (bForceUAVBarrier && oldState.m_State.IsAnySet(WGALResourceState::UnorderedAccess))
    return true;

  // Check whether the new stages are already covered by the old barrier's
  // destination scope. If not, we need an execution dependency to make the
  // resource visible at the new stage(s).
  if (!AreStagesCovered(oldState.m_Stages, newState.m_Stages))
    return true;

  return false;
}

bool WGALResourceStateTracker::AreStagesCovered(WBitflags<WGALShaderStageFlags> coveredStages, WBitflags<WGALShaderStageFlags> requiredStages)
{
  // Auto means "all stages inferred from the resource state" — covers everything.
  if (coveredStages.IsSet(WGALShaderStageFlags::Auto))
    return true;

  // If the caller requires Auto (all stages), explicit stages can't guarantee that.
  if (requiredStages.IsSet(WGALShaderStageFlags::Auto))
    return false;

  // If the required stages are already a subset of the covered stages, no barrier needed.
  if (coveredStages.AreAllSet(requiredStages))
    return true;

  // Graphics pipeline stage ordering: a barrier targeting an earlier stage
  // implicitly covers all logically later stages in the same pipeline.
  // VS -> HS -> DS -> GS -> PS
  // Compute is on a separate pipeline with no implicit ordering.
  constexpr WGALShaderStageFlags::Enum graphicsOrder[] = {
    WGALShaderStageFlags::VertexShader,
    WGALShaderStageFlags::HullShader,
    WGALShaderStageFlags::DomainShader,
    WGALShaderStageFlags::GeometryShader,
    WGALShaderStageFlags::PixelShader,
  };

  // Find the earliest covered graphics stage.
  int iEarliestCovered = -1;
  for (int i = 0; i < W_ARRAY_SIZE(graphicsOrder); ++i)
  {
    if (coveredStages.IsSet(graphicsOrder[i]))
    {
      iEarliestCovered = i;
      break;
    }
  }

  // Check each required stage.
  for (int i = 0; i < W_ARRAY_SIZE(graphicsOrder); ++i)
  {
    if (!requiredStages.IsSet(graphicsOrder[i]))
      continue;

    // Already directly covered.
    if (coveredStages.IsSet(graphicsOrder[i]))
      continue;

    // Covered by an earlier graphics stage in the pipeline.
    if (iEarliestCovered >= 0 && i > iEarliestCovered)
      continue;

    return false;
  }

  // Compute is never implicitly covered by graphics stages and vice versa.
  if (requiredStages.IsSet(WGALShaderStageFlags::ComputeShader) &&
      !coveredStages.IsSet(WGALShaderStageFlags::ComputeShader))
    return false;

  return true;
}
void WGALResourceStateTracker::ResolveProxyTexture(WGALTextureHandle& ref_hTexture, WGALTextureRange& ref_range) const
{
  const WGALTexture* pTexture = m_pDevice->GetTexture(ref_hTexture);
  W_ASSERT_DEBUG(pTexture != nullptr, "Invalid texture handle.");
  // Resolve proxy texture as they only cause pain down the pipeline.
  if (pTexture->GetDescription().m_Type == WGALTextureType::Texture2DProxy)
  {
    const auto pProxy = static_cast<const WGALProxyTexture*>(pTexture);
    ref_hTexture = pProxy->GetParentTextureHandle();
    ref_range = {pProxy->GetSlice(), 1, ref_range.m_uiBaseMipLevel, ref_range.m_uiMipLevels};
  }
}

// --- State Verification ---

const WGALResourceStateTracker::SubResourceState* WGALResourceStateTracker::GetBufferState(WGALBufferHandle hBuffer) const
{
  return m_BufferStates.GetValue(hBuffer);
}

const WGALResourceStateTracker::TextureState* WGALResourceStateTracker::GetTextureState(WGALTextureHandle hTexture) const
{
  WGALTextureRange range;
  ResolveProxyTexture(hTexture, range);
  return m_TextureStates.GetValue(hTexture);
}
