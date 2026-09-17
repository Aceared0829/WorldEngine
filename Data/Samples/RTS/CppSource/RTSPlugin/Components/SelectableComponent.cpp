#include <RTSPlugin/RTSPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <RTSPlugin/Components/SelectableComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(RtsSelectableComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SelectionRadius", m_fSelectionRadius)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.1f, 10.0f)),
  }
  W_END_PROPERTIES;
  // BEGIN-DOCS-CODE-SNIPPET: spatial-bounds-handler
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  W_END_MESSAGEHANDLERS;
  // END-DOCS-CODE-SNIPPET
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("RTS Sample"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

// BEGIN-DOCS-CODE-SNIPPET: spatial-category-registration
WSpatialData::Category RtsSelectableComponent::s_SelectableCategory = WSpatialData::RegisterCategory("Selectable", WSpatialData::Flags::None);
// END-DOCS-CODE-SNIPPET

RtsSelectableComponent::RtsSelectableComponent() = default;
RtsSelectableComponent::~RtsSelectableComponent() = default;

void RtsSelectableComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fSelectionRadius;
}

void RtsSelectableComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fSelectionRadius;
}


void RtsSelectableComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

// BEGIN-DOCS-CODE-SNIPPET: spatial-bounds-update
void RtsSelectableComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg)
{
  WBoundingBoxSphere bounds;
  bounds.m_fSphereRadius = m_fSelectionRadius;
  bounds.m_vCenter.SetZero();
  bounds.m_vBoxHalfExtents.Set(m_fSelectionRadius);

  ref_msg.AddBounds(bounds, s_SelectableCategory);
}
// END-DOCS-CODE-SNIPPET
