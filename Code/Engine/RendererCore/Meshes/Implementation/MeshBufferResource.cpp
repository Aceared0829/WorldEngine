#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Containers/IterateBits.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

namespace
{
  const char* s_szMeshVertexStreamTypeNames[] = {
    "Position",
    "NormalTangentAndTexCoord0",
    "TexCoord1",
    "Color0",
    "Color1",
    "SkinningData",
    "DataOffsets"};

  static_assert(W_ARRAY_SIZE(s_szMeshVertexStreamTypeNames) == WMeshVertexStreamType::Count);
} // namespace

// static
const char* WMeshVertexStreamType::GetName(Enum type)
{
  return s_szMeshVertexStreamTypeNames[type];
}

////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE static WUInt32 GetElementSize(WGALResourceFormat::Enum format)
{
  return WGALResourceFormat::GetBitsPerElement(format) / 8;
}

constexpr WGALResourceFormat::Enum s_PositionFormat = WGALResourceFormat::XYZFloat;

constexpr WGALResourceFormat::Enum s_NormalFormat_lp = WGALResourceFormat::RGB10A2UIntNormalized;
constexpr WGALResourceFormat::Enum s_NormalFormat_hp = WGALResourceFormat::RGBAUShortNormalized;

constexpr WGALResourceFormat::Enum s_TangentFormat_lp = WGALResourceFormat::RGB10A2UIntNormalized;
constexpr WGALResourceFormat::Enum s_TangentFormat_hp = WGALResourceFormat::RGBAUShortNormalized;

constexpr WGALResourceFormat::Enum s_TexCoordFormat_lp = WGALResourceFormat::UVHalf;
constexpr WGALResourceFormat::Enum s_TexCoordFormat_hp = WGALResourceFormat::UVFloat;

constexpr WGALResourceFormat::Enum s_ColorFormat_lp = WGALResourceFormat::RGBAUByteNormalized;
constexpr WGALResourceFormat::Enum s_ColorFormat_hp = WGALResourceFormat::RGBAHalf;

constexpr WGALResourceFormat::Enum s_BoneIndicesFormat = WGALResourceFormat::RGBAUShort;
constexpr WGALResourceFormat::Enum s_BoneWeightsFormat_lp = WGALResourceFormat::RGBAUByteNormalized;
constexpr WGALResourceFormat::Enum s_BoneWeightsFormat_hp = WGALResourceFormat::RGBAUShortNormalized;

constexpr WGALResourceFormat::Enum s_DataOffsetsFormat = WGALResourceFormat::RGBAUInt;

static WGALVertexAttribute s_VertexAttributes_lp[] = {
  WGALVertexAttribute(WGALVertexAttributeSemantic::Position, s_PositionFormat, 0, 0),
  WGALVertexAttribute(WGALVertexAttributeSemantic::Normal, s_NormalFormat_lp, 0, 1),
  WGALVertexAttribute(WGALVertexAttributeSemantic::Tangent, s_TangentFormat_lp, GetElementSize(s_NormalFormat_lp), 1),
  WGALVertexAttribute(WGALVertexAttributeSemantic::TexCoord0, s_TexCoordFormat_lp, GetElementSize(s_NormalFormat_lp) + GetElementSize(s_TangentFormat_lp), 1),
  WGALVertexAttribute(WGALVertexAttributeSemantic::TexCoord1, s_TexCoordFormat_lp, 0, 2),
  WGALVertexAttribute(WGALVertexAttributeSemantic::Color0, s_ColorFormat_lp, 0, 3),
  WGALVertexAttribute(WGALVertexAttributeSemantic::Color1, s_ColorFormat_lp, 0, 4),
  WGALVertexAttribute(WGALVertexAttributeSemantic::BoneIndices0, s_BoneIndicesFormat, 0, 5),
  WGALVertexAttribute(WGALVertexAttributeSemantic::BoneWeights0, s_BoneWeightsFormat_lp, GetElementSize(s_BoneIndicesFormat), 5),
  WGALVertexAttribute(WGALVertexAttributeSemantic::DataOffsets, s_DataOffsetsFormat, 0, 6),
};

static WGALVertexAttribute s_VertexAttributes_hp[] = {
  WGALVertexAttribute(WGALVertexAttributeSemantic::Position, s_PositionFormat, 0, 0),
  WGALVertexAttribute(WGALVertexAttributeSemantic::Normal, s_NormalFormat_hp, 0, 1),
  WGALVertexAttribute(WGALVertexAttributeSemantic::Tangent, s_TangentFormat_hp, GetElementSize(s_NormalFormat_hp), 1),
  WGALVertexAttribute(WGALVertexAttributeSemantic::TexCoord0, s_TexCoordFormat_hp, GetElementSize(s_NormalFormat_hp) + GetElementSize(s_TangentFormat_hp), 1),
  WGALVertexAttribute(WGALVertexAttributeSemantic::TexCoord1, s_TexCoordFormat_hp, 0, 2),
  WGALVertexAttribute(WGALVertexAttributeSemantic::Color0, s_ColorFormat_hp, 0, 3),
  WGALVertexAttribute(WGALVertexAttributeSemantic::Color1, s_ColorFormat_hp, 0, 4),
  WGALVertexAttribute(WGALVertexAttributeSemantic::BoneIndices0, s_BoneIndicesFormat, 0, 5),
  WGALVertexAttribute(WGALVertexAttributeSemantic::BoneWeights0, s_BoneWeightsFormat_hp, GetElementSize(s_BoneIndicesFormat), 5),
  WGALVertexAttribute(WGALVertexAttributeSemantic::DataOffsets, s_DataOffsetsFormat, 0, 6),
};

