#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>
#include <RendererTest/Advanced/RenderGraphTest.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>

static WRenderGraphTest s_RenderGraphTest;

void WRenderGraphTest::SetupSubTests()
{
  AddSubTest("DeadPassCulling", SubTests::ST_DeadPassCulling);
  AddSubTest("ResourceAliasing", SubTests::ST_ResourceAliasing);
  AddSubTest("ImportReplace", SubTests::ST_ImportReplace);
  AddSubTest("ExecuteCallbacks", SubTests::ST_ExecuteCallbacks);
  AddSubTest("EmptyGraph", SubTests::ST_EmptyGraph);
  AddSubTest("StressTest", SubTests::ST_StressTest);
  AddSubTest("MsaaResolve", SubTests::ST_MsaaResolve);
}

WResult WRenderGraphTest::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(WGraphicsTest::InitializeSubTest(iIdentifier));
  W_SUCCEED_OR_RETURN(CreateWindow());

  WGPUResourcePool* pResourcePool = W_DEFAULT_NEW(WGPUResourcePool);
  WGPUResourcePool::SetDefaultInstance(pResourcePool);
  m_pRenderGraph = WRenderGraphManager::CreateRenderGraph("RendererTest");

  return W_SUCCESS;
}

WResult WRenderGraphTest::DeInitializeSubTest(WInt32 iIdentifier)
{
  m_pRenderGraph = nullptr;
  WGPUResourcePool::SetDefaultInstance(nullptr);
  DestroyWindow();
  W_SUCCEED_OR_RETURN(WGraphicsTest::DeInitializeSubTest(iIdentifier));
  return W_SUCCESS;
}

WTestAppRun WRenderGraphTest::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;

  switch (iIdentifier)
  {
    case SubTests::ST_DeadPassCulling:
      DeadPassCulling();
      return WTestAppRun::Quit;
    case SubTests::ST_ResourceAliasing:
      ResourceAliasing();
      return WTestAppRun::Quit;
    case SubTests::ST_ImportReplace:
      ImportReplace();
      return WTestAppRun::Quit;
    case SubTests::ST_ExecuteCallbacks:
      ExecuteCallbacks();
      return WTestAppRun::Quit;
    case SubTests::ST_EmptyGraph:
      EmptyGraph();
      return WTestAppRun::Quit;
    case SubTests::ST_StressTest:
      return StressTestRenderGraph(256);
    case SubTests::ST_MsaaResolve:
      MsaaResolve();
      return WTestAppRun::Quit;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return WTestAppRun::Quit;
  }
}

// ============================================================
// Helper: create a simple render target texture description
// ============================================================

namespace
{
  WGALTextureCreationDescription MakeColorRT(WUInt32 w = 64, WUInt32 h = 64)
  {
    WGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(w, h, WGALResourceFormat::RGBAHalf);
    return desc;
  }

  auto NoopCallback()
  {
    return [](const WRenderGraphContext&) {};
  }
} // namespace

// ============================================================
// Test: Dead Pass Culling
// ============================================================

