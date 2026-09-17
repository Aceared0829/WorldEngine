#include <GameEngine/GameEnginePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/StateMachine/StateMachineResource.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/DecalResource.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Utils/BlackboardTemplateResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>

#if defined(BUILDSYSTEM_DEFAULT_RENDERER)
constexpr const char* szDefaultRenderer = BUILDSYSTEM_DEFAULT_RENDERER;
#elif defined(BUILDSYSTEM_ENABLE_D3D11_SUPPORT)
constexpr const char* szDefaultRenderer = "DX11";
#elif defined(BUILDSYSTEM_ENABLE_VULKAN_SUPPORT)
constexpr const char* szDefaultRenderer = "Vulkan";
#else
constexpr const char* szDefaultRenderer = "";
#endif

WCommandLineOptionString opt_Renderer("app", "-renderer", "The renderer implementation to use.", szDefaultRenderer);
WCommandLineOptionBool opt_RendererDebugDevice("app", "-debugdevice", "Whether to create a debug GAL device.", false);

void WGameApplication::Init_ConfigureAssetManagement()
{
  const WStringBuilder sAssetRedirFile("AssetCache/", m_PlatformProfile.GetConfigName(), ".WAidlt");

  // which redirection table to search
  WDataDirectory::FolderType::s_sRedirectionFile = sAssetRedirFile;

  // which platform assets to use
  WDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";

  WResourceManager::RegisterResourceForAssetType("Animated Mesh", WGetStaticRTTI<WMeshResource>());
  WResourceManager::RegisterResourceForAssetType("Animation Clip", WGetStaticRTTI<WAnimationClipResource>());
  WResourceManager::RegisterResourceForAssetType("Animation Graph", WGetStaticRTTI<WAnimGraphResource>());
  WResourceManager::RegisterResourceForAssetType("BlackboardTemplate", WGetStaticRTTI<WBlackboardTemplateResource>());
  WResourceManager::RegisterResourceForAssetType("Collection", WGetStaticRTTI<WCollectionResource>());
  WResourceManager::RegisterResourceForAssetType("ColorGradient", WGetStaticRTTI<WColorGradientResource>());
  WResourceManager::RegisterResourceForAssetType("Curve1D", WGetStaticRTTI<WCurve1DResource>());
  WResourceManager::RegisterResourceForAssetType("Decal", WGetStaticRTTI<WDecalResource>());
  WResourceManager::RegisterResourceForAssetType("Decal Atlas", WGetStaticRTTI<WDecalAtlasResource>());
  WResourceManager::RegisterResourceForAssetType("Image Data", WGetStaticRTTI<WImageDataResource>());
  WResourceManager::RegisterResourceForAssetType("LUT", WGetStaticRTTI<WTexture3DResource>());
  WResourceManager::RegisterResourceForAssetType("Material", WGetStaticRTTI<WMaterialResource>());
  WResourceManager::RegisterResourceForAssetType("Mesh", WGetStaticRTTI<WMeshResource>());
  WResourceManager::RegisterResourceForAssetType("Prefab", WGetStaticRTTI<WPrefabResource>());
  WResourceManager::RegisterResourceForAssetType("PropertyAnim", WGetStaticRTTI<WPropertyAnimResource>());
  WResourceManager::RegisterResourceForAssetType("RenderPipeline", WGetStaticRTTI<WRenderPipelineResource>());
  WResourceManager::RegisterResourceForAssetType("Render Target", WGetStaticRTTI<WTexture2DResource>());
  WResourceManager::RegisterResourceForAssetType("Shader", WGetStaticRTTI<WShaderResource>());
  WResourceManager::RegisterResourceForAssetType("Skeleton", WGetStaticRTTI<WSkeletonResource>());
  WResourceManager::RegisterResourceForAssetType("StateMachine", WGetStaticRTTI<WStateMachineResource>());
  WResourceManager::RegisterResourceForAssetType("Substance Texture", WGetStaticRTTI<WTexture2DResource>());
  WResourceManager::RegisterResourceForAssetType("Surface", WGetStaticRTTI<WSurfaceResource>());
  WResourceManager::RegisterResourceForAssetType("Texture 2D", WGetStaticRTTI<WTexture2DResource>());
  WResourceManager::RegisterResourceForAssetType("Texture Cube", WGetStaticRTTI<WTextureCubeResource>());
}

