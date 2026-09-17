#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/LSAOPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WLSAODepthCompareFunction, 1)
  W_ENUM_CONSTANT(WLSAODepthCompareFunction::Depth),
  W_ENUM_CONSTANT(WLSAODepthCompareFunction::Normal),
  W_ENUM_CONSTANT(WLSAODepthCompareFunction::NormalAndSampleDistance),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLSAOPass, 1, WRTTIDefaultAllocator<WLSAOPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Depth", m_PinDepthInput),
    W_MEMBER_PROPERTY("AmbientObscurance", m_PinOutput),
    W_ACCESSOR_PROPERTY("LineToLineDistance", GetLineToLinePixelOffset, SetLineToLinePixelOffset)->AddAttributes(new WDefaultValueAttribute(2), new WClampValueAttribute(1, 20)),
    W_ACCESSOR_PROPERTY("LineSampleDistanceFactor", GetLineSamplePixelOffset, SetLineSamplePixelOffset)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 10)),
    W_ACCESSOR_PROPERTY("OcclusionFalloff", GetOcclusionFalloff, SetOcclusionFalloff)->AddAttributes(new WDefaultValueAttribute(0.2f), new WClampValueAttribute(0.01f, 2.0f)),
    W_ENUM_MEMBER_PROPERTY("DepthCompareFunction", WLSAODepthCompareFunction, m_DepthCompareFunction),
    W_ACCESSOR_PROPERTY("DepthCutoffDistance", GetDepthCutoffDistance, SetDepthCutoffDistance)->AddAttributes(new WDefaultValueAttribute(4.0f), new WClampValueAttribute(0.1f, 100.0f)),
    W_MEMBER_PROPERTY("DistributedGathering", m_bDistributedGathering)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Post Processing")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  float HaltonSequence(int iBase, int j)
  {
    static int primes[61] = {
      2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283};

    W_ASSERT_DEV(iBase < 61, "Don't have prime number for this base.");

    // Halton sequence with reverse permutation
    const int p = primes[iBase];
    float h = 0.0f;
    float f = 1.0f / static_cast<float>(p);
    float fct = f;
    while (j > 0)
    {
      int i = j % p;
      h += (i == 0 ? i : p - i) * fct;
      j /= p;
      fct *= f;
    }
    return h;
  }
} // namespace

WLSAOPass::WLSAOPass()
  : WRenderPipelinePass("LSAOPass", true)

{
  {
    // Load shader.
    m_hShaderLineSweep = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/LSAOSweep.WShader");
    W_ASSERT_DEV(m_hShaderLineSweep.IsValid(), "Could not lsao sweep shader!");
    m_hShaderGather = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/LSAOGather.WShader");
    W_ASSERT_DEV(m_hShaderGather.IsValid(), "Could not lsao gather shader!");
    m_hShaderAverage = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/LSAOAverage.WShader");
    W_ASSERT_DEV(m_hShaderGather.IsValid(), "Could not lsao average shader!");
  }

  {
    m_hLineSweepCB = WRenderContext::CreateConstantBufferStorage<WLSAOConstants>();
  }
}

WLSAOPass::~WLSAOPass()
{
  DestroyLineSweepData();

  WRenderContext::DeleteConstantBufferStorage(m_hLineSweepCB);
}

