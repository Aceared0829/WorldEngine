#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/BindGroupLayout.h>

WGALBindGroupLayout::WGALBindGroupLayout(const WGALBindGroupLayoutCreationDescription& Description)
  : WGALObject(Description)
{
}

WGALBindGroupLayout::~WGALBindGroupLayout() = default;
