#include <RendererTest/RendererTestPCH.h>

#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererTest/Basics/IndirectDraw.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>

#include "../../../../Data/UnitTests/RendererTest/Shaders/IndirectArgs.h"

static WRendererTestIndirectDraw s_IndirectDrawTest;

void WRendererTestIndirectDraw::SetupSubTests()
{
  AddSubTest("DrawInstancedIndirect", SubTests::ST_DrawInstancedIndirect);
  AddSubTest("DrawIndexedInstancedIndirect", SubTests::ST_DrawIndexedInstancedIndirect);
  AddSubTest("DrawIndexedInstancedIndirect Offset", SubTests::ST_DrawIndexedInstancedIndirectOffset);
  AddSubTest("DispatchIndirect", SubTests::ST_DispatchIndirect);
}

WResult WRendererTestIndirectDraw::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(WGraphicsTest::InitializeSubTest(iIdentifier));
  W_SUCCEED_OR_RETURN(CreateWindow(s_uiRTSize, s_uiRTSize));

  WGPUResourcePool* pResourcePool = W_DEFAULT_NEW(WGPUResourcePool);
  WGPUResourcePool::SetDefaultInstance(pResourcePool);

  // Indirect args buffer: 32 bytes is enough for all argument structs (max 5 uint32 = 20 bytes) plus offset tests.
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = 4;
    desc.m_uiTotalSize = 64;
    desc.m_BufferFlags = WGALBufferUsageFlags::ByteAddressBuffer | WGALBufferUsageFlags::UnorderedAccess | WGALBufferUsageFlags::DrawIndirect;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hIndirectArgsBuffer = m_pDevice->CreateBuffer(desc);
    W_ASSERT_DEV(!m_hIndirectArgsBuffer.IsInvalidated(), "Failed to create indirect args buffer");
  }

  // Non-indexed triangle mesh: a full-NDC triangle (covers the entire viewport).
  {
    WGeometry geom;
    geom.AddRect(WVec2(2.0f, 2.0f), 1, 1);
    m_hTriangleMesh = CreateMesh(geom, "IndirectDrawTriangle");
  }

  // Indexed triangle mesh: same geometry but through the indexed path (CreateMesh already creates index buffers).
  m_hIndexedTriangleMesh = m_hTriangleMesh;

  // Load shaders.
  m_hFillArgsShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/FillIndirectArgs.WShader");
  m_hDrawShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/StencilColor.WShader");
  m_hInstancedDrawShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/IndirectDrawInstances.WShader");
  m_hIndexedInstancedDrawShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/IndirectDrawIndexedInstances.WShader");
  m_hDispatchWriteShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/IndirectDispatchWrite.WShader");

  m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);

  // Dispatch output texture (for DispatchIndirect test).
  if (iIdentifier == ST_DispatchIndirect)
  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = 16;
    desc.m_uiHeight = 16;
    desc.m_Format = WGALResourceFormat::RGBAFloat;
    desc.m_TextureFlags = WGALTextureUsageFlags::UnorderedAccess | WGALTextureUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hDispatchOutputTexture = m_pDevice->CreateTexture(desc);
    W_ASSERT_DEV(!m_hDispatchOutputTexture.IsInvalidated(), "Failed to create dispatch output texture");

    m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2D.WShader");
  }

  return W_SUCCESS;
}

WResult WRendererTestIndirectDraw::DeInitializeSubTest(WInt32 iIdentifier)
{
  m_hFillArgsShader.Invalidate();
  m_hDrawShader.Invalidate();
  m_hInstancedDrawShader.Invalidate();
  m_hIndexedInstancedDrawShader.Invalidate();
  m_hDispatchWriteShader.Invalidate();
  m_hTriangleMesh.Invalidate();
  m_hIndexedTriangleMesh.Invalidate();

  m_pDevice->DestroyBuffer(m_hIndirectArgsBuffer);
  m_pDevice->DestroyTexture(m_hDispatchOutputTexture);

  WGPUResourcePool::SetDefaultInstance(nullptr);
  DestroyWindow();
  W_SUCCEED_OR_RETURN(WGraphicsTest::DeInitializeSubTest(iIdentifier));
  return W_SUCCESS;
}

