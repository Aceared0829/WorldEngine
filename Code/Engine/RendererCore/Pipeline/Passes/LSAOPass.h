#pragma once

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/LSAOConstants.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Defines the depth compare function to be used to decide sample weights.
struct W_RENDERERCORE_DLL WLSAODepthCompareFunction
{
  using StorageType = WUInt8;

  enum Enum
  {
    Depth,                   ///< A hard cutoff function between the linear depth values. Samples with an absolute distance greater than
                             ///< WLSAOPass::SetDepthCutoffDistance are ignored.
    Normal,                  ///< Samples that are on the same plane as constructed by the center position and normal will be weighted higher than those samples that
                             ///< are above or below the plane.
    NormalAndSampleDistance, ///< Same as Normal, but if two samples are tested, their distance to the center position is is inversely multiplied as
                             ///< well, giving closer matches a higher weight.
    Default = NormalAndSampleDistance
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WLSAODepthCompareFunction);

/// Screen space ambient occlusion using "line sweep ambient occlusion" by Ville Timonen
///
/// Resources:
/// Use in Quantum Break: http://wili.cc/research/quantum_break/SIGGRAPH_2015_Remedy_Notes.pdf
/// Presentation slides EGSR: http://wili.cc/research/lsao/EGSR13_LSAO.pdf
/// Paper: http://wili.cc/research/lsao/lsao.pdf
///
/// There are a few adjustments and own ideas worked into this implementation.
/// The biggest change probably is that pixels in the gather pass compute their target linesample arithmetically instead of relying on lookups.
class W_RENDERERCORE_DLL WLSAOPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WLSAOPass, WRenderPipelinePass);

public:
  WLSAOPass();
  ~WLSAOPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WStatus AddRenderPassesInactive(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  WUInt32 GetLineToLinePixelOffset() const { return m_iLineToLinePixelOffset; }
  void SetLineToLinePixelOffset(WUInt32 uiPixelOffset);
  WUInt32 GetLineSamplePixelOffset() const { return m_iLineSamplePixelOffsetFactor; }
  void SetLineSamplePixelOffset(WUInt32 uiPixelOffset);

  // Factor used for depth cutoffs (determines when a depth difference is too large to be considered)
  float GetDepthCutoffDistance() const;
  void SetDepthCutoffDistance(float fDepthCutoffDistance);

  // Determines how quickly the occlusion falls of.
  float GetOcclusionFalloff() const;
  void SetOcclusionFalloff(float fFalloff);


protected:
  /// Destroys all GPU data that might have been created in in SetupLineSweepData
  void DestroyLineSweepData();
  void SetupLineSweepData(const WVec3I32& imageResolution);


  void AddLinesForDirection(const WVec3I32& imageResolution, const WVec2I32& sampleDir, WUInt32 lineIndex, WDynamicArray<LineInstruction>& outinLineInstructions, WUInt32& outinTotalNumberOfSamples);

  WRenderPipelineNodeInputPin m_PinDepthInput;
  WRenderPipelineNodeOutputPin m_PinOutput;

  WConstantBufferStorageHandle m_hLineSweepCB;

  bool m_bSweepDataDirty = true;
  bool m_bConstantsDirty = true;

  /// Output of the line sweep pass.
  WGALBufferHandle m_hLineSweepOutputBuffer;
  WGALBufferRange m_LineSweepOutputBufferRange;

  /// Structured buffer containing instructions for every single line to trace.
  WGALBufferHandle m_hLineInfoBuffer;

  /// Total number of lines to be traced.
  WUInt32 m_uiNumSweepLines = 0;

  WInt32 m_iLineToLinePixelOffset = 2;
  WInt32 m_iLineSamplePixelOffsetFactor = 1;
  float m_fOcclusionFalloff = 0.2f;
  float m_fDepthCutoffDistance = 4.0f;

  WEnum<WLSAODepthCompareFunction> m_DepthCompareFunction;
  bool m_bDistributedGathering = true;

  WShaderResourceHandle m_hShaderLineSweep;
  WShaderResourceHandle m_hShaderGather;
  WShaderResourceHandle m_hShaderAverage;
};
