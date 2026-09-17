#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/SpatialData.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <ParticlePlugin/Components/ParticleAttractorComponent.h>

static WSpatialData::Category s_AttractorSpatialCategory = WSpatialData::RegisterCategory("ParticleAttractor", WSpatialData::Flags::None);

// clang-format off
W_BEGIN_COMPONENT_TYPE(WParticleAttractorComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.01f, {})),
    W_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new WDefaultValueAttribute(5.0f)),
    W_MEMBER_PROPERTY("MinDistance", m_fMinDistance)->AddAttributes(new WDefaultValueAttribute(0.1f), new WClampValueAttribute(0.001f, {})),
    W_MEMBER_PROPERTY("KillDistance", m_fKillDistance)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, {})),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnMsgUpdateLocalBounds)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects"),
    new WSphereManipulatorAttribute("Radius"),
    new WSphereManipulatorAttribute("KillDistance"),
    new WSphereVisualizerAttribute("Radius", WColor::CornflowerBlue),
    new WSphereVisualizerAttribute("KillDistance", WColor::OrangeRed),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WParticleAttractorComponent::WParticleAttractorComponent() = default;
WParticleAttractorComponent::~WParticleAttractorComponent() = default;

// static
WSpatialData::Category WParticleAttractorComponent::GetSpatialCategory()
{
  return s_AttractorSpatialCategory;
}

void WParticleAttractorComponent::OnMsgUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fRadius), s_AttractorSpatialCategory);
}

void WParticleAttractorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fStrength;
  s << m_fMinDistance;
  s << m_fKillDistance;
}

void WParticleAttractorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fStrength;
  s >> m_fMinDistance;
  s >> m_fKillDistance;
}

void WParticleAttractorComponent::OnActivated()
{
  SUPER::OnActivated();
  GetOwner()->UpdateLocalBounds();
}

void WParticleAttractorComponent::OnDeactivated()
{
  SUPER::OnDeactivated();
  GetOwner()->UpdateLocalBounds();
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleAttractorComponent);