static WUInt32 s_StreamSizes_lp[] = {
  GetElementSize(s_PositionFormat),
  GetElementSize(s_NormalFormat_lp) + GetElementSize(s_TangentFormat_lp) + GetElementSize(s_TexCoordFormat_lp),
  GetElementSize(s_TexCoordFormat_lp),
  GetElementSize(s_ColorFormat_lp),
  GetElementSize(s_ColorFormat_lp),
  GetElementSize(s_BoneIndicesFormat) + GetElementSize(s_BoneWeightsFormat_lp),
  GetElementSize(s_DataOffsetsFormat),
};

static_assert(W_ARRAY_SIZE(s_StreamSizes_lp) == WMeshVertexStreamType::Count);

static WUInt32 s_StreamSizes_hp[] = {
  GetElementSize(s_PositionFormat),
  GetElementSize(s_NormalFormat_hp) + GetElementSize(s_TangentFormat_hp) + GetElementSize(s_TexCoordFormat_hp),
  GetElementSize(s_TexCoordFormat_hp),
  GetElementSize(s_ColorFormat_hp),
  GetElementSize(s_ColorFormat_hp),
  GetElementSize(s_BoneIndicesFormat) + GetElementSize(s_BoneWeightsFormat_hp),
  GetElementSize(s_DataOffsetsFormat),
};

static_assert(W_ARRAY_SIZE(s_StreamSizes_hp) == WMeshVertexStreamType::Count);

WGALResourceFormat::Enum WMeshVertexStreamConfig::GetPositionFormat() const
{
  return s_PositionFormat;
}

WGALResourceFormat::Enum WMeshVertexStreamConfig::GetNormalFormat() const
{
  return m_bUseHighPrecision ? s_NormalFormat_hp : s_NormalFormat_lp;
}

WGALResourceFormat::Enum WMeshVertexStreamConfig::GetTangentFormat() const
{
  return m_bUseHighPrecision ? s_TangentFormat_hp : s_TangentFormat_lp;
}

WGALResourceFormat::Enum WMeshVertexStreamConfig::GetTexCoordFormat() const
{
  return m_bUseHighPrecision ? s_TexCoordFormat_hp : s_TexCoordFormat_lp;
}

WGALResourceFormat::Enum WMeshVertexStreamConfig::GetColorFormat() const
{
  return m_bUseHighPrecision ? s_ColorFormat_hp : s_ColorFormat_lp;
}

WGALResourceFormat::Enum WMeshVertexStreamConfig::GetBoneIndicesFormat() const
{
  return s_BoneIndicesFormat;
}

WGALResourceFormat::Enum WMeshVertexStreamConfig::GetBoneWeightsFormat() const
{
  return m_bUseHighPrecision ? s_BoneWeightsFormat_hp : s_BoneWeightsFormat_lp;
}

WUInt32 WMeshVertexStreamConfig::GetNormalDataOffset() const
{
  W_ASSERT_DEBUG(s_VertexAttributes_lp[1].m_eSemantic == WGALVertexAttributeSemantic::Normal && s_VertexAttributes_hp[1].m_eSemantic == WGALVertexAttributeSemantic::Normal, "");
  return m_bUseHighPrecision ? s_VertexAttributes_hp[1].m_uiOffset : s_VertexAttributes_lp[1].m_uiOffset;
}

WUInt32 WMeshVertexStreamConfig::GetTangentDataOffset() const
{
  W_ASSERT_DEBUG(s_VertexAttributes_lp[2].m_eSemantic == WGALVertexAttributeSemantic::Tangent && s_VertexAttributes_hp[2].m_eSemantic == WGALVertexAttributeSemantic::Tangent, "");
  return m_bUseHighPrecision ? s_VertexAttributes_hp[2].m_uiOffset : s_VertexAttributes_lp[2].m_uiOffset;
}

WUInt32 WMeshVertexStreamConfig::GetTexCoord0DataOffset() const
{
  W_ASSERT_DEBUG(s_VertexAttributes_lp[3].m_eSemantic == WGALVertexAttributeSemantic::TexCoord0 && s_VertexAttributes_hp[3].m_eSemantic == WGALVertexAttributeSemantic::TexCoord0, "");
  return m_bUseHighPrecision ? s_VertexAttributes_hp[3].m_uiOffset : s_VertexAttributes_lp[3].m_uiOffset;
}

WUInt32 WMeshVertexStreamConfig::GetBoneIndicesDataOffset() const
{
  W_ASSERT_DEBUG(s_VertexAttributes_lp[7].m_eSemantic == WGALVertexAttributeSemantic::BoneIndices0 && s_VertexAttributes_hp[7].m_eSemantic == WGALVertexAttributeSemantic::BoneIndices0, "");
  return m_bUseHighPrecision ? s_VertexAttributes_hp[7].m_uiOffset : s_VertexAttributes_lp[7].m_uiOffset;
}

