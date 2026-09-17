#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/AlwaysVisibleComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WAlwaysVisibleComponent, 1, WComponentMode::Static)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WAlwaysVisibleComponent::WAlwaysVisibleComponent() = default;
WAlwaysVisibleComponent::~WAlwaysVisibleComponent() = default;

WResult WAlwaysVisibleComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return W_SUCCESS;
}


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_AlwaysVisibleComponent);
