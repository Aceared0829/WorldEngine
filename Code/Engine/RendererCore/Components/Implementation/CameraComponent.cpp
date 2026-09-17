#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/BlackboardComponent.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>


WCameraComponentManager::WCameraComponentManager(WWorld* pWorld)
  : WComponentManager<WCameraComponent, WBlockStorageType::Compact>(pWorld)
{
  WRenderWorld::s_CameraConfigsModifiedEvent.AddEventHandler(WMakeDelegate(&WCameraComponentManager::OnCameraConfigsChanged, this));
}

WCameraComponentManager::~WCameraComponentManager()
{
  WRenderWorld::s_CameraConfigsModifiedEvent.RemoveEventHandler(WMakeDelegate(&WCameraComponentManager::OnCameraConfigsChanged, this));
}

void WCameraComponentManager::Initialize()
{
  auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WCameraComponentManager::Update, this);
  desc.m_Phase = WWorldUpdatePhase::PostTransform;

  this->RegisterUpdateFunction(desc);

  WRenderWorld::s_ViewCreatedEvent.AddEventHandler(WMakeDelegate(&WCameraComponentManager::OnViewCreated, this));
}

void WCameraComponentManager::Deinitialize()
{
  WRenderWorld::s_ViewCreatedEvent.RemoveEventHandler(WMakeDelegate(&WCameraComponentManager::OnViewCreated, this));

  SUPER::Deinitialize();
}

void WCameraComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  for (auto hCameraComponent : m_ModifiedCameras)
  {
    WCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    if (WView* pView = WRenderWorld::GetViewByUsageHint(pCameraComponent->GetUsageHint(), WCameraUsageHint::None, GetWorld()))
    {
      pCameraComponent->ApplySettingsToView(pView);
    }

    pCameraComponent->m_bIsModified = false;
  }

  m_ModifiedCameras.Clear();

  for (auto hCameraComponent : m_RenderTargetCameras)
  {
    WCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    pCameraComponent->UpdateRenderTargetCamera();
  }

  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->m_bShowStats && it->GetUsageHint() == WCameraUsageHint::MainView)
    {
      if (WView* pView = WRenderWorld::GetViewByUsageHint(WCameraUsageHint::MainView, WCameraUsageHint::EditorView, GetWorld()))
      {
        it->ShowStats(pView);
      }
    }
  }
}

void WCameraComponentManager::ReinitializeAllRenderTargetCameras()
{
  W_LOCK(GetWorld()->GetWriteMarker());

  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->DeactivateRenderToTexture();
      it->ActivateRenderToTexture();
    }
  }
}

const WCameraComponent* WCameraComponentManager::GetCameraByUsageHint(WCameraUsageHint::Enum usageHint) const
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

WCameraComponent* WCameraComponentManager::GetCameraByUsageHint(WCameraUsageHint::Enum usageHint)
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

void WCameraComponentManager::AddRenderTargetCamera(WCameraComponent* pComponent)
{
  m_RenderTargetCameras.PushBack(pComponent->GetHandle());
}

void WCameraComponentManager::RemoveRenderTargetCamera(WCameraComponent* pComponent)
{
  m_RenderTargetCameras.RemoveAndSwap(pComponent->GetHandle());
}

void WCameraComponentManager::OnViewCreated(WView* pView)
{
  // Mark all cameras as modified so the new view gets the proper settings
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    it->MarkAsModified(this);
  }
}