WUInt32 WMeshVertexStreamConfig::GetBoneWeightsDataOffset() const
{
  W_ASSERT_DEBUG(s_VertexAttributes_lp[8].m_eSemantic == WGALVertexAttributeSemantic::BoneWeights0 && s_VertexAttributes_hp[8].m_eSemantic == WGALVertexAttributeSemantic::BoneWeights0, "");
  return m_bUseHighPrecision ? s_VertexAttributes_hp[8].m_uiOffset : s_VertexAttributes_lp[8].m_uiOffset;
}

WUInt32 WMeshVertexStreamConfig::GetStreamElementSize(WMeshVertexStreamType::Enum type) const
{
  return m_bUseHighPrecision ? s_StreamSizes_hp[type] : s_StreamSizes_lp[type];
}

// static
WGALVertexAttribute WMeshVertexStreamConfig::GetDataOffsetsVertexAttribute()
{
  W_ASSERT_DEBUG(s_VertexAttributes_lp[9].m_eSemantic == WGALVertexAttributeSemantic::DataOffsets, "");
  return s_VertexAttributes_lp[9];
}

WArrayPtr<WGALVertexAttribute> WMeshVertexStreamConfig::GetAllVertexAttributes()
{
  return WMakeArrayPtr(m_bUseHighPrecision ? s_VertexAttributes_hp : s_VertexAttributes_lp);
}

////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshBufferResource, 1, WRTTIDefaultAllocator<WMeshBufferResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WMeshBufferResource);
// clang-format on

WMeshBufferResourceDescriptor::WMeshBufferResourceDescriptor() = default;
WMeshBufferResourceDescriptor::~WMeshBufferResourceDescriptor() = default;

void WMeshBufferResourceDescriptor::Clear()
{
  m_Topology = WGALPrimitiveTopology::Triangles;
  m_uiVertexSize = 0;
  m_uiVertexCount = 0;
  m_VertexStreamConfig = WMeshVertexStreamConfig();
  m_VertexStreamsData.Clear();
  m_IndexBufferData.Clear();
}

void WMeshBufferResourceDescriptor::AddStream(WMeshVertexStreamType::Enum type, bool bUseHighPrecision /*= false*/)
{
  W_ASSERT_DEV(m_VertexStreamsData.IsEmpty(), "This function can only be called before 'AllocateStreams' is called");

  m_VertexStreamConfig.AddStream(type);
  m_VertexStreamConfig.m_bUseHighPrecision |= bUseHighPrecision;
}

void WMeshBufferResourceDescriptor::AddCommonStreams(bool bUseHighPrecision /*= false*/)
{
  AddStream(WMeshVertexStreamType::Position, bUseHighPrecision);
  AddStream(WMeshVertexStreamType::NormalTangentAndTexCoord0, bUseHighPrecision);
}

void WMeshBufferResourceDescriptor::AddStreamConfig(const WMeshVertexStreamConfig& streamConfig)
{
  W_ASSERT_DEV(m_VertexStreamsData.IsEmpty(), "This function can only be called before 'AllocateStreams' is called");

  m_VertexStreamConfig.m_uiTypesMask |= streamConfig.m_uiTypesMask;
  m_VertexStreamConfig.m_bUseHighPrecision |= streamConfig.m_bUseHighPrecision;
}

void WMeshBufferResourceDescriptor::AllocateStreams(WUInt32 uiNumVertices, WGALPrimitiveTopology::Enum topology, WUInt32 uiNumPrimitives, bool bZeroFill /*= false*/)
{
  W_ASSERT_DEV(m_VertexStreamConfig.m_uiTypesMask != 0, "You have to add streams via 'AddStream' before calling this function");

  m_Topology = topology;
  m_uiVertexCount = uiNumVertices;
  m_uiVertexSize = 0;

  const WUInt32 uiHighestStreamIndex = m_VertexStreamConfig.GetHighestStreamIndex();
  W_ASSERT_DEV(uiHighestStreamIndex < WMeshVertexStreamType::DataOffsets, "Data Offsets stream is reserved for internal use only");

  m_VertexStreamsData.SetCount(uiHighestStreamIndex + 1);

  for (WUInt32 uiIndex = WMeshVertexStreamType::Position; uiIndex < WMeshVertexStreamType::Count; ++uiIndex)
  {
    auto type = static_cast<WMeshVertexStreamType::Enum>(uiIndex);
    if (!m_VertexStreamConfig.HasStream(type))
      continue;

    const WUInt32 uiStreamElementSize = m_VertexStreamConfig.GetStreamElementSize(type);
    if (bZeroFill)
    {
      m_VertexStreamsData[uiIndex].SetCount(m_uiVertexCount * uiStreamElementSize);
    }
    else
    {
      m_VertexStreamsData[uiIndex].SetCountUninitialized(m_uiVertexCount * uiStreamElementSize);
    }

    m_uiVertexSize += uiStreamElementSize;
  }

  if (uiNumPrimitives > 0)
  {
    // use an index buffer at all
    WUInt32 uiIndexBufferSize = WGALPrimitiveTopology::GetIndexCount(topology, uiNumPrimitives);

    if (Uses32BitIndices())
    {
      uiIndexBufferSize *= sizeof(WUInt32);
    }
    else
    {
      uiIndexBufferSize *= sizeof(WUInt16);
    }

    m_IndexBufferData.SetCountUninitialized(uiIndexBufferSize);
  }
}

