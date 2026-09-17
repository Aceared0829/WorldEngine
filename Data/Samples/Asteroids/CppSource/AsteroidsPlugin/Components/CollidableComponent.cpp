#include <AsteroidsPlugin/Components/CollidableComponent.h>
#include <AsteroidsPlugin/GameState/Level.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(CollidableComponent, 1, WComponentMode::Static)
W_END_COMPONENT_TYPE
// clang-format on

CollidableComponent::CollidableComponent()
{
  m_fCollisionRadius = 1.0f;
}
