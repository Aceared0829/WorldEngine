#include <RendererTest/RendererTestPCH.h>

#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/State/GraphicsPipeline.h>
#include <RendererFoundation/State/State.h>
#include <RendererFoundation/Utils/RingBufferTracker.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

namespace
{
  void VerifyDependenciesExist(WGALDevice* pDevice, const WGALGraphicsPipelineCreationDescription& graphicsPipelineDesc)
  {
    W_TEST_BOOL(pDevice->GetShader(graphicsPipelineDesc.m_hShader) != nullptr);
    W_TEST_BOOL(pDevice->GetBlendState(graphicsPipelineDesc.m_hBlendState) != nullptr);
    W_TEST_BOOL(pDevice->GetRasterizerState(graphicsPipelineDesc.m_hRasterizerState) != nullptr);
    W_TEST_BOOL(pDevice->GetDepthStencilState(graphicsPipelineDesc.m_hDepthStencilState) != nullptr);
  }

  void VerifyDependenciesAreDestroyed(WGALDevice* pDevice, const WGALGraphicsPipelineCreationDescription& graphicsPipelineDesc)
  {
    W_TEST_BOOL(pDevice->GetShader(graphicsPipelineDesc.m_hShader) == nullptr);
    W_TEST_BOOL(pDevice->GetBlendState(graphicsPipelineDesc.m_hBlendState) == nullptr);
    W_TEST_BOOL(pDevice->GetRasterizerState(graphicsPipelineDesc.m_hRasterizerState) == nullptr);
    W_TEST_BOOL(pDevice->GetDepthStencilState(graphicsPipelineDesc.m_hDepthStencilState) == nullptr);
  }

  void VerifyDependenciesRefCount(WGALDevice* pDevice, const WGALGraphicsPipelineCreationDescription& graphicsPipelineDesc, WUInt32 uiRefCount)
  {
    W_TEST_INT(pDevice->GetShader(graphicsPipelineDesc.m_hShader)->GetRefCount(), uiRefCount);
    W_TEST_INT(pDevice->GetBlendState(graphicsPipelineDesc.m_hBlendState)->GetRefCount(), uiRefCount);
    W_TEST_INT(pDevice->GetRasterizerState(graphicsPipelineDesc.m_hRasterizerState)->GetRefCount(), uiRefCount);
    W_TEST_INT(pDevice->GetDepthStencilState(graphicsPipelineDesc.m_hDepthStencilState)->GetRefCount(), uiRefCount);
  }

  void DestroyDependencies(WGALDevice* pDevice, const WGALGraphicsPipelineCreationDescription& graphicsPipelineDesc)
  {
    // the destroy functions want a mutable reference but we don't want to modify the original description
    auto hShader = graphicsPipelineDesc.m_hShader;
    auto hRasterizerState = graphicsPipelineDesc.m_hRasterizerState;
    auto hBlendState = graphicsPipelineDesc.m_hBlendState;
    auto hDepthStencilState = graphicsPipelineDesc.m_hDepthStencilState;

    pDevice->DestroyShader(hShader);
    pDevice->DestroyRasterizerState(hRasterizerState);
    pDevice->DestroyBlendState(hBlendState);
    pDevice->DestroyDepthStencilState(hDepthStencilState);
  }
} // namespace

W_CREATE_SIMPLE_RENDERER_TEST(DataStructures, StateDeduplication)



