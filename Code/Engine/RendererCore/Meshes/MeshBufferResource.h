#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

using WMeshBufferResourceHandle = WTypedResourceHandle<class WMeshBufferResource>;
class WGeometry;

struct WMeshVertexStreamType
{
  using StorageType = WUInt8;

  enum Enum
  {
    Position,
    NormalTangentAndTexCoord0,
    TexCoord1,
    Color0,
    Color1,
    SkinningData,

    DataOffsets,

    Count,

    Default = Position
  };

  static const char* GetName(Enum type);
};

struct W_RENDERERCORE_DLL WMeshVertexStreamConfig
{
  WUInt16 m_uiTypesMask = 0;
  bool m_bUseHighPrecision = false;

  W_ALWAYS_INLINE void AddStream(WMeshVertexStreamType::Enum type) { m_uiTypesMask |= W_BIT(type); }
  W_ALWAYS_INLINE bool HasStream(WMeshVertexStreamType::Enum type) const { return (m_uiTypesMask & W_BIT(type)) != 0; }

  W_ALWAYS_INLINE bool HasPosition() const { return HasStream(WMeshVertexStreamType::Position); }
  W_ALWAYS_INLINE bool HasNormalTangentAndTexCoord0() const { return HasStream(WMeshVertexStreamType::NormalTangentAndTexCoord0); }
  W_ALWAYS_INLINE bool HasNormal() const { return HasStream(WMeshVertexStreamType::NormalTangentAndTexCoord0); }
  W_ALWAYS_INLINE bool HasTangent() const { return HasStream(WMeshVertexStreamType::NormalTangentAndTexCoord0); }
  W_ALWAYS_INLINE bool HasTexCoord0() const { return HasStream(WMeshVertexStreamType::NormalTangentAndTexCoord0); }
  W_ALWAYS_INLINE bool HasTexCoord1() const { return HasStream(WMeshVertexStreamType::TexCoord1); }
  W_ALWAYS_INLINE bool HasColor0() const { return HasStream(WMeshVertexStreamType::Color0); }
  W_ALWAYS_INLINE bool HasColor1() const { return HasStream(WMeshVertexStreamType::Color1); }
  W_ALWAYS_INLINE bool HasSkinningData() const { return HasStream(WMeshVertexStreamType::SkinningData); }
  W_ALWAYS_INLINE bool HasBoneIndices() const { return HasStream(WMeshVertexStreamType::SkinningData); }
  W_ALWAYS_INLINE bool HasBoneWeights() const { return HasStream(WMeshVertexStreamType::SkinningData); }

  W_ALWAYS_INLINE WUInt32 GetHighestStreamIndex() const { return WMath::FirstBitHigh(WUInt32(m_uiTypesMask)); }

  template <WUInt16 ArraySize>
  void FillVertexAttributes(WSmallArray<WGALVertexAttribute, ArraySize>& out_vertexAttributes)
  {
    for (auto vertexAttribute : GetAllVertexAttributes())
    {
      if ((m_uiTypesMask & W_BIT(vertexAttribute.m_uiVertexBufferSlot)) != 0)
      {
        out_vertexAttributes.PushBack(vertexAttribute);
      }
    }
  }

  WGALResourceFormat::Enum GetPositionFormat() const;
  WGALResourceFormat::Enum GetNormalFormat() const;
  WGALResourceFormat::Enum GetTangentFormat() const;
  WGALResourceFormat::Enum GetTexCoordFormat() const;
  WGALResourceFormat::Enum GetColorFormat() const;
  WGALResourceFormat::Enum GetBoneIndicesFormat() const;
  WGALResourceFormat::Enum GetBoneWeightsFormat() const;

  WUInt32 GetNormalDataOffset() const;
  WUInt32 GetTangentDataOffset() const;
  WUInt32 GetTexCoord0DataOffset() const;
  WUInt32 GetBoneIndicesDataOffset() const;
  WUInt32 GetBoneWeightsDataOffset() const;

  WUInt32 GetStreamElementSize(WMeshVertexStreamType::Enum type) const;

