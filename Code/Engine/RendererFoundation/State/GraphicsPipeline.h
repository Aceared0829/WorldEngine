#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Graphics pipeline state object combining shader, blend, depth-stencil, rasterizer state, and vertex declaration into a single object.
class W_RENDERERFOUNDATION_DLL WGALGraphicsPipeline : public WGALObject<WGALGraphicsPipelineCreationDescription>
{
public:
  WGALGraphicsPipeline(const WGALGraphicsPipelineCreationDescription& description)
    : WGALObject<WGALGraphicsPipelineCreationDescription>(description)
  {
  }

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;
  virtual void SetDebugName(const char* szName) = 0;
};