void WGameApplication::Init_SetupDefaultResources()
{
  SUPER::Init_SetupDefaultResources();

  WResourceManager::SetIncrementalUnloadForResourceType<WShaderPermutationResource>(false);

  // Shaders
  {
    WShaderResourceDescriptor desc;
    WShaderResourceHandle hFallbackShader = WResourceManager::CreateResource<WShaderResource>("FallbackShaderResource", std::move(desc), "FallbackShaderResource");

    WShaderResourceDescriptor desc2;
    WShaderResourceHandle hMissingShader = WResourceManager::CreateResource<WShaderResource>("MissingShaderResource", std::move(desc2), "MissingShaderResource");

    WResourceManager::SetResourceTypeLoadingFallback<WShaderResource>(hFallbackShader);
    WResourceManager::SetResourceTypeMissingFallback<WShaderResource>(hMissingShader);
  }

  // Shader Permutation
  {
    WShaderPermutationResourceDescriptor desc;
    WShaderPermutationResourceHandle hFallbackShaderPermutation = WResourceManager::CreateResource<WShaderPermutationResource>("FallbackShaderPermutationResource", std::move(desc), "FallbackShaderPermutationResource");

    WResourceManager::SetResourceTypeLoadingFallback<WShaderPermutationResource>(hFallbackShaderPermutation);
  }

  // 2D Textures
  {
    WTexture2DResourceHandle hFallbackTexture = WResourceManager::LoadResource<WTexture2DResource>("Textures/Loading_D.dds");
    WTexture2DResourceHandle hMissingTexture = WResourceManager::LoadResource<WTexture2DResource>("Textures/MissingResource_D.dds");

    WResourceManager::SetResourceTypeLoadingFallback<WTexture2DResource>(hFallbackTexture);
    WResourceManager::SetResourceTypeMissingFallback<WTexture2DResource>(hMissingTexture);
  }

  // Render to 2D Textures
  {
    WRenderToTexture2DResourceDescriptor desc;
    desc.m_uiWidth = 128;
    desc.m_uiHeight = 128;

    WRenderToTexture2DResourceHandle hMissingTexture = WResourceManager::CreateResource<WRenderToTexture2DResource>("R22DT_Missing", std::move(desc));

    WResourceManager::SetResourceTypeMissingFallback<WRenderToTexture2DResource>(hMissingTexture);
  }

  // Cube Textures
  {
    /// \todo Loading Cubemap Texture

    WTextureCubeResourceHandle hFallbackTexture = WResourceManager::LoadResource<WTextureCubeResource>("Textures/MissingCubeMap.dds");
    WTextureCubeResourceHandle hMissingTexture = WResourceManager::LoadResource<WTextureCubeResource>("Textures/MissingCubeMap.dds");

    WResourceManager::SetResourceTypeLoadingFallback<WTextureCubeResource>(hFallbackTexture);
    WResourceManager::SetResourceTypeMissingFallback<WTextureCubeResource>(hMissingTexture);
  }

  // Materials
  {
    WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<WMaterialResource, WMaterialResource>();

    WMaterialResourceHandle hMissingMaterial = WResourceManager::LoadResource<WMaterialResource>("Materials/Common/MissingMaterial.WMaterial");
    WMaterialResourceHandle hFallbackMaterial = WResourceManager::LoadResource<WMaterialResource>("Materials/Common/LoadingMaterial.WMaterial");

    WResourceManager::SetResourceTypeLoadingFallback<WMaterialResource>(hFallbackMaterial);
    WResourceManager::SetResourceTypeMissingFallback<WMaterialResource>(hMissingMaterial);
  }

  // Meshes
  {
    WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<WMeshResource, WMeshBufferResource>();

    WMeshResourceHandle hMissingMesh = WResourceManager::LoadResource<WMeshResource>("Meshes/MissingMesh.WBinMesh");
    WResourceManager::SetResourceTypeMissingFallback<WMeshResource>(hMissingMesh);
  }

  // Prefabs
  {
    // WPrefabResourceDescriptor emptyPrefab;
    // WPrefabResourceHandle hMissingPrefab = WResourceManager::CreateResource<WPrefabResource>("MissingPrefabResource", emptyPrefab,
    // "MissingPrefabResource");

    WPrefabResourceHandle hMissingPrefab = WResourceManager::LoadResource<WPrefabResource>("Prefabs/MissingPrefab.WBinPrefab");
    WResourceManager::SetResourceTypeMissingFallback<WPrefabResource>(hMissingPrefab);
  }

  // Collections
  {
    WCollectionResourceDescriptor desc;
    WCollectionResourceHandle hMissingCollection = WResourceManager::CreateResource<WCollectionResource>("MissingCollectionResource", std::move(desc), "MissingCollectionResource");

    WResourceManager::SetResourceTypeMissingFallback<WCollectionResource>(hMissingCollection);
  }

  // Render Pipelines
  {
    WRenderPipelineResourceHandle hMissingRenderPipeline = WRenderPipelineResource::CreateMissingPipeline();
    WResourceManager::SetResourceTypeMissingFallback<WRenderPipelineResource>(hMissingRenderPipeline);
  }

  // Color Gradient
  {
    WColorGradientResourceDescriptor cg;
    cg.m_Gradient.AddColorControlPoint(0, WColor::RebeccaPurple);
    cg.m_Gradient.AddColorControlPoint(1, WColor::LawnGreen);

    WColorGradientResourceHandle hResource = WResourceManager::CreateResource<WColorGradientResource>("MissingColorGradient", std::move(cg), "Missing Color Gradient Resource");
    WResourceManager::SetResourceTypeMissingFallback<WColorGradientResource>(hResource);
  }

  // 1D Curve
  {
    WCurve1DResourceDescriptor cd;
    auto& curve = cd.m_Curves.ExpandAndGetRef();
    curve.AddControlPoint(0);
    curve.AddControlPoint(1);
    curve.CreateLinearApproximation();

    WCurve1DResourceHandle hResource = WResourceManager::CreateResource<WCurve1DResource>("MissingCurve1D", std::move(cd), "Missing Curve1D Resource");
    WResourceManager::SetResourceTypeMissingFallback<WCurve1DResource>(hResource);
  }

  // Property Animations
  {
    WPropertyAnimResourceDescriptor desc;
    desc.m_AnimationDuration = WTime::MakeFromSeconds(0.1);

    WPropertyAnimResourceHandle hResource = WResourceManager::CreateResource<WPropertyAnimResource>("MissingPropertyAnim", std::move(desc), "Missing Property Animation Resource");
    WResourceManager::SetResourceTypeMissingFallback<WPropertyAnimResource>(hResource);
  }

  // Animation Skeleton
  {
    WSkeletonResourceDescriptor desc;

    WSkeletonResourceHandle hResource = WResourceManager::CreateResource<WSkeletonResource>("MissingSkeleton", std::move(desc), "Missing Skeleton Resource");
    WResourceManager::SetResourceTypeMissingFallback<WSkeletonResource>(hResource);
  }

  // Animation Clip
  {
    WAnimationClipResourceDescriptor desc;

    WAnimationClipResourceHandle hResource = WResourceManager::CreateResource<WAnimationClipResource>("MissingAnimationClip", std::move(desc), "Missing Animation Clip Resource");
    WResourceManager::SetResourceTypeMissingFallback<WAnimationClipResource>(hResource);
  }

  // Decal Atlas
  {
    WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<WDecalAtlasResource, WTexture2DResource>();
  }
}

