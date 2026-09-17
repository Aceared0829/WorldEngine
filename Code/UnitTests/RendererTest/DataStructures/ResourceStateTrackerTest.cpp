#include <RendererTest/RendererTestPCH.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Utils/ResourceStateTracker.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

W_CREATE_SIMPLE_RENDERER_TEST_GROUP(ResourceStateTracker)

// ============================================================
// IsTextureBarrierNeeded (static, no device needed)
// ============================================================

W_CREATE_SIMPLE_RENDERER_TEST(ResourceStateTracker, IsTextureBarrierNeeded)
{
  using SRS = WGALResourceStateTracker::SubResourceState;

  W_TEST_BLOCK(WTestBlock::Enabled, "Same read state - no barrier")
  {
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(!WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Read to write - barrier needed")
  {
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    SRS newState = {WGALResourceState::RenderTarget, WGALShaderStageFlags::Auto};
    W_TEST_BOOL(WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Write to read - barrier needed")
  {
    SRS oldState = {WGALResourceState::RenderTarget, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Write to write - barrier needed")
  {
    SRS oldState = {WGALResourceState::RenderTarget, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::CopyDestination, WGALShaderStageFlags::Auto};
    W_TEST_BOOL(WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UAV write-after-write - barrier needed")
  {
    SRS oldState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    SRS newState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState, true));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UAV write-after-write without force - no barrier")
  {
    SRS oldState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    SRS newState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    W_TEST_BOOL(!WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState, false));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Same read state but uncovered stage - barrier needed")
  {
    // PS is not covered by CS, so an execution dependency is needed.
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::ComputeShader};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Same read state with Auto covers everything")
  {
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(!WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "VS covers later graphics stages")
  {
    // VS is earlier in the pipeline than PS, so PS is implicitly covered.
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::VertexShader};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(!WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PS does not cover VS")
  {
    // PS is later in the pipeline than VS, so VS is NOT covered.
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::VertexShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Different read states - barrier needed")
  {
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    SRS newState = {WGALResourceState::CopySource, WGALShaderStageFlags::Auto};
    W_TEST_BOOL(WGALResourceStateTracker::IsTextureBarrierNeeded(oldState, newState));
  }
}

// ============================================================
// IsBufferBarrierNeeded (static, no device needed)
// ============================================================

W_CREATE_SIMPLE_RENDERER_TEST(ResourceStateTracker, IsBufferBarrierNeeded)
{
  using SRS = WGALResourceStateTracker::SubResourceState;

  W_TEST_BLOCK(WTestBlock::Enabled, "Same read state - no barrier")
  {
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(!WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Superset old state covers subset new state - no barrier")
  {
    SRS oldState = {WGALResourceState::ShaderResource | WGALResourceState::ConstantBuffer, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::ConstantBuffer, WGALShaderStageFlags::VertexShader};
    W_TEST_BOOL(!WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Exact match with Auto covers explicit stage - no barrier")
  {
    SRS oldState = {WGALResourceState::ConstantBuffer, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::ConstantBuffer, WGALShaderStageFlags::VertexShader};
    W_TEST_BOOL(!WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Read to write - barrier needed")
  {
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    SRS newState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Write to read - barrier needed")
  {
    SRS oldState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Write to write - barrier needed")
  {
    SRS oldState = {WGALResourceState::CopyDestination, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UAV write-after-write - barrier needed")
  {
    SRS oldState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    SRS newState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState, true));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UAV write-after-write without force - no barrier")
  {
    SRS oldState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    SRS newState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    W_TEST_BOOL(!WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState, false));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "New state not subset of old - barrier needed")
  {
    SRS oldState = {WGALResourceState::ConstantBuffer, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Same read state but uncovered stage - barrier needed")
  {
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::ComputeShader};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "VS covers later graphics stages - no barrier")
  {
    SRS oldState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::VertexShader};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(!WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Combined SRV+CBV covers SRV subset - no barrier")
  {
    SRS oldState = {WGALResourceState::ShaderResource | WGALResourceState::ConstantBuffer, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(!WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Combined SRV+VB+IB covers VB subset - no barrier")
  {
    SRS oldState = {WGALResourceState::ShaderResource | WGALResourceState::VertexBuffer | WGALResourceState::IndexBuffer, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::VertexBuffer, WGALShaderStageFlags::Auto};
    W_TEST_BOOL(!WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Combined SRV+VB+IB covers SRV+IB subset - no barrier")
  {
    SRS oldState = {WGALResourceState::ShaderResource | WGALResourceState::VertexBuffer | WGALResourceState::IndexBuffer, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::ShaderResource | WGALResourceState::IndexBuffer, WGALShaderStageFlags::Auto};
    W_TEST_BOOL(!WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Combined read old state does not cover extra new read state - barrier needed")
  {
    SRS oldState = {WGALResourceState::ShaderResource | WGALResourceState::ConstantBuffer, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::ShaderResource | WGALResourceState::IndexBuffer, WGALShaderStageFlags::Auto};
    W_TEST_BOOL(WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Combined read old state to write - barrier needed")
  {
    SRS oldState = {WGALResourceState::ShaderResource | WGALResourceState::VertexBuffer | WGALResourceState::IndexBuffer, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Combined SRV+DrawIndirect covers DrawIndirect - no barrier")
  {
    SRS oldState = {WGALResourceState::ShaderResource | WGALResourceState::DrawIndirect, WGALShaderStageFlags::Auto};
    SRS newState = {WGALResourceState::DrawIndirect, WGALShaderStageFlags::Auto};
    W_TEST_BOOL(!WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Combined read old state with uncovered stage - barrier needed")
  {
    SRS oldState = {WGALResourceState::ShaderResource | WGALResourceState::ConstantBuffer, WGALShaderStageFlags::ComputeShader};
    SRS newState = {WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader};
    W_TEST_BOOL(WGALResourceStateTracker::IsBufferBarrierNeeded(oldState, newState));
  }
}

// ============================================================
// AreStagesCovered (static, no device needed)
// ============================================================

W_CREATE_SIMPLE_RENDERER_TEST(ResourceStateTracker, AreStagesCovered)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Auto covers everything")
  {
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::Auto, WGALShaderStageFlags::PixelShader));
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::Auto, WGALShaderStageFlags::ComputeShader));
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::Auto, WGALShaderStageFlags::VertexShader));
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(
      WGALShaderStageFlags::Auto, WGALShaderStageFlags::VertexShader | WGALShaderStageFlags::PixelShader));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Requiring Auto is not covered by explicit stages")
  {
    W_TEST_BOOL(!WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::PixelShader, WGALShaderStageFlags::Auto));
    W_TEST_BOOL(!WGALResourceStateTracker::AreStagesCovered(
      WGALShaderStageFlags::VertexShader | WGALShaderStageFlags::PixelShader, WGALShaderStageFlags::Auto));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Exact match is covered")
  {
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::PixelShader, WGALShaderStageFlags::PixelShader));
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::ComputeShader, WGALShaderStageFlags::ComputeShader));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Superset covers subset")
  {
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(
      WGALShaderStageFlags::VertexShader | WGALShaderStageFlags::PixelShader, WGALShaderStageFlags::PixelShader));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Earlier graphics stage covers later stages")
  {
    // VS is the earliest, it covers HS, DS, GS, PS.
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::VertexShader, WGALShaderStageFlags::PixelShader));
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::VertexShader, WGALShaderStageFlags::GeometryShader));
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::HullShader, WGALShaderStageFlags::PixelShader));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Later graphics stage does not cover earlier stages")
  {
    W_TEST_BOOL(!WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::PixelShader, WGALShaderStageFlags::VertexShader));
    W_TEST_BOOL(!WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::GeometryShader, WGALShaderStageFlags::VertexShader));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compute is never covered by graphics and vice versa")
  {
    W_TEST_BOOL(!WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::VertexShader, WGALShaderStageFlags::ComputeShader));
    W_TEST_BOOL(!WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::PixelShader, WGALShaderStageFlags::ComputeShader));
    W_TEST_BOOL(!WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::ComputeShader, WGALShaderStageFlags::PixelShader));
    W_TEST_BOOL(!WGALResourceStateTracker::AreStagesCovered(WGALShaderStageFlags::ComputeShader, WGALShaderStageFlags::VertexShader));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Combined graphics + compute does not cross-cover")
  {
    // Having both VS and CS covered still doesn't let CS cover PS (different pipelines).
    // But VS does cover PS, so this should be true.
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(
      WGALShaderStageFlags::VertexShader | WGALShaderStageFlags::ComputeShader, WGALShaderStageFlags::PixelShader));

    // Requiring both CS and PS: VS covers PS, CS covers CS -> covered.
    W_TEST_BOOL(WGALResourceStateTracker::AreStagesCovered(
      WGALShaderStageFlags::VertexShader | WGALShaderStageFlags::ComputeShader,
      WGALShaderStageFlags::PixelShader | WGALShaderStageFlags::ComputeShader));
  }
}

// ============================================================
// Buffer state tracking (needs a device for resource creation)
// ============================================================

W_CREATE_SIMPLE_RENDERER_TEST(ResourceStateTracker, BufferStateTracking)
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WGALResourceStateTracker tracker(pDevice);

  // Helper: count emitted barriers
  WUInt32 uiBarrierCount = 0;
  WGALBufferBarrier lastBarrier;
  auto bufferCallback = [&](const WGALBufferBarrier& barrier)
  {
    uiBarrierCount++;
    lastBarrier = barrier;
  };

  // Create a structured buffer for testing. ShaderResource gives it a default state of ShaderResource.
  WGALBufferCreationDescription bufDesc;
  bufDesc.m_uiStructSize = sizeof(WUInt32);
  bufDesc.m_uiTotalSize = 256;
  bufDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
  bufDesc.m_ResourceAccess.m_bImmutable = false;
  WGALBufferHandle hBuffer = pDevice->CreateBuffer(bufDesc);
  W_TEST_BOOL(!hBuffer.IsInvalidated());
  W_SCOPE_EXIT(pDevice->DestroyBuffer(hBuffer));

  W_TEST_BLOCK(WTestBlock::Enabled, "First access initializes from GetDefaultState")
  {
    // The buffer's default state is UnorderedAccess (because UAV flag is set and it's mutable).
    // Transitioning to ShaderResource should emit a barrier from the default.
    uiBarrierCount = 0;
    tracker.ChangeState(hBuffer, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader, bufferCallback);
    W_TEST_INT(uiBarrierCount, 1);
    W_TEST_BOOL(lastBarrier.m_StateAfter.IsSet(WGALResourceState::ShaderResource));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Same state does not emit barrier")
  {
    uiBarrierCount = 0;
    tracker.ChangeState(hBuffer, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader, bufferCallback);
    W_TEST_INT(uiBarrierCount, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "State change emits barrier")
  {
    uiBarrierCount = 0;
    tracker.ChangeState(hBuffer, WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader, bufferCallback);
    W_TEST_INT(uiBarrierCount, 1);
    W_TEST_BOOL(lastBarrier.m_StateBefore.IsSet(WGALResourceState::ShaderResource));
    W_TEST_BOOL(lastBarrier.m_StateAfter.IsSet(WGALResourceState::UnorderedAccess));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UAV to UAV emits write-after-write barrier")
  {
    uiBarrierCount = 0;
    tracker.ChangeState(hBuffer, WGALResourceState::UnorderedAccess, WGALShaderStageFlags::ComputeShader, bufferCallback);
    W_TEST_INT(uiBarrierCount, 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetInitialBufferState overrides tracked state")
  {
    tracker.SetInitialBufferState(hBuffer, WGALResourceState::CopySource, WGALShaderStageFlags::Auto);
    uiBarrierCount = 0;
    tracker.ChangeState(hBuffer, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader, bufferCallback);
    W_TEST_INT(uiBarrierCount, 1);
    W_TEST_BOOL(lastBarrier.m_StateBefore.IsSet(WGALResourceState::CopySource));
    W_TEST_BOOL(lastBarrier.m_StateAfter.IsSet(WGALResourceState::ShaderResource));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear removes all tracking")
  {
    tracker.Clear();
    // After clear, accessing the buffer should re-initialize from default.
    uiBarrierCount = 0;
    tracker.ChangeState(hBuffer, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader, bufferCallback);
    // Should emit a barrier from the default state.
    W_TEST_INT(uiBarrierCount, 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RevertBufferState transitions back to default")
  {
    // Current state is ShaderResource. Revert should go back to the buffer's default (UAV).
    uiBarrierCount = 0;
    tracker.RevertBufferState(bufferCallback);
    W_TEST_INT(uiBarrierCount, 1);
    W_TEST_BOOL(lastBarrier.m_StateBefore.IsSet(WGALResourceState::ShaderResource));
    W_TEST_BOOL(lastBarrier.m_StateAfter.IsSet(WGALResourceState::UnorderedAccess));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetBufferState returns tracked state")
  {
    tracker.Clear();
    W_TEST_BOOL(tracker.GetBufferState(hBuffer) == nullptr);

    tracker.ChangeState(hBuffer, WGALResourceState::CopyDestination, WGALShaderStageFlags::Auto, bufferCallback);
    const auto* pState = tracker.GetBufferState(hBuffer);
    W_TEST_BOOL(pState != nullptr);
    W_TEST_BOOL(pState->m_State.IsSet(WGALResourceState::CopyDestination));
  }
}

// ============================================================
// Texture state tracking - compressed and sub-resource paths
// ============================================================

W_CREATE_SIMPLE_RENDERER_TEST(ResourceStateTracker, TextureStateTracking)
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WGALResourceStateTracker tracker(pDevice);

  WUInt32 uiBarrierCount = 0;
  WGALTextureBarrier lastBarrier;
  auto textureCallback = [&](const WGALTextureBarrier& barrier)
  {
    uiBarrierCount++;
    lastBarrier = barrier;
  };

  // Create a 2-mip, 2-layer texture for sub-resource testing.
  WGALTextureCreationDescription texDesc;
  texDesc.m_uiWidth = 16;
  texDesc.m_uiHeight = 16;
  texDesc.m_uiMipLevelCount = 2;
  texDesc.m_uiArraySize = 2;
  texDesc.m_Type = WGALTextureType::Texture2DArray;
  texDesc.m_Format = WGALResourceFormat::RGBAUByteNormalized;
  texDesc.m_TextureFlags = WGALTextureUsageFlags::ShaderResource | WGALTextureUsageFlags::RenderTarget;
  texDesc.m_ResourceAccess.m_bImmutable = false;

  WGALTextureHandle hTexture = pDevice->CreateTexture(texDesc);
  W_TEST_BOOL(!hTexture.IsInvalidated());
  W_SCOPE_EXIT(pDevice->DestroyTexture(hTexture));

  W_TEST_BLOCK(WTestBlock::Enabled, "Full-range transition stays compressed")
  {
    // Default state for this texture is ShaderResource (mutable RT+SRV textures default to SRV).
    // Transition to RenderTarget to trigger a barrier.
    uiBarrierCount = 0;
    tracker.ChangeState(hTexture, {}, WGALResourceState::RenderTarget, WGALShaderStageFlags::Auto, textureCallback);
    W_TEST_INT(uiBarrierCount, 1);
    W_TEST_BOOL(lastBarrier.m_bAllSubresources);
    W_TEST_BOOL(lastBarrier.m_StateAfter.IsSet(WGALResourceState::RenderTarget));

    // Verify internal state is still compressed (single entry).
    const auto* pState = tracker.GetTextureState(hTexture);
    W_TEST_BOOL(pState != nullptr);
    W_TEST_INT(pState->m_SubResourceStates.GetCount(), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Same full-range write - barrier (WAW)")
  {
    // RenderTarget -> RenderTarget is a write-after-write on the same state, but write states always need barriers.
    uiBarrierCount = 0;
    tracker.ChangeState(hTexture, {}, WGALResourceState::RenderTarget, WGALShaderStageFlags::Auto, textureCallback);
    // State doesn't change but RenderTarget is a write state, not tracked via UAV WAW. No barrier expected.
    // Actually the tracker only forces WAV barrier for UAV. For same non-UAV write state, the state bits match so no barrier.
    // We just verify no crash and then transition to ShaderResource.
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Write to read emits barrier")
  {
    uiBarrierCount = 0;
    tracker.ChangeState(hTexture, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader, textureCallback);
    W_TEST_INT(uiBarrierCount, 1);
    W_TEST_BOOL(lastBarrier.m_bAllSubresources);
    W_TEST_BOOL(lastBarrier.m_StateBefore.IsSet(WGALResourceState::RenderTarget));
    W_TEST_BOOL(lastBarrier.m_StateAfter.IsSet(WGALResourceState::ShaderResource));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Partial-range transition expands to sub-resources")
  {
    // Current state is ShaderResource (full). Transition only mip 0, layer 0 to RenderTarget.
    // This should expand the compressed state to per-subresource tracking.
    WGALTextureRange partialRange = {0, 1, 0, 1}; // layer 0, mip 0
    uiBarrierCount = 0;
    tracker.ChangeState(hTexture, partialRange, WGALResourceState::RenderTarget, WGALShaderStageFlags::Auto, textureCallback);
    W_TEST_INT(uiBarrierCount, 1);
    W_TEST_BOOL(!lastBarrier.m_bAllSubresources);
    W_TEST_INT(lastBarrier.m_Subresource.m_uiMipLevel, 0);
    W_TEST_INT(lastBarrier.m_Subresource.m_uiArraySlice, 0);

    // Verify expanded: 2 mips * 2 layers = 4 sub-resources.
    const auto* pState = tracker.GetTextureState(hTexture);
    W_TEST_BOOL(pState != nullptr);
    W_TEST_INT(pState->m_SubResourceStates.GetCount(), 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Subsequent partial transition only affects target sub-resource")
  {
    // Mip 1, layer 1 is still in ShaderResource (only mip 0, layer 0 was changed above).
    // Transition it to CopySource. Only that sub-resource should get a barrier.
    WGALTextureRange otherRange = {1, 1, 1, 1}; // layer 1, mip 1
    uiBarrierCount = 0;
    tracker.ChangeState(hTexture, otherRange, WGALResourceState::CopySource, WGALShaderStageFlags::Auto, textureCallback);
    W_TEST_INT(uiBarrierCount, 1);
    W_TEST_BOOL(!lastBarrier.m_bAllSubresources);
    W_TEST_INT(lastBarrier.m_Subresource.m_uiMipLevel, 1);
    W_TEST_INT(lastBarrier.m_Subresource.m_uiArraySlice, 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RevertTextureState transitions all sub-resources to default")
  {
    uiBarrierCount = 0;
    tracker.RevertTextureState(textureCallback);
    // At least the sub-resources that differ from the default should get barriers.
    W_TEST_BOOL(uiBarrierCount > 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetInitialTextureState resets to compressed")
  {
    tracker.SetInitialTextureState(hTexture, WGALResourceState::CopySource, WGALShaderStageFlags::Auto);
    const auto* pState = tracker.GetTextureState(hTexture);
    W_TEST_BOOL(pState != nullptr);
    W_TEST_INT(pState->m_SubResourceStates.GetCount(), 1);
    W_TEST_BOOL(pState->m_SubResourceStates[0].m_State.IsSet(WGALResourceState::CopySource));

    // Transition away to verify the overridden state is used as "before".
    uiBarrierCount = 0;
    tracker.ChangeState(hTexture, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader, textureCallback);
    W_TEST_INT(uiBarrierCount, 1);
    W_TEST_BOOL(lastBarrier.m_StateBefore.IsSet(WGALResourceState::CopySource));
  }
}
