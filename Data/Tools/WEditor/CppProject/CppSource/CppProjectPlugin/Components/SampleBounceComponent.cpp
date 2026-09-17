#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <CppProjectPlugin/Components/SampleBounceComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(SampleBounceComponent, 1 /* version */, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Amplitude", m_fAmplitude)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(0, 10)),
    W_MEMBER_PROPERTY("Speed", m_Speed)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(90))),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Game"), // Component menu group
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

SampleBounceComponent::SampleBounceComponent() = default;
SampleBounceComponent::~SampleBounceComponent() = default;

void SampleBounceComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // this component doesn't need to anything for initialization
}

void SampleBounceComponent::Update()
{
  const WTime curTime = GetWorld()->GetClock().GetAccumulatedTime();
  const WAngle curAngle = curTime.AsFloatInSeconds() * m_Speed;
  const float curHeight = WMath::Sin(curAngle) * m_fAmplitude;

  GetOwner()->SetLocalPosition(WVec3(0, 0, curHeight));
}

void SampleBounceComponent::SerializeComponent(WWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  if (OWNTYPE::GetStaticRTTI()->GetTypeVersion() == 1)
  {
    // this automatically serializes all properties
    // if you need more control, increase the component 'version' at the top of this file
    // and then use the code path below for manual serialization
    WReflectionSerializer::WriteObjectToBinary(s, GetDynamicRTTI(), this);
  }
  else
  {
    // do custom serialization, for example:
    // s << m_fAmplitude;
    // s << m_Speed;
  }
}

void SampleBounceComponent::DeserializeComponent(WWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion == 1)
  {
    // this automatically de-serializes all properties
    // if you need more control, increase the component 'version' at the top of this file
    // and then use the code path below for manual de-serialization
    WReflectionSerializer::ReadObjectPropertiesFromBinary(s, *GetDynamicRTTI(), this);
  }
  else
  {
    // do custom de-serialization, for example:
    // s >> m_fAmplitude;
    // s >> m_Speed;
  }
}
