#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WRenderComponent, 1)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  W_END_MESSAGEHANDLERS;
}
W_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

WRenderComponent::WRenderComponent() = default;
WRenderComponent::~WRenderComponent() = default;

void WRenderComponent::OnActivated()
{
  // Ensure that the render data manager exists.
  GetWorld()->GetOrCreateModule<WRenderDataManager>();

  TriggerLocalBoundsUpdate();
}

void WRenderComponent::OnDeactivated()
{
  // Can't call InvalidateCachedRenderData because it checks whether we are active, which is not the case anymore.
  WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  // Only update the local bounds if the owner is still active, if not no other components are active anymore and the bounds update would be pointless.
  // The bounds will be updated when the owner is re-activated anyway.
  if (GetOwner()->IsActive())
  {
    // Can't call TriggerLocalBoundsUpdate because it checks whether we are active, which is not the case anymore.
    GetOwner()->UpdateLocalBounds();
  }
}

void WRenderComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

  bool bAlwaysVisible = false;

  if (GetLocalBounds(bounds, bAlwaysVisible, msg).Succeeded())
  {
    WSpatialData::Category category = GetOwner()->IsDynamic() ? WDefaultSpatialDataCategories::RenderDynamic : WDefaultSpatialDataCategories::RenderStatic;

    if (bounds.IsValid())
    {
      msg.AddBounds(bounds, category);
    }

    if (bAlwaysVisible)
    {
      msg.SetAlwaysVisible(category);
    }
  }
}

void WRenderComponent::InvalidateCachedRenderData()
{
  if (IsActiveAndInitialized())
  {
    WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void WRenderComponent::TriggerLocalBoundsUpdate()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WRenderComponent::QueueLocalBoundsUpdate()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->QueueLocalBoundsUpdate();
  }
}

// static
WUInt32 WRenderComponent::GetUniqueIdForRendering(const WComponent& component)
{
  WUInt32 uniqueId = component.GetUniqueID();
  if (uniqueId == WInvalidIndex)
  {
    uniqueId = component.GetOwner()->GetHandle().GetInternalID().m_InstanceIndex;
  }

  const WUInt32 dynamicBit = (1 << 31);
  const WUInt32 dynamicBitMask = ~dynamicBit;
  return (uniqueId & dynamicBitMask) | (component.GetOwner()->IsDynamic() ? dynamicBit : 0);
}

W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderComponent);