WStringView GetRendererNameFromCommandLine()
{
  return opt_Renderer.GetOptionValue(WCommandLineOption::LogMode::FirstTimeIfSpecified);
}

WStringView WGameApplication::GetActiveRenderer()
{
  return GetRendererNameFromCommandLine();
}

void WGameApplication::Init_SetupGraphicsDevice()
{
  WGALDeviceCreationDescription DeviceInit;

  DeviceInit.m_bDebugDevice = opt_RendererDebugDevice.GetOptionValue(WCommandLineOption::LogMode::Never);
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  DeviceInit.m_bDebugDevice = true;
#endif

  {
    WGALDevice* pDevice = nullptr;

    if (s_DefaultDeviceCreator.IsValid())
    {
      pDevice = s_DefaultDeviceCreator(DeviceInit);
    }
    else
    {
      WStringView sRendererName = GetRendererNameFromCommandLine();
      pDevice = WGALDeviceFactory::CreateDevice(sRendererName, WFoundation::GetDefaultAllocator(), DeviceInit);
      W_ASSERT_DEV(pDevice != nullptr, "Device implementation for '{}' not found", sRendererName);
    }

    W_VERIFY(pDevice->Init() == W_SUCCESS, "Graphics device creation failed!");
    WGALDevice::SetDefaultDevice(pDevice);
  }

  // Create GPU resource pool
  WGPUResourcePool* pResourcePool = W_DEFAULT_NEW(WGPUResourcePool);
  WGPUResourcePool::SetDefaultInstance(pResourcePool);
}

