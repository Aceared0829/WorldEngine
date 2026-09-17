#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/BakedProbes/BakedProbesVolumeComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WBakedProbesVolumeComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new WDefaultValueAttribute(WVec3(10.0f)), new WClampValueAttribute(WVec3(0), WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WInDevelopmentAttribute(WInDevelopmentAttribute::Phase::Beta),
    new WCategoryAttribute("Lighting/Baking"),
    new WBoxManipulatorAttribute("Extents", 1.0f, true),
    new WBoxVisualizerAttribute("Extents", 1.0f, WColor::OrangeRed),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WBakedProbesVolumeComponent::WBakedProbesVolumeComponent() = default;
WBakedProbesVolumeComponent::~WBakedProbesVolumeComponent() = default;

void WBakedProbesVolumeComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void WBakedProbesVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void WBakedProbesVolumeComponent::SetExtents(const WVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void WBakedProbesVolumeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
}

void WBakedProbesVolumeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
}

void WBakedProbesVolumeComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), WInvalidSpatialDataCategory);
}


W_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesVolumeComponent);
