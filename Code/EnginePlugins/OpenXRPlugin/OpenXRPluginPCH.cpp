#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

W_STATICLINK_LIBRARY(OpenXRPlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(OpenXRPlugin_Input_Implementation_OpenXRInputDevice);
  W_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRSingleton);
}
