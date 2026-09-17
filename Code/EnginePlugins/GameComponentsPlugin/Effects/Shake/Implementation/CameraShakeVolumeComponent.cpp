#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeVolumeComponent.h>

WSpatialData::Category WCameraShakeVolumeComponent::SpatialDataCategory = WSpatialData::RegisterCategory("CameraShakeVolumes", WSpatialData::Flags::None);

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WCameraShakeVolumeComponent, 1)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Strength", m_fStrength),
    W_MEMBER_PROPERTY("BurstDuration", m_BurstDuration),
    W_ENUM_MEMBER_PROPERTY("OnFinishedAction", WOnComponentFinishedAction, m_OnFinishedAction),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnTriggered),
    W_MESSAGE_HANDLER(WMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects/CameraShake"),
  }
  W_END_ATTRIBUTES;
}
W_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

WCameraShakeVolumeComponent::WCameraShakeVolumeComponent() = default;
WCameraShakeVolumeComponent::~WCameraShakeVolumeComponent() = default;

void WCameraShakeVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void WCameraShakeVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void WCameraShakeVolumeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_BurstDuration.IsPositive())
  {
    WMsgComponentInternalTrigger msg;
    msg.m_sMessage.Assign("Suicide");

    PostMessage(msg, m_BurstDuration);
  }
}

void WCameraShakeVolumeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_BurstDuration;
  s << m_OnFinishedAction;
  s << m_fStrength;
}

void WCameraShakeVolumeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_BurstDuration;
  s >> m_OnFinishedAction;
  s >> m_fStrength;
}

float WCameraShakeVolumeComponent::ComputeForceAtGlobalPosition(const WSimdVec4f& vGlobalPos) const
{
  const WSimdTransform t = GetOwner()->GetGlobalTransformSimd();
  const WSimdTransform tInv = t.GetInverse();
  const WSimdVec4f localPos = tInv.TransformPosition(vGlobalPos);

  return ComputeForceAtLocalPosition(localPos);
}

void WCameraShakeVolumeComponent::OnTriggered(WMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != WTempHashedString("Suicide"))
    return;

  WOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);

  SetActiveFlag(false);
}

void WCameraShakeVolumeComponent::OnMsgDeleteGameObject(WMsgDeleteGameObject& msg)
{
  if (m_BurstDuration.IsPositive())
  {
    WOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WCameraShakeVolumeSphereComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WSphereVisualizerAttribute("Radius", WColor::SaddleBrown),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WCameraShakeVolumeSphereComponent::WCameraShakeVolumeSphereComponent() = default;
WCameraShakeVolumeSphereComponent::~WCameraShakeVolumeSphereComponent() = default;

void WCameraShakeVolumeSphereComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void WCameraShakeVolumeSphereComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;
}

float WCameraShakeVolumeSphereComponent::ComputeForceAtLocalPosition(const WSimdVec4f& vLocalPos) const
{
  WSimdFloat lenScaled = vLocalPos.GetLength<3>() * m_fOneDivRadius;

  // inverse quadratic falloff to have sharper edges
  WSimdFloat forceFactor = WSimdFloat(1.0f) - lenScaled;

  const WSimdFloat force = forceFactor.Max(0.0f);

  return m_fStrength * force;
}

void WCameraShakeVolumeSphereComponent::SetRadius(float fVal)
{
  m_fRadius = WMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WCameraShakeVolumeSphereComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fRadius), WCameraShakeVolumeComponent::SpatialDataCategory);
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Effects_Shake_Implementation_CameraShakeVolumeComponent);
