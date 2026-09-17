#include <ModelImporter2/ModelImporterPCH.h>

#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#include <assimp/scene.h>
#include <meshoptimizer/meshoptimizer.h>

namespace WModelImporter2
{
  void ImporterAssimp::SimplifyAiMesh(aiMesh* pMesh)
  {
    if (m_Options.m_uiMeshSimplification == 0)
      return;

    // already processed
    if (m_OptimizedMeshes.Contains(pMesh))
      return;

    m_OptimizedMeshes.Insert(pMesh);

    WUInt32 numIndices = pMesh->mNumFaces * 3;

    WTempArray<WUInt32> indices;
    indices.Reserve(numIndices);

    WTempArray<WUInt32> simplifiedIndices;
    simplifiedIndices.SetCountUninitialized(numIndices);

    for (WUInt32 face = 0; face < pMesh->mNumFaces; ++face)
    {
      indices.PushBack(pMesh->mFaces[face].mIndices[0]);
      indices.PushBack(pMesh->mFaces[face].mIndices[1]);
      indices.PushBack(pMesh->mFaces[face].mIndices[2]);
    }

    const float fTargetError = WMath::Clamp<WUInt32>(m_Options.m_uiMaxSimplificationError, 1, 99) / 100.0f;
    const size_t numTargetIndices = static_cast<size_t>((numIndices * (100 - WMath::Min<WUInt32>(m_Options.m_uiMeshSimplification, 99))) / 100.0f);

    float err;
    size_t numNewIndices = 0;

    if (m_Options.m_bAggressiveSimplification)
    {
      numNewIndices = meshopt_simplifySloppy(simplifiedIndices.GetData(), indices.GetData(), numIndices, &pMesh->mVertices[0].x, pMesh->mNumVertices, sizeof(aiVector3D), numTargetIndices, fTargetError, &err);
    }
    else
    {
      if (pMesh->HasNormals())
      {
        const float nrm_weight = m_Options.m_fNormalWeight;
        const float attr_weights[3] = {nrm_weight, nrm_weight, nrm_weight};

        numNewIndices = meshopt_simplifyWithAttributes(simplifiedIndices.GetData(), indices.GetData(), numIndices, &pMesh->mVertices[0].x, pMesh->mNumVertices, sizeof(aiVector3D),
          &pMesh->mNormals[0].x, sizeof(aiVector3D), attr_weights, 3, NULL,
          numTargetIndices, fTargetError, 0, &err);
      }
      else
      {
        numNewIndices = meshopt_simplify(simplifiedIndices.GetData(), indices.GetData(), numIndices, &pMesh->mVertices[0].x, pMesh->mNumVertices, sizeof(aiVector3D), numTargetIndices, fTargetError, 0, &err);
      }

      simplifiedIndices.SetCount(static_cast<WUInt32>(numNewIndices));
    }

    WTempArray<WUInt32> remapTable;
    remapTable.SetCountUninitialized(pMesh->mNumVertices);
    const size_t numUniqueVerts = meshopt_optimizeVertexFetchRemap(remapTable.GetData(), simplifiedIndices.GetData(), numNewIndices, pMesh->mNumVertices);

    meshopt_remapVertexBuffer(pMesh->mVertices, pMesh->mVertices, pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());

    if (pMesh->HasNormals())
    {
      meshopt_remapVertexBuffer(pMesh->mNormals, pMesh->mNormals, pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());
    }
    if (pMesh->HasTextureCoords(0))
    {
      meshopt_remapVertexBuffer(pMesh->mTextureCoords[0], pMesh->mTextureCoords[0], pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());
    }
    if (pMesh->HasTextureCoords(1))
    {
      meshopt_remapVertexBuffer(pMesh->mTextureCoords[1], pMesh->mTextureCoords[1], pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());
    }
    if (pMesh->HasVertexColors(0))
    {
      meshopt_remapVertexBuffer(pMesh->mColors[0], pMesh->mColors[0], pMesh->mNumVertices, sizeof(aiColor4D), remapTable.GetData());
    }
    if (pMesh->HasVertexColors(1))
    {
      meshopt_remapVertexBuffer(pMesh->mColors[1], pMesh->mColors[1], pMesh->mNumVertices, sizeof(aiColor4D), remapTable.GetData());
    }
    if (pMesh->HasTangentsAndBitangents())
    {
      meshopt_remapVertexBuffer(pMesh->mTangents, pMesh->mTangents, pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());
      meshopt_remapVertexBuffer(pMesh->mBitangents, pMesh->mBitangents, pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());
    }
    if (pMesh->HasBones() && m_Options.m_bImportSkinningData)
    {
      for (WUInt32 b = 0; b < pMesh->mNumBones; ++b)
      {
        auto& bone = pMesh->mBones[b];

        for (WUInt32 w = 0; w < bone->mNumWeights;)
        {
          auto& weight = bone->mWeights[w];
          const WUInt32 uiNewIdx = remapTable[weight.mVertexId];

          if (uiNewIdx == ~0u)
          {
            // this vertex got removed -> swap it with the last weight
            bone->mWeights[w] = bone->mWeights[bone->mNumWeights - 1];
            --bone->mNumWeights;
          }
          else
          {
            bone->mWeights[w].mVertexId = uiNewIdx;
            ++w;
          }
        }
      }
    }

    pMesh->mNumVertices = static_cast<WUInt32>(numUniqueVerts);

    WTempArray<WUInt32> newIndices;
    newIndices.SetCountUninitialized(static_cast<WUInt32>(numNewIndices));

    meshopt_remapIndexBuffer(newIndices.GetData(), simplifiedIndices.GetData(), numNewIndices, remapTable.GetData());

    pMesh->mNumFaces = static_cast<WUInt32>(numNewIndices / 3);

    WUInt32 nextIdx = 0;
    for (WUInt32 face = 0; face < pMesh->mNumFaces; ++face)
    {
      pMesh->mFaces[face].mIndices[0] = newIndices[nextIdx++];
      pMesh->mFaces[face].mIndices[1] = newIndices[nextIdx++];
      pMesh->mFaces[face].mIndices[2] = newIndices[nextIdx++];
    }
  }

