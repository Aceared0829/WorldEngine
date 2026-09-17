#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>

class WGALTextureHandle;
class WGALBufferHandle;
class WView;
class WWorld;
class WComponent;
struct WRenderWorldExtractionEvent;
struct WRenderWorldRenderEvent;
struct WMsgExtractRenderData;
struct WReflectionProbeDesc;
class WReflectionProbeRenderData;
using WReflectionProbeId = WGenericId<24, 8>;
class WReflectionProbeComponentBase;
class WSkyLightComponent;

class W_RENDERERCORE_DLL WReflectionPool
{
public:
  // Probes
  static WReflectionProbeId RegisterReflectionProbe(const WWorld* pWorld, const WReflectionProbeDesc& desc, const WReflectionProbeComponentBase* pComponent);
  static void DeregisterReflectionProbe(const WWorld* pWorld, WReflectionProbeId id);
  static void UpdateReflectionProbe(const WWorld* pWorld, WReflectionProbeId id, const WReflectionProbeDesc& desc, const WReflectionProbeComponentBase* pComponent);
  static void ExtractReflectionProbe(const WComponent* pComponent, WMsgExtractRenderData& ref_msg, WReflectionProbeRenderData* pRenderData, const WWorld* pWorld, WReflectionProbeId id, float fPriority);

  // SkyLight
  static WReflectionProbeId RegisterSkyLight(const WWorld* pWorld, WReflectionProbeDesc& ref_desc, const WSkyLightComponent* pComponent);
  static void DeregisterSkyLight(const WWorld* pWorld, WReflectionProbeId id);
  static void UpdateSkyLight(const WWorld* pWorld, WReflectionProbeId id, const WReflectionProbeDesc& desc, const WSkyLightComponent* pComponent);


  static void SetConstantSkyIrradiance(const WWorld* pWorld, const WAmbientCube<WColor>& skyIrradiance);
  static void ResetConstantSkyIrradiance(const WWorld* pWorld);

  static WUInt32 GetReflectionCubeMapSize();
  static WGALTextureHandle GetReflectionSpecularTexture(WUInt32 uiWorldIndex, WEnum<WCameraUsageHint> cameraUsageHint);
  static WGALTextureHandle GetSkyIrradianceTexture();

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ReflectionPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const WRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const WRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};
