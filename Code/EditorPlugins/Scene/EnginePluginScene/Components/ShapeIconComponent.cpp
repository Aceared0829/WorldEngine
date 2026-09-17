#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WShapeIconComponent, 1, WComponentMode::Static)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Editing"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WShapeIconComponent::WShapeIconComponent() = default;
WShapeIconComponent::~WShapeIconComponent() = default;
