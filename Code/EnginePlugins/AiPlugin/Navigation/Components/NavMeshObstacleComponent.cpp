#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Navigation/Components/NavMeshObstacleComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WNavMeshObstacleComponent, 1, WComponentMode::Static)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(InvalidateSectors),
  }
  W_END_FUNCTIONS;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("AI/Navigation"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WNavMeshObstacleComponent::WNavMeshObstacleComponent() = default;
WNavMeshObstacleComponent::~WNavMeshObstacleComponent() = default;

void WNavMeshObstacleComponent::OnActivated()
{
  SUPER::OnActivated();

  if (IsSimulationStarted())
    InvalidateSectors();
}

void WNavMeshObstacleComponent::OnSimulationStarted()
{
  WComponent::OnSimulationStarted();

  InvalidateSectors();
}

void WNavMeshObstacleComponent::OnDeactivated()
{
  InvalidateSectors();

  SUPER::OnDeactivated();
}

void WNavMeshObstacleComponent::InvalidateSectors()
{
  // TODO: dynamic obstacles not implemented yet
  if (GetOwner()->IsDynamic())
    return;

  auto* pPhysics = GetWorld()->GetModule<WPhysicsWorldModuleInterface>();
  if (pPhysics == nullptr)
    return;

  auto* pNavMeshModule = GetWorld()->GetOrCreateModule<WAiNavMeshWorldModule>();
  if (pNavMeshModule == nullptr)
    return;

  for (const auto& navConfig : pNavMeshModule->GetConfig().m_NavmeshConfigs)
  {
    WAiNavMesh* pNavMesh = pNavMeshModule->GetNavMesh(navConfig.m_sName);
    WUInt8 uiCollisionLayer = navConfig.m_uiCollisionLayer;

    // TODO: change to WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic when dynamic obstacles are supported
    auto bounds = pPhysics->GetWorldSpaceBounds(GetOwner(), uiCollisionLayer, WPhysicsShapeType::Static, true);
    if (bounds.IsValid())
    {
      pNavMesh->InvalidateSector(bounds.GetBox().GetCenter().GetAsVec2(), bounds.GetBox().GetHalfExtents().GetAsVec2(), false);
    }
  }
}


W_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Components_NavMeshObstacleComponent);