void WMeshBufferResourceDescriptor::AllocateStreamsFromGeometry(const WGeometry& geom, WGALPrimitiveTopology::Enum topology)
{
  WLogBlock _("Allocate Streams From Geometry");

  // Index Buffer Generation
  WDynamicArray<WUInt32> Indices;

  if (topology == WGALPrimitiveTopology::Points)
  {
    // Leaving indices empty disables indexed rendering.
  }
  else if (topology == WGALPrimitiveTopology::Lines)
  {
    Indices.Reserve(geom.GetLines().GetCount() * 2);

    for (WUInt32 p = 0; p < geom.GetLines().GetCount(); ++p)
    {
      Indices.PushBack(geom.GetLines()[p].m_uiStartVertex);
      Indices.PushBack(geom.GetLines()[p].m_uiEndVertex);
    }
  }
  else if (topology == WGALPrimitiveTopology::Triangles)
  {
    Indices.Reserve(geom.GetPolygons().GetCount() * 6);

    for (WUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      for (WUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
      {
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
      }
    }
  }
  AllocateStreams(geom.GetVertices().GetCount(), topology, Indices.GetCount() / (topology + 1), true);

  // Fill vertex buffer.
  {
    if (m_VertexStreamConfig.HasPosition())
    {
      for (WUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        SetPosition(v, geom.GetVertices()[v].m_vPosition);
      }
    }

    if (m_VertexStreamConfig.HasNormalTangentAndTexCoord0())
    {
      for (WUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        auto& vert = geom.GetVertices()[v];

        SetNormal(v, vert.m_vNormal);
        SetTangent(v, vert.m_vTangent.GetAsVec4(vert.m_fBiTangentSign));
        SetTexCoord0(v, vert.m_vTexCoord);
      }
    }

    if (m_VertexStreamConfig.HasTexCoord1())
    {
      for (WUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        SetTexCoord1(v, geom.GetVertices()[v].m_vTexCoord);
      }
    }

    if (m_VertexStreamConfig.HasColor0())
    {
      for (WUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        SetColor0(v, geom.GetVertices()[v].m_Color);
      }
    }

    if (m_VertexStreamConfig.HasColor1())
    {
      for (WUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        SetColor0(v, geom.GetVertices()[v].m_Color);
      }
    }

    if (m_VertexStreamConfig.HasSkinningData())
    {
      for (WUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        auto& vert = geom.GetVertices()[v];

        SetBoneIndices(v, vert.m_BoneIndices);
        SetBoneWeights(v, WColor(vert.m_BoneWeights).GetAsVec4());
      }
    }
  }

  // Fill index buffer.
  {
    if (topology == WGALPrimitiveTopology::Points)
    {
      for (WUInt32 t = 0; t < Indices.GetCount(); t += 1)
      {
        SetPointIndices(t, Indices[t]);
      }
    }
    else if (topology == WGALPrimitiveTopology::Triangles)
    {
      for (WUInt32 t = 0; t < Indices.GetCount(); t += 3)
      {
        SetTriangleIndices(t / 3, Indices[t], Indices[t + 1], Indices[t + 2]);
      }
    }
    else if (topology == WGALPrimitiveTopology::Lines)
    {
      for (WUInt32 t = 0; t < Indices.GetCount(); t += 2)
      {
        SetLineIndices(t / 2, Indices[t], Indices[t + 1]);
      }
    }
  }
}

WUInt32 WMeshBufferResourceDescriptor::GetNumVertexBuffers() const
{
  return m_VertexStreamsData.GetCount();
}

WArrayPtr<const WUInt8> WMeshBufferResourceDescriptor::GetVertexBufferData(WMeshVertexStreamType::Enum type) const
{
  return m_VertexStreamsData[type].GetArrayPtr();
}

WArrayPtr<const WUInt8> WMeshBufferResourceDescriptor::GetIndexBufferData() const
{
  return m_IndexBufferData.GetArrayPtr();
}

WDynamicArray<WUInt8, WAlignedAllocatorWrapper>& WMeshBufferResourceDescriptor::GetVertexBufferData(WMeshVertexStreamType::Enum type)
{
  return m_VertexStreamsData[type];
}

WDynamicArray<WUInt8, WAlignedAllocatorWrapper>& WMeshBufferResourceDescriptor::GetIndexBufferData()
{
  return m_IndexBufferData;
}

WArrayPtr<const WVec3> WMeshBufferResourceDescriptor::GetPositionData() const
{
  auto data = m_VertexStreamsData[WMeshVertexStreamType::Position].GetArrayPtr();
  return WMakeArrayPtr(reinterpret_cast<const WVec3*>(data.GetPtr()), data.GetCount() / sizeof(WVec3));
}

WArrayPtr<WVec3> WMeshBufferResourceDescriptor::GetPositionData()
{
  auto data = m_VertexStreamsData[WMeshVertexStreamType::Position].GetArrayPtr();
  return WMakeArrayPtr(reinterpret_cast<WVec3*>(data.GetPtr()), data.GetCount() / sizeof(WVec3));
}

WArrayPtr<const WUInt8> WMeshBufferResourceDescriptor::GetNormalData(WUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetNormalDataOffset());
}

