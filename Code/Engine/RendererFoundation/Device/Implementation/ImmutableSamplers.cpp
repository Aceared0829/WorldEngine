#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, ImmutableSamplers)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WGALImmutableSamplers::OnEngineStartup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WGALImmutableSamplers::OnEngineShutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool WGALImmutableSamplers::s_bInitialized = false;
WGALImmutableSamplers::ImmutableSamplers WGALImmutableSamplers::s_ImmutableSamplers;
WHashTable<WHashedString, WGALSamplerStateCreationDescription> WGALImmutableSamplers::s_ImmutableSamplerDesc;

void WGALImmutableSamplers::OnEngineStartup()
{
  WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WGALImmutableSamplers::GALDeviceEventHandler));
}

void WGALImmutableSamplers::OnEngineShutdown()
{
  WGALDevice::s_Events.RemoveEventHandler(WMakeDelegate(&WGALImmutableSamplers::GALDeviceEventHandler));

  W_ASSERT_DEBUG(s_ImmutableSamplers.IsEmpty(), "WGALDeviceEvent::BeforeShutdown should have been fired before engine shutdown");
  s_ImmutableSamplers.Clear();
  s_ImmutableSamplerDesc.Clear();
}

WResult WGALImmutableSamplers::RegisterImmutableSampler(WHashedString sSamplerName, const WGALSamplerStateCreationDescription& desc)
{
  W_ASSERT_ALWAYS(desc.m_useTextureQualitySlot == WGALTextureQualitySlot::None, "Immutable samplers cannot support dynamic quality levels");
  W_ASSERT_DEBUG(!s_bInitialized, "Registering immutable samplers is only allowed at sub-system startup");
  if (s_ImmutableSamplerDesc.Contains(sSamplerName))
    return W_FAILURE;

  s_ImmutableSamplerDesc.Insert(sSamplerName, desc);
  return W_SUCCESS;
}

void WGALImmutableSamplers::GALDeviceEventHandler(const WGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case WGALDeviceEvent::AfterInit:
      CreateSamplers(e.m_pDevice);
      break;
    case WGALDeviceEvent::BeforeShutdown:
      DestroySamplers(e.m_pDevice);
      break;
    default:
      break;
  }
}

void WGALImmutableSamplers::CreateSamplers(WGALDevice* pDevice)
{
  W_ASSERT_DEBUG(s_ImmutableSamplers.IsEmpty(), "Creating more than one GAL device is not supported");
  for (auto it : s_ImmutableSamplerDesc)
  {
    WGALSamplerStateHandle hSampler = pDevice->CreateSamplerState(it.Value());
    s_ImmutableSamplers.Insert(it.Key(), hSampler);
  }
  s_bInitialized = true;
}

void WGALImmutableSamplers::DestroySamplers(WGALDevice* pDevice)
{
  for (auto it : s_ImmutableSamplers)
  {
    pDevice->DestroySamplerState(it.Value());
  }
  s_ImmutableSamplers.Clear();
  s_bInitialized = false;
}

const WGALImmutableSamplers::ImmutableSamplers& WGALImmutableSamplers::GetImmutableSamplers()
{
  return s_ImmutableSamplers;
}

W_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_ImmutableSamplers);