void WGameApplication::Init_LoadRequiredPlugins()
{
  WPlugin::InitializeStaticallyLinkedPlugins();

  WStringView sRendererName = GetRendererNameFromCommandLine();
  const char* szShaderModel = "";
  const char* szShaderCompiler = "";
  WGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);
  WShaderManager::Configure(szShaderModel, true);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WPlugin::LoadPlugin("WInspectorPlugin", WPluginLoadFlags::PluginIsOptional).IgnoreResult();

  // The MCP server is a development-only tool, so it is not part of the project's plugin config and thus
  // never ends up in a shipping build. Load it on demand instead, when a port was actually requested.
  // In the editor's engine process this is not needed - there the Mcp plugin bundle pulls it in.
  if (WCommandLineUtils::GetGlobalInstance()->HasOption("-mcpport") ||
      WCommandLineUtils::GetGlobalInstance()->HasOption("-editor-mcpport"))
  {
    if (WPlugin::LoadPlugin("WMcpPlugin", WPluginLoadFlags::PluginIsOptional).Failed())
    {
      WLog::Warning("An MCP port was given on the command line, but 'WMcpPlugin' could not be loaded.");
    }
  }

  // on sandboxed platforms, we can only load data through fileserve, so enforce use of this plugin
#  if W_DISABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  WPlugin::LoadPlugin("WFileservePlugin").IgnoreResult(); // don't care if it fails to load
#  endif

#endif

  if (WPlugin::LoadPlugin(szShaderCompiler, WPluginLoadFlags::PluginIsOptional).Failed())
  {
    WLog::Warning("Shader compiler plugin '{}' not found", szShaderCompiler);
  }
}

void WGameApplication::Deinit_ShutdownGraphicsDevice()
{
  if (!WGALDevice::HasDefaultDevice())
    return;

  // Cleanup resource pool
  WGPUResourcePool::SetDefaultInstance(nullptr);

  WResourceManager::FreeAllUnusedResources();

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  pDevice->Shutdown().IgnoreResult();
  W_DEFAULT_DELETE(pDevice);
  WGALDevice::SetDefaultDevice(nullptr);
}
