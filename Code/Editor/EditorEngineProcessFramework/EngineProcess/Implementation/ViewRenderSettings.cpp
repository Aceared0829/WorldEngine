#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/SkyLightComponent.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSceneViewPerspective, 1)
  W_ENUM_CONSTANTS(WSceneViewPerspective::Orthogonal_Front, WSceneViewPerspective::Orthogonal_Right, WSceneViewPerspective::Orthogonal_Top,
    WSceneViewPerspective::Perspective)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEngineViewLightSettings, 1, WRTTIDefaultAllocator<WEngineViewLightSettings>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("SkyBox", m_bSkyBox),
      W_MEMBER_PROPERTY("SkyLight", m_bSkyLight),
      W_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap),
      W_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity),
      W_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight),
      W_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle),
      W_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
      W_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity),
      W_MEMBER_PROPERTY("Fog", m_bFog)
    }
    W_END_PROPERTIES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WEngineViewConfig::ApplyPerspectiveSetting(float fFov, float fNearPlane, float fFarPlane)
{
  const float fOrthoRange = 1000.0f;

  switch (m_Perspective)
  {
    case WSceneViewPerspective::Perspective:
    {
      m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovY, fFov == 0.0f ? 70.0f : fFov, fNearPlane, fFarPlane);
    }
    break;

    case WSceneViewPerspective::Orthogonal_Front:
    {
      m_Camera.SetCameraMode(WCameraMode::OrthoFixedHeight, fFov == 0.0f ? 20.0f : fFov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + WVec3(-1, 0, 0), WVec3(0, 0, 1));
    }
    break;

    case WSceneViewPerspective::Orthogonal_Right:
    {
      m_Camera.SetCameraMode(WCameraMode::OrthoFixedHeight, fFov == 0.0f ? 20.0f : fFov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + WVec3(0, -1, 0), WVec3(0, 0, 1));
    }
    break;

    case WSceneViewPerspective::Orthogonal_Top:
    {
      m_Camera.SetCameraMode(WCameraMode::OrthoFixedHeight, fFov == 0.0f ? 20.0f : fFov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + WVec3(0, 0, -1), WVec3(1, 0, 0));
    }
    break;
  }
}

WEngineViewLightSettings::WEngineViewLightSettings(bool bEnable)
{
  if (!bEnable)
  {
    m_bSkyBox = false;
    m_bSkyLight = false;
    m_bDirectionalLight = false;
    m_bFog = false;
  }
}

WEngineViewLightSettings::~WEngineViewLightSettings()
{
  if (m_hGameObject.IsInvalidated())
    return;

  m_pWorld->DeleteObjectDelayed(m_hGameObject);
}

bool WEngineViewLightSettings::GetSkyBox() const
{
  return m_bSkyBox;
}

void WEngineViewLightSettings::SetSkyBox(bool bVal)
{
  m_bSkyBox = bVal;
  SetModifiedInternal(WEngineViewLightSettingsEvent::Type::SkyBoxChanged);
}

bool WEngineViewLightSettings::GetSkyLight() const
{
  return m_bSkyLight;
}

void WEngineViewLightSettings::SetSkyLight(bool bVal)
{
  m_bSkyLight = bVal;
  SetModifiedInternal(WEngineViewLightSettingsEvent::Type::SkyLightChanged);
}

const char* WEngineViewLightSettings::GetSkyLightCubeMap() const
{
  return m_sSkyLightCubeMap;
}

void WEngineViewLightSettings::SetSkyLightCubeMap(const char* szVal)
{
  m_sSkyLightCubeMap = szVal;
  SetModifiedInternal(WEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged);
}

float WEngineViewLightSettings::GetSkyLightIntensity() const
{
  return m_fSkyLightIntensity;
}

void WEngineViewLightSettings::SetSkyLightIntensity(float fVal)
{
  m_fSkyLightIntensity = fVal;
  SetModifiedInternal(WEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged);
}

bool WEngineViewLightSettings::GetDirectionalLight() const
{
  return m_bDirectionalLight;
}

void WEngineViewLightSettings::SetDirectionalLight(bool bVal)
{
  m_bDirectionalLight = bVal;
  SetModifiedInternal(WEngineViewLightSettingsEvent::Type::DirectionalLightChanged);
}

WAngle WEngineViewLightSettings::GetDirectionalLightAngle() const
{
  return m_DirectionalLightAngle;
}

void WEngineViewLightSettings::SetDirectionalLightAngle(WAngle val)
{
  m_DirectionalLightAngle = val;
  SetModifiedInternal(WEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged);
}

bool WEngineViewLightSettings::GetDirectionalLightShadows() const
{
  return m_bDirectionalLightShadows;
}

