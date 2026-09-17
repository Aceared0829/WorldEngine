#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"

WTestAppRun WRendererTestBasics::SubtestLineRendering()
{
  BeginFrame();
  BeginCommands("RendererTest");
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

  WColor clear(0, 0, 0, 0);
  BeginRendering(clear);

  RenderLineObjects(WShaderBindFlags::Default);

  EndRendering();
  TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
  W_TEST_LINE_IMAGE(0, 150);
  EndCommands();
  EndFrame();

  return m_iFrame < 0 ? WTestAppRun::Continue : WTestAppRun::Quit;
}