void WRenderGraphTest::DeadPassCulling()
{
  BeginFrame();

  // Build a graph with a mix of side-effecting and non-side-effecting passes.
  auto& graph = *m_pRenderGraph;
  graph.Reset();

  auto desc = MakeColorRT();
  auto hTex = graph.CreateTexture(desc);

  // Pass 0: writes hTex, no side effects, no downstream consumer -> should be culled.
  bool bCulledPassExecuted = false;
  {
    auto pass = graph.AddGraphicsPass("ShouldBeCulled");
    pass.AddColorTarget(hTex, {}, WGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0, WColor::Red);
    pass.SetExecuteCallback([&](const WRenderGraphContext&)
      { bCulledPassExecuted = true; });
  }

  // Pass 1: writes to a different texture, has side effects -> should survive.
  auto hTex2 = graph.CreateTexture(desc);
  bool bSideEffectPassExecuted = false;
  {
    auto pass = graph.AddGraphicsPass("HasSideEffects");
    pass.AddColorTarget(hTex2, {}, WGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0, WColor::Blue);
    pass.HasSideEffects();
    pass.SetExecuteCallback([&](const WRenderGraphContext&)
      { bSideEffectPassExecuted = true; });
  }

  // Pass 2: writes to an imported texture -> should survive (implicit side effect).
  const WGALSwapChain* pSwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
  auto hImported = graph.ImportTexture(pSwapChain->GetBackBufferTexture());
  bool bImportWritePassExecuted = false;
  {
    auto pass = graph.AddGraphicsPass("WritesToImported");
    pass.AddColorTarget(hImported, {}, WGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0, WColor::Green);
    pass.SetExecuteCallback([&](const WRenderGraphContext&)
      { bImportWritePassExecuted = true; });
  }

  // Pass 3: reads hTex but hTex's writer is culled. This pass has no side effects either -> should be culled.
  bool bReaderOfCulledExecuted = false;
  {
    auto pass = graph.AddComputePass("ReadsFromCulled");
    pass.ReadTexture(hTex, {}, WGALResourceState::ShaderResource);
    pass.SetExecuteCallback([&](const WRenderGraphContext&)
      { bReaderOfCulledExecuted = true; });
  }

  // Pass 4: Transitive dependency chain: A->B->C where C has side effects.
  auto hChainTex1 = graph.CreateTexture(desc);
  auto hChainTex2 = graph.CreateTexture(desc);
  bool bChainAExecuted = false, bChainBExecuted = false, bChainCExecuted = false;
  {
    auto passA = graph.AddGraphicsPass("ChainA");
    passA.AddColorTarget(hChainTex1, {}, WGALRenderTargetLoadOp::Clear);
    passA.SetClearColor(0, WColor::White);
    passA.SetExecuteCallback([&](const WRenderGraphContext&)
      { bChainAExecuted = true; });
  }
  {
    auto passB = graph.AddGraphicsPass("ChainB");
    passB.ReadTexture(hChainTex1, {}, WGALResourceState::ShaderResource);
    passB.AddColorTarget(hChainTex2, {}, WGALRenderTargetLoadOp::Clear);
    passB.SetClearColor(0, WColor::White);
    passB.SetExecuteCallback([&](const WRenderGraphContext&)
      { bChainBExecuted = true; });
  }
  {
    auto passC = graph.AddGraphicsPass("ChainC");
    passC.ReadTexture(hChainTex2, {}, WGALResourceState::ShaderResource);
    passC.AddColorTarget(hChainTex2, {}, WGALRenderTargetLoadOp::Clear);
    passC.HasSideEffects();
    passC.SetExecuteCallback([&](const WRenderGraphContext&)
      { bChainCExecuted = true; });
  }

  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  WRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  W_TEST_BOOL_MSG(!bCulledPassExecuted, "Pass without side effects and no consumer should be culled");
  W_TEST_BOOL_MSG(bSideEffectPassExecuted, "Pass with HasSideEffects should survive");
  W_TEST_BOOL_MSG(bImportWritePassExecuted, "Pass writing to imported texture should survive");
  W_TEST_BOOL_MSG(!bReaderOfCulledExecuted, "Reader of a culled pass should also be culled");
  W_TEST_BOOL_MSG(bChainAExecuted, "Chain source should survive via transitive dependency");
  W_TEST_BOOL_MSG(bChainBExecuted, "Chain middle should survive via transitive dependency");
  W_TEST_BOOL_MSG(bChainCExecuted, "Chain sink with side effects should survive");
}

// ============================================================
// Test: Resource Aliasing
// ============================================================

