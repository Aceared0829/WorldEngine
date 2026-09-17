#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class W_RENDERERFOUNDATION_DLL WGALComputePipeline : public WGALObject<WGALComputePipelineCreationDescription>
{
public:
  WGALComputePipeline(const WGALComputePipelineCreationDescription& description)
    : WGALObject<WGALComputePipelineCreationDescription>(description)
  {
  }

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;
  virtual void SetDebugName(const char* szName) = 0;
};