void WCameraComponentManager::OnCameraConfigsChanged(void* dummy)
{
  ReinitializeAllRenderTargetCameras();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WCameraComponent, 11, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("EditorShortcut", m_iEditorShortcut)->AddAttributes(new WDefaultValueAttribute(-1), new WClampValueAttribute(-1, 9)),
    W_ENUM_ACCESSOR_PROPERTY("UsageHint", WCameraUsageHint, GetUsageHint, SetUsageHint),
    W_ENUM_ACCESSOR_PROPERTY("Mode", WCameraMode, GetCameraMode, SetCameraMode),
    W_ACCESSOR_PROPERTY("RenderTarget", GetRenderTargetFile, SetRenderTargetFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_Target", WDependencyFlags::Package)),
    W_ACCESSOR_PROPERTY("RenderTargetOffset", GetRenderTargetRectOffset, SetRenderTargetRectOffset)->AddAttributes(new WClampValueAttribute(WVec2(0.0f), WVec2(0.9f))),
    W_ACCESSOR_PROPERTY("RenderTargetSize", GetRenderTargetRectSize, SetRenderTargetRectSize)->AddAttributes(new WDefaultValueAttribute(WVec2(1.0f)), new WClampValueAttribute(WVec2(0.1f), WVec2(1.0f))),
    W_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new WDefaultValueAttribute(0.25f), new WClampValueAttribute(0.01f, 4.0f)),
    W_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new WDefaultValueAttribute(1000.0f), new WClampValueAttribute(5.0, 10000.0f)),
    W_ACCESSOR_PROPERTY("FOV", GetFieldOfView, SetFieldOfView)->AddAttributes(new WDefaultValueAttribute(60.0f), new WClampValueAttribute(1.0f, 170.0f)),
    W_ACCESSOR_PROPERTY("Dimensions", GetOrthoDimension, SetOrthoDimension)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.01f, 10000.0f)),
    W_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new WTagSetWidgetAttribute("Default")),
    W_SET_MEMBER_PROPERTY("ExcludeTags", m_ExcludeTags)->AddAttributes(new WTagSetWidgetAttribute("Default")),
    W_ACCESSOR_PROPERTY("CameraRenderPipeline", GetRenderPipelineEnum, SetRenderPipelineEnum)->AddAttributes(new WDynamicStringEnumAttribute("CameraPipelines")),
    W_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName),
    W_ACCESSOR_PROPERTY("Aperture", GetAperture, SetAperture)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(1.0f, 32.0f), new WSuffixAttribute(" f-stop(s)")),
    W_ACCESSOR_PROPERTY("ShutterTime", GetShutterTime, SetShutterTime)->AddAttributes(new WDefaultValueAttribute(WTime::MakeFromSeconds(1.0)), new WClampValueAttribute(WTime::MakeFromSeconds(1.0f / 100000.0f), WTime::MakeFromSeconds(600.0f))),
    W_ACCESSOR_PROPERTY("ISO", GetISO, SetISO)->AddAttributes(new WDefaultValueAttribute(100.0f), new WClampValueAttribute(50.0f, 64000.0f)),
    W_ACCESSOR_PROPERTY("ExposureCompensation", GetExposureCompensation, SetExposureCompensation)->AddAttributes(new WClampValueAttribute(-32.0f, 32.0f)),
    W_MEMBER_PROPERTY("ShowStats", m_bShowStats),
    //W_ACCESSOR_PROPERTY_READ_ONLY("EV100", GetEV100),
    //W_ACCESSOR_PROPERTY_READ_ONLY("FinalExposure", GetExposure),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering"),
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 1.0f, WColor::DarkSlateBlue),
    new WCameraVisualizerAttribute("Mode", "FOV", "Dimensions", "NearPlane", "FarPlane"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCameraComponent::WCameraComponent() = default;
WCameraComponent::~WCameraComponent() = default;

void WCameraComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_UsageHint.GetValue();
  s << m_Mode.GetValue();
  s << m_fNearPlane;
  s << m_fFarPlane;
  s << m_fPerspectiveFieldOfView;
  s << m_fOrthoDimension;

  // Version 2 till 7
  // s << m_hRenderPipeline;

  // Version 3
  s << m_fAperture;
  s << static_cast<float>(m_ShutterTime.GetSeconds());
  s << m_fISO;
  s << m_fExposureCompensation;

  // Version 4
  m_IncludeTags.Save(s);
  m_ExcludeTags.Save(s);

  // Version 6
  s << m_hRenderTarget;

  // Version 7
  s << m_vRenderTargetRectOffset;
  s << m_vRenderTargetRectSize;

  // Version 8
  s << m_sRenderPipeline;

  // Version 10
  s << m_bShowStats;

  // Version 11
  s << m_sBlackboardName;
}

void WCameraComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  WCameraUsageHint::StorageType usage;
  s >> usage;
  if (uiVersion == 1 && usage > WCameraUsageHint::MainView)
    usage = WCameraUsageHint::None;
  m_UsageHint.SetValue(usage);

  WCameraMode::StorageType cam;
  s >> cam;
  m_Mode.SetValue(cam);

  s >> m_fNearPlane;
  s >> m_fFarPlane;
  s >> m_fPerspectiveFieldOfView;
  s >> m_fOrthoDimension;

  if (uiVersion >= 2 && uiVersion <= 7)
  {
    WRenderPipelineResourceHandle m_hRenderPipeline;
    s >> m_hRenderPipeline;
  }

  if (uiVersion >= 3)
  {
    s >> m_fAperture;
    float shutterTime;
    s >> shutterTime;
    m_ShutterTime = WTime::MakeFromSeconds(shutterTime);
    s >> m_fISO;
    s >> m_fExposureCompensation;
  }

  if (uiVersion >= 4)
  {
    m_IncludeTags.Load(s, WTagRegistry::GetGlobalRegistry());
    m_ExcludeTags.Load(s, WTagRegistry::GetGlobalRegistry());
  }

  if (uiVersion >= 6)
  {
    s >> m_hRenderTarget;
  }

  if (uiVersion >= 7)
  {
    s >> m_vRenderTargetRectOffset;
    s >> m_vRenderTargetRectSize;
  }

  if (uiVersion >= 8)
  {
    s >> m_sRenderPipeline;
  }

  if (uiVersion >= 10)
  {
    s >> m_bShowStats;
  }

  if (uiVersion >= 11)
  {
    s >> m_sBlackboardName;
  }

  MarkAsModified();
}

void WCameraComponent::UpdateRenderTargetCamera()
{
  if (!m_bRenderTargetInitialized)
    return;

  // recreate everything, if the view got invalidated in between
  if (m_hRenderTargetView.IsInvalidated())
  {
    DeactivateRenderToTexture();
    ActivateRenderToTexture();
  }

  WView* pView = nullptr;
  if (!WRenderWorld::TryGetView(m_hRenderTargetView, pView))
    return;

  ApplySettingsToView(pView);

  if (m_Mode == WCameraMode::PerspectiveFixedFovX || m_Mode == WCameraMode::PerspectiveFixedFovY)
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fPerspectiveFieldOfView, m_fNearPlane, m_fFarPlane);
  else
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fOrthoDimension, m_fNearPlane, m_fFarPlane);

  m_RenderTargetCamera.LookAt(
    GetOwner()->GetGlobalPosition(), GetOwner()->GetGlobalPosition() + GetOwner()->GetGlobalDirForwards(), GetOwner()->GetGlobalDirUp());
}

void WCameraComponent::ShowStats(WView* pView)
{
  if (!m_bShowStats)
    return;

  // draw stats
  {
    const WStringView sName = GetOwner()->GetName();

    WStringBuilder sb;
    sb.SetFormat("Camera '{0}':\nEV100: {1}, Exposure: {2}", sName.IsEmpty() ? pView->GetName() : sName, GetEV100(), GetExposure());
    WDebugRenderer::DrawInfoText(pView->GetHandle(), WDebugTextPlacement::TopLeft, "CamStats", sb, WColor::White);
  }

  // draw frustum
  {
    const WGameObject* pOwner = GetOwner();
    WVec3 vPosition = pOwner->GetGlobalPosition();
    WVec3 vForward = pOwner->GetGlobalDirForwards();
    WVec3 vUp = pOwner->GetGlobalDirUp();

    const WMat4 viewMatrix = WGraphicsUtils::CreateLookAtViewMatrix(vPosition, vPosition + vForward, vUp);

    WMat4 projectionMatrix = pView->GetProjectionMatrix(WCameraEye::Left); // todo: Stereo support
    WMat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

    WFrustum frustum = WFrustum::MakeFromMVP(viewProjectionMatrix);

    // TODO: limit far plane to 10 meters

    WDebugRenderer::DrawLineFrustum(GetWorld(), frustum, WColor::LimeGreen);
  }
}

void WCameraComponent::SetUsageHint(WEnum<WCameraUsageHint> val)
{
  if (val == m_UsageHint)
    return;

  DeactivateRenderToTexture();

  m_UsageHint = val;

  ActivateRenderToTexture();

  MarkAsModified();
}

