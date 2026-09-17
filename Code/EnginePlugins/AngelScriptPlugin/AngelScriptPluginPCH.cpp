#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>

#include <Foundation/Configuration/Plugin.h>

W_STATICLINK_LIBRARY(AngelScriptPlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(AngelScriptPlugin_Resources_AngelScriptResource);
  W_STATICLINK_REFERENCE(AngelScriptPlugin_Runtime_AsEngineSingleton);
  W_STATICLINK_REFERENCE(AngelScriptPlugin_Runtime_AsFunctionDispatch);
}
