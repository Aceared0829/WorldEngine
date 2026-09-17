#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <SampleGamePlugin/Components/DemoComponent.h>

// BEGIN-DOCS-CODE-SNIPPET: customcomp-reflection
// clang-format off
// BEGIN-DOCS-CODE-SNIPPET: component-reflection
W_BEGIN_COMPONENT_TYPE(DemoComponent, 3 /* version */, WComponentMode::Dynamic)
// END-DOCS-CODE-SNIPPET
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Amplitude", m_fAmplitude)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(0, 10)),
    W_MEMBER_PROPERTY("Speed", m_Speed)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(90))),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("SampleGamePlugin"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: customcomp-basics
DemoComponent::DemoComponent() = default;
DemoComponent::~DemoComponent() = default;

void DemoComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // this component doesn't need to anything for initialization
}

void DemoComponent::Update()
{
  const WTime curTime = GetWorld()->GetClock().GetAccumulatedTime();
  const WAngle curAngle = curTime.AsFloatInSeconds() * m_Speed;
  const float curHeight = WMath::Sin(curAngle) * m_fAmplitude;

  GetOwner()->SetLocalPosition(WVec3(0, 0, curHeight));
}

// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: component-serialize
void DemoComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fAmplitude;
  s << m_Speed;
}
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: component-deserialize
void DemoComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fAmplitude;

  if (uiVersion <= 2)
  {
    // up to version 2 the angle was stored as a float in degree
    // convert this to WAngle
    float fDegree;
    s >> fDegree;
    m_Speed = WAngle::MakeFromDegree(fDegree);
  }
  else
  {
    s >> m_Speed;
  }
}
// END-DOCS-CODE-SNIPPET
