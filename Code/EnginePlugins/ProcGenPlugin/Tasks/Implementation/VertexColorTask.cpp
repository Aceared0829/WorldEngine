#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Tasks/Utils.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

namespace
{
  template <typename T>
  W_ALWAYS_INLINE WProcessingStream MakeStream(WArrayPtr<T> data, WUInt32 uiOffset, const WHashedString& sName, WProcessingStream::DataType dataType = WProcessingStream::DataType::Float)
  {
    return WProcessingStream(sName, data.ToByteArray().GetSubArray(uiOffset), dataType, sizeof(T));
  }

  W_ALWAYS_INLINE float Remap(WEnum<WProcVertexColorChannelMapping> channelMapping, const WColor& srcColor)
  {
    if (channelMapping >= WProcVertexColorChannelMapping::R && channelMapping <= WProcVertexColorChannelMapping::A)
    {
      return (&srcColor.r)[channelMapping];
    }
    else
    {
      return channelMapping == WProcVertexColorChannelMapping::White ? 1.0f : 0.0f;
    }
  }
} // namespace

using namespace WProcGenInternal;

VertexColorTask::VertexColorTask()
{
  m_VM.RegisterFunction(WExtendedExpressionFunctions::s_SampleCurveFunc);
  m_VM.RegisterFunction(WProcGenExpressionFunctions::s_ApplyVolumesFunc);
  m_VM.RegisterFunction(WProcGenExpressionFunctions::s_GetInstanceSeedFunc);
}

VertexColorTask::~VertexColorTask() = default;

void VertexColorTask::Prepare(const WWorld& world, const WMeshBufferResourceDescriptor& desc, const WTransform& transform, const WBoundingBox& bbox, WArrayPtr<WSharedPtr<const VertexColorOutput>> outputs, WArrayPtr<WProcVertexColorMapping> outputMappings, WArrayPtr<WColorLinearUB> outputVertexColors)
{
  W_PROFILE_SCOPE("VertexColorPrepare");

  m_InputVertices.Clear();
  m_InputVertices.Reserve(desc.GetVertexCount());

  const WVec3* pPositions = desc.GetPositionData().GetPtr();

  WUInt32 uiNormalDataStride = 0;
  const WUInt8* pNormals = desc.GetNormalData(&uiNormalDataStride).GetPtr();
  const WGALResourceFormat::Enum normalFormat = desc.GetVertexStreamConfig().GetNormalFormat();

  WUInt32 uiColorDataStride = 0;
  const WUInt8* pColors = nullptr;
  const WGALResourceFormat::Enum colorFormat = desc.GetVertexStreamConfig().GetColorFormat();
  if (desc.GetVertexStreamConfig().HasColor0())
  {
    pColors = desc.GetColor0Data(&uiColorDataStride).GetPtr();
  }

  if (pPositions == nullptr || pNormals == nullptr)
  {
    WLog::Error("No position and normal stream found in CPU mesh");
    return;
  }

  WUInt8 dummySource[16] = {};
  WVec3 vNormal;
  if (WMeshBufferUtils::DecodeNormal(WMakeArrayPtr(dummySource), normalFormat, vNormal).Failed())
  {
    WLog::Error("Unsupported CPU mesh vertex normal format {0}", normalFormat);
    return;
  }

  WMat3 normalTransform = transform.GetAsMat4().GetRotationalPart();
  normalTransform.Invert(0.0f).IgnoreResult();
  normalTransform.Transpose();

  // write out all vertices
  for (WUInt32 i = 0; i < desc.GetVertexCount(); ++i)
  {
    WMeshBufferUtils::DecodeNormal(WMakeArrayPtr(pNormals, sizeof(WVec3)), normalFormat, vNormal).IgnoreResult();

    auto& vert = m_InputVertices.ExpandAndGetRef();
    vert.m_vPosition = transform.TransformPosition(*pPositions);
    vert.m_vNormal = normalTransform.TransformDirection(vNormal).GetNormalized();
    vert.m_uiIndex = i;

    ++pPositions;
    pNormals = WMemoryUtils::AddByteOffset(pNormals, uiNormalDataStride);

    if (pColors != nullptr)
    {
      WVec4 c;
      WMeshBufferUtils::DecodeToVec4(WMakeArrayPtr(pColors, sizeof(WColor)), colorFormat, c).IgnoreResult();

      vert.m_Color = WColor(c.x, c.y, c.z, c.w);

      pColors = WMemoryUtils::AddByteOffset(pColors, uiColorDataStride);
    }
    else
    {
      vert.m_Color = WColor::MakeZero();
    }
  }

  m_Outputs = outputs;
  m_OutputMappings = outputMappings;
  m_OutputVertexColors = outputVertexColors;

  //////////////////////////////////////////////////////////////////////////

  WBoundingBox box = bbox;
  box.TransformFromOrigin(transform.GetAsMat4());

  m_VolumeCollections.Clear();
  m_GlobalData.Clear();

  for (auto& pOutput : outputs)
  {
    if (pOutput != nullptr)
    {
      WProcGenGlobalData::ExtractVolumeCollections(world, box, *pOutput, m_VolumeCollections, m_GlobalData);
      WProcGenGlobalData::SetCurves(*pOutput, m_GlobalData);
    }
  }

  const WUInt32 uiTransformHash = WHashingUtils::xxHash32(&transform, sizeof(WTransform));
  WProcGenGlobalData::SetInstanceSeed(uiTransformHash, m_GlobalData);
}