void WRenderGraphTest::ResourceAliasing()
{
  BeginFrame();

  auto& graph = *m_pRenderGraph;
  graph.Reset();

  auto desc = MakeColorRT();

  // Two textures with non-overlapping lifetimes and identical descriptions.
  // Pass 0 writes texA, Pass 1 reads texA (texA's lifetime ends).
  // Pass 2 writes texB, Pass 3 reads texB (texB's lifetime ends).
  // texA and texB should alias to the same GPU resource.
  auto hTexA = graph.CreateTexture(desc);
  auto hTexB = graph.CreateTexture(desc);

  WGALTextureHandle hResolvedA, hResolvedB;

  {
    auto pass0 = graph.AddGraphicsPass("WriteA");
    pass0.AddColorTarget(hTexA, {}, WGALRenderTargetLoadOp::Clear);
    pass0.SetClearColor(0, WColor::Red);
    pass0.SetExecuteCallback([&](const WRenderGraphContext& ctx)
      { hResolvedA = ctx.ResolveTexture(hTexA); });
  }
  {
    auto pass1 = graph.AddGraphicsPass("ReadA");
    pass1.ReadTexture(hTexA);
    pass1.HasSideEffects();
    pass1.SetExecuteCallback(NoopCallback());
  }
  {
    auto pass2 = graph.AddGraphicsPass("WriteB");
    pass2.AddColorTarget(hTexB, {}, WGALRenderTargetLoadOp::Clear);
    pass2.SetClearColor(0, WColor::Blue);
    pass2.SetExecuteCallback([&](const WRenderGraphContext& ctx)
      { hResolvedB = ctx.ResolveTexture(hTexB); });
  }
  {
    auto pass3 = graph.AddGraphicsPass("ReadB");
    pass3.ReadTexture(hTexB);
    pass3.HasSideEffects();
    pass3.SetExecuteCallback(NoopCallback());
  }

  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  WRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  W_TEST_BOOL_MSG(!hResolvedA.IsInvalidated(), "texA should resolve to a valid GPU handle");
  W_TEST_BOOL_MSG(!hResolvedB.IsInvalidated(), "texB should resolve to a valid GPU handle");
  W_TEST_BOOL_MSG(hResolvedA == hResolvedB, "Non-overlapping same-desc textures should alias");

  // Now test overlapping lifetimes: texC and texD are both alive during pass1.
  BeginFrame();
  graph.Reset();

  auto hTexC = graph.CreateTexture(desc);
  auto hTexD = graph.CreateTexture(desc);

  WGALTextureHandle hResolvedC, hResolvedD;

  {
    auto pass0 = graph.AddGraphicsPass("WriteC");
    pass0.AddColorTarget(hTexC, {}, WGALRenderTargetLoadOp::Clear);
    pass0.SetClearColor(0);
    pass0.SetExecuteCallback([&](const WRenderGraphContext& ctx)
      { hResolvedC = ctx.ResolveTexture(hTexC); });
  }
  {
    auto pass1 = graph.AddGraphicsPass("WriteDReadC");
    pass1.ReadTexture(hTexC);
    pass1.AddColorTarget(hTexD, {}, WGALRenderTargetLoadOp::Clear);
    pass1.SetClearColor(0);
    pass1.SetExecuteCallback([&](const WRenderGraphContext& ctx)
      { hResolvedD = ctx.ResolveTexture(hTexD); });
  }
  {
    auto pass2 = graph.AddGraphicsPass("ReadD");
    pass2.ReadTexture(hTexD);
    pass2.HasSideEffects();
    pass2.SetExecuteCallback(NoopCallback());
  }

  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  WRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  W_TEST_BOOL_MSG(hResolvedC != hResolvedD, "Overlapping-lifetime textures must not alias");
}

// ============================================================
// Test: Import / Replace
// ============================================================

