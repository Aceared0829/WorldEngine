#pragma once

#include <RendererCore/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct WRenderWorldExtractionEvent;
struct WRenderWorldRenderEvent;
class WView;

class W_RENDERERCORE_DLL WDecalManager
{
public:
  static WDecalId GetOrCreateRuntimeDecal(const WTexture2DResourceHandle& hTexture);
  static WDecalId GetOrCreateRuntimeDecal(const WMaterialResourceHandle& hMaterial, WUInt32 uiResolution, WTime updateInterval);
  static void DeleteRuntimeDecal(WDecalId& ref_decalId);

  /// Marks the runtime decal as in use with the given screen space size by the given reference view. Should be called every frame.
  /// This is used to calculate how much space the decal needs in the atlas.
  static void MarkRuntimeDecalAsUsed(WDecalId decalId, float fScreenSpaceSize, const WView* pReferenceView);

  static WDecalAtlasResourceHandle GetBakedDecalAtlas();
  static WGALTextureHandle GetRuntimeDecalAtlasTexture();

  static WGALBufferHandle GetDecalAtlasDataBufferForRendering();

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, DecalManager);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const WRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const WRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};
