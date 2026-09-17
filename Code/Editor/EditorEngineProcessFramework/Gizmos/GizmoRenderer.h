#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Pipeline/Renderer.h>

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WGizmoRenderer : public WRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WGizmoRenderer, WRenderer);

public:
  WGizmoRenderer();
  ~WGizmoRenderer();

  // WRenderer implementation
  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual void RenderBatch(const WRenderViewContext& renderContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;

  static float s_fGizmoScale;
};