void WCameraComponent::SetRenderTargetFile(WStringView sFile)
{
  DeactivateRenderToTexture();

  if (!sFile.IsEmpty())
  {
    m_hRenderTarget = WResourceManager::LoadResource<WRenderToTexture2DResource>(sFile);
  }
  else
  {
    m_hRenderTarget.Invalidate();
  }

  ActivateRenderToTexture();

  MarkAsModified();
}

WStringView WCameraComponent::GetRenderTargetFile() const
{
  return m_hRenderTarget.GetResourceID();
}

void WCameraComponent::SetRenderTargetRectOffset(WVec2 value)
{
  DeactivateRenderToTexture();

  m_vRenderTargetRectOffset.x = WMath::Clamp(value.x, 0.0f, 0.9f);
  m_vRenderTargetRectOffset.y = WMath::Clamp(value.y, 0.0f, 0.9f);

  ActivateRenderToTexture();
}

void WCameraComponent::SetRenderTargetRectSize(WVec2 value)
{
  DeactivateRenderToTexture();

  m_vRenderTargetRectSize.x = WMath::Clamp(value.x, 0.1f, 1.0f);
  m_vRenderTargetRectSize.y = WMath::Clamp(value.y, 0.1f, 1.0f);

  ActivateRenderToTexture();
}

void WCameraComponent::SetCameraMode(WEnum<WCameraMode> val)
{
  if (val == m_Mode)
    return;
  m_Mode = val;

  MarkAsModified();
}


void WCameraComponent::SetNearPlane(float fVal)
{
  if (fVal == m_fNearPlane)
    return;
  m_fNearPlane = fVal;

  MarkAsModified();
}


void WCameraComponent::SetFarPlane(float fVal)
{
  if (fVal == m_fFarPlane)
    return;
  m_fFarPlane = fVal;

  MarkAsModified();
}


void WCameraComponent::SetFieldOfView(float fVal)
{
  if (fVal == m_fPerspectiveFieldOfView)
    return;
  m_fPerspectiveFieldOfView = fVal;

  MarkAsModified();
}


void WCameraComponent::SetOrthoDimension(float fVal)
{
  if (fVal == m_fOrthoDimension)
    return;
  m_fOrthoDimension = fVal;

  MarkAsModified();
}

WRenderPipelineResourceHandle WCameraComponent::GetRenderPipeline() const
{
  return m_hCachedRenderPipeline;
}

WSharedPtr<WBlackboard> WCameraComponent::GetBlackboard() const
{
  return m_pBlackboard;
}

WViewHandle WCameraComponent::GetRenderTargetView() const
{
  return m_hRenderTargetView;
}

const char* WCameraComponent::GetRenderPipelineEnum() const
{
  return m_sRenderPipeline.GetData();
}

void WCameraComponent::SetBlackboardName(const char* szName)
{
  if (m_sBlackboardName == szName)
    return;

  m_sBlackboardName.Assign(szName);

  if (IsActiveAndInitialized())
  {
    m_pBlackboard = WBlackboardComponent::FindBlackboard(*GetOwner(), m_sBlackboardName);
  }

  MarkAsModified();
}

void WCameraComponent::SetRenderPipelineEnum(const char* szFile)
{
  DeactivateRenderToTexture();

  m_sRenderPipeline.Assign(szFile);

  ActivateRenderToTexture();

  MarkAsModified();
}

void WCameraComponent::SetAperture(float fAperture)
{
  if (m_fAperture == fAperture)
    return;
  m_fAperture = fAperture;

  MarkAsModified();
}

void WCameraComponent::SetShutterTime(WTime shutterTime)
{
  if (m_ShutterTime == shutterTime)
    return;
  m_ShutterTime = shutterTime;

  MarkAsModified();
}

void WCameraComponent::SetISO(float fISO)
{
  if (m_fISO == fISO)
    return;
  m_fISO = fISO;

  MarkAsModified();
}

void WCameraComponent::SetExposureCompensation(float fEC)
{
  if (m_fExposureCompensation == fEC)
    return;
  m_fExposureCompensation = fEC;

  MarkAsModified();
}

