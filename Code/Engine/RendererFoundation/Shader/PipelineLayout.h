#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class W_RENDERERFOUNDATION_DLL WGALPipelineLayout : public WGALObject<WGALPipelineLayoutCreationDescription>
{
public:
protected:
  friend class WGALDevice;

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;

  WGALPipelineLayout(const WGALPipelineLayoutCreationDescription& Description);
  virtual ~WGALPipelineLayout();

private:
};
