#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

struct WGALDeviceEvent;
class WGALDevice;

/// Manages and creates immutable samplers.
/// Once registered, the immutable sampler will be automatically be bound to a shader and can't be changed at runtime.
/// Besides convenience of not having to set these in code, GAL implementations like Vulkan can use this to optimize resource binding.
/// Immutable samplers should be registered at Core Startup via a subsystem, see WRenderContext::RegisterImmutableSamplers for example.
/// There is no unregister, these samples are immutable after all. The list is cleared on Core Shutdown.
class W_RENDERERFOUNDATION_DLL WGALImmutableSamplers
{
public:
  using ImmutableSamplers = WHashTable<WHashedString, WGALSamplerStateHandle>;

  /// Registers an immutable sampler. Should be called during Core Startup via a subsystem.
  /// \param sSamplerName The shader resource name of the sampler to be added.
  /// \param desc The description of the sampler. It will be created on GAL device init.
  /// \return Returns success if the sampler was not already registered.
  static WResult RegisterImmutableSampler(WHashedString sSamplerName, const WGALSamplerStateCreationDescription& desc);

  /// Returns the table of immutable samplers. Only valid to call after GAL device init.
  /// \return Table of sampler name to sampler handle.
  static const ImmutableSamplers& GetImmutableSamplers();

private:
  static void OnEngineStartup();
  static void OnEngineShutdown();
  static void GALDeviceEventHandler(const WGALDeviceEvent& e);
  static void CreateSamplers(WGALDevice* m_pDevice);
  static void DestroySamplers(WGALDevice* m_pDevice);

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, ImmutableSamplers);

  static bool s_bInitialized;
  static ImmutableSamplers s_ImmutableSamplers;
  static WHashTable<WHashedString, WGALSamplerStateCreationDescription> s_ImmutableSamplerDesc;
};