  WResult ImporterAssimp::ProcessAiMesh(aiMesh* pMesh, const WMat4& transform)
  {
    if ((pMesh->mPrimitiveTypes & aiPrimitiveType::aiPrimitiveType_TRIANGLE) == 0) // no triangles in there ?
      return W_SUCCESS;

    m_OutputMeshNames.PushBack(pMesh->mName.C_Str());

    if (!m_Options.m_MeshIncludeTags.IsEmpty() || !m_Options.m_MeshExcludeTags.IsEmpty())
    {
      WLog::Dev("Found mesh with name: '{}'", pMesh->mName.C_Str());
    }

    if (!m_Options.m_MeshIncludeTags.IsEmpty())
    {
      for (const auto& str : m_Options.m_MeshIncludeTags)
      {
        if (WStringUtils::StartsWith_NoCase(pMesh->mName.C_Str(), str) || WStringUtils::EndsWith_NoCase(pMesh->mName.C_Str(), str))
        {
          WLog::Dev("Including mesh '{}' because of include-tag '{}'", pMesh->mName.C_Str(), str);
          goto do_import;
        }
      }

      WLog::Dev("Skipping mesh '{}', because it doesn't match any include-tag.", pMesh->mName.C_Str());
      return W_SUCCESS; // not a failure case
    }

    for (const auto& str : m_Options.m_MeshExcludeTags)
    {
      if (WStringUtils::StartsWith_NoCase(pMesh->mName.C_Str(), str) || WStringUtils::EndsWith_NoCase(pMesh->mName.C_Str(), str))
      {
        WLog::Dev("Skipping mesh '{}' because of exclude-tag '{}'", pMesh->mName.C_Str(), str);
        return W_SUCCESS; // not a failure case
      }
    }


  do_import:

    if (m_Options.m_bImportSkinningData && !pMesh->HasBones())
    {
      WLog::Warning("Mesh contains an unskinned part ('{}' - {} triangles)", pMesh->mName.C_Str(), pMesh->mNumFaces);
      return W_SUCCESS;
    }

    // if enabled, the aiMesh is modified in-place to have less detail
    SimplifyAiMesh(pMesh);

    {
      auto& mi = m_MeshInstances[pMesh->mMaterialIndex].ExpandAndGetRef();
      mi.m_GlobalTransform = transform;
      mi.m_pMesh = pMesh;

      m_uiTotalMeshVertices += pMesh->mNumVertices;
      m_uiTotalMeshTriangles += pMesh->mNumFaces;
    }

    return W_SUCCESS;
  }

