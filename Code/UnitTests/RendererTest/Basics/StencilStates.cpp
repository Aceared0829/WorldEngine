#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <RendererTest/Basics/StencilStates.h>

static WRendererTestStencilStates g_StencilStatesTest;

WResult WRendererTestStencilStates::InitializeSubTest(WInt32 iIdentifier)
{
  m_iFrame = -1;

  if (WGraphicsTest::InitializeSubTest(iIdentifier).Failed())
    return W_FAILURE;

  if (CreateWindow().Failed())
    return W_FAILURE;

  // Create a simple quad mesh for stencil testing
  {
    WGeometry geom;
    geom.AddRect(WVec2(2.0f, 2.0f), 1, 1);

    WMeshBufferResourceDescriptor desc;
    desc.AddStream(WMeshVertexStreamType::Position);
    desc.AddStream(WMeshVertexStreamType::Color0);
    desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

    m_hQuadMesh = WResourceManager::GetOrCreateResource<WMeshBufferResource>("StencilTestQuad", std::move(desc), "StencilTestQuad");
  }

  m_hStencilShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/StencilColor.WShader");

  return W_SUCCESS;
}

WResult WRendererTestStencilStates::DeInitializeSubTest(WInt32 iIdentifier)
{
  m_hQuadMesh.Invalidate();
  m_hStencilShader.Invalidate();

  DestroyWindow();

  if (WGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

void WRendererTestStencilStates::RenderQuad(const WMat4& mTransform, const WColor& color, WBitflags<WShaderBindFlags> ShaderBindFlags)
{
  // Bind our stencil shader (not the base class shader) with the provided flags
  WRenderContext::GetDefaultInstance()->BindShader(m_hStencilShader, ShaderBindFlags);

  ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
  ocb->m_MVP = mTransform;
  ocb->m_Color = color;

  WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
  bindGroupTest.BindBuffer("PerObject", m_hObjectTransformCB);

  WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hQuadMesh);
  WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();
}

WTestAppRun WRendererTestStencilStates::SubtestStencilOperations()
{
  BeginFrame();
  BeginCommands("StencilOperations");
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

  // Setup camera/projection
  WCamera cam;
  cam.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(WVec3(0, 0, 0), WVec3(0, 0, -1), WVec3(0, 1, 0));
  WMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  WMat4 mView = cam.GetViewMatrix();

  // Create depth-stencil state that writes to stencil using Replace operation
  WGALDepthStencilStateCreationDescription depthStencilDescWrite;
  depthStencilDescWrite.m_bDepthEnable = false;
  depthStencilDescWrite.m_bDepthWrite = false;
  depthStencilDescWrite.m_bStencilEnable = true;
  depthStencilDescWrite.m_uiStencilReadMask = 0xFF;
  depthStencilDescWrite.m_uiStencilWriteMask = 0xFF;

  // Create depth-stencil state that tests against stencil
  WGALDepthStencilStateCreationDescription depthStencilDescTest;
  depthStencilDescTest.m_bDepthEnable = false;
  depthStencilDescTest.m_bDepthWrite = false;
  depthStencilDescTest.m_bStencilEnable = true;
  depthStencilDescTest.m_uiStencilReadMask = 0xFF;
  depthStencilDescTest.m_uiStencilWriteMask = 0x00; // Don't write to stencil during test

  WRenderContext* pRenderContext = WRenderContext::GetDefaultInstance();

  if (m_iFrame == 1)
  {
    // Test: StencilOp::Replace
    // Draw a small quad to write value 1 to stencil using Replace
    // Then draw a larger quad that only passes where stencil == 1
    BeginRendering(WColor::Black);

    // First pass: Write to stencil with Replace operation
    depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = WGALStencilOp::Replace;
    depthStencilDescWrite.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Always;
    depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

    WGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(1);

    // Draw small quad in center
    WMat4 mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Second pass: Test stencil == 1
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Equal;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    WGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(1);

    // Draw larger quad - should only appear where stencil was written, so the blue quad should turn red.
    mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, WColor::Red, WShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 2)
  {
    // Test: StencilOp::Increment
    // Draw multiple overlapping quads, stencil value increases with each
    // Then test for specific stencil values
    BeginRendering(WColor::Black);

    depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = WGALStencilOp::IncrementSaturated;
    depthStencilDescWrite.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Always;
    depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

    WGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(0);

    // Draw overlapping quads - each increments stencil
    WMat4 mTransform = WMat4::MakeTranslation(WVec3(-0.2f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.5f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    mTransform = WMat4::MakeTranslation(WVec3(0.2f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.5f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Test: Draw where stencil == 2 (overlapping region)
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Equal;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    WGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(2);

    // Draw fullscreen - only the overlap area should be visible
    mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(1.0f));
    RenderQuad(mProj * mView * mTransform, WColor::Green, WShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 3)
  {
    // Test: StencilOp::Zero
    // Write to stencil, then zero part of it
    BeginRendering(WColor::Black);

    // First: Fill stencil with 1
    depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = WGALStencilOp::Replace;
    depthStencilDescWrite.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Always;
    depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

    WGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(1);

    WMat4 mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Zero a smaller region
    depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = WGALStencilOp::Zero;
    depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

    m_pDevice->DestroyDepthStencilState(hWriteState);
    hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);

    mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Test: Draw where stencil == 1 (non-zeroed region)
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Equal;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    WGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(1);

    mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(1.0f));
    RenderQuad(mProj * mView * mTransform, WColor::Yellow, WShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
  W_TEST_IMAGE(m_iFrame, 100);
  EndCommands();
  EndFrame();

  return m_iFrame < 3 ? WTestAppRun::Continue : WTestAppRun::Quit;
}

WTestAppRun WRendererTestStencilStates::SubtestStencilCompareFunctions()
{
  BeginFrame();
  BeginCommands("StencilCompareFunctions");
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

  // Setup camera/projection
  WCamera cam;
  cam.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(WVec3(0, 0, 0), WVec3(0, 0, -1), WVec3(0, 1, 0));
  WMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  WMat4 mView = cam.GetViewMatrix();

  WGALDepthStencilStateCreationDescription depthStencilDescWrite;
  depthStencilDescWrite.m_bDepthEnable = false;
  depthStencilDescWrite.m_bDepthWrite = false;
  depthStencilDescWrite.m_bStencilEnable = true;
  depthStencilDescWrite.m_uiStencilReadMask = 0xFF;
  depthStencilDescWrite.m_uiStencilWriteMask = 0xFF;
  depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = WGALStencilOp::Replace;
  depthStencilDescWrite.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Always;
  depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

  WGALDepthStencilStateCreationDescription depthStencilDescTest;
  depthStencilDescTest.m_bDepthEnable = false;
  depthStencilDescTest.m_bDepthWrite = false;
  depthStencilDescTest.m_bStencilEnable = true;
  depthStencilDescTest.m_uiStencilReadMask = 0xFF;
  depthStencilDescTest.m_uiStencilWriteMask = 0x00;

  WRenderContext* pRenderContext = WRenderContext::GetDefaultInstance();

  if (m_iFrame == 1)
  {
    // Test: CompareFunc::NotEqual
    BeginRendering(WColor::Black);

    WGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(1);

    // Write stencil value of 1 to center
    WMat4 mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.4f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Test NotEqual - should pass everywhere except center
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::NotEqual;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    WGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(1);

    mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, WColor::Green, WShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 2)
  {
    // Test: CompareFunc::Greater
    BeginRendering(WColor::Black);

    WGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(5);

    // Write stencil value of 5 to center
    WMat4 mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.5f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Test Greater with ref value 3: ref (3) > stencil (5) should not pass, while outside the blue sqaure (ref (3) > stencil 0) should pass
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Greater;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    WGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(3);

    mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, WColor::Red, WShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 3)
  {
    // Test: CompareFunc::LessEqual
    BeginRendering(WColor::Black);

    // Write stencil value 2 to right half, value 4 to left half
    WGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);

    pRenderContext->SetStencilRefValue(2);
    WMat4 mTransform = WMat4::MakeTranslation(WVec3(-0.3f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.4f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    pRenderContext->SetStencilRefValue(4);
    mTransform = WMat4::MakeTranslation(WVec3(0.3f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.4f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Test LessEqual with ref value 3 - should fail on the right (3 <= 2) but pass on the left (3 <= 4)
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::LessEqual;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    WGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(3);

    mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(1.0f));
    RenderQuad(mProj * mView * mTransform, WColor::Yellow, WShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 4)
  {
    // Test: CompareFunc::Never - should never pass
    BeginRendering(WColor::Black);

    WGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(1);

    WMat4 mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.5f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Never compare - nothing should be drawn
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Never;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    WGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(1);

    mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, WColor::Red, WShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
  W_TEST_IMAGE(m_iFrame, 100);
  EndCommands();
  EndFrame();

  return m_iFrame < 4 ? WTestAppRun::Continue : WTestAppRun::Quit;
}

WTestAppRun WRendererTestStencilStates::SubtestStencilRefValue()
{
  if (m_iFrame == 1)
  {
    BeginFrame();
    EndFrame();
  }


  BeginFrame();
  BeginCommands("StencilRefValue");
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

  // Setup camera/projection
  WCamera cam;
  cam.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(WVec3(0, 0, 0), WVec3(0, 0, -1), WVec3(0, 1, 0));
  WMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  WMat4 mView = cam.GetViewMatrix();

  WGALDepthStencilStateCreationDescription depthStencilDescWrite;
  depthStencilDescWrite.m_bDepthEnable = false;
  depthStencilDescWrite.m_bDepthWrite = false;
  depthStencilDescWrite.m_bStencilEnable = true;
  depthStencilDescWrite.m_uiStencilReadMask = 0xFF;
  depthStencilDescWrite.m_uiStencilWriteMask = 0xFF;
  depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = WGALStencilOp::Replace;
  depthStencilDescWrite.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Always;
  depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

  WGALDepthStencilStateCreationDescription depthStencilDescTest;
  depthStencilDescTest.m_bDepthEnable = false;
  depthStencilDescTest.m_bDepthWrite = false;
  depthStencilDescTest.m_bStencilEnable = true;
  depthStencilDescTest.m_uiStencilReadMask = 0xFF;
  depthStencilDescTest.m_uiStencilWriteMask = 0x00;
  depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Equal;
  depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

  WRenderContext* pRenderContext = WRenderContext::GetDefaultInstance();

  if (m_iFrame == 1)
  {
    // Test: Using SetStencilRefValue to write and test different reference values
    BeginRendering(WColor::Black);

    WGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);

    // Write stencil value 10 to upper right
    pRenderContext->SetStencilRefValue(10);
    WMat4 mTransform = WMat4::MakeTranslation(WVec3(-0.3f, 0.3f, -2.0f)) * WMat4::MakeScaling(WVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Write stencil value 20 to upper left
    pRenderContext->SetStencilRefValue(20);
    mTransform = WMat4::MakeTranslation(WVec3(0.3f, 0.3f, -2.0f)) * WMat4::MakeScaling(WVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Write stencil value 30 to lower right
    pRenderContext->SetStencilRefValue(30);
    mTransform = WMat4::MakeTranslation(WVec3(-0.3f, -0.3f, -2.0f)) * WMat4::MakeScaling(WVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Write stencil value 40 to lower left
    pRenderContext->SetStencilRefValue(40);
    mTransform = WMat4::MakeTranslation(WVec3(0.3f, -0.3f, -2.0f)) * WMat4::MakeScaling(WVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Now test each quadrant
    WGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);

    // Test for value 10 - upper right should be red
    pRenderContext->SetStencilRefValue(10);
    mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(1.0f));
    RenderQuad(mProj * mView * mTransform, WColor::Red, WShaderBindFlags::NoDepthStencilState);

    // Test for value 20 - upper left should be green
    pRenderContext->SetStencilRefValue(20);
    RenderQuad(mProj * mView * mTransform, WColor::Green, WShaderBindFlags::NoDepthStencilState);

    // Test for value 30 - lower right should be yellow
    pRenderContext->SetStencilRefValue(30);
    RenderQuad(mProj * mView * mTransform, WColor::Yellow, WShaderBindFlags::NoDepthStencilState);

    // Test for value 40 - lower left should be cyan
    pRenderContext->SetStencilRefValue(40);
    RenderQuad(mProj * mView * mTransform, WColor::Cyan, WShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 2)
  {
    // Test: Stencil mask functionality
    BeginRendering(WColor::Black);

    // Use a write mask that only writes to lower 4 bits
    depthStencilDescWrite.m_uiStencilWriteMask = 0x0F;

    WGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);

    // Write 0xFF - but only lower 4 bits should be written (0x0F)
    pRenderContext->SetStencilRefValue(0xFF);
    WMat4 mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.5f));
    RenderQuad(mProj * mView * mTransform, WColor::Blue, WShaderBindFlags::NoDepthStencilState);

    // Test: Read only lower 4 bits and compare to 0x0F
    depthStencilDescTest.m_uiStencilReadMask = 0x0F;

    WGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(0x0F);

    mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -2.0f)) * WMat4::MakeScaling(WVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, WColor::Green, WShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
  W_TEST_IMAGE(m_iFrame, 100);
  EndCommands();
  EndFrame();

  return m_iFrame < 2 ? WTestAppRun::Continue : WTestAppRun::Quit;
}
