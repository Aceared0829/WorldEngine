#pragma once

#include <Foundation/Basics.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/State/ComputePipeline.h>

class WGALDeviceDX11;

class W_RENDERERDX11_DLL WGALComputePipelineDX11 : public WGALComputePipeline
{
public:
  WGALComputePipelineDX11(const WGALComputePipelineCreationDescription& description);
  ~WGALComputePipelineDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void SetDebugName(const char* szName) override;

private:
};