  static void SetMeshTriangleIndices(WMeshBufferResourceDescriptor& ref_mb, const aiMesh* pMesh, WUInt32 uiTriangleIndexOffset, WUInt32 uiVertexIndexOffset, bool bFlipTriangles)
  {
    if (bFlipTriangles)
    {
      for (WUInt32 triIdx = 0; triIdx < pMesh->mNumFaces; ++triIdx)
      {
        const WUInt32 finalTriIdx = uiTriangleIndexOffset + triIdx;

        const WUInt32 f0 = pMesh->mFaces[triIdx].mIndices[0];
        const WUInt32 f1 = pMesh->mFaces[triIdx].mIndices[1];
        const WUInt32 f2 = pMesh->mFaces[triIdx].mIndices[2];

        ref_mb.SetTriangleIndices(finalTriIdx, uiVertexIndexOffset + f0, uiVertexIndexOffset + f2, uiVertexIndexOffset + f1);
      }
    }
    else
    {
      for (WUInt32 triIdx = 0; triIdx < pMesh->mNumFaces; ++triIdx)
      {
        const WUInt32 finalTriIdx = uiTriangleIndexOffset + triIdx;

        const WUInt32 f0 = pMesh->mFaces[triIdx].mIndices[0];
        const WUInt32 f1 = pMesh->mFaces[triIdx].mIndices[1];
        const WUInt32 f2 = pMesh->mFaces[triIdx].mIndices[2];

        ref_mb.SetTriangleIndices(finalTriIdx, uiVertexIndexOffset + f0, uiVertexIndexOffset + f1, uiVertexIndexOffset + f2);
      }
    }
  }

  static void SetMeshBoneData(WMeshBufferResourceDescriptor& ref_mb, WMeshResourceDescriptor& ref_mrd, float& inout_fMaxBoneOffset, const aiMesh* pMesh, WUInt32 uiVertexIndexOffset, bool bNormalizeWeights)
  {
    if (!pMesh->HasBones())
      return;

    WHashedString hs;

    for (WUInt32 b = 0; b < pMesh->mNumBones; ++b)
    {
      const aiBone* pBone = pMesh->mBones[b];

      hs.Assign(pBone->mName.C_Str());
      const WUInt32 uiBoneIndex = ref_mrd.m_Bones[hs].m_uiBoneIndex;

      for (WUInt32 w = 0; w < pBone->mNumWeights; ++w)
      {
        const auto& vertexWeight = pBone->mWeights[w];

        const WUInt32 finalVertIdx = uiVertexIndexOffset + vertexWeight.mVertexId;

        WVec4U16 indices = ref_mb.GetBoneIndices(finalVertIdx);
        WVec4 weights = ref_mb.GetBoneWeights(finalVertIdx);

        // pBoneWeights are initialized with 0
        // so for the first 4 bones we always assign to one slot that is 0 (least weight)
        // if we have 5 bones or more, we then replace the currently lowest value each time

        WUInt32 uiLeastWeightIdx = 0;

        for (int i = 1; i < 4; ++i)
        {
          if (weights.GetData()[i] < weights.GetData()[uiLeastWeightIdx])
          {
            uiLeastWeightIdx = i;
          }
        }

        const float fEncodedWeight = vertexWeight.mWeight;

        if (weights.GetData()[uiLeastWeightIdx] < fEncodedWeight)
        {
          indices.GetData()[uiLeastWeightIdx] = uiBoneIndex;
          weights.GetData()[uiLeastWeightIdx] = fEncodedWeight;

          ref_mb.SetBoneIndices(finalVertIdx, indices);
          ref_mb.SetBoneWeights(finalVertIdx, weights);
        }
      }
    }
  }

