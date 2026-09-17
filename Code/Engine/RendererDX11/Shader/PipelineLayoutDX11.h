#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/PipelineLayout.h>

class WGALPipelineLayoutDX11 : public WGALPipelineLayout
{
public:
protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  WGALPipelineLayoutDX11(const WGALPipelineLayoutCreationDescription& Description);

  virtual ~WGALPipelineLayoutDX11();
};