void VertexColorTask::Execute()
{
  if (m_InputVertices.IsEmpty())
    return;

  const WUInt32 uiNumOutputs = m_Outputs.GetCount();
  for (WUInt32 uiOutputIndex = 0; uiOutputIndex < uiNumOutputs; ++uiOutputIndex)
  {
    auto& pOutput = m_Outputs[uiOutputIndex];
    if (pOutput == nullptr || pOutput->m_pByteCode == nullptr)
      continue;

    W_PROFILE_SCOPE("ExecuteVM");

    WUInt32 uiNumVertices = m_InputVertices.GetCount();
    m_TempData.SetCountUninitialized(uiNumVertices);

    WTempHybridArray<WProcessingStream, 8> inputs;
    {
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.x), ExpressionInputs::s_sPositionX));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.y), ExpressionInputs::s_sPositionY));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.z), ExpressionInputs::s_sPositionZ));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.x), ExpressionInputs::s_sNormalX));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.y), ExpressionInputs::s_sNormalY));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.z), ExpressionInputs::s_sNormalZ));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.r), ExpressionInputs::s_sColorR));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.g), ExpressionInputs::s_sColorG));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.b), ExpressionInputs::s_sColorB));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.a), ExpressionInputs::s_sColorA));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_uiIndex), ExpressionInputs::s_sPointIndex, WProcessingStream::DataType::Int));
    }

    WTempHybridArray<WProcessingStream, 8> outputs;
    {
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(WColor, r), ExpressionOutputs::s_sOutColorR));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(WColor, g), ExpressionOutputs::s_sOutColorG));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(WColor, b), ExpressionOutputs::s_sOutColorB));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(WColor, a), ExpressionOutputs::s_sOutColorA));
    }

    // Execute expression bytecode
    if (m_VM.Execute(*(pOutput->m_pByteCode), inputs, outputs, uiNumVertices, m_GlobalData, WExpressionVM::Flags::BestPerformance).Failed())
    {
      continue;
    }

    auto& outputMapping = m_OutputMappings[uiOutputIndex];
    for (WUInt32 i = 0; i < uiNumVertices; ++i)
    {
      WColor srcColor = m_TempData[i];
      WColor remappedColor;
      remappedColor.r = Remap(outputMapping.m_R, srcColor);
      remappedColor.g = Remap(outputMapping.m_G, srcColor);
      remappedColor.b = Remap(outputMapping.m_B, srcColor);
      remappedColor.a = Remap(outputMapping.m_A, srcColor);

      // Store output vertex colors interleaved
      m_OutputVertexColors[i * uiNumOutputs + uiOutputIndex] = remappedColor;
    }
  }
}
