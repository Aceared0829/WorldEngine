#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/Color16f.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using WDynamicMeshBufferResourceHandle = WTypedResourceHandle<class WDynamicMeshBufferResource>;

struct WDynamicMeshBufferResourceDescriptor
{
  WEnum<WGALPrimitiveTopology> m_Topology = WGALPrimitiveTopology::Triangles;
  WEnum<WGALIndexType> m_IndexType = WGALIndexType::UInt;
  bool m_bColorStream = false;
  WUInt32 m_uiMaxPrimitives = 0;
  WUInt32 m_uiMaxVertices = 0;
};

// Contains the normal, tangent and texcoord0 of a dynamic mesh vertex.
struct W_RENDERERCORE_DLL WDynamicMeshVertexNTT
{
  W_DECLARE_POD_TYPE();

  WVec4U16 m_vEncodedNormal;
  WVec4U16 m_vEncodedTangent;
  WVec2 m_vTexCoord;

  W_ALWAYS_INLINE void EncodeNormal(const WVec3& vNormal)
  {
    // store in [0; 1] range
    m_vEncodedNormal.x = WMath::ColorFloatToShort(vNormal.x * 0.5f + 0.5f);
    m_vEncodedNormal.y = WMath::ColorFloatToShort(vNormal.y * 0.5f + 0.5f);
    m_vEncodedNormal.z = WMath::ColorFloatToShort(vNormal.z * 0.5f + 0.5f);
    m_vEncodedNormal.w = 0;

    // this is the same but slower
    // WMeshBufferUtils::EncodeNormal(vNormal, WByteArrayPtr(reinterpret_cast<WUInt8*>(&m_vEncodedNormal), sizeof(WVec4U16)), WGALResourceFormat::RGBAUShortNormalized).IgnoreResult();
  }

  W_ALWAYS_INLINE void EncodeTangent(const WVec3& vTangent, float fBitangentSign)
  {
    // store in [0; 1] range
    m_vEncodedTangent.x = WMath::ColorFloatToShort(vTangent.x * 0.5f + 0.5f);
    m_vEncodedTangent.y = WMath::ColorFloatToShort(vTangent.y * 0.5f + 0.5f);
    m_vEncodedTangent.z = WMath::ColorFloatToShort(vTangent.z * 0.5f + 0.5f);
    m_vEncodedTangent.w = WMath::ColorFloatToShort(fBitangentSign < 0.0f ? 0.0f : 1.0f);

    // this is the same but slower
    WMeshBufferUtils::EncodeTangent(vTangent, fBitangentSign, WByteArrayPtr(reinterpret_cast<WUInt8*>(&m_vEncodedTangent), sizeof(WVec4U16)), WGALResourceFormat::RGBAUShortNormalized).IgnoreResult();
  }
};

class W_RENDERERCORE_DLL WDynamicMeshBufferResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WDynamicMeshBufferResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WDynamicMeshBufferResource);
  W_RESOURCE_DECLARE_CREATEABLE(WDynamicMeshBufferResource, WDynamicMeshBufferResourceDescriptor);

