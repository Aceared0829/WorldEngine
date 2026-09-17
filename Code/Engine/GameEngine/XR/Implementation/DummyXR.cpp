#include <GameEngine/GameEnginePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/System/WindowManager.h>
#include <Core/World/World.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/Configuration/XRConfig.h>
#include <GameEngine/XR/DummyXR.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <GameEngine/XR/XRWindow.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

W_IMPLEMENT_SINGLETON(WDummyXR);

WDummyXR::WDummyXR()
  : m_SingletonRegistrar(this)
{
}

bool WDummyXR::IsHmdPresent() const
{
  return true;
}

WResult WDummyXR::Initialize()
{
  if (m_bInitialized)
    return W_FAILURE;

  m_Info.m_sDeviceName = "Dummy VR device";
  m_Info.m_vEyeRenderTargetSize = WSizeU32(640, 720);

  m_GALdeviceEventsId = WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WDummyXR::GALDeviceEventHandler, this));
  m_ExecutionEventsId = WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(WMakeDelegate(&WDummyXR::GameApplicationEventHandler, this));

  m_bInitialized = true;
  return W_SUCCESS;
}

void WDummyXR::Deinitialize()
{
  WWindowManager::GetSingleton()->CloseAll(this);

  m_bInitialized = false;
  if (m_GALdeviceEventsId != 0)
  {
    WGALDevice::s_Events.RemoveEventHandler(m_GALdeviceEventsId);
  }
  if (m_ExecutionEventsId != 0)
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(m_ExecutionEventsId);
  }
}

bool WDummyXR::IsInitialized() const
{
  return m_bInitialized;
}

const WHMDInfo& WDummyXR::GetHmdInfo() const
{
  return m_Info;
}

WXRInputDevice& WDummyXR::GetXRInput() const
{
  return m_Input;
}

bool WDummyXR::SupportsCompanionView()
{
  return true;
}

WRegisteredWndHandle WDummyXR::CreateXRWindow(WView* pView, WGALMSAASampleCount::Enum msaaCount, WUniquePtr<WWindowBase> pCompanionWindow, WUniquePtr<WWindowOutputTargetGAL> pCompanionWindowOutput)
{
  W_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  // Create dummy swap chain
  {
    WGALTextureCreationDescription textureDesc;
    textureDesc.SetAsRenderTarget(m_Info.m_vEyeRenderTargetSize.width, m_Info.m_vEyeRenderTargetSize.height, WGALResourceFormat::RGBAUByteNormalizedsRGB, msaaCount);
    textureDesc.m_Type = WGALTextureType::Texture2DArray;
    textureDesc.m_uiArraySize = 2;

    m_hColorRT = pDevice->CreateTexture(textureDesc);

    textureDesc.m_Format = WGALResourceFormat::D24S8;
    m_hDepthRT = pDevice->CreateTexture(textureDesc);
  }

  // SetHMDCamera
  {
    m_pCameraToSynchronize = pView->GetCamera();
    m_pCameraToSynchronize->SetCameraMode(WCameraMode::Stereo, m_pCameraToSynchronize->GetFovOrDim(), m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
  }

  W_ASSERT_DEV((pCompanionWindow != nullptr) == (pCompanionWindowOutput != nullptr), "Both companionWindow and companionWindowOutput must either be null or valid.");

  WUniquePtr<WWindowXR> pXRWindow = W_DEFAULT_NEW(WWindowXR, this, std::move(pCompanionWindow));
  WUniquePtr<WWindowOutputTargetXR> pXRWindowOutputTarget = W_DEFAULT_NEW(WWindowOutputTargetXR, this, std::move(pCompanionWindowOutput));

  m_pCompanion = static_cast<WWindowOutputTargetXR*>(pXRWindowOutputTarget.Borrow());

  m_hView = pView->GetHandle();

  WGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0] = m_hColorRT;
  renderTargets.m_hDSTarget = m_hDepthRT;
  pView->SetRenderTargets(renderTargets);

  pView->SetViewport(WRectFloat((float)m_Info.m_vEyeRenderTargetSize.width, (float)m_Info.m_vEyeRenderTargetSize.height));

  auto pWinMan = WWindowManager::GetSingleton();
  WRegisteredWndHandle id = pWinMan->Register("DummyXR", this, std::move(pXRWindow));
  pWinMan->SetOutputTarget(id, std::move(pXRWindowOutputTarget));
  pWinMan->SetDestroyCallback(id, [this](WRegisteredWndHandle)
    { this->OnActorDestroyed(); });

  return id;
}

WGALTextureHandle WDummyXR::GetCurrentTexture()
{
  return m_hColorRT;
}

void WDummyXR::OnActorDestroyed()
{
  if (m_hView.IsInvalidated())
    return;

  m_pCompanion = nullptr;
  m_pCameraToSynchronize = nullptr;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  pDevice->DestroyTexture(m_hColorRT);
  pDevice->DestroyTexture(m_hDepthRT);

  WRenderWorld::RemoveMainView(m_hView);
  m_hView.Invalidate();
}

void WDummyXR::GALDeviceEventHandler(const WGALDeviceEvent& e)
{
  if (e.m_Type == WGALDeviceEvent::Type::BeforeBeginFrame)
  {
    if (m_pCompanion)
    {
      // We capture the companion view in unit tests so we don't want to skip any frames.
      m_pCompanion->CompanionViewBeginFrame(false);
    }
  }
  else if (e.m_Type == WGALDeviceEvent::Type::BeforeEndFrame)
  {
  }
}

