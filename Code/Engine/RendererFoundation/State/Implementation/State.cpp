#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/State/State.h>

WGALBlendState::WGALBlendState(const WGALBlendStateCreationDescription& Description)
  : WGALObject(Description)
{
}

WGALBlendState::~WGALBlendState() = default;



WGALDepthStencilState::WGALDepthStencilState(const WGALDepthStencilStateCreationDescription& Description)
  : WGALObject(Description)
{
}

WGALDepthStencilState::~WGALDepthStencilState() = default;



WGALRasterizerState::WGALRasterizerState(const WGALRasterizerStateCreationDescription& Description)
  : WGALObject(Description)
{
}

WGALRasterizerState::~WGALRasterizerState() = default;


WGALSamplerState::WGALSamplerState(const WGALSamplerStateCreationDescription& Description)
  : WGALResource(Description)
{
}

WGALSamplerState::~WGALSamplerState() = default;