float WCameraComponent::GetEV100() const
{
  // From: course_notes_moving_frostbite_to_pbr.pdf
  // EV number is defined as:
  // 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
  // This gives
  // EV_s = log2 (N^2 / t)
  // EV_100 + log2 (S /100) = log2 (N^2 / t)
  // EV_100 = log2 (N^2 / t) - log2 (S /100)
  // EV_100 = log2 (N^2 / t . 100 / S)
  return WMath::Log2((m_fAperture * m_fAperture) / m_ShutterTime.AsFloatInSeconds() * 100.0f / m_fISO) - m_fExposureCompensation;
}

float WCameraComponent::GetExposure() const
{
  // Compute the maximum luminance possible with H_sbs sensitivity
  // maxLum = 78 / ( S * q ) * N^2 / t
  // = 78 / ( S * q ) * 2^ EV_100
  // = 78 / (100 * 0.65) * 2^ EV_100
  // = 1.2 * 2^ EV
  // Reference : http://en.wikipedia.org/wiki/Film_speed
  float maxLuminance = 1.2f * WMath::Pow2(GetEV100());
  return 1.0f / maxLuminance;
}

void WCameraComponent::ApplySettingsToView(WView* pView) const
{
  if (m_UsageHint == WCameraUsageHint::None)
    return;

  float fFovOrDim = m_fPerspectiveFieldOfView;
  if (m_Mode == WCameraMode::OrthoFixedWidth || m_Mode == WCameraMode::OrthoFixedHeight)
  {
    fFovOrDim = m_fOrthoDimension;
  }

  WCamera* pCamera = pView->GetCamera();
  pCamera->SetCameraMode(m_Mode, fFovOrDim, m_fNearPlane, WMath::Max(m_fNearPlane + 0.00001f, m_fFarPlane));
  pCamera->SetExposure(GetExposure());

  pView->m_IncludeTags = m_IncludeTags;
  pView->m_ExcludeTags = m_ExcludeTags;

  const WTag& tagEditor = WTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  pView->m_ExcludeTags.Set(tagEditor);

  if (m_hCachedRenderPipeline.IsValid())
  {
    pView->SetRenderPipelineResource(m_hCachedRenderPipeline);
  }

  pView->SetBlackboard(m_pBlackboard);
}

void WCameraComponent::ResourceChangeEventHandler(const WResourceEvent& e)
{
  switch (e.m_Type)
  {
    case WResourceEvent::Type::ResourceExists:
    case WResourceEvent::Type::ResourceCreated:
      return;

    case WResourceEvent::Type::ResourceDeleted:
    case WResourceEvent::Type::ResourceContentUnloading:
    case WResourceEvent::Type::ResourceContentUpdated:
      // triggers a recreation of the view
      WRenderWorld::DeleteView(m_hRenderTargetView);
      m_hRenderTargetView.Invalidate();
      break;

    default:
      break;
  }
}

void WCameraComponent::MarkAsModified()
{
  if (!m_bIsModified)
  {
    GetWorld()->GetComponentManager<WCameraComponentManager>()->m_ModifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}


void WCameraComponent::MarkAsModified(WCameraComponentManager* pCameraManager)
{
  if (!m_bIsModified)
  {
    pCameraManager->m_ModifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}

void WCameraComponent::ActivateRenderToTexture()
{
  if (m_UsageHint != WCameraUsageHint::RenderTarget)
    return;

  if (m_bRenderTargetInitialized || !m_hRenderTarget.IsValid() || m_sRenderPipeline.IsEmpty() || !IsActiveAndInitialized())
    return;

  WResourceLock<WRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, WResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pRenderTarget.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    return;
  }

  // query the render pipeline to use
  if (const auto* pConfig = WRenderWorld::FindCameraConfig(m_sRenderPipeline))
  {
    m_hCachedRenderPipeline = pConfig->m_hRenderPipeline;
  }

  if (!m_hCachedRenderPipeline.IsValid())
    return;

  m_bRenderTargetInitialized = true;

  W_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  WStringBuilder name;
  name.SetFormat("Camera RT: {0}", GetOwner()->GetName());

  WView* pRenderTargetView = nullptr;
  m_hRenderTargetView = WRenderWorld::CreateView(name, pRenderTargetView);

  pRenderTargetView->SetRenderPipelineResource(m_hCachedRenderPipeline);

  pRenderTargetView->SetWorld(GetWorld());
  pRenderTargetView->SetCamera(&m_RenderTargetCamera);

  pRenderTarget->m_ResourceEvents.AddEventHandler(WMakeDelegate(&WCameraComponent::ResourceChangeEventHandler, this));

  WGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0] = pRenderTarget->GetGALTexture();
  pRenderTargetView->SetRenderTargets(renderTargets);

  const float maxSizeX = 1.0f - m_vRenderTargetRectOffset.x;
  const float maxSizeY = 1.0f - m_vRenderTargetRectOffset.y;

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  const float width = resX * WMath::Min(maxSizeX, m_vRenderTargetRectSize.x);
  const float height = resY * WMath::Min(maxSizeY, m_vRenderTargetRectSize.y);

  const float offsetX = m_vRenderTargetRectOffset.x * resX;
  const float offsetY = m_vRenderTargetRectOffset.y * resY;

  pRenderTargetView->SetViewport(WRectFloat(offsetX, offsetY, width, height));

  pRenderTarget->AddRenderView(m_hRenderTargetView);

  GetWorld()->GetComponentManager<WCameraComponentManager>()->AddRenderTargetCamera(this);
}

