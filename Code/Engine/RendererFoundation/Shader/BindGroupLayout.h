
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class W_RENDERERFOUNDATION_DLL WGALBindGroupLayout : public WGALObject<WGALBindGroupLayoutCreationDescription>
{
protected:
  friend class WGALDevice;

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;

  WGALBindGroupLayout(const WGALBindGroupLayoutCreationDescription& Description);
  virtual ~WGALBindGroupLayout();
};