void WEngineViewLightSettings::SetDirectionalLightShadows(bool bVal)
{
  m_bDirectionalLightShadows = bVal;
  SetModifiedInternal(WEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
}

float WEngineViewLightSettings::GetDirectionalLightIntensity() const
{
  return m_fDirectionalLightIntensity;
}

void WEngineViewLightSettings::SetDirectionalLightIntensity(float fVal)
{
  m_fDirectionalLightIntensity = fVal;
  SetModifiedInternal(WEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
}

bool WEngineViewLightSettings::GetFog() const
{
  return m_bFog;
}

void WEngineViewLightSettings::SetFog(bool bVal)
{
  m_bFog = bVal;
  SetModifiedInternal(WEngineViewLightSettingsEvent::Type::FogChanged);
}

bool WEngineViewLightSettings::SetupForEngine(WWorld* pWorld, WUInt32 uiNextComponentPickingID)
{
  m_pWorld = pWorld;
  UpdateForEngine(pWorld);
  return false;
}

namespace
{
  template <typename T>
  T* SyncComponent(WWorld* pWorld, WGameObject* pParent, WComponentHandle& inout_hHandle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      T* pComp = nullptr;
      if (inout_hHandle.IsInvalidated() || !pWorld->TryGetComponent(inout_hHandle, pComp))
      {
        inout_hHandle = T::CreateComponent(pParent, pComp);
      }
      return pComp;
    }
    else
    {
      if (!inout_hHandle.IsInvalidated())
      {
        T* pComp = nullptr;
        if (pWorld->TryGetComponent(inout_hHandle, pComp))
        {
          pComp->DeleteComponent();
          inout_hHandle.Invalidate();
        }
      }
      return nullptr;
    }
  }

  WGameObject* SyncGameObject(WWorld* pWorld, WGameObjectHandle& inout_hHandle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      WGameObject* pObj = nullptr;
      if (inout_hHandle.IsInvalidated() || !pWorld->TryGetObject(inout_hHandle, pObj))
      {
        WGameObjectDesc obj;
        obj.m_sName.Assign("ViewLightSettings");
        inout_hHandle = pWorld->CreateObject(obj, pObj);
        pObj->MakeDynamic();
      }
      return pObj;
    }
    else
    {
      if (!inout_hHandle.IsInvalidated())
      {
        pWorld->DeleteObjectDelayed(inout_hHandle);
      }
      return nullptr;
    }
  }
} // namespace

void WEngineViewLightSettings::UpdateForEngine(WWorld* pWorld)
{
  if (WGameObject* pParent = SyncGameObject(m_pWorld, m_hSkyBoxObject, m_bSkyBox))
  {
    pParent->SetTag(WTagRegistry::GetGlobalRegistry().RegisterTag("SkyLight"));

    if (WSkyBoxComponent* pSkyBox = SyncComponent<WSkyBoxComponent>(m_pWorld, pParent, m_hSkyBox, m_bSkyBox))
    {
      pSkyBox->SetCubeMapFile(m_sSkyLightCubeMap);
    }
  }

  const bool bNeedGameObject = m_bDirectionalLight || m_bSkyLight;
  if (WGameObject* pParent = SyncGameObject(m_pWorld, m_hGameObject, bNeedGameObject))
  {
    WQuat rotY = WQuat::MakeFromAxisAndAngle(WVec3(0.0f, 1.0f, 0.0f), WAngle::MakeFromDegree(120.0));
    WQuat rotZ = WQuat::MakeFromAxisAndAngle(WVec3(0.0f, 0.0f, 1.0f), m_DirectionalLightAngle);
    pParent->SetLocalRotation(rotZ * rotY);

    if (WDirectionalLightComponent* pDirLight = SyncComponent<WDirectionalLightComponent>(m_pWorld, pParent, m_hDirLight, m_bDirectionalLight))
    {
      pDirLight->SetCastShadows(m_bDirectionalLightShadows);
      pDirLight->SetIntensity(m_fDirectionalLightIntensity);
    }

    if (WSkyLightComponent* pSkyLight = SyncComponent<WSkyLightComponent>(m_pWorld, pParent, m_hSkyLight, m_bSkyLight))
    {
      pSkyLight->SetDiffuseIntensity(m_fSkyLightIntensity);
      pSkyLight->SetSpecularIntensity(m_fSkyLightIntensity);
      pSkyLight->SetReflectionProbeMode(WReflectionProbeMode::Static);
      pSkyLight->SetCubeMapFile(m_sSkyLightCubeMap);
    }

    if (WFogComponent* pFog = SyncComponent<WFogComponent>(m_pWorld, pParent, m_hFog, m_bFog))
    {
      // pFog->SetColor(WColor(0.1f, 0.1f, 0.1f));
      pFog->SetDensity(5.0f);
      pFog->SetHeightFalloff(0);
      pFog->SetModulateWithSkyColor(m_bSkyBox);
      pFog->SetSkyDistance(100.0f);
    }
  }
}

void WEngineViewLightSettings::SetModifiedInternal(WEngineViewLightSettingsEvent::Type type)
{
  SetModified();
  WEngineViewLightSettingsEvent e;
  e.m_Type = type;
  m_EngineViewLightSettingsEvents.Broadcast(e);
}
