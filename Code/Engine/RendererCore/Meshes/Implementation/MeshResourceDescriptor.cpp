#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Containers/IterateBits.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <meshoptimizer/meshoptimizer.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

namespace
{
  /// Number of mantissa bits that are kept when the position stream is stored with the exponential filter.
  ///
  /// The filter rounds every position component to a multiple of a power of two that is shared by all vertices,
  /// which is the same precision that a 16 bit normalized position format would give, but the data stays float
  /// in memory and on the GPU. Because the lowest byte of every component becomes zero, the generic compression
  /// that is applied on top of this shrinks the position stream noticeably.
  /// Do not raise this above 16 without measuring, the compression gain comes from the zeroed byte.
  constexpr int s_iPositionFilterBits = 16;

  enum class WMeshPositionFilter : WUInt8
  {
    None = 0,
    Exponential = 1,
  };
} // namespace

WMeshResourceDescriptor::WMeshResourceDescriptor()
{
  m_Bounds = WBoundingBoxSphere::MakeInvalid();
}

void WMeshResourceDescriptor::Clear()
{
  m_Bounds = WBoundingBoxSphere::MakeInvalid();
  m_hMeshBuffer.Invalidate();
  m_Materials.Clear();
  m_MeshBufferDescriptor.Clear();
  m_SubMeshes.Clear();
}

WMeshBufferResourceDescriptor& WMeshResourceDescriptor::MeshBufferDesc()
{
  return m_MeshBufferDescriptor;
}

const WMeshBufferResourceDescriptor& WMeshResourceDescriptor::MeshBufferDesc() const
{
  return m_MeshBufferDescriptor;
}

void WMeshResourceDescriptor::UseExistingMeshBuffer(const WMeshBufferResourceHandle& hBuffer)
{
  m_hMeshBuffer = hBuffer;
}

const WMeshBufferResourceHandle& WMeshResourceDescriptor::GetExistingMeshBuffer() const
{
  return m_hMeshBuffer;
}

WArrayPtr<const WMeshResourceDescriptor::Material> WMeshResourceDescriptor::GetMaterials() const
{
  return m_Materials;
}

WArrayPtr<const WMeshResourceDescriptor::SubMesh> WMeshResourceDescriptor::GetSubMeshes() const
{
  return m_SubMeshes;
}

void WMeshResourceDescriptor::CollapseSubMeshes()
{
  for (WUInt32 idx = 1; idx < m_SubMeshes.GetCount(); ++idx)
  {
    m_SubMeshes[0].m_uiFirstPrimitive = WMath::Min(m_SubMeshes[0].m_uiFirstPrimitive, m_SubMeshes[idx].m_uiFirstPrimitive);
    m_SubMeshes[0].m_uiPrimitiveCount += m_SubMeshes[idx].m_uiPrimitiveCount;

    if (m_SubMeshes[0].m_Bounds.IsValid() && m_SubMeshes[idx].m_Bounds.IsValid())
    {
      m_SubMeshes[0].m_Bounds.ExpandToInclude(m_SubMeshes[idx].m_Bounds);
    }
  }

  m_SubMeshes.SetCount(1);
  m_SubMeshes[0].m_uiMaterialIndex = 0;

  m_Materials.SetCount(1);
}

const WBoundingBoxSphere& WMeshResourceDescriptor::GetBounds() const
{
  return m_Bounds;
}

void WMeshResourceDescriptor::AddSubMesh(WUInt32 uiPrimitiveCount, WUInt32 uiFirstPrimitive, WUInt32 uiMaterialIndex)
{
  SubMesh p;
  p.m_uiFirstPrimitive = uiFirstPrimitive;
  p.m_uiPrimitiveCount = uiPrimitiveCount;
  p.m_uiMaterialIndex = uiMaterialIndex;
  p.m_Bounds = WBoundingBoxSphere::MakeInvalid();

  m_SubMeshes.PushBack(p);
}

void WMeshResourceDescriptor::SetMaterial(WUInt32 uiMaterialIndex, WStringView sPathToMaterial)
{
  m_Materials.EnsureCount(uiMaterialIndex + 1);

  m_Materials[uiMaterialIndex].m_sPath = sPathToMaterial;
}

WResult WMeshResourceDescriptor::Save(const char* szFile)
{
  W_LOG_BLOCK("WMeshResourceDescriptor::Save", szFile);

  WFileWriter file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    WLog::Error("Failed to open file '{0}'", szFile);
    return W_FAILURE;
  }

  Save(file);
  return W_SUCCESS;
}