void WRenderGraphTest::ImportReplace()
{
  BeginFrame();

  auto& graph = *m_pRenderGraph;
  graph.Reset();

  // Create three GPU textures: A and B have matching descriptions, C has a different format.
  WGALTextureCreationDescription texDesc;
  texDesc.SetAsRenderTarget(64, 64, WGALResourceFormat::RGBAHalf);
  WGALTextureHandle hGPUTexA = m_pDevice->CreateTexture(texDesc);
  WGALTextureHandle hGPUTexB = m_pDevice->CreateTexture(texDesc);
  W_SCOPE_EXIT(m_pDevice->DestroyTexture(hGPUTexA); m_pDevice->DestroyTexture(hGPUTexB));

  WGALTextureCreationDescription texDescDiff;
  texDescDiff.SetAsRenderTarget(64, 64, WGALResourceFormat::RFloat);
  WGALTextureHandle hGPUTexDiff = m_pDevice->CreateTexture(texDescDiff);
  W_SCOPE_EXIT(m_pDevice->DestroyTexture(hGPUTexDiff));

  // Import returns the same handle for the same GPU texture.
  auto hImportA = graph.ImportTexture(hGPUTexA);
  auto hImportA2 = graph.ImportTexture(hGPUTexA);
  W_TEST_BOOL(hImportA == hImportA2);

  // Add a side-effect pass so the graph compiles.
  {
    auto pass = graph.AddGraphicsPass("Use");
    pass.AddColorTarget(hImportA, {}, WGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0);
    pass.HasSideEffects();
    pass.SetExecuteCallback(NoopCallback());
  }

  // Replace A with B (matching description, B not yet imported) -> should succeed.
  W_TEST_BOOL(graph.ReplaceImportedTexture(hImportA, hGPUTexB).Succeeded());

  // Replace A (now pointing to B) with a different format -> should fail.
  W_TEST_BOOL(graph.ReplaceImportedTexture(hImportA, hGPUTexDiff).Failed());

  // Compile so that resolved textures are available.
  W_TEST_RESULT(graph.Compile());

  // After replace, re-compiling and executing should use the new texture (B).
  WGALTextureHandle hResolved;
  graph.Reset();
  auto hImportNew = graph.ImportTexture(hGPUTexB);
  {
    auto pass = graph.AddGraphicsPass("VerifyReplace");
    pass.AddColorTarget(hImportNew, {}, WGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0);
    pass.HasSideEffects();
    pass.SetExecuteCallback([&](const WRenderGraphContext& ctx)
      { hResolved = ctx.ResolveTexture(hImportNew); });
  }

  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  WRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  W_TEST_BOOL(hResolved == hGPUTexB);
}

// ============================================================
// Test: Execute Callbacks
// ============================================================

void WRenderGraphTest::ExecuteCallbacks()
{
  BeginFrame();

  auto& graph = *m_pRenderGraph;
  graph.Reset();

  auto desc = MakeColorRT();

  // Track execution order via a list of IDs.
  WDynamicArray<int> executionOrder;

  auto hTex = graph.CreateTexture(desc);

  // Pass 0: graphics, side effect
  {
    auto pass = graph.AddGraphicsPass("Pass0");
    pass.AddColorTarget(hTex, {}, WGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0);
    pass.HasSideEffects();
    pass.SetExecuteCallback([&](const WRenderGraphContext&)
      { executionOrder.PushBack(0); });
  }

  // Pass 1: compute, reads tex from pass 0
  {
    auto pass = graph.AddComputePass("Pass1");
    pass.ReadTexture(hTex, {}, WGALResourceState::ShaderResource);
    pass.HasSideEffects();
    pass.SetExecuteCallback([&](const WRenderGraphContext&)
      { executionOrder.PushBack(1); });
  }

  // Pass 2: should be culled (no side effects, no consumer)
  auto hDeadTex = graph.CreateTexture(desc);
  {
    auto pass = graph.AddGraphicsPass("Pass2-Dead");
    pass.AddColorTarget(hDeadTex, {}, WGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0);
    pass.SetExecuteCallback([&](const WRenderGraphContext&)
      { executionOrder.PushBack(2); });
  }

  // Pass 3: transfer, side effect
  {
    auto pass = graph.AddTransferPass("Pass3");
    pass.HasSideEffects();
    pass.SetExecuteCallback([&](const WRenderGraphContext&)
      { executionOrder.PushBack(3); });
  }

  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  WRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  // Pass2 should be culled. Remaining passes should execute in declaration order.
  W_TEST_INT(executionOrder.GetCount(), 3);
  if (executionOrder.GetCount() == 3)
  {
    W_TEST_INT(executionOrder[0], 0);
    W_TEST_INT(executionOrder[1], 1);
    W_TEST_INT(executionOrder[2], 3);
  }
}