WArrayPtr<WUInt8> WMeshBufferResourceDescriptor::GetNormalData(WUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetNormalDataOffset());
}

WArrayPtr<const WUInt8> WMeshBufferResourceDescriptor::GetTangentData(WUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetTangentDataOffset());
}

WArrayPtr<WUInt8> WMeshBufferResourceDescriptor::GetTangentData(WUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetTangentDataOffset());
}

WArrayPtr<const WUInt8> WMeshBufferResourceDescriptor::GetTexCoord0Data(WUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetTexCoord0DataOffset());
}

WArrayPtr<WUInt8> WMeshBufferResourceDescriptor::GetTexCoord0Data(WUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetTexCoord0DataOffset());
}

WArrayPtr<const WUInt8> WMeshBufferResourceDescriptor::GetTexCoord1Data(WUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetTexCoord1ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::TexCoord1].GetArrayPtr();
  return data;
}

WArrayPtr<WUInt8> WMeshBufferResourceDescriptor::GetTexCoord1Data(WUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetTexCoord1ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::TexCoord1].GetArrayPtr();
  return data;
}

WArrayPtr<const WUInt8> WMeshBufferResourceDescriptor::GetColor0Data(WUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetColor0ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::Color0].GetArrayPtr();
  return data;
}

WArrayPtr<WUInt8> WMeshBufferResourceDescriptor::GetColor0Data(WUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetColor0ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::Color0].GetArrayPtr();
  return data;
}

WArrayPtr<const WUInt8> WMeshBufferResourceDescriptor::GetColor1Data(WUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetColor1ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::Color1].GetArrayPtr();
  return data;
}

WArrayPtr<WUInt8> WMeshBufferResourceDescriptor::GetColor1Data(WUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetColor1ElementSize();

  auto data = m_VertexStreamsData[WMeshVertexStreamType::Color1].GetArrayPtr();
  return data;
}

const WVec3& WMeshBufferResourceDescriptor::GetPosition(WUInt32 uiVertexIndex) const
{
  return reinterpret_cast<const WVec3&>(m_VertexStreamsData[WMeshVertexStreamType::Position][uiVertexIndex * sizeof(WVec3)]);
}

void WMeshBufferResourceDescriptor::SetPosition(WUInt32 uiVertexIndex, const WVec3& vPos)
{
  *reinterpret_cast<WVec3*>(&m_VertexStreamsData[WMeshVertexStreamType::Position][uiVertexIndex * sizeof(WVec3)]) = vPos;
}

WVec3 WMeshBufferResourceDescriptor::GetNormal(WUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(WMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetNormalDataOffset());

  WVec3 res;
  WMeshBufferUtils::DecodeNormal(data, m_VertexStreamConfig.GetNormalFormat(), res).AssertSuccess();

  return res;
}

void WMeshBufferResourceDescriptor::SetNormal(WUInt32 uiVertexIndex, const WVec3& vNormal)
{
  auto data = GetVertexData(WMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetNormalDataOffset());

  WMeshBufferUtils::EncodeNormal(vNormal, data, m_VertexStreamConfig.GetNormalFormat()).AssertSuccess();
}

WVec4 WMeshBufferResourceDescriptor::GetTangent(WUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(WMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetTangentDataOffset());

  WVec3 vTangent = WVec3::MakeZero();
  float fBiTangentSign = 0.0f;
  WMeshBufferUtils::DecodeTangent(data, m_VertexStreamConfig.GetTangentFormat(), vTangent, fBiTangentSign).AssertSuccess();

  return vTangent.GetAsVec4(fBiTangentSign);
}

void WMeshBufferResourceDescriptor::SetTangent(WUInt32 uiVertexIndex, const WVec4& vTangent)
{
  auto data = GetVertexData(WMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetTangentDataOffset());

  WMeshBufferUtils::EncodeTangent(vTangent.GetAsVec3(), vTangent.w, data, m_VertexStreamConfig.GetTangentFormat()).AssertSuccess();
}

WVec2 WMeshBufferResourceDescriptor::GetTexCoord0(WUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(WMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetTexCoord0DataOffset());

  WVec2 res;
  WMeshBufferUtils::DecodeTexCoord(data, m_VertexStreamConfig.GetTexCoordFormat(), res).AssertSuccess();

  return res;
}

void WMeshBufferResourceDescriptor::SetTexCoord0(WUInt32 uiVertexIndex, const WVec2& vTexCoord)
{
  auto data = GetVertexData(WMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetTexCoord0DataOffset());

  WMeshBufferUtils::EncodeTexCoord(vTexCoord, data, m_VertexStreamConfig.GetTexCoordFormat()).AssertSuccess();
}

