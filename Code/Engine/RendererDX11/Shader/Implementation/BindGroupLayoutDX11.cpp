#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/BindGroupLayoutDX11.h>

WGALBindGroupLayoutDX11::WGALBindGroupLayoutDX11(const WGALBindGroupLayoutCreationDescription& Description)
  : WGALBindGroupLayout(Description)
{
}

WGALBindGroupLayoutDX11::~WGALBindGroupLayoutDX11() = default;

WResult WGALBindGroupLayoutDX11::InitPlatform(WGALDevice*)
{
  return W_SUCCESS;
}

WResult WGALBindGroupLayoutDX11::DeInitPlatform(WGALDevice*)
{
  return W_SUCCESS;
}
