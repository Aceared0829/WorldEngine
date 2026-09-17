#include <PacManPlugin/PacManPluginPCH.h>

W_STATICLINK_LIBRARY(PacManPlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(PacManPlugin_Components_GhostComponent);
  W_STATICLINK_REFERENCE(PacManPlugin_Components_PacManComponent);
  W_STATICLINK_REFERENCE(PacManPlugin_GameState_PacManGameState);
}
