#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/AiPluginDLL.h>

W_STATICLINK_LIBRARY(AiPlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(AiPlugin_Navigation3D_Implementation_VoxelGridComponent);
  W_STATICLINK_REFERENCE(AiPlugin_Navigation3D_Implementation_VoxelNavigationComponent);
  W_STATICLINK_REFERENCE(AiPlugin_Navigation3D_Implementation_VoxelPathTestComponent);
  W_STATICLINK_REFERENCE(AiPlugin_Navigation3D_Implementation_VoxelWorldModule);
  W_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_DetourCrowdAgentComponent);
  W_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_NavMeshObstacleComponent);
  W_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_NavMeshPathTestComponent);
  W_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_NavigationComponent);
  W_STATICLINK_REFERENCE(AiPlugin_Navigation_Implementation_NavMeshWorldModule);
}