  static void CheckBoneWeights(WMeshBufferResourceDescriptor& ref_mb, WMeshResourceDescriptor& ref_mrd, float& inout_fMaxBoneOffset, bool bNormalizeWeights)
  {
    WUInt32 uiZeroWeights = 0;

    for (WUInt32 vtx = 0; vtx < ref_mb.GetVertexCount(); ++vtx)
    {
      WVec4 weights = ref_mb.GetBoneWeights(vtx);

      if (weights.IsZero(0.001f))
      {
        ++uiZeroWeights;
      }
      else if (bNormalizeWeights)
      {
        // NOTE: This is absolutely crucial for some meshes to work right
        // On the other hand, it is also possible that some meshes don't like this

        const float summedWeights = weights.x + weights.y + weights.z + weights.w;

        weights /= summedWeights;
        ref_mb.SetBoneWeights(vtx, weights);
      }

      const WVec3 vVertexPos = ref_mb.GetPosition(vtx);
      const WVec4U16 vBoneIndices = ref_mb.GetBoneIndices(vtx);

      // also find the maximum distance of any vertex to its influencing bones
      // this is used to adjust the bounding box for culling at runtime
      // ie. we can compute the bounding box from a pose, but that only covers the skeleton, not the full mesh
      // so we then grow the bbox by this maximum distance
      // that usually creates a far larger bbox than necessary, but means there are no culling artifacts

      for (const auto& bone : ref_mrd.m_Bones)
      {
        for (int b = 0; b < 4; ++b)
        {
          if (weights.GetData()[b] < 0.2f) // only look at bones that have a proper weight
            continue;

          if (bone.Value().m_uiBoneIndex == vBoneIndices.GetData()[b])
          {
            // move the vertex into local space of the bone, then determine how far it is away from the bone
            const WVec3 vOffPos = bone.Value().m_GlobalInverseRestPoseMatrix * vVertexPos;
            const float length = vOffPos.GetLength();

            if (length > inout_fMaxBoneOffset)
            {
              inout_fMaxBoneOffset = length;
            }
          }
        }
      }
    }


    if (uiZeroWeights > 0)
    {
      WLog::Error("Mesh has {} vertices with bone weights that are zero. These will not show up!", uiZeroWeights);
    }
  }

  static void SetMeshVertexData(WMeshBufferResourceDescriptor& ref_mb, const aiMesh* pMesh, const WMat4& mGlobalTransform, WUInt32 uiVertexIndexOffset, WEnum<WMeshVertexColorConversion> meshVertexColorConversion)
  {
    WMat3 normalsTransform = mGlobalTransform.GetRotationalPart();
    if (normalsTransform.Invert(0.0f).Failed())
    {
      WLog::Warning("Couldn't invert a mesh's transform matrix.");
      normalsTransform.SetIdentity();
    }

    normalsTransform.Transpose();

    for (WUInt32 vertIdx = 0; vertIdx < pMesh->mNumVertices; ++vertIdx)
    {
      const WUInt32 finalVertIdx = uiVertexIndexOffset + vertIdx;

      const WVec3 position = mGlobalTransform * ConvertAssimpType(pMesh->mVertices[vertIdx]);
      ref_mb.SetPosition(finalVertIdx, position);

      if (ref_mb.GetVertexStreamConfig().HasNormal() && pMesh->HasNormals())
      {
        WVec3 normal = normalsTransform * ConvertAssimpType(pMesh->mNormals[vertIdx]);
        normal.NormalizeIfNotZero(WVec3::MakeZero()).IgnoreResult();

        ref_mb.SetNormal(finalVertIdx, normal);
      }

      if (ref_mb.GetVertexStreamConfig().HasTexCoord0() && pMesh->HasTextureCoords(0))
      {
        const WVec2 texcoord = ConvertAssimpType(pMesh->mTextureCoords[0][vertIdx]).GetAsVec2();

        ref_mb.SetTexCoord0(finalVertIdx, texcoord);
      }

      if (ref_mb.GetVertexStreamConfig().HasTexCoord1() && pMesh->HasTextureCoords(1))
      {
        const WVec2 texcoord = ConvertAssimpType(pMesh->mTextureCoords[1][vertIdx]).GetAsVec2();

        ref_mb.SetTexCoord1(finalVertIdx, texcoord);
      }

      if (ref_mb.GetVertexStreamConfig().HasColor0() && pMesh->HasVertexColors(0))
      {
        const WColor color = ConvertAssimpType(pMesh->mColors[0][vertIdx]);

        ref_mb.SetColor0(finalVertIdx, color, meshVertexColorConversion);
      }

      if (ref_mb.GetVertexStreamConfig().HasColor1() && pMesh->HasVertexColors(1))
      {
        const WColor color = ConvertAssimpType(pMesh->mColors[1][vertIdx]);

        ref_mb.SetColor1(finalVertIdx, color, meshVertexColorConversion);
      }

      if (ref_mb.GetVertexStreamConfig().HasTangent() && pMesh->HasTangentsAndBitangents())
      {
        WVec3 normal = normalsTransform * ConvertAssimpType(pMesh->mNormals[vertIdx]);
        WVec3 tangent = normalsTransform * ConvertAssimpType(pMesh->mTangents[vertIdx]);
        WVec3 bitangent = normalsTransform * ConvertAssimpType(pMesh->mBitangents[vertIdx]);

        normal.NormalizeIfNotZero(WVec3::MakeZero()).IgnoreResult();
        tangent.NormalizeIfNotZero(WVec3::MakeZero()).IgnoreResult();
        bitangent.NormalizeIfNotZero(WVec3::MakeZero()).IgnoreResult();

        const float fBitangentSign = WMath::Abs(tangent.CrossRH(bitangent).Dot(normal));

        ref_mb.SetTangent(finalVertIdx, tangent.GetAsVec4(fBitangentSign));
      }
    }
  }

