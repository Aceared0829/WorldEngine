#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/State/ComputePipelineDX11.h>

WGALComputePipelineDX11::WGALComputePipelineDX11(const WGALComputePipelineCreationDescription& description)
  : WGALComputePipeline(description)
{
}

WGALComputePipelineDX11::~WGALComputePipelineDX11() = default;

WResult WGALComputePipelineDX11::InitPlatform(WGALDevice*)
{
  return W_SUCCESS;
}

WResult WGALComputePipelineDX11::DeInitPlatform(WGALDevice*)
{
  return W_SUCCESS;
}

void WGALComputePipelineDX11::SetDebugName(const char*)
{
}
