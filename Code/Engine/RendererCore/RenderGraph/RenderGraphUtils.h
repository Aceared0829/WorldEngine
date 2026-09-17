#pragma once

#include <RendererCore/RendererCoreDLL.h>

class WRenderGraph;
class WGALTextureHandle;
struct WGALTextureRange;

class W_RENDERERCORE_DLL WRenderGraphUtils
{
public:
  static WRenderGraphTextureHandle GenerateMipMaps(WGALTextureHandle hTexture, WGALTextureRange range, WRenderGraph& ref_renderGraph);
};
