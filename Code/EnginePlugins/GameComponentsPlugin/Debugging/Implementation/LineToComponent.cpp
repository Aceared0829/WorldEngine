#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Debugging/LineToComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WLineToComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    // BEGIN-DOCS-CODE-SNIPPET: object-reference-property
    W_ACCESSOR_PROPERTY("Target", GetLineToTargetGuid, SetLineToTargetGuid)->AddAttributes(new WGameObjectReferenceAttribute()),
    // END-DOCS-CODE-SNIPPET
    W_MEMBER_PROPERTY("Color", m_LineColor)->AddAttributes(new WDefaultValueAttribute(WColor::Orange)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Utilities/Debug"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WLineToComponent::WLineToComponent() = default;
WLineToComponent::~WLineToComponent() = default;

void WLineToComponent::Update()
{
  if (m_hTargetObject.IsInvalidated())
    return;

  WGameObject* pTarget = nullptr;
  if (!GetWorld()->TryGetObject(m_hTargetObject, pTarget))
  {
    m_hTargetObject.Invalidate();
    return;
  }

  WTempHybridArray<WDebugRendererLine, 1> lines;

  auto& line = lines.ExpandAndGetRef();
  line.m_start = GetOwner()->GetGlobalPosition();
  line.m_end = pTarget->GetGlobalPosition();

  WDebugRenderer::DrawLinesOccluded(GetWorld(), lines, m_LineColor.GetDarker());
  WDebugRenderer::DrawLines(GetWorld(), lines, m_LineColor);
}

void WLineToComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hTargetObject);
  s << m_LineColor;
}

void WLineToComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  m_hTargetObject = inout_stream.ReadGameObjectHandle();
  s >> m_LineColor;
}

void WLineToComponent::SetLineToTarget(const WGameObjectHandle& hTargetObject)
{
  m_hTargetObject = hTargetObject;
}

// BEGIN-DOCS-CODE-SNIPPET: object-reference-funcs
void WLineToComponent::SetLineToTargetGuid(const char* szTargetGuid)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (resolver.IsValid())
  {
    // tell the resolver our component handle and the name of the property for the object reference
    m_hTargetObject = resolver(szTargetGuid, GetHandle(), "Target");
  }
}

const char* WLineToComponent::GetLineToTargetGuid() const
{
  // this function is never called
  return nullptr;
}
// END-DOCS-CODE-SNIPPET


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Debugging_Implementation_LineToComponent);
