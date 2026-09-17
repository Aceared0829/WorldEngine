#pragma once

#include <RendererCore/Declarations.h>

class WDirectionalLightComponent;
class WPointLightComponent;
class WSpotLightComponent;
class WGALTextureHandle;
class WGALBufferHandle;
class WView;
struct WRenderWorldExtractionEvent;
struct WRenderWorldRenderEvent;

class W_RENDERERCORE_DLL WShadowPool
{
public:
  static WUInt32 AddDirectionalLight(const WDirectionalLightComponent* pDirLight, const WView* pReferenceView);
  static WUInt32 AddPointLight(const WPointLightComponent* pPointLight, float fScreenSpaceSize, const WView* pReferenceView);
  static WUInt32 AddSpotLight(const WSpotLightComponent* pSpotLight, float fScreenSpaceSize, const WView* pReferenceView);

  static WGALTextureHandle GetShadowAtlasTexture();
  static WGALBufferHandle GetShadowDataBuffer();

  /// All exclude tags on this white list are copied from the reference views to the shadow views.
  static void AddExcludeTagToWhiteList(const WTag& tag);

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ShadowPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const WRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const WRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};