  W_ALWAYS_INLINE WUInt32 GetPositionElementSize() const { return GetStreamElementSize(WMeshVertexStreamType::Position); }
  W_ALWAYS_INLINE WUInt32 GetNormalTangentAndTexCoord0ElementSize() const { return GetStreamElementSize(WMeshVertexStreamType::NormalTangentAndTexCoord0); }
  W_ALWAYS_INLINE WUInt32 GetTexCoord1ElementSize() const { return GetStreamElementSize(WMeshVertexStreamType::TexCoord1); }
  W_ALWAYS_INLINE WUInt32 GetColor0ElementSize() const { return GetStreamElementSize(WMeshVertexStreamType::Color0); }
  W_ALWAYS_INLINE WUInt32 GetColor1ElementSize() const { return GetStreamElementSize(WMeshVertexStreamType::Color1); }
  W_ALWAYS_INLINE WUInt32 GetSkinningDataElementSize() const { return GetStreamElementSize(WMeshVertexStreamType::SkinningData); }

  static WGALVertexAttribute GetDataOffsetsVertexAttribute();

private:
  WArrayPtr<WGALVertexAttribute> GetAllVertexAttributes();
};

struct W_RENDERERCORE_DLL WMeshBufferResourceDescriptor
{
public:
  WMeshBufferResourceDescriptor();
  ~WMeshBufferResourceDescriptor();

  void Clear();

  /// Use this function to add vertex streams to the mesh buffer.
  void AddStream(WMeshVertexStreamType::Enum type, bool bUseHighPrecision = false);

  /// Adds common vertex streams to the mesh buffer.
  ///
  /// The streams are added
  /// * Position
  /// * NormalTangentAndTexCoord0
  void AddCommonStreams(bool bUseHighPrecision = false);

  /// Adds all streams from the given stream config.
  void AddStreamConfig(const WMeshVertexStreamConfig& streamConfig);

  /// After all streams are added, call this to allocate the data for the streams. If uiNumPrimitives is 0, the mesh buffer will not
  /// use indexed rendering.
  void AllocateStreams(WUInt32 uiNumVertices, WGALPrimitiveTopology::Enum topology = WGALPrimitiveTopology::Triangles, WUInt32 uiNumPrimitives = 0, bool bZeroFill = false);

  /// Creates streams and fills them with data from the WGeometry. Only the geometry matching the given topology is used.
  ///  Streams that do not match any of the data inside the WGeometry directly are skipped.
  void AllocateStreamsFromGeometry(const WGeometry& geom, WGALPrimitiveTopology::Enum topology = WGALPrimitiveTopology::Triangles);


  /// Returns the number of vertex buffer used. Note that some vertex buffers in between might be empty if unused.
  WUInt32 GetNumVertexBuffers() const;

  /// Gives read access to the allocated vertex data
  WArrayPtr<const WUInt8> GetVertexBufferData(WMeshVertexStreamType::Enum type) const;

  /// Gives read access to the allocated index data
  WArrayPtr<const WUInt8> GetIndexBufferData() const;

  /// Allows write access to the allocated vertex data. This can be used for copying data fast into the array.
  WDynamicArray<WUInt8, WAlignedAllocatorWrapper>& GetVertexBufferData(WMeshVertexStreamType::Enum type);

  /// Allows write access to the allocated index data. This can be used for copying data fast into the array.
  WDynamicArray<WUInt8, WAlignedAllocatorWrapper>& GetIndexBufferData();


  /// Gives access to the position data
  WArrayPtr<const WVec3> GetPositionData() const;
  WArrayPtr<WVec3> GetPositionData();

  /// Gives access to the normal data. Use WMeshBufferUtils::EncodeNormal/WMeshBufferUtils::DecodeNormal to pack/unpack the normal.
  WArrayPtr<const WUInt8> GetNormalData(WUInt32* out_pStride = nullptr) const;
  WArrayPtr<WUInt8> GetNormalData(WUInt32* out_pStride = nullptr);

  /// Gives access to the tangent data. Use WMeshBufferUtils::EncodeTangent/WMeshBufferUtils::DecodeTangent to pack/unpack the tangent.
  WArrayPtr<const WUInt8> GetTangentData(WUInt32* out_pStride = nullptr) const;
  WArrayPtr<WUInt8> GetTangentData(WUInt32* out_pStride = nullptr);

  /// Gives access to the tex coord 0 data. Use WMeshBufferUtils::EncodeTexCoord/WMeshBufferUtils::DecodeTexCoord to pack/unpack the tex coord.
  WArrayPtr<const WUInt8> GetTexCoord0Data(WUInt32* out_pStride = nullptr) const;
  WArrayPtr<WUInt8> GetTexCoord0Data(WUInt32* out_pStride = nullptr);

