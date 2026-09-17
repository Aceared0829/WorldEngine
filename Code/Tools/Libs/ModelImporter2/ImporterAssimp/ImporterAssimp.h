#pragma once

#include <ModelImporter2/Importer/Importer.h>

#include <assimp/Importer.hpp>

class WEditableSkeletonJoint;
struct aiNode;
struct aiMesh;

namespace WModelImporter2
{
  class ImporterAssimp : public Importer
  {
  public:
    ImporterAssimp();
    ~ImporterAssimp();

  protected:
    virtual WResult DoImport() override;

  private:
    WResult TraverseAiScene();

    WResult PrepareOutputMesh();
    WResult RecomputeTangents();

    WResult TraverseAiNode(aiNode* pNode, const WMat4& parentTransform, WEditableSkeletonJoint* pCurJoint);
    WResult ProcessAiMesh(aiMesh* pMesh, const WMat4& transform);

    WResult ImportMaterials();
    WResult ImportAnimations();

    WResult ImportBoneColliders(WEditableSkeletonJoint* pJoint);

    void SimplifyAiMesh(aiMesh* pMesh);

    Assimp::Importer m_Importer;
    const aiScene* m_pScene = nullptr;
    WUInt32 m_uiTotalMeshVertices = 0;
    WUInt32 m_uiTotalMeshTriangles = 0;

    struct MeshInstance
    {
      WMat4 m_GlobalTransform;
      aiMesh* m_pMesh;
    };

    WMap<WUInt32, WHybridArray<MeshInstance, 4>> m_MeshInstances;

    WSet<aiMesh*> m_OptimizedMeshes;
  };

  extern WColor ConvertAssimpType(const aiColor3D& value, bool bInvert = false);
  extern WColor ConvertAssimpType(const aiColor4D& value, bool bInvert = false);
  extern WMat4 ConvertAssimpType(const aiMatrix4x4& value, bool bDummy = false);
  extern WVec3 ConvertAssimpType(const aiVector3D& value, bool bDummy = false);
  extern WQuat ConvertAssimpType(const aiQuaternion& value, bool bDummy = false);
  extern float ConvertAssimpType(float value, bool bDummy = false);
  extern int ConvertAssimpType(int value, bool bDummy = false);

} // namespace WModelImporter2
