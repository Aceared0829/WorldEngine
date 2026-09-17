#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/ResetTransformComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WResetTransformComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ResetPositionX", m_bResetLocalPositionX)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("ResetPositionY", m_bResetLocalPositionY)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("ResetPositionZ", m_bResetLocalPositionZ)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    W_MEMBER_PROPERTY("ResetRotation", m_bResetLocalRotation)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation)->AddAttributes(new WDefaultValueAttribute(WQuat::MakeIdentity())),
    W_MEMBER_PROPERTY("ResetScaling", m_bResetLocalScaling)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("LocalScaling", m_vLocalScaling)->AddAttributes(new WDefaultValueAttribute(WVec3(1))),
    W_MEMBER_PROPERTY("LocalUniformScaling", m_fLocalUniformScaling)->AddAttributes(new WDefaultValueAttribute(1)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResetTransformComponent::WResetTransformComponent() = default;
WResetTransformComponent::~WResetTransformComponent() = default;

void WResetTransformComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  WVec3 vLocalPos = GetOwner()->GetLocalPosition();

  if (m_bResetLocalPositionX)
    vLocalPos.x = m_vLocalPosition.x;
  if (m_bResetLocalPositionY)
    vLocalPos.y = m_vLocalPosition.y;
  if (m_bResetLocalPositionZ)
    vLocalPos.z = m_vLocalPosition.z;

  GetOwner()->SetLocalPosition(vLocalPos);

  if (m_bResetLocalRotation)
  {
    GetOwner()->SetLocalRotation(m_qLocalRotation);
  }

  if (m_bResetLocalScaling)
  {
    GetOwner()->SetLocalScaling(m_vLocalScaling);
    GetOwner()->SetLocalUniformScaling(m_fLocalUniformScaling);
  }

  // update the global transform right away
  GetOwner()->UpdateGlobalTransform();
}

void WResetTransformComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_vLocalPosition;
  s << m_qLocalRotation;
  s << m_vLocalScaling;
  s << m_bResetLocalPositionX;
  s << m_bResetLocalPositionY;
  s << m_bResetLocalPositionZ;
  s << m_bResetLocalRotation;
  s << m_bResetLocalScaling;
  s << m_fLocalUniformScaling;
}

void WResetTransformComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_vLocalPosition;
  s >> m_qLocalRotation;
  s >> m_vLocalScaling;
  s >> m_bResetLocalPositionX;
  s >> m_bResetLocalPositionY;
  s >> m_bResetLocalPositionZ;
  s >> m_bResetLocalRotation;
  s >> m_bResetLocalScaling;
  s >> m_fLocalUniformScaling;
}


W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_ResetTransformComponent);