public:
  WDynamicMeshBufferResource();
  ~WDynamicMeshBufferResource();

  W_ALWAYS_INLINE const WDynamicMeshBufferResourceDescriptor& GetDescriptor() const { return m_Descriptor; }
  W_ALWAYS_INLINE WArrayPtr<const WGALBufferHandle> GetVertexBuffers() const { return WMakeArrayPtr(m_hVertexBuffers); }
  W_ALWAYS_INLINE WGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }

  /// Grants write access to the position data, and flags the data as 'dirty'.
  WArrayPtr<WVec3> AccessPositionData(WUInt32 uiFirstVertex = 0, WUInt32 uiNumVertices = WInvalidIndex)
  {
    m_ModifiedPositionDataRange.SetToIncludeRange(uiFirstVertex, uiFirstVertex + WMath::Min(uiNumVertices, m_PositionData.GetCount() - uiFirstVertex) - 1);
    MarkAsDirty();

    return m_PositionData;
  }

  /// Grants write access to the normal, tangent and texcoord0 data, and flags the data as 'dirty'.
  WArrayPtr<WDynamicMeshVertexNTT> AccessNormalTangentTexCoord0Data(WUInt32 uiFirstVertex = 0, WUInt32 uiNumVertices = WInvalidIndex)
  {
    m_ModifiedNTTDataRange.SetToIncludeRange(uiFirstVertex, uiFirstVertex + WMath::Min(uiNumVertices, m_NTTData.GetCount() - uiFirstVertex) - 1);
    MarkAsDirty();

    return m_NTTData;
  }

  /// Grants write access to the color data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if creation of the color buffer was enabled.
  WArrayPtr<WColorLinear16f> AccessColorData(WUInt32 uiFirstVertex = 0, WUInt32 uiNumVertices = WInvalidIndex)
  {
    m_ModifiedColorDataRange.SetToIncludeRange(uiFirstVertex, uiFirstVertex + WMath::Min(uiNumVertices, m_ColorData.GetCount() - uiFirstVertex) - 1);
    MarkAsDirty();

    return m_ColorData;
  }

  /// Grants write access to the 16 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 16 bit indices.
  WArrayPtr<WUInt16> AccessIndex16Data(WUInt32 uiFirstIndex = 0, WUInt32 uiNumIndices = WInvalidIndex)
  {
    constexpr WUInt32 uiIndexByteSize = sizeof(WUInt16);
    const WUInt32 uiMinByte = uiFirstIndex * uiIndexByteSize;
    const WUInt32 uiMaxByte = uiMinByte + WMath::Min(uiNumIndices * uiIndexByteSize, m_IndexData.GetCount() - uiMinByte) - 1;
    m_ModifiedIndexDataRange.SetToIncludeRange(uiMinByte, uiMaxByte);
    MarkAsDirty();

    return WMakeArrayPtr(reinterpret_cast<WUInt16*>(m_IndexData.GetData()), m_IndexData.GetCount() / uiIndexByteSize);
  }

  /// Grants write access to the 32 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 32 bit indices.
  WArrayPtr<WUInt32> AccessIndex32Data(WUInt32 uiFirstIndex = 0, WUInt32 uiNumIndices = WInvalidIndex)
  {
    constexpr WUInt32 uiIndexByteSize = sizeof(WUInt32);
    const WUInt32 uiMinByte = uiFirstIndex * uiIndexByteSize;
    const WUInt32 uiMaxByte = uiMinByte + WMath::Min(uiNumIndices * uiIndexByteSize, m_IndexData.GetCount() - uiMinByte) - 1;
    m_ModifiedIndexDataRange.SetToIncludeRange(uiMinByte, uiMaxByte);
    MarkAsDirty();

    return WMakeArrayPtr(reinterpret_cast<WUInt32*>(m_IndexData.GetData()), m_IndexData.GetCount() / uiIndexByteSize);
  }

  /// Returns the vertex attributes that describes the data layout of the vertex buffers.
  W_ALWAYS_INLINE WArrayPtr<const WGALVertexAttribute> GetVertexAttributes() const { return m_VertexAttributes; }

  /// Helper function to create a grid aligned to the XY plane with the given size and number of vertices. The mesh buffer must already have the right number of vertices.
  static void CreateGridXY(WDynamicMeshBufferResource* pDynamicMeshBuffer, const WVec2& vSize, const WVec2U32& vNumVertices, const WVec2& vTextureScale = WVec2(1));

  /// Helper function to calculate smooth normals and tangents for a grid mesh created with the function above after its positions have been updated.
  static void CalculateGridNormalAndTangents(WDynamicMeshBufferResource* pDynamicMeshBuffer, const WVec2U32& vNumVertices);

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  friend struct WDynamicMeshBufferManager;
  void MarkAsDirty();
  void UploadChangesForNextFrame();

  WDynamicMeshBufferResourceDescriptor m_Descriptor;

  WSmallArray<WGALVertexAttribute, 8> m_VertexAttributes;

  WGALBufferHandle m_hVertexBuffers[WMeshVertexStreamType::Color0 + 1];
  WGALBufferHandle m_hIndexBuffer;

  WDynamicArray<WVec3, WAlignedAllocatorWrapper> m_PositionData;
  WDynamicArray<WDynamicMeshVertexNTT, WAlignedAllocatorWrapper> m_NTTData;
  WDynamicArray<WColorLinear16f, WAlignedAllocatorWrapper> m_ColorData;

  WDynamicArray<WUInt8, WAlignedAllocatorWrapper> m_IndexData;

  WGAL::ModifiedRange m_ModifiedPositionDataRange;
  WGAL::ModifiedRange m_ModifiedNTTDataRange;
  WGAL::ModifiedRange m_ModifiedColorDataRange;
  WGAL::ModifiedRange m_ModifiedIndexDataRange; // In bytes
};
