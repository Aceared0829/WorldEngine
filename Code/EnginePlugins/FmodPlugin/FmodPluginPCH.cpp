#include <FmodPlugin/FmodPluginPCH.h>

#include <FmodPlugin/FmodPluginDLL.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>

W_STATICLINK_LIBRARY(FmodPlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(FmodPlugin_Components_FmodComponent);
  W_STATICLINK_REFERENCE(FmodPlugin_Components_FmodEventComponent);
  W_STATICLINK_REFERENCE(FmodPlugin_Components_FmodListenerComponent);
  W_STATICLINK_REFERENCE(FmodPlugin_FmodSingleton);
  W_STATICLINK_REFERENCE(FmodPlugin_FmodStartup);
  W_STATICLINK_REFERENCE(FmodPlugin_Resources_FmodSoundBankResource);
  W_STATICLINK_REFERENCE(FmodPlugin_Resources_FmodSoundEventResource);
}