WTestAppRun WRendererTestIndirectDraw::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;

  BeginFrame();

  switch (iIdentifier)
  {
    case ST_DrawInstancedIndirect:
      DrawInstancedIndirect();
      break;
    case ST_DrawIndexedInstancedIndirect:
      DrawIndexedInstancedIndirect();
      break;
    case ST_DrawIndexedInstancedIndirectOffset:
      DrawIndexedInstancedIndirectOffset();
      break;
    case ST_DispatchIndirect:
      DispatchIndirect();
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  EndFrame();
  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return WTestAppRun::Quit;
  }
  return WTestAppRun::Continue;
}

void WRendererTestIndirectDraw::DrawInstancedIndirect()
{
  WRenderContext* pContext = WRenderContext::GetDefaultInstance();

  BeginCommands("DrawInstancedIndirect");

  // Compute pass: fill indirect args with {vertexCount=6, instanceCount=4, startVertex=0, startInstance=0}
  FillIndirectArgsViaCompute(6, 4, 0, 0);

  TransitionBuffer(m_hIndirectArgsBuffer, WGALResourceState::DrawIndirect);
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

  {
    BeginRendering(WColor::Black);

    pContext->BindShader(m_hInstancedDrawShader);
    pContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
    pContext->ApplyContextStates().AssertSuccess();
    m_pEncoder->DrawInstancedIndirect(m_hIndirectArgsBuffer, 0).AssertSuccess();

    EndRendering();
  }

  CaptureImage();
  EndCommands();
}

void WRendererTestIndirectDraw::DrawIndexedInstancedIndirect()
{
  WRenderContext* pContext = WRenderContext::GetDefaultInstance();

  BeginCommands("DrawIndexedInstancedIndirect");

  // Compute pass: fill indexed indirect args {indexCount=6, instanceCount=4, startIndex=0, baseVertex=0, startInstance=0}
  FillIndirectArgsViaCompute(6, 4, 0, 0, 0);

  TransitionBuffer(m_hIndirectArgsBuffer, WGALResourceState::DrawIndirect);
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

  {
    BeginRendering(WColor::Black);

    pContext->BindShader(m_hIndexedInstancedDrawShader);
    pContext->BindMeshBuffer(m_hIndexedTriangleMesh);
    pContext->ApplyContextStates().AssertSuccess();
    m_pEncoder->DrawIndexedInstancedIndirect(m_hIndirectArgsBuffer, 0).AssertSuccess();

    EndRendering();
  }

  CaptureImage();
  EndCommands();
}

void WRendererTestIndirectDraw::DrawIndexedInstancedIndirectOffset()
{
  WRenderContext* pContext = WRenderContext::GetDefaultInstance();
  WMat4 mMVP = WMat4::MakeIdentity();
  if (WClipSpaceYMode::RenderToTextureDefault == WClipSpaceYMode::Flipped)
  {
    mMVP = WMat4::MakeScaling(WVec3(1.0f, -1.0f, 1.0f)) * mMVP;
  }

  BeginCommands("DrawIndexedIndirectOffset");

  // Zero arguments at offset 0 must leave the cleared target untouched.
  FillIndirectArgsViaCompute(0, 0, 0, 0, 0);

  TransitionBuffer(m_hIndirectArgsBuffer, WGALResourceState::DrawIndirect);
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

  {
    const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
    WRectFloat viewport = WRectFloat(0, 0, fWidth * 0.5f, fHeight);

    BeginRendering(WColor::Black, 0xFFFFFFFF, &viewport);

    ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mMVP;
    ocb->m_Color = WColor(1.0f, 0.0f, 1.0f, 1.0f);
    pContext->GetBindGroup().BindBuffer("PerObject", m_hObjectTransformCB);
    pContext->BindShader(m_hDrawShader);
    pContext->BindMeshBuffer(m_hIndexedTriangleMesh);
    pContext->ApplyContextStates().AssertSuccess();
    m_pEncoder->DrawIndexedInstancedIndirect(m_hIndirectArgsBuffer, 0).AssertSuccess();

    EndRendering();
  }

  EndCommands();

  BeginCommands("DrawIndexedIndirectOffset");

  // Fill valid args at byte offset 20 using UpdateBuffer from CPU.
  // DrawIndexedInstanced args: {indexCount=6, instanceCount=1, startIndex=0, baseVertex=0, startInstance=0}
  {
    alignas(16) WUInt32 args[5] = {6, 1, 0, 0, 0};
    m_pEncoder->UpdateBuffer(m_hIndirectArgsBuffer, 20, WMakeArrayPtr(reinterpret_cast<const WUInt8*>(args), sizeof(args)), WGALUpdateMode::AheadOfTime);
  }

  TransitionBuffer(m_hIndirectArgsBuffer, WGALResourceState::DrawIndirect);
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

  {
    const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
    WRectFloat viewport = WRectFloat(fWidth * 0.5f, 0, fWidth * 0.5f, fHeight);

    BeginRendering(WColor::Black, 0, &viewport);

    ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mMVP;
    ocb->m_Color = WColor(1.0f, 1.0f, 0.0f, 1.0f);
    pContext->GetBindGroup().BindBuffer("PerObject", m_hObjectTransformCB);
    pContext->BindShader(m_hDrawShader);
    pContext->BindMeshBuffer(m_hIndexedTriangleMesh);
    pContext->ApplyContextStates().AssertSuccess();
    // Use offset 20 into the buffer.
    m_pEncoder->DrawIndexedInstancedIndirect(m_hIndirectArgsBuffer, 20).AssertSuccess();

    EndRendering();
  }

  CaptureImage();
  EndCommands();
}