{
  WGALShaderCreationDescription shaderDesc;
  WGALBlendStateCreationDescription blendDesc;
  WGALRasterizerStateCreationDescription rasterDesc;
  WGALDepthStencilStateCreationDescription depthDesc;
  WGALRenderPassDescriptor renderPassDesc;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  auto CreateDependencies = [&](WGALGraphicsPipelineCreationDescription& ref_graphicsPipelineDesc)
  {
    ref_graphicsPipelineDesc.m_hShader = pDevice->CreateShader(shaderDesc);
    ref_graphicsPipelineDesc.m_hBlendState = pDevice->CreateBlendState(blendDesc);
    ref_graphicsPipelineDesc.m_hDepthStencilState = pDevice->CreateDepthStencilState(depthDesc);
    ref_graphicsPipelineDesc.m_hRasterizerState = pDevice->CreateRasterizerState(rasterDesc);
    ref_graphicsPipelineDesc.m_RenderPass = renderPassDesc;
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "LoadAndFreeTestData")
  {
    renderPassDesc.m_uiRTCount = 1;
    renderPassDesc.m_DepthFormat = WGALResourceFormat::D24S8;
    renderPassDesc.m_ColorFormat[0] = WGALResourceFormat::RGBAUByteNormalizedsRGB;
    renderPassDesc.m_ColorLoadOp[0] = WGALRenderTargetLoadOp::Clear;
    renderPassDesc.m_ColorStoreOp[0] = WGALRenderTargetStoreOp::Store;

    WGALGraphicsPipelineCreationDescription graphicsPipelineDesc;
    WGALGraphicsPipelineHandle hGraphicsPipeline;
    {
      WShaderResourceHandle hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/MostBasicTriangle.WShader");

      WHashedString sFalse = WMakeHashedString("FALSE");
      WHashTable<WHashedString, WHashedString> permutationVariables;
      permutationVariables[WMakeHashedString("CLIP_SPACE_FLIPPED")] = sFalse;
      permutationVariables[WMakeHashedString("MSAA")] = sFalse;
      permutationVariables[WMakeHashedString("TOPOLOGY")] = WMakeHashedString("TOPOLOGY_TRIANGLES");

      WShaderPermutationResourceHandle hActiveShaderPermutation = WShaderManager::PreloadSinglePermutation(hShader, permutationVariables, false);

      WResourceLock<WShaderPermutationResource> pShaderPermutation(hActiveShaderPermutation, WResourceAcquireMode::BlockTillLoaded);

      graphicsPipelineDesc.m_hShader = pShaderPermutation->GetGALShader();
      graphicsPipelineDesc.m_hBlendState = pShaderPermutation->GetBlendState();
      graphicsPipelineDesc.m_hRasterizerState = pShaderPermutation->GetRasterizerState();
      graphicsPipelineDesc.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
      graphicsPipelineDesc.m_RenderPass = renderPassDesc;

      shaderDesc = pDevice->GetShader(graphicsPipelineDesc.m_hShader)->GetDescription();
      blendDesc = pDevice->GetBlendState(graphicsPipelineDesc.m_hBlendState)->GetDescription();
      rasterDesc = pDevice->GetRasterizerState(graphicsPipelineDesc.m_hRasterizerState)->GetDescription();
      depthDesc = pDevice->GetDepthStencilState(graphicsPipelineDesc.m_hDepthStencilState)->GetDescription();

      hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);
    }

    WUInt32 uiUnloaded = 0;
    for (WUInt32 tries = 0; tries < 3; ++tries)
    {
      // if a resource is in a loading queue, unloading it can actually 'fail' for a short time
      uiUnloaded += WResourceManager::FreeAllUnusedResources();

      if (uiUnloaded == 2)
        break;

      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
    }
    W_TEST_INT_MSG(uiUnloaded, 2, "This should have freed the WShaderResource and the WShaderPermutationResource");

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    VerifyDependenciesExist(pDevice, graphicsPipelineDesc);

    // This will mark the pipeline and its dependencies as dead objects which will be freed in the next frame loop.
    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    W_TEST_BOOL(pDevice->GetGraphicsPipeline(hGraphicsPipeline) == nullptr);
    VerifyDependenciesAreDestroyed(pDevice, graphicsPipelineDesc);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReferenceCounting")
  {
    WGALGraphicsPipelineCreationDescription graphicsPipelineDesc;
    WGALGraphicsPipelineHandle hGraphicsPipeline;

    CreateDependencies(graphicsPipelineDesc);

    VerifyDependenciesExist(pDevice, graphicsPipelineDesc);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 1);

    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    W_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 1);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 2);

    // The destroy function below will invalidate the handle, so we need to copy it for later use.
    auto hGraphicsPipelineCopy = hGraphicsPipeline;
    pDevice->DestroyGraphicsPipeline(hGraphicsPipelineCopy);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 1);
    DestroyDependencies(pDevice, graphicsPipelineDesc);

    W_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 0);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 0);

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    W_TEST_BOOL(pDevice->GetGraphicsPipeline(hGraphicsPipeline) == nullptr);
    VerifyDependenciesAreDestroyed(pDevice, graphicsPipelineDesc);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReviveDeadObjects")
  {
    WGALGraphicsPipelineCreationDescription graphicsPipelineDesc;
    WGALGraphicsPipelineHandle hGraphicsPipeline;

    CreateDependencies(graphicsPipelineDesc);
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    // The destroy function below will invalidate the handle, so we need to copy it for later use.
    auto hGraphicsPipelineCopy = hGraphicsPipeline;
    pDevice->DestroyGraphicsPipeline(hGraphicsPipelineCopy);
    DestroyDependencies(pDevice, graphicsPipelineDesc);

    W_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 0);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 0);

    // This should revive the dead objects
    CreateDependencies(graphicsPipelineDesc);
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    W_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 1);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 2);

    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);
    DestroyDependencies(pDevice, graphicsPipelineDesc);

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    W_TEST_BOOL(pDevice->GetGraphicsPipeline(hGraphicsPipeline) == nullptr);
    VerifyDependenciesAreDestroyed(pDevice, graphicsPipelineDesc);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "OddDestructionOrder")
  {
    WGALGraphicsPipelineCreationDescription graphicsPipelineDesc;
    WGALGraphicsPipelineHandle hGraphicsPipeline;

    CreateDependencies(graphicsPipelineDesc);
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    // Destroy dependencies first this time
    DestroyDependencies(pDevice, graphicsPipelineDesc);

    // The destroy function below will invalidate the handle, so we need to copy it for later use.
    auto hGraphicsPipelineCopy = hGraphicsPipeline;
    pDevice->DestroyGraphicsPipeline(hGraphicsPipelineCopy);

    W_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 0);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 0);

    // This should revive the dead objects
    CreateDependencies(graphicsPipelineDesc);
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    W_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 1);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 2);

    // Destroy the dependencies only partially
    pDevice->DestroyShader(graphicsPipelineDesc.m_hShader);
    pDevice->DestroyBlendState(graphicsPipelineDesc.m_hBlendState);
    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);

    graphicsPipelineDesc.m_hShader = pDevice->CreateShader(shaderDesc);
    graphicsPipelineDesc.m_hBlendState = pDevice->CreateBlendState(blendDesc);
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    W_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 1);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 2);

    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);
    DestroyDependencies(pDevice, graphicsPipelineDesc);

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    W_TEST_BOOL(pDevice->GetGraphicsPipeline(hGraphicsPipeline) == nullptr);
    VerifyDependenciesAreDestroyed(pDevice, graphicsPipelineDesc);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReviveDeadDependencies")
  {
    WGALGraphicsPipelineCreationDescription graphicsPipelineDesc;
    WGALGraphicsPipelineHandle hGraphicsPipeline;

    CreateDependencies(graphicsPipelineDesc);
    DestroyDependencies(pDevice, graphicsPipelineDesc);
    // Now this is technically invalid code as we are creating a pipeline referencing dead handles. However, this works for all GAL state objects right now and is a side-effect of the fact that objects are deleted at the end of the frame.
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    W_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 1);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 1);

    // The destroy function below will invalidate the handle, so we need to copy it for later use.
    auto hGraphicsPipelineCopy = hGraphicsPipeline;
    pDevice->DestroyGraphicsPipeline(hGraphicsPipelineCopy);

    W_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 0);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 0);

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    W_TEST_BOOL(pDevice->GetGraphicsPipeline(hGraphicsPipeline) == nullptr);
    VerifyDependenciesAreDestroyed(pDevice, graphicsPipelineDesc);
  }
}
