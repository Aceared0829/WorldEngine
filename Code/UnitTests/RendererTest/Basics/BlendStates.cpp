#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"

WTestAppRun WRendererTestBasics::SubtestBlendStates()
{
  BeginFrame();
  BeginCommands("BlendStates");
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
  WGALBlendStateHandle hState;

  WGALBlendStateCreationDescription StateDesc;
  StateDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled = true;

  if (m_iFrame == 0)
  {
    // StateDesc.m_RenderTargetBlendDescriptions[0].
  }

  if (m_iFrame == 1)
  {
    StateDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend = WGALBlend::SrcAlpha;
    StateDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend = WGALBlend::InvSrcAlpha;
  }

  WColor clear(0, 0, 0, 0);
  // if (StateDesc.m_bDepthClip)
  //  clear.r = 0.5f;
  // if (StateDesc.m_bFrontCounterClockwise)
  //  clear.g = 0.5f;
  // if (StateDesc.m_CullMode == WGALCullMode::Front)
  //  clear.b = 0.5f;
  // if (StateDesc.m_CullMode == WGALCullMode::Back)
  //  clear.b = 1.0f;

  BeginRendering(clear);

  hState = m_pDevice->CreateBlendState(StateDesc);
  W_ASSERT_DEV(!hState.IsInvalidated(), "Couldn't create blend state!");

  WRenderContext::GetDefaultInstance()->SetBlendState(hState);

  RenderObjects(WShaderBindFlags::NoBlendState);

  EndRendering();
  TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
  W_TEST_IMAGE(m_iFrame, 150);
  EndCommands();
  EndFrame();

  m_pDevice->DestroyBlendState(hState);

  return m_iFrame < 1 ? WTestAppRun::Continue : WTestAppRun::Quit;
}
