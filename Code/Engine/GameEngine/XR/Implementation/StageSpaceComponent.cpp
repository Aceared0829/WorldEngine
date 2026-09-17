#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/XR/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WStageSpaceComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("StageSpace", WXRStageSpace, GetStageSpace, SetStageSpace)->AddAttributes(new WDefaultValueAttribute((WInt32)WXRStageSpace::Enum::Standing)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("XR"),
    new WInDevelopmentAttribute(WInDevelopmentAttribute::Phase::Beta),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WStageSpaceComponent::WStageSpaceComponent() = default;
WStageSpaceComponent::~WStageSpaceComponent() = default;

WEnum<WXRStageSpace> WStageSpaceComponent::GetStageSpace() const
{
  return m_Space;
}

void WStageSpaceComponent::SetStageSpace(WEnum<WXRStageSpace> space)
{
  m_Space = space;
}

void WStageSpaceComponent::SerializeComponent(WWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  WStreamWriter& s = stream.GetStream();

  s << m_Space;
}

void WStageSpaceComponent::DeserializeComponent(WWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = stream.GetStream();

  s >> m_Space;
}

void WStageSpaceComponent::OnActivated() {}

void WStageSpaceComponent::OnDeactivated() {}

W_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_StageSpaceComponent);
