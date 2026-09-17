#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>

WResult WRendererTestBasics::InitializeSubTest(WInt32 iIdentifier)
{
  m_iFrame = -1;

  if (WGraphicsTest::InitializeSubTest(iIdentifier).Failed())
    return W_FAILURE;

  if (iIdentifier == SubTests::ST_ClearScreen)
  {
    return CreateWindow(320, 240);
  }

  if (CreateWindow().Failed())
    return W_FAILURE;

  m_hSphere = CreateSphere(3, 1.0f);
  m_hSphere2 = CreateSphere(1, 0.75f);
  m_hTorus = CreateTorus(16, 0.5f, 0.75f);
  m_hLongBox = CreateBox(0.4f, 0.2f, 2.0f);
  m_hLineBox = CreateLineBox(0.4f, 0.2f, 2.0f);



  return W_SUCCESS;
}

WResult WRendererTestBasics::DeInitializeSubTest(WInt32 iIdentifier)
{
  m_hSphere.Invalidate();
  m_hSphere2.Invalidate();
  m_hTorus.Invalidate();
  m_hLongBox.Invalidate();
  m_hLineBox.Invalidate();
  m_hTexture2D.Invalidate();
  m_hTextureCube.Invalidate();

  DestroyWindow();

  if (WGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return W_FAILURE;

  return W_SUCCESS;
}


WTestAppRun WRendererTestBasics::SubtestClearScreen()
{
  BeginFrame();
  BeginCommands("ClearScreen");
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
  switch (m_iFrame)
  {
    case 0:
      BeginRendering(WColor(1, 0, 0));
      break;
    case 1:
      BeginRendering(WColor(0, 1, 0));
      break;
    case 2:
      BeginRendering(WColor(0, 0, 1));
      break;
    case 3:
      BeginRendering(WColor(0.5f, 0.5f, 0.5f, 0.5f));
      break;
  }

  EndRendering();
  TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
  W_TEST_IMAGE(m_iFrame, 1);
  EndCommands();
  EndFrame();

  return m_iFrame < 3 ? WTestAppRun::Continue : WTestAppRun::Quit;
}

void WRendererTestBasics::RenderObjects(WBitflags<WShaderBindFlags> ShaderBindFlags)
{
  WCamera cam;
  cam.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(WVec3(0, 0, 0), WVec3(0, 0, -1), WVec3(0, 1, 0));
  WMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  WMat4 mView = cam.GetViewMatrix();

  WMat4 mTransform, mOther, mRot;

  mRot = WMat4::MakeRotationX(WAngle::MakeFromDegree(-90));

  mOther = WMat4::MakeScaling(WVec3(1.0f, 1.0f, 1.0f));
  mTransform = WMat4::MakeTranslation(WVec3(-0.3f, -0.3f, 0.0f));
  RenderObject(m_hLongBox, mProj * mView * mTransform * mOther, WColor(1, 0, 1, 0.25f), ShaderBindFlags);

  mOther = WMat4::MakeRotationX(WAngle::MakeFromDegree(80.0f));
  mTransform = WMat4::MakeTranslation(WVec3(0.75f, 0, -1.8f));
  RenderObject(m_hTorus, mProj * mView * mTransform * mOther * mRot, WColor(1, 0, 0, 0.5f), ShaderBindFlags);

  mOther.SetIdentity();
  mTransform = WMat4::MakeTranslation(WVec3(0, 0.1f, -2.0f));
  RenderObject(m_hSphere, mProj * mView * mTransform * mOther, WColor(0, 1, 0, 0.75f), ShaderBindFlags);

  mOther = WMat4::MakeScaling(WVec3(1.5f, 1.0f, 1.0f));
  mTransform = WMat4::MakeTranslation(WVec3(-0.6f, -0.2f, -2.2f));
  RenderObject(m_hSphere2, mProj * mView * mTransform * mOther * mRot, WColor(0, 0, 1, 1), ShaderBindFlags);
}

void WRendererTestBasics::RenderLineObjects(WBitflags<WShaderBindFlags> ShaderBindFlags)
{
  WCamera cam;
  cam.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(WVec3(0, 0, 0), WVec3(0, 0, -1), WVec3(0, 1, 0));
  WMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  WMat4 mView = cam.GetViewMatrix();

  WMat4 mTransform, mOther, mRot;

  mRot = WMat4::MakeRotationX(WAngle::MakeFromDegree(-90));

  mOther = WMat4::MakeScaling(WVec3(1.0f, 1.0f, 1.0f));
  mTransform = WMat4::MakeTranslation(WVec3(-0.3f, -0.3f, 0.0f));
  RenderObject(m_hLineBox, mProj * mView * mTransform * mOther, WColor(1, 0, 1, 0.25f), ShaderBindFlags);
}

static WRendererTestBasics g_Test;
