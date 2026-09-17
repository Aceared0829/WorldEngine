#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/GraphicsPipelineDX11.h>
#include <RendererDX11/State/StateDX11.h>

WGALGraphicsPipelineDX11::WGALGraphicsPipelineDX11(const WGALGraphicsPipelineCreationDescription& description)
  : WGALGraphicsPipeline(description)
{
}

WGALGraphicsPipelineDX11::~WGALGraphicsPipelineDX11() = default;

WResult WGALGraphicsPipelineDX11::InitPlatform(WGALDevice*)
{
  return W_SUCCESS;
}

WResult WGALGraphicsPipelineDX11::DeInitPlatform(WGALDevice*)
{
  return W_SUCCESS;
}

void WGALGraphicsPipelineDX11::SetDebugName(const char*)
{
}
