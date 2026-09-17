#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Particle, ParticlePlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WResourceManager::RegisterResourceForAssetType("Particle Effect", WGetStaticRTTI<WParticleEffectResource>());

    WParticleEffectResourceDescriptor desc;
    WParticleEffectResourceHandle hEffect = WResourceManager::CreateResource<WParticleEffectResource>("ParticleEffectMissing", std::move(desc), "Fallback for missing Particle Effects");
    WResourceManager::SetResourceTypeMissingFallback<WParticleEffectResource>(hEffect);

    WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<WParticleEffectResource, WCurve1DResource>();
    WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<WParticleEffectResource, WColorGradientResource>();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WParticleEffectResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Startup);
