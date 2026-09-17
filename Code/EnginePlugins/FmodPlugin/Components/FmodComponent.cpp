#include <FmodPlugin/FmodPluginPCH.h>

#include <FmodPlugin/Components/FmodComponent.h>
#include <FmodPlugin/FmodIncludes.h>

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WFmodComponent, 1)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Sound/FMOD"),
  }
  W_END_ATTRIBUTES;
}
W_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

WFmodComponent::WFmodComponent() = default;
WFmodComponent::~WFmodComponent() = default;


W_STATICLINK_FILE(FmodPlugin, FmodPlugin_Components_FmodComponent);