// ============================================================
// Test: Empty Graph
// ============================================================

void WRenderGraphTest::EmptyGraph()
{
  // A graph with no passes should compile and execute without error.
  {
    BeginFrame();
    auto& graph = *m_pRenderGraph;
    graph.Reset();
    WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
    WRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
    EndFrame();
  }

  // A graph where ALL passes are culled should also work.
  {
    BeginFrame();
    auto& graph = *m_pRenderGraph;
    graph.Reset();

    auto desc = MakeColorRT();
    auto hTex = graph.CreateTexture(desc);

    {
      auto pass = graph.AddGraphicsPass("AllCulled");
      pass.AddColorTarget(hTex, {}, WGALRenderTargetLoadOp::Clear);
      pass.SetClearColor(0);
      // No HasSideEffects, no downstream consumer -> culled.
      pass.SetExecuteCallback(NoopCallback());
    }
    WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
    WRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
    EndFrame();
  }
}

// ============================================================
// Test: MSAA Resolve
// ============================================================

void WRenderGraphTest::MsaaResolve()
{
  BeginFrame();

  WRenderGraph& graph = *m_pRenderGraph.Borrow();
  graph.Reset();

  // BEGIN-DOCS-CODE-SNIPPET: rendergraph-msaa-create-resources
  // Create transient MSAA texture
  WGALTextureCreationDescription descMsaa;
  descMsaa.SetAsRenderTarget(64, 64, WGALResourceFormat::RGBAHalf, WGALMSAASampleCount::FourSamples);
  WRenderGraphTextureHandle hMsaaColor = graph.CreateTexture(descMsaa);

  // Import persistent non-MSAA resolve target
  WGALTextureCreationDescription descResolved;
  descResolved.SetAsRenderTarget(64, 64, WGALResourceFormat::RGBAHalf);
  WGALTextureHandle hResolvedGAL = m_pDevice->CreateTexture(descResolved);
  WRenderGraphTextureHandle hResolved = graph.ImportTexture(hResolvedGAL);
  // END-DOCS-CODE-SNIPPET

  // Create transient MSAA depth texture
  WGALTextureCreationDescription descMsaaDepth;
  descMsaaDepth.SetAsRenderTarget(64, 64, WGALResourceFormat::D24S8, WGALMSAASampleCount::FourSamples);
  WRenderGraphTextureHandle hMsaaDepth = graph.CreateTexture(descMsaaDepth);

  // Render into the MSAA target.
  {
    // BEGIN-DOCS-CODE-SNIPPET: rendergraph-graphics-pass
    auto pass = graph.AddGraphicsPass("RenderMSAA");
    pass.AddColorTarget(hMsaaColor, {}, WGALRenderTargetLoadOp::Clear, WGALRenderTargetStoreOp::Store);
    pass.SetClearColor(0, WColor::CornflowerBlue);
    pass.AddDepthStencilTarget(hMsaaDepth, {}, WGALRenderTargetLoadOp::Clear, WGALRenderTargetStoreOp::Store, WGALRenderTargetLoadOp::Clear, WGALRenderTargetStoreOp::Store);
    pass.SetClearDepth(1.0f);
    pass.SetClearStencil(0);
    pass.SetStereoscopic(false);
    pass.HasSideEffects();
    // END-DOCS-CODE-SNIPPET
  }

  // Resolve the MSAA texture into the imported target.
  {
    // BEGIN-DOCS-CODE-SNIPPET: rendergraph-msaa-barriers
    auto pass = graph.AddTransferPass("MsaaColorResolve");
    pass.ReadTexture(hMsaaColor, {}, WGALResourceState::ResolveSource);
    pass.WriteTexture(hResolved, {}, WGALResourceState::ResolveDestination);
    // END-DOCS-CODE-SNIPPET

    // BEGIN-DOCS-CODE-SNIPPET: rendergraph-msaa-execute-callback
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      WGALTextureSubresource subresource;
      subresource.m_uiMipLevel = 0;
      subresource.m_uiArraySlice = 0;
      ctx.GetCommandEncoder()->ResolveTexture(ctx.ResolveTexture(hResolved), subresource, ctx.ResolveTexture(hMsaaColor), subresource); });
    // END-DOCS-CODE-SNIPPET
  }

  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  WRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  m_pDevice->DestroyTexture(hResolvedGAL);
}

