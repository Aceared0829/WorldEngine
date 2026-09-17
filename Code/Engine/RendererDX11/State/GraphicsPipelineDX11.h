#pragma once

#include <Foundation/Basics.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/State/GraphicsPipeline.h>

class WGALDeviceDX11;

class W_RENDERERDX11_DLL WGALGraphicsPipelineDX11 : public WGALGraphicsPipeline
{
public:
  WGALGraphicsPipelineDX11(const WGALGraphicsPipelineCreationDescription& description);
  ~WGALGraphicsPipelineDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void SetDebugName(const char* szName) override;

private:
};