void WRendererTestIndirectDraw::DispatchIndirect()
{
  WRenderContext* pContext = WRenderContext::GetDefaultInstance();

  BeginCommands("DispatchIndirect");

  // Fill dispatch args: {threadGroupCountX=2, threadGroupCountY=2, threadGroupCountZ=1}
  // The compute shader uses [numthreads(8,8,1)], so 2*2 groups = 16x16 threads = fills the 16x16 texture.
  FillIndirectArgsViaCompute(2, 2, 1, 0);

  TransitionBuffer(m_hIndirectArgsBuffer, WGALResourceState::DrawIndirect);
  TransitionTexture(m_hDispatchOutputTexture, WGALResourceState::UnorderedAccess);

  // Dispatch pass.
  {
    pContext->BeginCompute("IndirectDispatch");
    pContext->BindShader(m_hDispatchWriteShader);

    WBindGroupBuilder& bg = pContext->GetBindGroup();
    bg.BindTexture("OutputTexture", m_hDispatchOutputTexture);
    pContext->ApplyContextStates().AssertSuccess();
    m_pEncoder->DispatchIndirect(m_hIndirectArgsBuffer, 0).AssertSuccess();

    pContext->EndCompute();
  }

  TransitionTexture(m_hDispatchOutputTexture, WGALResourceState::ShaderResource);

  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  WRectFloat viewport = WRectFloat(0, 0, fWidth, fHeight);
  const WMat4 mTextureMVP = CreateSimpleMVP(fWidth / fHeight);
  m_bCaptureImage = m_ImgCompFrames.Contains(m_iFrame);
  RenderCube(viewport, mTextureMVP, 0xFFFFFFFF, m_hDispatchOutputTexture);
  EndCommands();
}

// ============================================================
// Helper: Fill indirect args via a compute dispatch
// ============================================================

void WRendererTestIndirectDraw::FillIndirectArgsViaCompute(WUInt32 arg0, WUInt32 arg1, WUInt32 arg2, WUInt32 arg3, WUInt32 arg4)
{
  WRenderContext* pContext = WRenderContext::GetDefaultInstance();

  TransitionBuffer(m_hIndirectArgsBuffer, WGALResourceState::UnorderedAccess);

  pContext->BeginCompute("FillIndirectArgs");
  pContext->BindShader(m_hFillArgsShader);

  WIndirectArgs constants;
  constants.Arg0 = arg0;
  constants.Arg1 = arg1;
  constants.Arg2 = arg2;
  constants.Arg3 = arg3;
  constants.Arg4 = arg4;
  pContext->SetPushConstants("WIndirectArgs", constants);

  WBindGroupBuilder& bg = pContext->GetBindGroup();
  bg.BindBuffer("IndirectArgsBuffer", m_hIndirectArgsBuffer);
  pContext->Dispatch(1).AssertSuccess();
  pContext->EndCompute();
}

void WRendererTestIndirectDraw::CaptureImage()
{
  if (m_ImgCompFrames.Contains(m_iFrame))
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
    W_TEST_IMAGE(m_iFrame, 100);
  }
}
