#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/Importer/Importer.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <meshoptimizer/meshoptimizer.h>

namespace WModelImporter2
{
  namespace
  {
    /// Reorders the triangles of an imported mesh for GPU vertex cache efficiency and its vertices for fetch locality.
    ///
    /// Triangles are only reordered within a sub-mesh, so that the index range of every sub-mesh stays valid.
    /// The vertex reordering is done across the entire mesh buffer and all indices are remapped accordingly.
    /// Only indexed triangle meshes are touched, everything else is left as it is.
    void OptimizeMeshForRendering(WMeshResourceDescriptor& ref_desc)
    {
      WMeshBufferResourceDescriptor& mb = ref_desc.MeshBufferDesc();

      if (mb.GetTopology() != WGALPrimitiveTopology::Triangles || !mb.HasIndexBuffer())
        return;

      const WUInt32 uiVertexCount = mb.GetVertexCount();
      const WUInt32 uiIndexCount = mb.GetPrimitiveCount() * 3;

      if (uiVertexCount == 0 || uiIndexCount == 0)
        return;

      // meshoptimizer only works on 32 bit indices, so 16 bit index data is widened here and narrowed again at the end
      const bool bIndices32Bit = mb.Uses32BitIndices();

      WTempArray<WUInt32> indices;
      indices.SetCountUninitialized(uiIndexCount);

      {
        const auto& indexData = mb.GetIndexBufferData();

        if (bIndices32Bit)
        {
          WMemoryUtils::Copy(indices.GetData(), reinterpret_cast<const WUInt32*>(indexData.GetData()), uiIndexCount);
        }
        else
        {
          const WUInt16* pSrc = reinterpret_cast<const WUInt16*>(indexData.GetData());

          for (WUInt32 i = 0; i < uiIndexCount; ++i)
          {
            indices[i] = pSrc[i];
          }
        }
      }

      // every sub-mesh is a separate draw call and thus has to be optimized on its own,
      // otherwise triangles would move out of the index range that the sub-mesh refers to
      for (const auto& subMesh : ref_desc.GetSubMeshes())
      {
        const WUInt32 uiFirstIndex = subMesh.m_uiFirstPrimitive * 3;
        const WUInt32 uiNumIndices = subMesh.m_uiPrimitiveCount * 3;

        if (uiNumIndices == 0 || uiFirstIndex + uiNumIndices > uiIndexCount)
          continue;

        meshopt_optimizeVertexCache(indices.GetData() + uiFirstIndex, indices.GetData() + uiFirstIndex, uiNumIndices, uiVertexCount);
      }

      // reorder the vertices in the order in which the (now optimized) index buffer references them
      {
        WTempArray<WUInt32> remap;
        remap.SetCountUninitialized(uiVertexCount);

        const WUInt32 uiUniqueVertices = static_cast<WUInt32>(meshopt_optimizeVertexFetchRemap(remap.GetData(), indices.GetData(), uiIndexCount, uiVertexCount));

        // if the mesh contains vertices that no triangle references, the remap table would shrink the vertex buffer.
        // that would require patching up everything that refers to vertex indices, so instead the vertex order is left alone in this case.
        if (uiUniqueVertices == uiVertexCount)
        {
          WTempArray<WUInt32> remappedIndices;
          remappedIndices.SetCountUninitialized(uiIndexCount);
          meshopt_remapIndexBuffer(remappedIndices.GetData(), indices.GetData(), uiIndexCount, remap.GetData());
          indices.Swap(remappedIndices);

          WDynamicArray<WUInt8, WAlignedAllocatorWrapper> remappedStream;

          for (WUInt32 i = 0; i < mb.GetNumVertexBuffers(); ++i)
          {
            const auto type = static_cast<WMeshVertexStreamType::Enum>(i);
            auto& streamData = mb.GetVertexBufferData(type);

            if (streamData.IsEmpty())
              continue;

            const WUInt32 uiElementSize = mb.GetVertexStreamConfig().GetStreamElementSize(type);

            remappedStream.SetCountUninitialized(streamData.GetCount());
            meshopt_remapVertexBuffer(remappedStream.GetData(), streamData.GetData(), uiVertexCount, uiElementSize, remap.GetData());
            streamData = remappedStream;
          }
        }
        else
        {
          WLog::Dev("Mesh has {} unused vertices, skipping vertex fetch optimization.", uiVertexCount - uiUniqueVertices);
        }
      }

      // write the indices back
      {
        auto& indexData = mb.GetIndexBufferData();

        if (bIndices32Bit)
        {
          WMemoryUtils::Copy(reinterpret_cast<WUInt32*>(indexData.GetData()), indices.GetData(), uiIndexCount);
        }
        else
        {
          WUInt16* pDst = reinterpret_cast<WUInt16*>(indexData.GetData());

          for (WUInt32 i = 0; i < uiIndexCount; ++i)
          {
            pDst[i] = static_cast<WUInt16>(indices[i]);
          }
        }
      }
    }
  } // namespace

  Importer::Importer() = default;
  Importer::~Importer() = default;

  WResult Importer::Import(const ImportOptions& options, WLogInterface* pLogInterface /*= nullptr*/, WProgress* pProgress /*= nullptr*/)
  {
    WResult res = W_FAILURE;

    WLogInterface* pPrevLogSystem = WLog::GetThreadLocalLogSystem();

    if (pLogInterface)
    {
      WLog::SetThreadLocalLogSystem(pLogInterface);
    }

    {
      m_pProgress = pProgress;
      m_Options = options;

      W_LOG_BLOCK("ModelImport", m_Options.m_sSourceFile);

      res = DoImport();

      if (res.Succeeded() && m_Options.m_pMeshOutput != nullptr)
      {
        OptimizeMeshForRendering(*m_Options.m_pMeshOutput);
      }
    }


    WLog::SetThreadLocalLogSystem(pPrevLogSystem);

    return res;
  }

  void OutputTexture::GenerateFileName(WStringBuilder& out_sName) const
  {
    WStringBuilder tmp("Embedded_", m_sFilename);

    WPathUtils::MakeValidFilename(tmp.GetFileName(), '_', out_sName);
    out_sName.ChangeFileExtension(m_sFileFormatExtension);
  }

} // namespace WModelImporter2
