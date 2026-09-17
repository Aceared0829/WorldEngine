#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>

WTestAppRun WRendererTestBasics::SubtestRasterizerStates()
{
  BeginFrame();
  BeginCommands("RasterizerStates");
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
  WGALRasterizerStateHandle hState;

  WGALRasterizerStateCreationDescription RasterStateDesc;

  if (m_iFrame == 0)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = WGALCullMode::None;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 1)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = WGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 2)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = WGALCullMode::Front;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 3)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = WGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 4)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = WGALCullMode::Front;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 5)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = WGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 6)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = WGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = true;
  }


  if (m_iFrame == 7)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = WGALCullMode::None;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 8)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = WGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 9)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = WGALCullMode::Front;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 10)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = WGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 11)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = WGALCullMode::Front;
    RasterStateDesc.m_bScissorTest = false;
  }

  WColor clear(0, 0, 0, 0);
  if (!RasterStateDesc.m_bFrontCounterClockwise)
    clear.g = 0.5f;
  if (RasterStateDesc.m_CullMode == WGALCullMode::Front)
    clear.b = 0.5f;
  if (RasterStateDesc.m_CullMode == WGALCullMode::Back)
    clear.b = 1.0f;

  BeginRendering(clear);

  hState = m_pDevice->CreateRasterizerState(RasterStateDesc);
  W_ASSERT_DEV(!hState.IsInvalidated(), "Couldn't create rasterizer state!");

  WRenderContext::GetDefaultInstance()->SetRasterizerState(hState);

  WRenderContext::GetDefaultInstance()->GetCommandEncoder()->SetScissorRect(WRectU32(100, 50, GetResolution().width / 2, GetResolution().height / 2));

  RenderObjects(WShaderBindFlags::NoRasterizerState);

  EndRendering();
  TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
  if (RasterStateDesc.m_bWireFrame)
  {
    const bool bSupportsWireframe = GetDeviceCapabilities().m_bSupportsWireframe;
    if (bSupportsWireframe)
    {
      WStringView sRendererName = m_pDevice->GetRenderer();
      const bool bRandomlyChangesLineThicknessOnDriverUpdate = sRendererName.IsEqual_NoCase("DX11") && m_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Nvidia");

      W_TEST_LINE_IMAGE(m_iFrame, bRandomlyChangesLineThicknessOnDriverUpdate ? 1000 : 300);
    }
  }
  else
    W_TEST_IMAGE(m_iFrame, 200);
  EndCommands();
  EndFrame();

  m_pDevice->DestroyRasterizerState(hState);

  return m_iFrame < 11 ? WTestAppRun::Continue : WTestAppRun::Quit;
}