  static void AllocateMeshStreams(WMeshBufferResourceDescriptor& ref_mb, WArrayPtr<aiMesh*> referenceMeshes, WUInt32 uiTotalMeshVertices, WUInt32 uiTotalMeshTriangles, bool bHighPrecision, bool bImportSkinningData)
  {
    ref_mb.AddCommonStreams(bHighPrecision);

    if (bImportSkinningData)
    {
      ref_mb.AddStream(WMeshVertexStreamType::SkinningData);
    }

    bool bTexCoords1 = false;
    bool bVertexColors0 = false;
    bool bVertexColors1 = false;

    for (auto pMesh : referenceMeshes)
    {
      if (pMesh->HasTextureCoords(1))
        bTexCoords1 = true;
      if (pMesh->HasVertexColors(0))
        bVertexColors0 = true;
      if (pMesh->HasVertexColors(1))
        bVertexColors1 = true;
    }

    if (bTexCoords1)
    {
      ref_mb.AddStream(WMeshVertexStreamType::TexCoord1);
    }

    if (bVertexColors0)
    {
      ref_mb.AddStream(WMeshVertexStreamType::Color0);
    }
    if (bVertexColors1)
    {
      ref_mb.AddStream(WMeshVertexStreamType::Color1);
    }

    ref_mb.AllocateStreams(uiTotalMeshVertices, WGALPrimitiveTopology::Triangles, uiTotalMeshTriangles, true);
  }

  static void SetMeshBindPoseData(WMeshResourceDescriptor& ref_mrd, const aiMesh* pMesh, const WMat4& mGlobalTransform)
  {
    if (!pMesh->HasBones())
      return;

    WHashedString hs;

    for (WUInt32 b = 0; b < pMesh->mNumBones; ++b)
    {
      auto pBone = pMesh->mBones[b];

      auto invPose = ConvertAssimpType(pBone->mOffsetMatrix);
      W_VERIFY(invPose.Invert(0.0f).Succeeded(), "Inverting the bind pose matrix failed");
      invPose = mGlobalTransform * invPose;
      W_VERIFY(invPose.Invert(0.0f).Succeeded(), "Inverting the bind pose matrix failed");

      hs.Assign(pBone->mName.C_Str());
      ref_mrd.m_Bones[hs].m_GlobalInverseRestPoseMatrix = invPose;
    }
  }