WStatus WLSAOPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hDepthInput = inputs[m_PinDepthInput.m_uiInputIndex].m_TextureHandle;
  if (hDepthInput.IsInvalidated())
    return WStatus(WFmt("Depth: Not connected"));

  const WGALTextureCreationDescription depthDesc = ref_graph.GetTextureDesc(hDepthInput);
  if (depthDesc.m_SampleCount != WGALMSAASampleCount::None)
    return WStatus(WFmt("Depth input must be resolved"));

  // Create output
  WGALTextureCreationDescription outputDesc = depthDesc;
  outputDesc.m_Format = WGALResourceFormat::RGHalf;
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  // Setup line sweep data
  SetupLineSweepData(WVec3I32(depthDesc.m_uiWidth, depthDesc.m_uiHeight, depthDesc.m_uiArraySize));

  // Import persistent buffers
  WRenderGraphBufferHandle hLineSweepOutputBuffer = ref_graph.ImportBuffer(m_hLineSweepOutputBuffer);

  // Temp texture for distributed gathering
  WRenderGraphTextureHandle hTempTexture;
  if (m_bDistributedGathering)
  {
    hTempTexture = ref_graph.CreateTexture(outputDesc);
  }

  // Line Sweep (compute)
  {
    auto pass = ref_graph.AddComputePass("LSAOLineSweep");
    pass.ReadTexture(hDepthInput, {}, WGALResourceState::ShaderResource);
    pass.WriteBuffer(hLineSweepOutputBuffer);
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      // Update constants
      if (m_bConstantsDirty)
      {
        WLSAOConstants* cb = WRenderContext::GetConstantBufferData<WLSAOConstants>(m_hLineSweepCB);
        cb->DepthCutoffDistance = m_fDepthCutoffDistance;
        cb->OcclusionFalloff = m_fOcclusionFalloff;
        m_bConstantsDirty = false;
      }
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
      WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
      bindGroup.BindBuffer("WLSAOConstants", m_hLineSweepCB);
      bindGroup.BindTexture("DepthBuffer", ctx.ResolveTexture(hDepthInput));
      renderViewContext.m_pRenderContext->BindShader(m_hShaderLineSweep);
      bindGroup.BindBuffer("LineInstructions", m_hLineInfoBuffer);
      bindGroup.BindBuffer("LineSweepOutputBuffer", ctx.ResolveBuffer(hLineSweepOutputBuffer), m_LineSweepOutputBufferRange);

      const WUInt32 dispatchSize = m_uiNumSweepLines / SSAO_LINESWEEP_THREAD_GROUP + (m_uiNumSweepLines % SSAO_LINESWEEP_THREAD_GROUP != 0 ? 1 : 0);
      const WUInt32 uiRenderedInstances = renderViewContext.m_pCamera->IsStereoscopic() ? 2 : 1;
      renderViewContext.m_pRenderContext->Dispatch(dispatchSize, uiRenderedInstances).IgnoreResult(); });
  }

  // Gather pass
  {
    WRenderGraphTextureHandle hGatherOutput = m_bDistributedGathering ? hTempTexture : hOutput;

    auto pass = ref_graph.AddGraphicsPass("LSAOGather");
    pass.AddColorTarget(hGatherOutput);
    pass.ReadTexture(hDepthInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.ReadBuffer(hLineSweepOutputBuffer);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();

      if (m_bDistributedGathering)
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DISTRIBUTED_SSAO_GATHERING", "TRUE");
      else
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DISTRIBUTED_SSAO_GATHERING", "FALSE");

      switch (m_DepthCompareFunction)
      {
        case WLSAODepthCompareFunction::Depth:
          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_DEPTH");
          break;
        case WLSAODepthCompareFunction::Normal:
          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL");
          break;
        case WLSAODepthCompareFunction::NormalAndSampleDistance:
          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL_AND_SAMPLE_DISTANCE");
          break;
      }

      WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
      bindGroup.BindBuffer("WLSAOConstants", m_hLineSweepCB);
      bindGroup.BindTexture("DepthBuffer", ctx.ResolveTexture(hDepthInput));
      renderViewContext.m_pRenderContext->BindShader(m_hShaderGather);
      bindGroup.BindBuffer("LineInstructions", m_hLineInfoBuffer);
      bindGroup.BindBuffer("LineSweepOutputBuffer", ctx.ResolveBuffer(hLineSweepOutputBuffer), m_LineSweepOutputBufferRange);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  // Average pass (only for distributed gathering)
  if (m_bDistributedGathering)
  {
    auto pass = ref_graph.AddGraphicsPass("LSAOAverage");
    pass.AddColorTarget(hOutput);
    pass.ReadTexture(hTempTexture, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.ReadTexture(hDepthInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();

      switch (m_DepthCompareFunction)
      {
        case WLSAODepthCompareFunction::Depth:
          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_DEPTH");
          break;
        case WLSAODepthCompareFunction::Normal:
          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL");
          break;
        case WLSAODepthCompareFunction::NormalAndSampleDistance:
          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL_AND_SAMPLE_DISTANCE");
          break;
      }

      WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
      bindGroup.BindBuffer("WLSAOConstants", m_hLineSweepCB);
      bindGroup.BindTexture("DepthBuffer", ctx.ResolveTexture(hDepthInput));
      renderViewContext.m_pRenderContext->BindShader(m_hShaderAverage);
      bindGroup.BindTexture("SSAOGatherOutput", ctx.ResolveTexture(hTempTexture));

      renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  return W_SUCCESS;
}

WStatus WLSAOPass::AddRenderPassesInactive(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hDepthInput = inputs[m_PinDepthInput.m_uiInputIndex].m_TextureHandle;
  if (hDepthInput.IsInvalidated())
    return WStatus(WFmt("Depth: Not connected"));

  WGALTextureCreationDescription outputDesc = ref_graph.GetTextureDesc(hDepthInput);
  outputDesc.m_Format = WGALResourceFormat::RGHalf;
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddGraphicsPass("InactiveLSAO");
  pass.AddColorTarget(hOutput, {}, WGALRenderTargetLoadOp::Clear);
  pass.SetClearColor(0, WColor::White);
  return W_SUCCESS;
}

WResult WLSAOPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_iLineToLinePixelOffset;
  inout_stream << m_iLineSamplePixelOffsetFactor;
  inout_stream << m_fOcclusionFalloff;
  inout_stream << m_fDepthCutoffDistance;
  inout_stream << m_DepthCompareFunction;
  inout_stream << m_bDistributedGathering;
  return W_SUCCESS;
}

WResult WLSAOPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_iLineToLinePixelOffset;
  inout_stream >> m_iLineSamplePixelOffsetFactor;
  inout_stream >> m_fOcclusionFalloff;
  inout_stream >> m_fDepthCutoffDistance;
  inout_stream >> m_DepthCompareFunction;
  inout_stream >> m_bDistributedGathering;
  return W_SUCCESS;
}

void WLSAOPass::SetLineToLinePixelOffset(WUInt32 uiPixelOffset)
{
  m_iLineToLinePixelOffset = uiPixelOffset;
  m_bSweepDataDirty = true;
}

void WLSAOPass::SetLineSamplePixelOffset(WUInt32 uiPixelOffset)
{
  m_iLineSamplePixelOffsetFactor = uiPixelOffset;
  m_bSweepDataDirty = true;
}

float WLSAOPass::GetDepthCutoffDistance() const
{
  return m_fDepthCutoffDistance;
}

void WLSAOPass::SetDepthCutoffDistance(float fDepthCutoffDistance)
{
  m_fDepthCutoffDistance = fDepthCutoffDistance;
  m_bConstantsDirty = true;
}

float WLSAOPass::GetOcclusionFalloff() const
{
  return m_fOcclusionFalloff;
}

void WLSAOPass::SetOcclusionFalloff(float fFalloff)
{
  m_fOcclusionFalloff = fFalloff;
  m_bConstantsDirty = true;
}

void WLSAOPass::DestroyLineSweepData()
{
  WGALDevice* device = WGALDevice::GetDefaultDevice();

  device->DestroyBuffer(m_hLineSweepOutputBuffer);
  device->DestroyBuffer(m_hLineInfoBuffer);
}

void WLSAOPass::SetupLineSweepData(const WVec3I32& imageResolution)
{
  // imageResolution.z defines the number of render layers (1 for mono, 2 for stereo rendering).
  DestroyLineSweepData();

  WDynamicArray<LineInstruction> lineInstructions;
  WUInt32 totalNumberOfSamples = 0;
  WLSAOConstants* cb = WRenderContext::GetConstantBufferData<WLSAOConstants>(m_hLineSweepCB);
  cb->LineToLinePixelOffset = m_iLineToLinePixelOffset;

  // Compute general information per direction and create line instructions.

  // As long as we don't span out different line samplings across multiple frames, the number of prepared directions here is always equal to
  // the number of directions per frame. Note that if we were to do temporal sampling with a different line set every frame, we would need
  // to precompute all *possible* sampling directions still as a whole here!
  WVec2I32 samplingDir[NUM_SWEEP_DIRECTIONS_PER_FRAME];
  {
    constexpr int numSweepDirs = NUM_SWEEP_DIRECTIONS_PER_FRAME;

    // As described in the paper, all directions are aligned so that we always hit  pixels on a square.
    static_assert(numSweepDirs % 4 == 0, "Invalid number of sweep directions for LSAO!");
    // static_assert((numSweepDirs * NUM_SWEEP_DIRECTIONS_PER_PIXEL) % 9 == 0, "Invalid number of sweep directions for LSAO!");
    const int perSide = (numSweepDirs + 4) / 4 - 1; // side length of the square on which all directions lie -1
    const int halfPerSide = perSide / 2 + (perSide % 2);
    for (int i = 0; i < perSide; ++i)
    {
      // Put opposing directions next to each other, so that a gather pass that doesn't sample all directions, only needs to sample an even
      // number of directions to end up with non-negative occlusion.
      samplingDir[i * 4 + 0] = WVec2I32(i - halfPerSide, halfPerSide) * m_iLineSamplePixelOffsetFactor; // Top
      samplingDir[i * 4 + 1] = -samplingDir[i * 4 + 0];                                                  // Bottom
      samplingDir[i * 4 + 2] = WVec2I32(halfPerSide, halfPerSide - i) * m_iLineSamplePixelOffsetFactor; // Right
      samplingDir[i * 4 + 3] = -samplingDir[i * 4 + 2];                                                  // Left
    }

                                                                                                         // todo: Ddd debug test to check whether any direction is duplicated. Mistakes in the equations above can easily happen!
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    for (int i = 0; i < numSweepDirs - 1; ++i)
    {
      for (int j = i + 1; j < numSweepDirs; ++j)
        W_ASSERT_DEBUG(samplingDir[i] != samplingDir[j], "Two SSAO sampling directions are equal. Implementation for direction determination is broken.");
    }
#endif
  }

  for (int dirIndex = 0; dirIndex < W_ARRAY_SIZE(samplingDir); ++dirIndex)
  {
    WUInt32 totalLineCountBefore = lineInstructions.GetCount();
    AddLinesForDirection(imageResolution, samplingDir[dirIndex], dirIndex, lineInstructions, totalNumberOfSamples);
    W_ASSERT_DEBUG(totalNumberOfSamples % 2 == 0, "Only even number of line samples are allowed");

    cb->Directions[dirIndex].Direction = WVec2(static_cast<float>(samplingDir[dirIndex].x), static_cast<float>(samplingDir[dirIndex].y));
    cb->Directions[dirIndex].NumLines = lineInstructions.GetCount() - totalLineCountBefore;
    cb->Directions[dirIndex].LineInstructionOffset = totalLineCountBefore;
  }
  m_uiNumSweepLines = lineInstructions.GetCount();
  cb->TotalLineNumber = m_uiNumSweepLines;
  cb->TotalNumberOfSamples = totalNumberOfSamples;
  // Allocate and upload data structures to GPU
  {
    WGALDevice* device = WGALDevice::GetDefaultDevice();
    DestroyLineSweepData();

    // Output UAV for line sweep pass.
    // DX11 allows only float and int for writing RWBuffer, so we need to do manual packing.
    {
      WGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiStructSize = 4;
      bufferDesc.m_uiTotalSize = imageResolution.z * 2 * totalNumberOfSamples;
      bufferDesc.m_BufferFlags = WGALBufferUsageFlags::TexelBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
      bufferDesc.m_ResourceAccess.m_bImmutable = false;
      bufferDesc.m_Format = WGALResourceFormat::RUInt;

      m_hLineSweepOutputBuffer = device->CreateBuffer(bufferDesc);

      m_LineSweepOutputBufferRange = {0, static_cast<WUInt32>(imageResolution.z * totalNumberOfSamples / 2 * sizeof(WUInt32))};
    }

    // Structured buffer per line.
    {
      WGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiStructSize = sizeof(LineInstruction);
      bufferDesc.m_uiTotalSize = sizeof(LineInstruction) * m_uiNumSweepLines;
      bufferDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
      bufferDesc.m_ResourceAccess.m_bImmutable = true;

      m_hLineInfoBuffer = device->CreateBuffer(bufferDesc, WArrayPtr<const WUInt8>(reinterpret_cast<const WUInt8*>(lineInstructions.GetData()), lineInstructions.GetCount() * sizeof(LineInstruction)));
    }
  }

  m_bSweepDataDirty = false;
}

void WLSAOPass::AddLinesForDirection(const WVec3I32& imageResolution, const WVec2I32& sampleDir, WUInt32 lineIndex, WDynamicArray<LineInstruction>& outinLineInstructions, WUInt32& outinTotalNumberOfSamples)
{
  W_ASSERT_DEBUG(sampleDir.x != 0 || sampleDir.y != 0, "Sample direction is null (not pointing anywhere)");

  WUInt32 firstNewLineInstructionIndex = outinLineInstructions.GetCount();

  // Always walk positive and flip if necessary later.
  WVec2I32 walkDir(WMath::Abs(sampleDir.x), WMath::Abs(sampleDir.y));
  WVec2 walkDirF(static_cast<float>(walkDir.x), static_cast<float>(walkDir.y));

  // Line "creation" always starts from 0,0 and walks along EITHER x or y depending which one is the less dominant axis.

  // Helper to avoid duplication for dominant x/y
  int domDir = walkDir.x > walkDir.y ? 0 : 1;
  int secDir = 1 - domDir;
#define DOM GetData()[domDir]
#define SEC GetData()[secDir]

  // Walk along secondary axis backwards.
  for (WInt32 sec = imageResolution.SEC - 1; true; sec -= m_iLineToLinePixelOffset)
  {
    LineInstruction& newLine = outinLineInstructions.ExpandAndGetRef();
    newLine.FirstSamplePos.DOM = 0.0f;
    newLine.FirstSamplePos.SEC = static_cast<float>(sec);

    // If we are already outside of the screen with sec, this is not a point inside the screen!
    if (sec < 0)
    {
      // If we don't walk in the secondary direction at all this means that we're done.
      if (walkDir.SEC == 0)
      {
        outinLineInstructions.PopBack();
        break;
      }
      // Otherwise we just need to walk long enough to hit the screen again.
      else
      {
        // Find new start on the sec axis. (dom axis is fine)
        WVec2 minimalStepToBorder = walkDirF * WMath::Ceil(static_cast<float>(-sec) / walkDirF.SEC); // Remember: Only walk discrete steps!
        newLine.FirstSamplePos.DOM += minimalStepToBorder.DOM;
        newLine.FirstSamplePos.SEC += minimalStepToBorder.SEC;

        // Outside, we're done.
        if (newLine.FirstSamplePos.DOM >= imageResolution.DOM - walkDir.DOM * 2)
        {
          outinLineInstructions.PopBack();
          break;
        }
      }
    }

    // Add a pseudo random offset to distributed the samples a bit.
    // We still want to go from discrete pixel to discrete pixel so we have to round which can mess up our line placement.
    // So this is introducing some error. Visual comparison clearly shows that it's worth it though.
    float offset = HaltonSequence(lineIndex, sec + lineIndex);
    newLine.FirstSamplePos.DOM += WMath::Round(offset * walkDir.DOM);
    newLine.FirstSamplePos.SEC += WMath::Round(offset * walkDir.SEC);

    // Clamp back to possible area.
    // Due to the way we jump from pixels to line in the gather shader, we can't just discard lines.
    newLine.FirstSamplePos.x = WMath::Clamp<float>(newLine.FirstSamplePos.x, 0.0f, imageResolution.x - 1.0f);
    newLine.FirstSamplePos.y = WMath::Clamp<float>(newLine.FirstSamplePos.y, 0.0f, imageResolution.y - 1.0f);

    // Compute how many samples this line will consume.
    unsigned int stepsToDOMBorder = static_cast<unsigned int>((imageResolution.DOM - newLine.FirstSamplePos.DOM) / walkDir.DOM + 1);
    unsigned int numSamples = 0;
    if (walkDir.SEC > 0)
    {
      unsigned int stepsToSECBorder = static_cast<unsigned int>((imageResolution.SEC - newLine.FirstSamplePos.SEC) / walkDir.SEC + 1);
      numSamples = WMath::Min(stepsToSECBorder, stepsToDOMBorder);
    }
    else
      numSamples = stepsToDOMBorder;

    // Due to output packing restrictions only even number of samples are allowed. Remove one if necessary.
    if (numSamples % 2 != 0)
      --numSamples;

    newLine.LineSweepOutputBufferOffset = outinTotalNumberOfSamples;
    outinTotalNumberOfSamples += numSamples;
    newLine.LineDirIndex_NumSamples = lineIndex | (numSamples << 16);
  }

#undef SEC
#undef DOM

  // Now consider x/y being negative.
  for (int c = 0; c < 2; ++c)
  {
    if (sampleDir.GetData()[c] < 0)
    {
      for (WUInt32 i = firstNewLineInstructionIndex; i < outinLineInstructions.GetCount(); ++i)
      {
        outinLineInstructions[i].FirstSamplePos.GetData()[c] = imageResolution.GetData()[c] - 1 - outinLineInstructions[i].FirstSamplePos.GetData()[c];
      }
    }
  }

  // Validation.
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  for (WUInt32 i = firstNewLineInstructionIndex; i < outinLineInstructions.GetCount(); ++i)
  {
    auto p = outinLineInstructions[i].FirstSamplePos;
    W_ASSERT_DEV(p.x >= 0 && p.y >= 0 && p.x < imageResolution.x && p.y < imageResolution.y, "First sweep line sample pos is invalid. Something is wrong with the sweep line generation algorithm.");
  }
#endif
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_LSAOPass);
