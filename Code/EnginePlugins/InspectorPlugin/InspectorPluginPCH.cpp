#include <InspectorPlugin/InspectorPluginPCH.h>

W_STATICLINK_LIBRARY(InspectorPlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(InspectorPlugin_InspectorPlugin);
  W_STATICLINK_REFERENCE(InspectorPlugin_RenderGraphObserver);
  W_STATICLINK_REFERENCE(InspectorPlugin_Startup);
}
