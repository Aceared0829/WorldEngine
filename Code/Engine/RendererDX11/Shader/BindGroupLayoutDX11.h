
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>

class WGALBindGroupLayoutDX11 : public WGALBindGroupLayout
{
public:
protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  WGALBindGroupLayoutDX11(const WGALBindGroupLayoutCreationDescription& Description);

  virtual ~WGALBindGroupLayoutDX11();
};
