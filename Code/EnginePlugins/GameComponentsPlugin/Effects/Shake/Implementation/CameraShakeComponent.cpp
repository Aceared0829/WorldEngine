#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeComponent.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeVolumeComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WCameraShakeComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MinShake", m_MinShake),
    W_MEMBER_PROPERTY("MaxShake", m_MaxShake)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(5))),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects/CameraShake"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCameraShakeComponent::WCameraShakeComponent() = default;
WCameraShakeComponent::~WCameraShakeComponent() = default;

void WCameraShakeComponent::Update()
{
  const WTime tDuration = WTime::MakeFromSeconds(1.0 / 30.0); // 30 Hz vibration seems to work well

  const WTime tNow = WTime::Now();

  if (tNow >= m_ReferenceTime + tDuration)
  {
    GetOwner()->SetLocalRotation(m_qNextTarget);
    GenerateKeyframe();
  }
  else
  {
    const float fLerp = WMath::Clamp((tNow - m_ReferenceTime).AsFloatInSeconds() / tDuration.AsFloatInSeconds(), 0.0f, 1.0f);

    WQuat q = WQuat::MakeSlerp(m_qPrevTarget, m_qNextTarget, fLerp);

    GetOwner()->SetLocalRotation(q);
  }
}

void WCameraShakeComponent::GenerateKeyframe()
{
  m_qPrevTarget = m_qNextTarget;

  m_ReferenceTime = WTime::Now();

  WWorld* pWorld = GetWorld();

  // fade out shaking over a second, if the vibration stopped
  m_fLastStrength -= pWorld->GetClock().GetTimeDiff().AsFloatInSeconds();

  const float fShake = WMath::Clamp(GetStrengthAtPosition(), 0.0f, 1.0f);

  m_fLastStrength = WMath::Max(m_fLastStrength, fShake);

  WAngle deviation;
  deviation = WMath::Lerp(m_MinShake, m_MaxShake, m_fLastStrength);

  if (deviation > WAngle())
  {
    m_Rotation += WAngle::MakeFromRadian(pWorld->GetRandomNumberGenerator().FloatMinMax(WAngle::MakeFromDegree(120).GetRadian(), WAngle::MakeFromDegree(240).GetRadian()));
    m_Rotation.NormalizeRange();

    WQuat qRot;
    qRot = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisX(), m_Rotation);

    const WVec3 tiltAxis = qRot * WVec3::MakeAxisZ();

    m_qNextTarget = WQuat::MakeFromAxisAndAngle(tiltAxis, deviation);
  }
  else
  {
    m_qNextTarget.SetIdentity();
  }
}

float WCameraShakeComponent::GetStrengthAtPosition() const
{
  float force = 0;

  if (auto pSpatial = GetWorld()->GetSpatialSystem())
  {
    const WVec3 vPosition = GetOwner()->GetGlobalPosition();

    WTempHybridArray<WGameObject*, 16> volumes;

    WSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = WCameraShakeVolumeComponent::SpatialDataCategory.GetBitmask();

    pSpatial->FindObjectsInSphere(WBoundingSphere::MakeFromCenterAndRadius(vPosition, 0.5f), queryParams, volumes);

    const WSimdVec4f pos = WSimdConversion::ToVec3(vPosition);

    for (WGameObject* pObj : volumes)
    {
      WCameraShakeVolumeComponent* pVol;
      if (pObj->TryGetComponentOfBaseType(pVol))
      {
        force = WMath::Max(force, pVol->ComputeForceAtGlobalPosition(pos));
      }
    }
  }

  return force;
}

void WCameraShakeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_MinShake;
  s << m_MaxShake;
}

void WCameraShakeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_MinShake;
  s >> m_MaxShake;
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Effects_Shake_Implementation_CameraShakeComponent);
