#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WPickingRenderPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WPickingRenderPass, WRenderPipelinePass);

public:
  WPickingRenderPass();
  ~WPickingRenderPass();

  WGALTextureHandle GetPickingIdRT() const;
  WGALTextureHandle GetPickingDepthRT() const;

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

  virtual void ReadBackProperties(WView* pView) override;

  bool m_bPickSelected = true;
  bool m_bPickTransparent = true;

  WVec2 m_PickingPosition = WVec2(-1);
  WUInt32 m_PickingIdOut = 0;
  float m_PickingDepthOut = 0.0f;
  WVec2 m_MarqueePickPosition0 = WVec2(-1);
  WVec2 m_MarqueePickPosition1 = WVec2(-1);
  WUInt32 m_uiMarqueeActionID = 0xFFFFFFFF; // used to prevent reusing an old result for a new marquee action
  WUInt32 m_uiWindowWidth = 0;
  WUInt32 m_uiWindowHeight = 0;

private:
  void CreateTarget();
  void DestroyTarget();

  void ReadBackPropertiesSinglePick(WView* pView);
  void ReadBackPropertiesMarqueePick(WView* pView);

  void ProcessPickingRenderData(WExtractedRenderData& extractedRenderData);

private:
  WRectFloat m_TargetRect;
  const WRTTI* m_pGridRenderDataType = nullptr;

  WGALTextureHandle m_hPickingIdRT;
  WGALTextureHandle m_hPickingDepthRT;
  WRenderGraphTextureHandle m_hPickingIdGraphRT;
  WRenderGraphTextureHandle m_hPickingDepthGraphRT;

  WHashSet<WGameObjectHandle> m_SelectionSet;

  // Readback
  struct PickingReadback
  {
    WGALReadbackTextureHelper m_PickingReadback;
    WGALReadbackTextureHelper m_PickingDepthReadback;

    bool m_bReadbackInProgress = false;
    WUInt32 m_uiWindowWidth = 0;
    WUInt32 m_uiWindowHeight = 0;
    /// we need this matrix to compute the world space position of picked pixels
    WMat4 m_mPickingInverseViewProjectionMatrix = WMat4::MakeZero();
  };

  PickingReadback m_PendingReadback;

  // Picking Results
  WMat4 m_mPickingInverseViewProjectionMatrix = WMat4::MakeZero();
  /// stores the 2D depth buffer image (32 Bit depth precision), to compute pixel positions from
  WDynamicArray<float> m_PickingResultsDepth;
  /// Stores the 32 Bit picking ID values of each pixel. This can lead back to the WComponent, etc. that rendered to that pixel
  WDynamicArray<WUInt32> m_PickingResultsID;

  WUInt32 m_uiProcessorId = WInvalidIndex;
};
