#include <Core/CorePCH.h>

#include <Core/World/SettingsComponent.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSettingsComponent, 1, WRTTINoAllocator)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Settings"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSettingsComponent::WSettingsComponent()
{
  SetModified();
}

WSettingsComponent::~WSettingsComponent() = default;


W_STATICLINK_FILE(Core, Core_World_Implementation_SettingsComponent);
