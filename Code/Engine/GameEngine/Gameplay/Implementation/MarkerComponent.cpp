#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/MarkerComponent.h>
#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WMarkerComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Marker", GetMarkerType, SetMarkerType)->AddAttributes(new WDynamicStringEnumAttribute("SpatialDataCategoryEnum")),
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(0.1)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnMsgUpdateLocalBounds)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
    new WSphereVisualizerAttribute("Radius", WColor::LightSkyBlue),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMarkerComponent::WMarkerComponent() = default;
WMarkerComponent::~WMarkerComponent() = default;

void WMarkerComponent::SetMarkerType(const char* szType)
{
  m_sMarkerType.Assign(szType);

  UpdateMarker();
}

const char* WMarkerComponent::GetMarkerType() const
{
  return m_sMarkerType;
}

void WMarkerComponent::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  UpdateMarker();
}

float WMarkerComponent::GetRadius() const
{
  return m_fRadius;
}

void WMarkerComponent::OnMsgUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3(0), m_fRadius), m_SpatialCategory);
}

void WMarkerComponent::UpdateMarker()
{
  if (!m_sMarkerType.IsEmpty())
  {
    m_SpatialCategory = WSpatialData::RegisterCategory(m_sMarkerType.GetString(), WSpatialData::Flags::None);
  }
  else
  {
    m_SpatialCategory = WInvalidSpatialDataCategory;
  }

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WMarkerComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sMarkerType;
  s << m_fRadius;
}

void WMarkerComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sMarkerType;
  s >> m_fRadius;
}

void WMarkerComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMarker();
}

void WMarkerComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  GetOwner()->UpdateLocalBounds();
}

W_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_MarkerComponent);