// ============================================================
// Test: Stress Test (kept from original)
// ============================================================

WTestAppRun WRenderGraphTest::StressTestRenderGraph(WUInt32 uiNumPasses)
{
  BeginFrame();

  WRenderGraph& graph = *m_pRenderGraph;
  graph.Reset();

  W_PROFILE_SCOPE("StressTestRenderGraph");
  {
    W_PROFILE_SCOPE("BuildGraph");
    WRandom rng;
    rng.Initialize(0x52454E44);

    const WSizeU32 res = GetResolution();
    const WUInt32 uiBaseWidth = res.width;
    const WUInt32 uiBaseHeight = res.height;

    constexpr WUInt32 uiNumTextures = 32;
    constexpr WUInt32 uiNumBuffers = 8;
    constexpr WUInt32 uiNumResolutions = 3;

    WGALResourceFormat::Enum textureFormats[] = {
      WGALResourceFormat::RGBAHalf,
      WGALResourceFormat::RGHalf,
      WGALResourceFormat::RHalf,
      WGALResourceFormat::RGBAFloat,
      WGALResourceFormat::BGRAUByteNormalized,
      WGALResourceFormat::RFloat,
    };

    WStaticArray<WRenderGraphTextureHandle, uiNumTextures> textures;
    WStaticArray<WUInt32, uiNumTextures> textureResolutionIndices;
    for (WUInt32 i = 0; i < uiNumTextures; ++i)
    {
      const WUInt32 uiResolutionIndex = rng.UIntInRange(uiNumResolutions);
      const WUInt32 uiDivisor = 1u << uiResolutionIndex;
      const WUInt32 w = WMath::Max(1u, uiBaseWidth / uiDivisor);
      const WUInt32 h = WMath::Max(1u, uiBaseHeight / uiDivisor);
      const auto format = textureFormats[rng.UIntInRange(W_ARRAY_SIZE(textureFormats))];

      WGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(w, h, format, WGALMSAASampleCount::None);
      desc.m_TextureFlags.Add(WGALTextureUsageFlags::UnorderedAccess);
      textures.PushBack(graph.CreateTexture(desc));
      textureResolutionIndices.PushBack(uiResolutionIndex);
    }

    WStaticArray<WRenderGraphTextureHandle, uiNumResolutions> depthTextures;
    for (WUInt32 i = 0; i < uiNumResolutions; ++i)
    {
      const WUInt32 uiDivisor = 1u << i;

      WGALTextureCreationDescription depthDesc;
      depthDesc.SetAsRenderTarget(WMath::Max(1u, uiBaseWidth / uiDivisor), WMath::Max(1u, uiBaseHeight / uiDivisor), WGALResourceFormat::D16, WGALMSAASampleCount::None);
      depthTextures.PushBack(graph.CreateTexture(depthDesc));
    }

    WStaticArray<WRenderGraphBufferHandle, uiNumBuffers> buffers;
    for (WUInt32 i = 0; i < uiNumBuffers; ++i)
    {
      WGALBufferCreationDescription desc;
      desc.m_uiStructSize = 4;
      desc.m_uiTotalSize = (128 + rng.UIntInRange(1024)) * 4;
      desc.m_BufferFlags = WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
      desc.m_ResourceAccess.m_bImmutable = false;
      buffers.PushBack(graph.CreateBuffer(desc));
    }

    const WGALSwapChain* pSwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
    WRenderGraphTextureHandle hBackbuffer = graph.ImportTexture(pSwapChain->GetBackBufferTexture());

    auto emptyCallback = [](const WRenderGraphContext&) {};

    for (WUInt32 p = 0; p < uiNumPasses; ++p)
    {
      const WUInt32 uiPassType = rng.UIntInRange(3);

      if (uiPassType == 0)
      {
        auto pass = graph.AddGraphicsPass("StressGraphics");
        const WUInt32 uiReads = 1 + rng.UIntInRange(3);
        for (WUInt32 r = 0; r < uiReads; ++r)
          pass.ReadTexture(textures[rng.UIntInRange(uiNumTextures)]);
        const WUInt32 uiColorTextureIndex = rng.UIntInRange(uiNumTextures);
        pass.AddColorTarget(textures[uiColorTextureIndex], {}, WGALRenderTargetLoadOp::Clear);
        pass.SetClearColor(0, WColor::Black);
        if (rng.Bool())
        {
          pass.AddDepthStencilTarget(depthTextures[textureResolutionIndices[uiColorTextureIndex]]);
          pass.SetClearDepth();
        }
        if (rng.Bool())
          pass.ReadBuffer(buffers[rng.UIntInRange(uiNumBuffers)], WGALResourceState::ShaderResource);
        pass.HasSideEffects();
        pass.SetExecuteCallback(emptyCallback);
      }
      else if (uiPassType == 1)
      {
        auto pass = graph.AddComputePass("StressCompute");
        const WUInt32 uiReads = 1 + rng.UIntInRange(2);
        for (WUInt32 r = 0; r < uiReads; ++r)
          pass.ReadTexture(textures[rng.UIntInRange(uiNumTextures)], {}, WGALResourceState::ShaderResource);
        pass.WriteTexture(textures[rng.UIntInRange(uiNumTextures)], {}, WGALResourceState::UnorderedAccess);
        if (rng.Bool())
          pass.ReadBuffer(buffers[rng.UIntInRange(uiNumBuffers)], WGALResourceState::ShaderResource);
        if (rng.Bool())
          pass.WriteBuffer(buffers[rng.UIntInRange(uiNumBuffers)]);
        pass.HasSideEffects();
        pass.SetExecuteCallback(emptyCallback);
      }
      else
      {
        auto pass = graph.AddTransferPass("StressTransfer");
        pass.ReadTexture(textures[rng.UIntInRange(uiNumTextures)], {}, WGALResourceState::CopySource);
        pass.WriteTexture(textures[rng.UIntInRange(uiNumTextures)], {}, WGALResourceState::CopyDestination);
        if (rng.Bool())
        {
          pass.ReadBuffer(buffers[rng.UIntInRange(uiNumBuffers)], WGALResourceState::CopySource);
          pass.WriteBuffer(buffers[rng.UIntInRange(uiNumBuffers)], WGALResourceState::CopyDestination);
        }
        pass.HasSideEffects();
        pass.SetExecuteCallback(emptyCallback);
      }
    }

    {
      auto pass = graph.AddGraphicsPass("StressFinalComposite");
      pass.AddColorTarget(hBackbuffer, {}, WGALRenderTargetLoadOp::Clear);
      pass.SetClearColor(0, WColor::Black);
      for (WUInt32 r = 0; r < 4; ++r)
        pass.ReadTexture(textures[rng.UIntInRange(uiNumTextures)]);
      pass.SetExecuteCallback(emptyCallback);
    }
  }

  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  WRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  if (m_iFrame < 255)
    return WTestAppRun::Continue;

  WProfilingUtils::SaveProfilingCapture(":imgout/Profiling/profiling.json").IgnoreResult();
  return WTestAppRun::Quit;
}