WVec2 WMeshBufferResourceDescriptor::GetTexCoord1(WUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(WMeshVertexStreamType::TexCoord1, uiVertexIndex, m_VertexStreamConfig.GetTexCoord1ElementSize());

  WVec2 res;
  WMeshBufferUtils::DecodeTexCoord(data, m_VertexStreamConfig.GetTexCoordFormat(), res).AssertSuccess();

  return res;
}

void WMeshBufferResourceDescriptor::SetTexCoord1(WUInt32 uiVertexIndex, const WVec2& vTexCoord)
{
  auto data = GetVertexData(WMeshVertexStreamType::TexCoord1, uiVertexIndex, m_VertexStreamConfig.GetTexCoord1ElementSize());

  WMeshBufferUtils::EncodeTexCoord(vTexCoord, data, m_VertexStreamConfig.GetTexCoordFormat()).AssertSuccess();
}

WColor WMeshBufferResourceDescriptor::GetColor0(WUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(WMeshVertexStreamType::Color0, uiVertexIndex, m_VertexStreamConfig.GetColor0ElementSize());

  WColor res;
  WMeshBufferUtils::DecodeColor(data, m_VertexStreamConfig.GetColorFormat(), res).AssertSuccess();

  return res;
}

void WMeshBufferResourceDescriptor::SetColor0(WUInt32 uiVertexIndex, const WColorLinearUB& color)
{
  auto data = GetVertexData(WMeshVertexStreamType::Color0, uiVertexIndex, m_VertexStreamConfig.GetColor0ElementSize());

  if (m_VertexStreamConfig.m_bUseHighPrecision)
  {
    WMeshBufferUtils::EncodeColor(color, data, m_VertexStreamConfig.GetColorFormat(), WMeshVertexColorConversion::None).AssertSuccess();
  }
  else
  {
    *reinterpret_cast<WColorLinearUB*>(data.GetPtr()) = color;
  }
}

void WMeshBufferResourceDescriptor::SetColor0(WUInt32 uiVertexIndex, const WColor& color, WMeshVertexColorConversion::Enum conversion /*= WMeshVertexColorConversion::Default*/)
{
  auto data = GetVertexData(WMeshVertexStreamType::Color0, uiVertexIndex, m_VertexStreamConfig.GetColor0ElementSize());

  WMeshBufferUtils::EncodeColor(color, data, m_VertexStreamConfig.GetColorFormat(), conversion).AssertSuccess();
}

WColor WMeshBufferResourceDescriptor::GetColor1(WUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(WMeshVertexStreamType::Color1, uiVertexIndex, m_VertexStreamConfig.GetColor1ElementSize());

  WColor res;
  WMeshBufferUtils::DecodeColor(data, m_VertexStreamConfig.GetColorFormat(), res).AssertSuccess();

  return res;
}

void WMeshBufferResourceDescriptor::SetColor1(WUInt32 uiVertexIndex, const WColorLinearUB& color)
{
  auto data = GetVertexData(WMeshVertexStreamType::Color1, uiVertexIndex, m_VertexStreamConfig.GetColor1ElementSize());

  if (m_VertexStreamConfig.m_bUseHighPrecision)
  {
    WMeshBufferUtils::EncodeColor(color, data, m_VertexStreamConfig.GetColorFormat(), WMeshVertexColorConversion::None).AssertSuccess();
  }
  else
  {
    *reinterpret_cast<WColorLinearUB*>(data.GetPtr()) = color;
  }
}

void WMeshBufferResourceDescriptor::SetColor1(WUInt32 uiVertexIndex, const WColor& color, WMeshVertexColorConversion::Enum conversion /*= WMeshVertexColorConversion::Default*/)
{
  auto data = GetVertexData(WMeshVertexStreamType::Color1, uiVertexIndex, m_VertexStreamConfig.GetColor1ElementSize());

  WMeshBufferUtils::EncodeColor(color, data, m_VertexStreamConfig.GetColorFormat(), conversion).AssertSuccess();
}

const WVec4U16& WMeshBufferResourceDescriptor::GetBoneIndices(WUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(WMeshVertexStreamType::SkinningData, uiVertexIndex, m_VertexStreamConfig.GetSkinningDataElementSize(), m_VertexStreamConfig.GetBoneIndicesDataOffset());

  return *reinterpret_cast<const WVec4U16*>(data.GetPtr());
}

void WMeshBufferResourceDescriptor::SetBoneIndices(WUInt32 uiVertexIndex, const WVec4U16& vIndices)
{
  auto data = GetVertexData(WMeshVertexStreamType::SkinningData, uiVertexIndex, m_VertexStreamConfig.GetSkinningDataElementSize(), m_VertexStreamConfig.GetBoneIndicesDataOffset());

  *reinterpret_cast<WVec4U16*>(data.GetPtr()) = vIndices;
}

WVec4 WMeshBufferResourceDescriptor::GetBoneWeights(WUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(WMeshVertexStreamType::SkinningData, uiVertexIndex, m_VertexStreamConfig.GetSkinningDataElementSize(), m_VertexStreamConfig.GetBoneWeightsDataOffset());

  WVec4 res;
  WMeshBufferUtils::DecodeBoneWeights(data, m_VertexStreamConfig.GetBoneWeightsFormat(), res).AssertSuccess();

  return res;
}

