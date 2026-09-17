#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

static WRmlUiResourceLoader s_RmlUiResourceLoader;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RmlUi, RmlUiPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderGraphManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {    
  }

  ON_CORESYSTEMS_SHUTDOWN
  {    
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WResourceManager::SetResourceTypeLoader<WRmlUiResource>(&s_RmlUiResourceLoader);

    WResourceManager::RegisterResourceForAssetType("RmlUi", WGetStaticRTTI<WRmlUiResource>());

    {
      WRmlUiResourceDescriptor desc;
      WRmlUiResourceHandle hResource = WResourceManager::CreateResource<WRmlUiResource>("RmlUiMissing", std::move(desc), "Fallback for missing rml ui resource");
      WResourceManager::SetResourceTypeMissingFallback<WRmlUiResource>(hResource);
    }

    if (WRmlUi::GetSingleton() == nullptr)
    {
      W_DEFAULT_NEW(WRmlUi);
    }
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (WRmlUi* pRmlUi = WRmlUi::GetSingleton())
    {
      W_DEFAULT_DELETE(pRmlUi);
    }

    WResourceManager::SetResourceTypeLoader<WRmlUiResource>(nullptr);

    WRmlUiResource::CleanupDynamicPluginReferences();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on


W_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Startup);
