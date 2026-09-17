#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/System/JoltCore.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Jolt, JoltPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WResourceManager::RegisterResourceForAssetType("Jolt_Colmesh_Triangle", WGetStaticRTTI<WJoltMeshResource>());
    WResourceManager::RegisterResourceForAssetType("Jolt_Colmesh_Convex", WGetStaticRTTI<WJoltMeshResource>());

    WJoltMeshResourceDescriptor desc;
    WJoltMeshResourceHandle hResource = WResourceManager::CreateResource<WJoltMeshResource>("Missing Jolt Mesh", std::move(desc), "Empty collision mesh");
    WResourceManager::SetResourceTypeMissingFallback<WJoltMeshResource>(hResource);

    WJoltCore::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WResourceManager::SetResourceTypeMissingFallback<WJoltMeshResource>(WJoltMeshResourceHandle());
    WJoltCore::Shutdown();

    WJoltMeshResource::CleanupDynamicPluginReferences();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Startup);