void WMeshBufferResourceDescriptor::SetBoneWeights(WUInt32 uiVertexIndex, const WVec4& vWeights)
{
  auto data = GetVertexData(WMeshVertexStreamType::SkinningData, uiVertexIndex, m_VertexStreamConfig.GetSkinningDataElementSize(), m_VertexStreamConfig.GetBoneWeightsDataOffset());

  WMeshBufferUtils::EncodeBoneWeights(vWeights, data, m_VertexStreamConfig.GetBoneWeightsFormat()).AssertSuccess();
}



void WMeshBufferResourceDescriptor::SetPointIndices(WUInt32 uiPoint, WUInt32 uiVertex0)
{
  W_ASSERT_DEBUG(m_Topology == WGALPrimitiveTopology::Points, "Wrong topology");

  if (Uses32BitIndices())
  {
    WUInt32* pIndices = reinterpret_cast<WUInt32*>(&m_IndexBufferData[uiPoint * sizeof(WUInt32) * 1]);
    pIndices[0] = uiVertex0;
  }
  else
  {
    WUInt16* pIndices = reinterpret_cast<WUInt16*>(&m_IndexBufferData[uiPoint * sizeof(WUInt16) * 1]);
    pIndices[0] = static_cast<WUInt16>(uiVertex0);
  }
}