  /// Gives access to the tex coord 1 data. Use WMeshBufferUtils::EncodeTexCoord/WMeshBufferUtils::DecodeTexCoord to pack/unpack the tex coord.
  WArrayPtr<const WUInt8> GetTexCoord1Data(WUInt32* out_pStride = nullptr) const;
  WArrayPtr<WUInt8> GetTexCoord1Data(WUInt32* out_pStride = nullptr);

  /// Gives access to the color 0 data. Use WMeshBufferUtils::EncodeFromVec4/WMeshBufferUtils::DecodeToVec4 to pack/unpack the color.
  WArrayPtr<const WUInt8> GetColor0Data(WUInt32* out_pStride = nullptr) const;
  WArrayPtr<WUInt8> GetColor0Data(WUInt32* out_pStride = nullptr);

  /// Gives access to the color 1 data. Use WMeshBufferUtils::EncodeFromVec4/WMeshBufferUtils::DecodeToVec4 to pack/unpack the color.
  WArrayPtr<const WUInt8> GetColor1Data(WUInt32* out_pStride = nullptr) const;
  WArrayPtr<WUInt8> GetColor1Data(WUInt32* out_pStride = nullptr);


  /// Slow, but convenient access to the position of a specific vertex.
  const WVec3& GetPosition(WUInt32 uiVertexIndex) const;
  void SetPosition(WUInt32 uiVertexIndex, const WVec3& vPos);

  /// Slow, but convenient access to the normal of a specific vertex.
  WVec3 GetNormal(WUInt32 uiVertexIndex) const;
  void SetNormal(WUInt32 uiVertexIndex, const WVec3& vNormal);

  /// Slow, but convenient access to the tangent of a specific vertex. The w component contains the bi-tangent sign.
  WVec4 GetTangent(WUInt32 uiVertexIndex) const;
  void SetTangent(WUInt32 uiVertexIndex, const WVec4& vTangent);

  /// Slow, but convenient access to the tex coord 0 of a specific vertex.
  WVec2 GetTexCoord0(WUInt32 uiVertexIndex) const;
  void SetTexCoord0(WUInt32 uiVertexIndex, const WVec2& vTexCoord);

  /// Slow, but convenient access to the tex coord 1 of a specific vertex.
  WVec2 GetTexCoord1(WUInt32 uiVertexIndex) const;
  void SetTexCoord1(WUInt32 uiVertexIndex, const WVec2& vTexCoord);

  /// Slow, but convenient access to the color 0 of a specific vertex.
  WColor GetColor0(WUInt32 uiVertexIndex) const;
  void SetColor0(WUInt32 uiVertexIndex, const WColorLinearUB& color);
  void SetColor0(WUInt32 uiVertexIndex, const WColor& color, WMeshVertexColorConversion::Enum conversion = WMeshVertexColorConversion::Default);

  /// Slow, but convenient access to the color 1 of a specific vertex.
  WColor GetColor1(WUInt32 uiVertexIndex) const;
  void SetColor1(WUInt32 uiVertexIndex, const WColorLinearUB& color);
  void SetColor1(WUInt32 uiVertexIndex, const WColor& color, WMeshVertexColorConversion::Enum conversion = WMeshVertexColorConversion::Default);

  /// Slow, but convenient access to the bone indices of a specific vertex.
  const WVec4U16& GetBoneIndices(WUInt32 uiVertexIndex) const;
  void SetBoneIndices(WUInt32 uiVertexIndex, const WVec4U16& vIndices);

  /// Slow, but convenient access to the bone weights of a specific vertex.
  WVec4 GetBoneWeights(WUInt32 uiVertexIndex) const;
  void SetBoneWeights(WUInt32 uiVertexIndex, const WVec4& vWeights);


  /// Writes the vertex index for the given point into the index buffer.
  void SetPointIndices(WUInt32 uiPoint, WUInt32 uiVertex0);

  /// Writes the two vertex indices for the given line into the index buffer.
  void SetLineIndices(WUInt32 uiLine, WUInt32 uiVertex0, WUInt32 uiVertex1);

  /// Writes the three vertex indices for the given triangle into the index buffer.
  void SetTriangleIndices(WUInt32 uiTriangle, WUInt32 uiVertex0, WUInt32 uiVertex1, WUInt32 uiVertex2);


