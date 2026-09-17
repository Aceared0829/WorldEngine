#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/PipelineLayoutDX11.h>

WGALPipelineLayoutDX11::WGALPipelineLayoutDX11(const WGALPipelineLayoutCreationDescription& Description)
  : WGALPipelineLayout(Description)
{
}

WGALPipelineLayoutDX11::~WGALPipelineLayoutDX11() = default;

WResult WGALPipelineLayoutDX11::InitPlatform(WGALDevice*)
{
  return W_SUCCESS;
}

WResult WGALPipelineLayoutDX11::DeInitPlatform(WGALDevice*)
{
  return W_SUCCESS;
}
