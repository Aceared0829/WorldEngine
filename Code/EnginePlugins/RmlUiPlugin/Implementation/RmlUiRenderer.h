#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

using WShaderResourceHandle = WTypedResourceHandle<class WShaderResource>;

class W_RMLUIPLUGIN_DLL WRmlUiRenderer : public WRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WRmlUiRenderer, WRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WRmlUiRenderer);

public:
  WRmlUiRenderer();
  ~WRmlUiRenderer();

  // WRenderer implementation
  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual void RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;

private:
  WShaderResourceHandle m_hShader;
  WConstantBufferStorageHandle m_hConstantBuffer;
};