void WDummyXR::GameApplicationEventHandler(const WGameApplicationExecutionEvent& e)
{
  if (e.m_Type == WGameApplicationExecutionEvent::Type::BeforePresent)
  {
  }
  else if (e.m_Type == WGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    WView* pView0 = nullptr;
    if (WRenderWorld::TryGetView(m_hView, pView0))
    {
      if (WWorld* pWorld0 = pView0->GetWorld())
      {
        W_LOCK(pWorld0->GetWriteMarker());
        WCameraComponentManager* pCameraComponentManager = pWorld0->GetComponentManager<WCameraComponentManager>();
        if (!pCameraComponentManager)
          return;

        WCameraComponent* pCameraComponent = pCameraComponentManager->GetCameraByUsageHint(WCameraUsageHint::MainView);
        if (!pCameraComponent)
          return;

        pCameraComponent->SetCameraMode(WCameraMode::Stereo);

        // Projection
        {
          const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.width / (float)m_Info.m_vEyeRenderTargetSize.height;

          WMat4 mProj = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(WAngle::MakeFromDegree(pCameraComponent->GetFieldOfView()), fAspectRatio,
            pCameraComponent->GetNearPlane(), WMath::Max(pCameraComponent->GetNearPlane() + 0.00001f, pCameraComponent->GetFarPlane()));

          m_pCameraToSynchronize->SetStereoProjection(mProj, mProj, fAspectRatio);
        }

        // Update camera view
        {
          WTransform add;
          add.SetIdentity();
          WView* pView = nullptr;
          if (WRenderWorld::TryGetView(m_hView, pView))
          {
            if (const WWorld* pWorld = pView->GetWorld())
            {
              W_LOCK(pWorld->GetReadMarker());
              if (const WStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<WStageSpaceComponentManager>())
              {
                if (const WStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
                {
                  WEnum<WXRStageSpace> stageSpace = pStage->GetStageSpace();
                  if (m_StageSpace != stageSpace)
                    m_StageSpace = pStage->GetStageSpace();
                  add = pStage->GetOwner()->GetGlobalTransform();
                }
              }
            }
          }

          {
            // Update device state
            WQuat rot;
            rot.SetIdentity();
            WVec3 pos = WVec3::MakeZero();
            if (m_StageSpace == WXRStageSpace::Standing)
            {
              pos.z = m_fHeadHeight;
            }

            m_Input.m_DeviceState[0].m_vGripPosition = pos;
            m_Input.m_DeviceState[0].m_qGripRotation = rot;
            m_Input.m_DeviceState[0].m_vAimPosition = pos;
            m_Input.m_DeviceState[0].m_qAimRotation = rot;
            m_Input.m_DeviceState[0].m_Type = WXRDeviceType::HMD;
            m_Input.m_DeviceState[0].m_bGripPoseIsValid = true;
            m_Input.m_DeviceState[0].m_bAimPoseIsValid = true;
            m_Input.m_DeviceState[0].m_bDeviceIsConnected = true;
          }

          // Set view matrix
          {
            const float fHeight = m_StageSpace == WXRStageSpace::Standing ? m_fHeadHeight : 0.0f;
            const WMat4 mStageTransform = add.GetInverse().GetAsMat4();
            WMat4 poseLeft = WMat4::MakeTranslation(WVec3(0, -m_fEyeOffset, fHeight));
            WMat4 poseRight = WMat4::MakeTranslation(WVec3(0, m_fEyeOffset, fHeight));

            // W Forward is +X, need to add this to align the forward projection
            const WMat4 viewMatrix = WGraphicsUtils::CreateLookAtViewMatrix(WVec3::MakeZero(), WVec3(1, 0, 0), WVec3(0, 0, 1));
            const WMat4 mViewTransformLeft = viewMatrix * mStageTransform * poseLeft.GetInverse();
            const WMat4 mViewTransformRight = viewMatrix * mStageTransform * poseRight.GetInverse();

            m_pCameraToSynchronize->SetViewMatrix(mViewTransformLeft, WCameraEye::Left);
            m_pCameraToSynchronize->SetViewMatrix(mViewTransformRight, WCameraEye::Right);
          }
        }
      }
    }
  }
}


//////////////////////////////////////////////////////////////////////////

void WDummyXRInput::GetDeviceList(WHybridArray<WXRDeviceID, 64>& out_devices) const
{
  out_devices.PushBack(0);
}

WXRDeviceID WDummyXRInput::GetDeviceIDByType(WXRDeviceType::Enum type) const
{
  WXRDeviceID deviceID = -1;
  switch (type)
  {
    case WXRDeviceType::HMD:
      deviceID = 0;
      break;
    default:
      deviceID = -1;
  }

  return deviceID;
}

const WXRDeviceState& WDummyXRInput::GetDeviceState(WXRDeviceID deviceID) const
{
  W_ASSERT_DEV(deviceID < 1 && deviceID >= 0, "Invalid device ID.");
  return m_DeviceState[deviceID];
}

WString WDummyXRInput::GetDeviceName(WXRDeviceID deviceID) const
{
  W_ASSERT_DEV(deviceID < 1 && deviceID >= 0, "Invalid device ID.");
  return "Dummy HMD";
}

WBitflags<WXRDeviceFeatures> WDummyXRInput::GetDeviceFeatures(WXRDeviceID deviceID) const
{
  W_ASSERT_DEV(deviceID < 1 && deviceID >= 0, "Invalid device ID.");
  return WXRDeviceFeatures::AimPose | WXRDeviceFeatures::GripPose;
}

void WDummyXRInput::InitializeDevice()
{
}

void WDummyXRInput::UpdateInputSlotValues()
{
}

void WDummyXRInput::RegisterInputSlots()
{
}