void WMeshBufferResourceDescriptor::SetLineIndices(WUInt32 uiLine, WUInt32 uiVertex0, WUInt32 uiVertex1)
{
  W_ASSERT_DEBUG(m_Topology == WGALPrimitiveTopology::Lines, "Wrong topology");

  if (Uses32BitIndices())
  {
    WUInt32* pIndices = reinterpret_cast<WUInt32*>(&m_IndexBufferData[uiLine * sizeof(WUInt32) * 2]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
  }
  else
  {
    WUInt16* pIndices = reinterpret_cast<WUInt16*>(&m_IndexBufferData[uiLine * sizeof(WUInt16) * 2]);
    pIndices[0] = static_cast<WUInt16>(uiVertex0);
    pIndices[1] = static_cast<WUInt16>(uiVertex1);
  }
}

void WMeshBufferResourceDescriptor::SetTriangleIndices(WUInt32 uiTriangle, WUInt32 uiVertex0, WUInt32 uiVertex1, WUInt32 uiVertex2)
{
  W_ASSERT_DEBUG(m_Topology == WGALPrimitiveTopology::Triangles, "Wrong topology");
  W_ASSERT_DEBUG(uiVertex0 < m_uiVertexCount && uiVertex1 < m_uiVertexCount && uiVertex2 < m_uiVertexCount, "Vertex indices out of range.");

  if (Uses32BitIndices())
  {
    WUInt32* pIndices = reinterpret_cast<WUInt32*>(&m_IndexBufferData[uiTriangle * sizeof(WUInt32) * 3]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
    pIndices[2] = uiVertex2;
  }
  else
  {
    WUInt16* pIndices = reinterpret_cast<WUInt16*>(&m_IndexBufferData[uiTriangle * sizeof(WUInt16) * 3]);
    pIndices[0] = static_cast<WUInt16>(uiVertex0);
    pIndices[1] = static_cast<WUInt16>(uiVertex1);
    pIndices[2] = static_cast<WUInt16>(uiVertex2);
  }
}

WUInt32 WMeshBufferResourceDescriptor::GetPrimitiveCount() const
{
  const WUInt32 divider = m_Topology + 1;

  if (!m_IndexBufferData.IsEmpty())
  {
    if (Uses32BitIndices())
      return (m_IndexBufferData.GetCount() / sizeof(WUInt32)) / divider;
    else
      return (m_IndexBufferData.GetCount() / sizeof(WUInt16)) / divider;
  }
  else
  {
    return m_uiVertexCount / divider;
  }
}

WBoundingBoxSphere WMeshBufferResourceDescriptor::ComputeBounds() const
{
  WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

  if (m_VertexStreamConfig.HasPosition() && !m_VertexStreamsData.IsEmpty() && m_uiVertexCount > 0)
  {
    const WVec3* pPositions = GetPositionData().GetPtr();
    bounds = WBoundingBoxSphere::MakeFromPoints(pPositions, m_uiVertexCount);
  }

  if (!bounds.IsValid())
  {
    bounds = WBoundingBoxSphere::MakeFromCenterExtents(WVec3::MakeZero(), WVec3(0.1f), 0.1f);
  }

  return bounds;
}

WResult WMeshBufferResourceDescriptor::RecomputeNormals()
{
  if (m_Topology != WGALPrimitiveTopology::Triangles)
    return W_FAILURE; // normals not needed

  if (!m_VertexStreamConfig.HasPosition() || !m_VertexStreamConfig.HasNormal())
    return W_FAILURE; // there are no normals that could be recomputed

  const WUInt32 uiVertexSize = m_uiVertexSize;
  const WVec3* pPositions = GetPositionData().GetPtr();

  WDynamicArray<WVec3> newNormals;
  newNormals.SetCountUninitialized(m_uiVertexCount);

  for (auto& n : newNormals)
  {
    n.SetZero();
  }

  WResult res = W_SUCCESS;

  const WUInt16* pIndices16 = reinterpret_cast<const WUInt16*>(m_IndexBufferData.GetData());
  const WUInt32* pIndices32 = reinterpret_cast<const WUInt32*>(m_IndexBufferData.GetData());
  const bool bUseIndices32 = Uses32BitIndices();

  // Compute unnormalized triangle normals and add them to all vertices.
  // This way large triangles have an higher influence on the vertex normal.
  for (WUInt32 triIdx = 0; triIdx < GetPrimitiveCount(); ++triIdx)
  {
    const WUInt32 v0 = bUseIndices32 ? pIndices32[triIdx * 3 + 0] : pIndices16[triIdx * 3 + 0];
    const WUInt32 v1 = bUseIndices32 ? pIndices32[triIdx * 3 + 1] : pIndices16[triIdx * 3 + 1];
    const WUInt32 v2 = bUseIndices32 ? pIndices32[triIdx * 3 + 2] : pIndices16[triIdx * 3 + 2];

    const WVec3 p0 = pPositions[v0];
    const WVec3 p1 = pPositions[v1];
    const WVec3 p2 = pPositions[v2];

    const WVec3 d01 = p1 - p0;
    const WVec3 d02 = p2 - p0;

    const WVec3 triNormal = d01.CrossRH(d02);

    if (triNormal.IsValid())
    {
      newNormals[v0] += triNormal;
      newNormals[v1] += triNormal;
      newNormals[v2] += triNormal;
    }
  }

  for (WUInt32 i = 0; i < newNormals.GetCount(); ++i)
  {
    // normalize the new normal
    if (newNormals[i].NormalizeIfNotZero(WVec3::MakeAxisX()).Failed())
      res = W_FAILURE;

    SetNormal(i, newNormals[i]);
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WMeshBufferResource::WMeshBufferResource()
  : WResource(DoUpdate::OnGraphicsResourceThreads, 1)
{
}

WMeshBufferResource::~WMeshBufferResource()
{
  for (auto hVertexBuffer : m_hVertexBuffers)
  {
    W_ASSERT_DEBUG(hVertexBuffer.IsInvalidated(), "Implementation error");
  }
  W_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
}

WResourceLoadDesc WMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  for (auto& hVertexBuffer : m_hVertexBuffers)
  {
    WGALDevice::GetDefaultDevice()->DestroyBuffer(hVertexBuffer);
  }

  WGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);

  m_uiPrimitiveCount = 0;

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WMeshBufferResource::UpdateContent(WStreamReader* Stream)
{
  W_REPORT_FAILURE("This resource type does not support loading data from file.");

  return WResourceLoadDesc();
}

void WMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WMeshBufferResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WMeshBufferResource, WMeshBufferResourceDescriptor)
{
  for (auto hVertexBuffer : m_hVertexBuffers)
  {
    W_ASSERT_DEBUG(hVertexBuffer.IsInvalidated(), "Implementation error");
  }
  W_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");

  m_VertexStreamConfig = descriptor.GetVertexStreamConfig();
  m_VertexStreamConfig.FillVertexAttributes(m_VertexAttributes);
  m_uiPrimitiveCount = descriptor.GetPrimitiveCount();
  m_Topology = descriptor.GetTopology();

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WStringBuilder sName;

  for (WUInt32 uiIndex : WIterateBitIndices(m_VertexStreamConfig.m_uiTypesMask))
  {
    auto type = static_cast<WMeshVertexStreamType::Enum>(uiIndex);
    const WUInt32 uiElementSize = m_VertexStreamConfig.GetStreamElementSize(type);

    m_hVertexBuffers[uiIndex] = pDevice->CreateVertexBuffer(uiElementSize, descriptor.GetVertexCount(), descriptor.GetVertexBufferData(type));

    sName.SetFormat("{0} Vertex Buffer {1}", GetResourceIdOrDescription(), WMeshVertexStreamType::GetName(type));
    pDevice->GetBuffer(m_hVertexBuffers[uiIndex])->SetDebugName(sName);
  }

  WUInt32 uiIndexBufferSize = 0;
  if (descriptor.HasIndexBuffer())
  {
    const WUInt32 uiIndexCount = WGALPrimitiveTopology::GetIndexCount(m_Topology, m_uiPrimitiveCount);
    m_hIndexBuffer = pDevice->CreateIndexBuffer(descriptor.Uses32BitIndices() ? WGALIndexType::UInt : WGALIndexType::UShort, uiIndexCount, descriptor.GetIndexBufferData());

    sName.SetFormat("{0} Index Buffer", GetResourceIdOrDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);

    uiIndexBufferSize = descriptor.GetIndexBufferData().GetCount();
  }

  {
    // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
    ModifyMemoryUsage().m_uiMemoryGPU = (descriptor.GetVertexDataSize() * descriptor.GetVertexCount()) + uiIndexBufferSize;
  }

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  m_Bounds = descriptor.ComputeBounds();

  return res;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferResource);
