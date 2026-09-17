#include <RmlUiPlugin/RmlUiPluginPCH.h>

W_STATICLINK_LIBRARY(RmlUiPlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(RmlUiPlugin_Components_Implementation_RmlUiCanvas2DComponent);
  W_STATICLINK_REFERENCE(RmlUiPlugin_Components_Implementation_RmlUiCanvas3DComponent);
  W_STATICLINK_REFERENCE(RmlUiPlugin_Components_Implementation_RmlUiCanvas3DInteractionExampleComponent);
  W_STATICLINK_REFERENCE(RmlUiPlugin_Components_Implementation_RmlUiCanvasComponentBase);
  W_STATICLINK_REFERENCE(RmlUiPlugin_Components_Implementation_RmlUiMessages);
  W_STATICLINK_REFERENCE(RmlUiPlugin_Implementation_RmlUiRenderer);
  W_STATICLINK_REFERENCE(RmlUiPlugin_Implementation_RmlUiSingleton);
  W_STATICLINK_REFERENCE(RmlUiPlugin_Resources_RmlUiResource);
  W_STATICLINK_REFERENCE(RmlUiPlugin_Startup);
}
