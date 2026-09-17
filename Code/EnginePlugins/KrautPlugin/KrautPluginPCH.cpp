#include <KrautPlugin/KrautPluginPCH.h>

#include <KrautPlugin/KrautDeclarations.h>

#include <Foundation/Configuration/Startup.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Kraut, KrautPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WResourceManager::RegisterResourceForAssetType("Kraut Tree", WGetStaticRTTI<WKrautGeneratorResource>());

    WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<WKrautTreeResource, WMaterialResource>();
    WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<WKrautTreeResource, WMeshResource>();

    {
      WKrautTreeResourceDescriptor desc;
      desc.m_Details.m_Bounds = WBoundingBoxSphere::MakeInvalid();

        WKrautTreeResourceHandle hResource = WResourceManager::CreateResource<WKrautTreeResource>("Missing Kraut Tree Mesh", std::move(desc), "Empty Kraut Tree Mesh");
      WResourceManager::SetResourceTypeMissingFallback<WKrautTreeResource>(hResource);
    }

    //{
    //  WKrautGeneratorResourceHandle hResource = WResourceManager::LoadResource<WKrautGeneratorResource>("Kraut/KrautFallback.tree");
    //  WResourceManager::SetResourceTypeMissingFallback<WKrautGeneratorResource>(hResource);
    //}
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WResourceManager::SetResourceTypeMissingFallback<WKrautTreeResource>(WKrautTreeResourceHandle());
    WResourceManager::SetResourceTypeMissingFallback<WKrautGeneratorResource>(WKrautGeneratorResourceHandle());

    WKrautTreeResource::CleanupDynamicPluginReferences();
    WKrautGeneratorResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

W_STATICLINK_LIBRARY(KrautPlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(KrautPlugin_Components_KrautTreeComponent);
  W_STATICLINK_REFERENCE(KrautPlugin_Resources_KrautGeneratorResource);
  W_STATICLINK_REFERENCE(KrautPlugin_Resources_KrautTreeResource);
}