  /// Allows to read the stream info of the descriptor, which is filled out by AddStream()
  const WMeshVertexStreamConfig& GetVertexStreamConfig() const { return m_VertexStreamConfig; }

  /// Returns the byte size of all the data for one vertex.
  WUInt32 GetVertexDataSize() const { return m_uiVertexSize; }

  /// Return the number of vertices, with which AllocateStreams() was called.
  WUInt32 GetVertexCount() const { return m_uiVertexCount; }

  /// Returns the number of primitives that the array holds.
  WUInt32 GetPrimitiveCount() const;

  /// Returns whether 16 or 32 Bit indices are to be used.
  bool Uses32BitIndices() const { return m_uiVertexCount > 0xFFFF; }

  /// Returns whether an index buffer is available.
  bool HasIndexBuffer() const { return !m_IndexBufferData.IsEmpty(); }

  /// Calculates the bounds using the data from the position stream
  WBoundingBoxSphere ComputeBounds() const;

  /// Returns the primitive topology
  WGALPrimitiveTopology::Enum GetTopology() const { return m_Topology; }

  WResult RecomputeNormals();

private:
  W_ALWAYS_INLINE WByteArrayPtr GetVertexData(WMeshVertexStreamType::Enum type, WUInt32 uiVertexIndex, WUInt32 uiElementSize, WUInt32 uiOffset = 0)
  {
    return m_VertexStreamsData[type].GetArrayPtr().GetSubArray(uiVertexIndex * uiElementSize + uiOffset);
  }

  W_ALWAYS_INLINE WConstByteArrayPtr GetVertexData(WMeshVertexStreamType::Enum type, WUInt32 uiVertexIndex, WUInt32 uiElementSize, WUInt32 uiOffset = 0) const
  {
    return m_VertexStreamsData[type].GetArrayPtr().GetSubArray(uiVertexIndex * uiElementSize + uiOffset);
  }

  WGALPrimitiveTopology::Enum m_Topology = WGALPrimitiveTopology::Triangles;
  WUInt32 m_uiVertexSize = 0;
  WUInt32 m_uiVertexCount = 0;
  WMeshVertexStreamConfig m_VertexStreamConfig;
  WHybridArray<WDynamicArray<WUInt8, WAlignedAllocatorWrapper>, WMeshVertexStreamType::Count> m_VertexStreamsData;
  WDynamicArray<WUInt8, WAlignedAllocatorWrapper> m_IndexBufferData;
};

class W_RENDERERCORE_DLL WMeshBufferResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WMeshBufferResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WMeshBufferResource);
  W_RESOURCE_DECLARE_CREATEABLE(WMeshBufferResource, WMeshBufferResourceDescriptor);

public:
  WMeshBufferResource();
  ~WMeshBufferResource();

  W_ALWAYS_INLINE WUInt32 GetPrimitiveCount() const { return m_uiPrimitiveCount; }

  W_ALWAYS_INLINE WArrayPtr<const WGALBufferHandle> GetVertexBuffers() const { return WMakeArrayPtr(m_hVertexBuffers, m_VertexStreamConfig.GetHighestStreamIndex() + 1); }

  W_ALWAYS_INLINE WGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }

  W_ALWAYS_INLINE WGALPrimitiveTopology::Enum GetTopology() const { return m_Topology; }

  /// Returns the stream config used by this mesh buffer.
  W_ALWAYS_INLINE const WMeshVertexStreamConfig& GetVertexStreamConfig() const { return m_VertexStreamConfig; }

  /// Returns the vertex attributes that describes the data layout of the vertex buffers.
  W_ALWAYS_INLINE WArrayPtr<const WGALVertexAttribute> GetVertexAttributes() const { return m_VertexAttributes; }

  /// Returns the bounds of the mesh
  W_ALWAYS_INLINE const WBoundingBoxSphere& GetBounds() const { return m_Bounds; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WBoundingBoxSphere m_Bounds;
  WMeshVertexStreamConfig m_VertexStreamConfig;
  WSmallArray<WGALVertexAttribute, 8> m_VertexAttributes;
  WUInt32 m_uiPrimitiveCount = 0;
  WGALBufferHandle m_hVertexBuffers[WMeshVertexStreamType::Count];
  WGALBufferHandle m_hIndexBuffer;
  WGALPrimitiveTopology::Enum m_Topology = WGALPrimitiveTopology::Enum::Default;
};
