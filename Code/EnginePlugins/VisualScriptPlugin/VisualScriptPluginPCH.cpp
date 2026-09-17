#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/VisualScriptPluginDLL.h>

#include <Foundation/Configuration/Plugin.h>

W_STATICLINK_LIBRARY(VisualScriptPlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(VisualScriptPlugin_Resources_VisualScriptClassResource);
  W_STATICLINK_REFERENCE(VisualScriptPlugin_Runtime_VisualScript);
  W_STATICLINK_REFERENCE(VisualScriptPlugin_Runtime_VisualScriptDataType);
}
