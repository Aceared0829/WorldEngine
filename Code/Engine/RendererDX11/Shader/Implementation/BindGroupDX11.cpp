#include <RendererDX11/Shader/BindGroupDX11.h>

WGALBindGroupDX11::WGALBindGroupDX11(const WGALBindGroupCreationDescription& Description)
  : WGALBindGroup(Description)
{
}

WGALBindGroupDX11::~WGALBindGroupDX11() = default;

WResult WGALBindGroupDX11::InitPlatform(WGALDevice*)
{
  return W_SUCCESS;
}

WResult WGALBindGroupDX11::DeInitPlatform(WGALDevice*)
{
  return W_SUCCESS;
}

void WGALBindGroupDX11::Invalidate(WGALDevice*)
{
  m_bInvalidated = true;
}

bool WGALBindGroupDX11::IsInvalidated() const
{
  return m_bInvalidated;
}

void WGALBindGroupDX11::SetDebugNamePlatform(const char*) const
{
}
