#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(ProcGen, ProcGenPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WResourceManager::RegisterResourceForAssetType("ProcGen Graph", WGetStaticRTTI<WProcGenGraphResource>());

    WProcGenGraphResourceDescriptor desc;
    WProcGenGraphResourceHandle hResource = WResourceManager::CreateResource<WProcGenGraphResource>("ProcGenGraphMissing", std::move(desc), "Fallback for missing ProcGen Graph Resource");
    WResourceManager::SetResourceTypeMissingFallback<WProcGenGraphResource>(hResource);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WProcGenGraphResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on


W_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Startup);