  WResult ImporterAssimp::RecomputeTangents()
  {
    auto& md = m_Options.m_pMeshOutput->MeshBufferDesc();

    if (!md.HasIndexBuffer())
      return W_FAILURE;

    WUInt32 uiNormalsStride = 0;
    WUInt32 uiTexCoordsStride = 0;
    WUInt32 uiTangentsStride = 0;

    const WVec3* pPositions = md.GetPositionData().GetPtr();
    const WUInt8* pNormals = md.GetNormalData(&uiNormalsStride).GetPtr();
    const WUInt8* pTexCoords = md.GetTexCoord0Data(&uiTexCoordsStride).GetPtr();
    WUInt8* pTangents = md.GetTangentData(&uiTangentsStride).GetPtr();

    if (pPositions == nullptr || pNormals == nullptr || pTexCoords == nullptr || pTangents == nullptr)
      return W_FAILURE;

    const WGALResourceFormat::Enum normalsFormat = md.GetVertexStreamConfig().GetNormalFormat();
    const WGALResourceFormat::Enum texCoordsFormat = md.GetVertexStreamConfig().GetTexCoordFormat();
    const WGALResourceFormat::Enum tangentsFormat = md.GetVertexStreamConfig().GetTangentFormat();

    const WUInt32 uiVertexCount = md.GetVertexCount();
    const WUInt32 uiIndexCount = md.GetPrimitiveCount() * 3;

    // meshopt needs plain float data, the streams are stored in packed GPU formats
    WTempArray<WVec3> normals;
    normals.SetCountUninitialized(uiVertexCount);

    WTempArray<WVec2> texCoords;
    texCoords.SetCountUninitialized(uiVertexCount);

    for (WUInt32 v = 0; v < uiVertexCount; ++v)
    {
      WMeshBufferUtils::DecodeNormal(WConstByteArrayPtr(pNormals + (v * uiNormalsStride), 32), normalsFormat, normals[v]).IgnoreResult();
      WMeshBufferUtils::DecodeTexCoord(WConstByteArrayPtr(pTexCoords + (v * uiTexCoordsStride), 32), texCoordsFormat, texCoords[v]).IgnoreResult();
    }

    // one tangent per triangle corner
    WTempArray<WVec4> tangents;
    tangents.SetCountUninitialized(uiIndexCount);

    if (md.Uses32BitIndices())
    {
      const WUInt32* pIndices = reinterpret_cast<const WUInt32*>(md.GetIndexBufferData().GetData());
      meshopt_generateTangents(&tangents[0].x, pIndices, uiIndexCount, &pPositions[0].x, uiVertexCount, sizeof(WVec3), &normals[0].x, sizeof(WVec3), &texCoords[0].x, sizeof(WVec2), meshopt_TangentCompatible);

      // Corners of a vertex that is shared between faces with different tangent spaces (UV mirror seams)
      // overwrite each other, since the vertices are not split up here.
      for (WUInt32 i = 0; i < uiIndexCount; ++i)
      {
        WMeshBufferUtils::EncodeTangent(tangents[i].GetAsVec3(), tangents[i].w, WByteArrayPtr(pTangents + (pIndices[i] * uiTangentsStride), 32), tangentsFormat).IgnoreResult();
      }
    }
    else
    {
      const WUInt16* pIndices = reinterpret_cast<const WUInt16*>(md.GetIndexBufferData().GetData());
      meshopt_generateTangents(&tangents[0].x, pIndices, uiIndexCount, &pPositions[0].x, uiVertexCount, sizeof(WVec3), &normals[0].x, sizeof(WVec3), &texCoords[0].x, sizeof(WVec2), meshopt_TangentCompatible);

      for (WUInt32 i = 0; i < uiIndexCount; ++i)
      {
        WMeshBufferUtils::EncodeTangent(tangents[i].GetAsVec3(), tangents[i].w, WByteArrayPtr(pTangents + (pIndices[i] * uiTangentsStride), 32), tangentsFormat).IgnoreResult();
      }
    }

    return W_SUCCESS;
  }