void WCameraComponent::DeactivateRenderToTexture()
{
  if (!m_bRenderTargetInitialized)
    return;

  m_bRenderTargetInitialized = false;
  m_hCachedRenderPipeline.Invalidate();

  W_ASSERT_DEBUG(m_hRenderTarget.IsValid(), "Render Target should be valid");

  if (m_hRenderTarget.IsValid())
  {
    WResourceLock<WRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, WResourceAcquireMode::BlockTillLoaded);
    pRenderTarget->RemoveRenderView(m_hRenderTargetView);

    pRenderTarget->m_ResourceEvents.RemoveEventHandler(WMakeDelegate(&WCameraComponent::ResourceChangeEventHandler, this));
  }

  if (!m_hRenderTargetView.IsInvalidated())
  {
    WRenderWorld::DeleteView(m_hRenderTargetView);
    m_hRenderTargetView.Invalidate();
  }

  GetWorld()->GetComponentManager<WCameraComponentManager>()->RemoveRenderTargetCamera(this);
}

void WCameraComponent::OnActivated()
{
  SUPER::OnActivated();

  ActivateRenderToTexture();

  m_pBlackboard = WBlackboardComponent::FindBlackboard(*GetOwner(), m_sBlackboardName);
}

void WCameraComponent::OnDeactivated()
{
  DeactivateRenderToTexture();

  SUPER::OnDeactivated();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WCameraComponentPatch_4_5 : public WGraphPatch
{
public:
  WCameraComponentPatch_4_5()
    : WGraphPatch("WCameraComponent", 5)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Usage Hint", "UsageHint");
    pNode->RenameProperty("Near Plane", "NearPlane");
    pNode->RenameProperty("Far Plane", "FarPlane");
    pNode->RenameProperty("Include Tags", "IncludeTags");
    pNode->RenameProperty("Exclude Tags", "ExcludeTags");
    pNode->RenameProperty("Render Pipeline", "RenderPipeline");
    pNode->RenameProperty("Shutter Time", "ShutterTime");
    pNode->RenameProperty("Exposure Compensation", "ExposureCompensation");
  }
};

WCameraComponentPatch_4_5 g_WCameraComponentPatch_4_5;

//////////////////////////////////////////////////////////////////////////

class WCameraComponentPatch_8_9 : public WGraphPatch
{
public:
  WCameraComponentPatch_8_9()
    : WGraphPatch("WCameraComponent", 9)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // convert the "ShutterTime" property from float to WTime
    if (auto pProp = pNode->FindProperty("ShutterTime"))
    {
      if (pProp->m_Value.IsA<float>())
      {
        const float shutterTime = pProp->m_Value.Get<float>();
        pProp->m_Value = WTime::MakeFromSeconds(shutterTime);
      }
    }
  }
};

WCameraComponentPatch_8_9 g_WCameraComponentPatch_8_9;


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_CameraComponent);