void WMeshResourceDescriptor::Save(WStreamWriter& inout_stream)
{
  WUInt8 uiVersion = 7;
  inout_stream << uiVersion;

  WUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  WCompressedStreamWriterZstd compressor(&inout_stream, 0, WCompressedStreamWriterZstd::Compression::Average);
  WChunkStreamWriter chunk(compressor);
#else
  WChunkStreamWriter chunk(stream);
#endif

  inout_stream << uiCompressionMode;

  chunk.BeginStream(1);

  {
    chunk.BeginChunk("Materials", 1);

    // number of materials
    chunk << m_Materials.GetCount();

    // each material
    for (WUInt32 idx = 0; idx < m_Materials.GetCount(); ++idx)
    {
      chunk << idx;                      // Material Index
      chunk << m_Materials[idx].m_sPath; // Material Path (data directory relative)
      /// \todo Material Path (relative to mesh file)
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("SubMeshes", 1);

    // number of sub-meshes
    chunk << m_SubMeshes.GetCount();

    for (WUInt32 idx = 0; idx < m_SubMeshes.GetCount(); ++idx)
    {
      chunk << idx;                                // Sub-Mesh index
      chunk << m_SubMeshes[idx].m_uiMaterialIndex; // The material to use
      chunk << m_SubMeshes[idx].m_uiFirstPrimitive;
      chunk << m_SubMeshes[idx].m_uiPrimitiveCount;
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("MeshInfo", 5);

    chunk << m_MeshBufferDescriptor.GetVertexCount();
    chunk << m_MeshBufferDescriptor.GetPrimitiveCount();

    // Version 5: Stream config
    chunk << m_MeshBufferDescriptor.GetVertexStreamConfig().m_uiTypesMask;
    chunk << m_MeshBufferDescriptor.GetVertexStreamConfig().m_bUseHighPrecision;

    // Version 3: Topology
    chunk << (WUInt8)m_MeshBufferDescriptor.GetTopology();

    // Version 2
    if (!m_Bounds.IsValid())
    {
      ComputeBounds();
    }

    chunk << m_Bounds.m_vCenter;
    chunk << m_Bounds.m_vBoxHalfExtents;
    chunk << m_Bounds.m_fSphereRadius;
    // Version 4
    chunk << m_fMaxBoneVertexOffset;

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("VertexBuffer", 3);

    const auto& streamConfig = m_MeshBufferDescriptor.GetVertexStreamConfig();

    // the position filter is lossy, so it is only applied to meshes that were not imported with high precision
    const bool bFilterPositions = !streamConfig.m_bUseHighPrecision && streamConfig.GetPositionFormat() == WGALResourceFormat::XYZFloat;

    chunk << static_cast<WUInt8>(bFilterPositions ? WMeshPositionFilter::Exponential : WMeshPositionFilter::None);

    WDynamicArray<WUInt8> filteredPositions;

    const WUInt32 uiNumBuffers = m_MeshBufferDescriptor.GetNumVertexBuffers();
    for (WUInt32 i = 0; i < uiNumBuffers; ++i)
    {
      auto type = static_cast<WMeshVertexStreamType::Enum>(i);
      const auto& data = m_MeshBufferDescriptor.GetVertexBufferData(type);

      // size in bytes
      chunk << data.GetCount();

      if (data.IsEmpty())
        continue;

      if (bFilterPositions && type == WMeshVertexStreamType::Position)
      {
        filteredPositions.SetCountUninitialized(data.GetCount());

        meshopt_encodeFilterExp(filteredPositions.GetData(), m_MeshBufferDescriptor.GetVertexCount(), streamConfig.GetPositionElementSize(),
          s_iPositionFilterBits, reinterpret_cast<const float*>(data.GetData()), meshopt_EncodeExpSharedComponent);

        chunk.WriteBytes(filteredPositions.GetData(), filteredPositions.GetCount()).IgnoreResult();
      }
      else
      {
        chunk.WriteBytes(data.GetData(), data.GetCount()).IgnoreResult();
      }
    }

    chunk.EndChunk();
  }

  // always write the index buffer chunk, even if it is empty
  {
    chunk.BeginChunk("IndexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetIndexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
    {
      chunk.WriteBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount()).IgnoreResult();
    }

    chunk.EndChunk();
  }

  if (!m_Bones.IsEmpty())
  {
    chunk.BeginChunk("BindPose", 1);

    chunk.WriteHashTable(m_Bones).IgnoreResult();

    chunk.EndChunk();
  }

  if (m_hDefaultSkeleton.IsValid())
  {
    chunk.BeginChunk("Skeleton", 1);

    chunk << m_hDefaultSkeleton;

    chunk.EndChunk();
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  compressor.FinishCompressedStream().IgnoreResult();

  WLog::Dev("Compressed mesh data from {0} KB to {1} KB ({2}%%)", WArgF((float)compressor.GetUncompressedSize() / 1024.0f, 1), WArgF((float)compressor.GetCompressedSize() / 1024.0f, 1), WArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));
#endif
}

WResult WMeshResourceDescriptor::Load(const char* szFile)
{
  W_LOG_BLOCK("WMeshResourceDescriptor::Load", szFile);

  WFileReader file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    WLog::Error("Failed to open file '{0}'", szFile);
    return W_FAILURE;
  }

  // skip asset header
  WAssetFileHeader assetHeader;
  W_SUCCEED_OR_RETURN(assetHeader.Read(file));

  return Load(file);
}

WResult WMeshResourceDescriptor::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  // version 4 and below is broken
  if (uiVersion <= 4)
    return W_FAILURE;

  WUInt8 uiCompressionMode = 0;
  if (uiVersion >= 6)
  {
    inout_stream >> uiCompressionMode;
  }

  WStreamReader* pCompressor = &inout_stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  WCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(&inout_stream);
      pCompressor = &decompressorZstd;
      break;
#else
      WLog::Error("Mesh is compressed with zstandard, but support for this compressor is not compiled in.");
      return W_FAILURE;
#endif

    default:
      WLog::Error("Mesh is compressed with an unknown algorithm.");
      return W_FAILURE;
  }

  WChunkStreamReader chunk(*pCompressor);
  chunk.BeginStream();

  WUInt32 count;
  bool bCalculateBounds = true;

  while (chunk.GetCurrentChunk().m_bValid)
  {
    const auto& ci = chunk.GetCurrentChunk();

    if (ci.m_sChunkName == "Materials")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        WLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return W_FAILURE;
      }

      // number of materials
      chunk >> count;
      m_Materials.SetCount(count);

      // each material
      for (WUInt32 i = 0; i < m_Materials.GetCount(); ++i)
      {
        WUInt32 idx;
        chunk >> idx;                      // Material Index
        chunk >> m_Materials[idx].m_sPath; // Material Path (data directory relative)
        /// \todo Material Path (relative to mesh file)
      }
    }

    if (chunk.GetCurrentChunk().m_sChunkName == "SubMeshes")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        WLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return W_FAILURE;
      }

      // number of sub-meshes
      chunk >> count;
      m_SubMeshes.SetCount(count);

      for (WUInt32 i = 0; i < m_SubMeshes.GetCount(); ++i)
      {
        WUInt32 idx;
        chunk >> idx;                                // Sub-Mesh index
        chunk >> m_SubMeshes[idx].m_uiMaterialIndex; // The material to use
        chunk >> m_SubMeshes[idx].m_uiFirstPrimitive;
        chunk >> m_SubMeshes[idx].m_uiPrimitiveCount;

        /// \todo load from file
        m_SubMeshes[idx].m_Bounds = WBoundingBoxSphere::MakeInvalid();
      }
    }

    if (ci.m_sChunkName == "MeshInfo")
    {
      if (ci.m_uiChunkVersion != 5)
      {
        WLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return W_FAILURE;
      }

      // Number of vertices
      WUInt32 uiVertexCount = 0;
      chunk >> uiVertexCount;

      // Number of primitives
      WUInt32 uiPrimitiveCount = 0;
      chunk >> uiPrimitiveCount;

      WMeshVertexStreamConfig streamConfig;
      chunk >> streamConfig.m_uiTypesMask;
      chunk >> streamConfig.m_bUseHighPrecision;

      // Topology
      WUInt8 uiTopology = WGALPrimitiveTopology::Triangles;
      chunk >> uiTopology;


      for (WUInt32 idx : WIterateBitIndices(streamConfig.m_uiTypesMask))
      {
        auto type = static_cast<WMeshVertexStreamType::Enum>(idx);
        m_MeshBufferDescriptor.AddStream(type, streamConfig.m_bUseHighPrecision);
      }

      m_MeshBufferDescriptor.AllocateStreams(uiVertexCount, (WGALPrimitiveTopology::Enum)uiTopology, uiPrimitiveCount);

      // Version 2
      if (ci.m_uiChunkVersion >= 2)
      {
        chunk >> m_Bounds.m_vCenter;
        chunk >> m_Bounds.m_vBoxHalfExtents;
        chunk >> m_Bounds.m_fSphereRadius;
        bCalculateBounds = !m_Bounds.IsValid();
      }

      if (ci.m_uiChunkVersion >= 4)
      {
        chunk >> m_fMaxBoneVertexOffset;
      }
    }

    if (ci.m_sChunkName == "VertexBuffer")
    {
      if (ci.m_uiChunkVersion != 2 && ci.m_uiChunkVersion != 3)
      {
        WLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return W_FAILURE;
      }

      WUInt8 uiPositionFilter = static_cast<WUInt8>(WMeshPositionFilter::None);

      // Version 3: the position stream may be stored in a filtered representation
      if (ci.m_uiChunkVersion >= 3)
      {
        chunk >> uiPositionFilter;
      }

      const WUInt32 uiNumBuffers = m_MeshBufferDescriptor.GetNumVertexBuffers();
      for (WUInt32 i = 0; i < uiNumBuffers; ++i)
      {
        auto type = static_cast<WMeshVertexStreamType::Enum>(i);
        auto& data = m_MeshBufferDescriptor.GetVertexBufferData(type);

        // size in bytes
        chunk >> count;
        if (data.GetCount() != count)
        {
          WLog::Error("Buffer data size mismatch: Expected {} but got {}", WArgFileSize(data.GetCount()), WArgFileSize(count));
          return W_FAILURE;
        }

        if (data.IsEmpty())
          continue;

        chunk.ReadBytes(data.GetData(), data.GetCount());

        if (type == WMeshVertexStreamType::Position && uiPositionFilter == static_cast<WUInt8>(WMeshPositionFilter::Exponential))
        {
          meshopt_decodeFilterExp(data.GetData(), m_MeshBufferDescriptor.GetVertexCount(), m_MeshBufferDescriptor.GetVertexStreamConfig().GetPositionElementSize());
        }
      }
    }

    if (ci.m_sChunkName == "IndexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        WLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return W_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetIndexBufferData().SetCountUninitialized(count);

      if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount());
    }

    if (ci.m_sChunkName == "BindPose")
    {
      W_SUCCEED_OR_RETURN(chunk.ReadHashTable(m_Bones));
    }

    if (ci.m_sChunkName == "Skeleton")
    {
      chunk >> m_hDefaultSkeleton;
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  if (bCalculateBounds)
  {
    ComputeBounds();

    auto b = m_Bounds;
    WLog::Info("Calculated Bounds: {0} | {1} | {2} - {3} | {4} | {5}", WArgF(b.m_vCenter.x, 2), WArgF(b.m_vCenter.y, 2), WArgF(b.m_vCenter.z, 2), WArgF(b.m_vBoxHalfExtents.x, 2), WArgF(b.m_vBoxHalfExtents.y, 2), WArgF(b.m_vBoxHalfExtents.z, 2));
  }

  return W_SUCCESS;
}

void WMeshResourceDescriptor::ComputeBounds()
{
  if (m_hMeshBuffer.IsValid())
  {
    WResourceLock<WMeshBufferResource> pMeshBuffer(m_hMeshBuffer, WResourceAcquireMode::AllowLoadingFallback);
    m_Bounds = pMeshBuffer->GetBounds();
  }
  else
  {
    m_Bounds = m_MeshBufferDescriptor.ComputeBounds();
  }

  if (!m_Bounds.IsValid())
  {
    m_Bounds = WBoundingBoxSphere::MakeFromCenterExtents(WVec3::MakeZero(), WVec3(0.1f), 0.1f);
  }
}

WResult WMeshResourceDescriptor::BoneData::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_GlobalInverseRestPoseMatrix;
  inout_stream << m_uiBoneIndex;

  return W_SUCCESS;
}

WResult WMeshResourceDescriptor::BoneData::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_GlobalInverseRestPoseMatrix;
  inout_stream >> m_uiBoneIndex;

  return W_SUCCESS;
}
