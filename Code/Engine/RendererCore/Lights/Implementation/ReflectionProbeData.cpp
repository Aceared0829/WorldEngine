#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WReflectionProbeMode, 1)
  W_BITFLAGS_CONSTANTS(WReflectionProbeMode::Static, WReflectionProbeMode::Dynamic)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WProbeFlags, 1)
  W_BITFLAGS_CONSTANTS(WProbeFlags::SkyLight, WProbeFlags::HasCustomCubeMap, WProbeFlags::Sphere, WProbeFlags::Box, WProbeFlags::Dynamic)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WReflectionProbeRenderData, 1, WRTTIDefaultAllocator<WReflectionProbeRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeData);