  WResult ImporterAssimp::PrepareOutputMesh()
  {
    if (m_Options.m_pMeshOutput == nullptr)
      return W_SUCCESS;

    auto& mb = m_Options.m_pMeshOutput->MeshBufferDesc();

    if (m_Options.m_bImportSkinningData)
    {
      for (auto itMesh : m_MeshInstances)
      {
        for (const auto& mi : itMesh.Value())
        {
          SetMeshBindPoseData(*m_Options.m_pMeshOutput, mi.m_pMesh, mi.m_GlobalTransform);
        }
      }

      WUInt16 uiBoneCounter = 0;
      for (auto itBone : m_Options.m_pMeshOutput->m_Bones)
      {
        itBone.Value().m_uiBoneIndex = uiBoneCounter;
        ++uiBoneCounter;
      }
    }

    const bool b8BitBoneIndices = m_Options.m_pMeshOutput->m_Bones.GetCount() <= 255;

    AllocateMeshStreams(mb, WArrayPtr<aiMesh*>(m_pScene->mMeshes, m_pScene->mNumMeshes), m_uiTotalMeshVertices, m_uiTotalMeshTriangles, m_Options.m_bHighPrecision, m_Options.m_bImportSkinningData);

    WUInt32 uiMeshPrevTriangleIdx = 0;
    WUInt32 uiMeshCurVertexIdx = 0;
    WUInt32 uiMeshCurTriangleIdx = 0;
    WUInt32 uiMeshCurSubmeshIdx = 0;

    const bool bFlipTriangles = WGraphicsUtils::IsTriangleFlipRequired(m_Options.m_RootTransform);

    for (auto itMesh : m_MeshInstances)
    {
      const WUInt32 uiMaterialIdx = itMesh.Key();

      for (const auto& mi : itMesh.Value())
      {
        if (m_Options.m_bImportSkinningData && !mi.m_pMesh->HasBones())
        {
          // skip meshes that have no bones
          continue;
        }

        SetMeshVertexData(mb, mi.m_pMesh, mi.m_GlobalTransform, uiMeshCurVertexIdx, m_Options.m_MeshVertexColorConversion);

        if (m_Options.m_bImportSkinningData)
        {
          SetMeshBoneData(mb, *m_Options.m_pMeshOutput, m_Options.m_pMeshOutput->m_fMaxBoneVertexOffset, mi.m_pMesh, uiMeshCurVertexIdx, m_Options.m_bNormalizeWeights);
        }

        SetMeshTriangleIndices(mb, mi.m_pMesh, uiMeshCurTriangleIdx, uiMeshCurVertexIdx, bFlipTriangles);

        uiMeshCurTriangleIdx += mi.m_pMesh->mNumFaces;
        uiMeshCurVertexIdx += mi.m_pMesh->mNumVertices;
      }

      if (uiMeshCurTriangleIdx - uiMeshPrevTriangleIdx == 0)
      {
        // skip empty submeshes
        continue;
      }

      if (uiMaterialIdx >= m_OutputMaterials.GetCount())
      {
        m_Options.m_pMeshOutput->SetMaterial(uiMeshCurSubmeshIdx, "");
      }
      else
      {
        m_OutputMaterials[uiMaterialIdx].m_iReferencedByMesh = static_cast<WInt32>(uiMeshCurSubmeshIdx);
        m_Options.m_pMeshOutput->SetMaterial(uiMeshCurSubmeshIdx, m_OutputMaterials[uiMaterialIdx].m_sName);
      }

      m_Options.m_pMeshOutput->AddSubMesh(uiMeshCurTriangleIdx - uiMeshPrevTriangleIdx, uiMeshPrevTriangleIdx, uiMeshCurSubmeshIdx);

      uiMeshPrevTriangleIdx = uiMeshCurTriangleIdx;
      ++uiMeshCurSubmeshIdx;
    }

    if (m_Options.m_bImportSkinningData)
    {
      CheckBoneWeights(mb, *m_Options.m_pMeshOutput, m_Options.m_pMeshOutput->m_fMaxBoneVertexOffset, m_Options.m_bNormalizeWeights);
    }

    m_Options.m_pMeshOutput->ComputeBounds();

    return W_SUCCESS;
  }
} // namespace WModelImporter2
