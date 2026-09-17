#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

/// Descriptor for creating mesh resources.
///
/// Defines sub-meshes with materials, references or creates mesh buffer data,
/// and stores bounding information. Used both for procedural mesh generation and
/// loading from files.
class W_RENDERERCORE_DLL WMeshResourceDescriptor
{
public:
  /// Describes a sub-mesh within the mesh.
  ///
  /// Each sub-mesh references a range of primitives and a material slot.
  struct SubMesh
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_uiPrimitiveCount;  ///< Number of primitives in this sub-mesh.
    WUInt32 m_uiFirstPrimitive;  ///< Index of the first primitive.
    WUInt32 m_uiMaterialIndex;   ///< Index into the material array.

    WBoundingBoxSphere m_Bounds; ///< Bounding volume of this sub-mesh.
  };

  /// Material slot information.
  struct Material
  {
    WString m_sPath; ///< Path or GUID of the material resource.
  };

  WMeshResourceDescriptor();

  void Clear();

  /// Returns the mesh buffer descriptor for creating a new mesh buffer.
  ///
  /// Use this when building mesh data procedurally. Mutually exclusive with UseExistingMeshBuffer.
  WMeshBufferResourceDescriptor& MeshBufferDesc();

  const WMeshBufferResourceDescriptor& MeshBufferDesc() const;

  /// Uses an existing mesh buffer instead of creating a new one.
  ///
  /// Mutually exclusive with modifying MeshBufferDesc.
  void UseExistingMeshBuffer(const WMeshBufferResourceHandle& hBuffer);

  /// Adds a sub-mesh to the descriptor.
  void AddSubMesh(WUInt32 uiPrimitiveCount, WUInt32 uiFirstPrimitive, WUInt32 uiMaterialIndex);

  /// Sets the material path for a material slot.
  void SetMaterial(WUInt32 uiMaterialIndex, WStringView sPathToMaterial);

  void Save(WStreamWriter& inout_stream);
  WResult Save(const char* szFile);

  WResult Load(WStreamReader& inout_stream);
  WResult Load(const char* szFile);

  const WMeshBufferResourceHandle& GetExistingMeshBuffer() const;

  WArrayPtr<const Material> GetMaterials() const;

  WArrayPtr<const SubMesh> GetSubMeshes() const;

  /// Merges all submeshes into just one.
  void CollapseSubMeshes();

  void ComputeBounds();
  const WBoundingBoxSphere& GetBounds() const;
  void SetBounds(const WBoundingBoxSphere& bounds) { m_Bounds = bounds; }

  /// Data for a bone used in skinned meshes.
  struct BoneData
  {
    WMat4 m_GlobalInverseRestPoseMatrix;         ///< Transform from mesh space to bone space.
    WUInt16 m_uiBoneIndex = WInvalidJointIndex; ///< Index into the skeleton.

    WResult Serialize(WStreamWriter& inout_stream) const;
    WResult Deserialize(WStreamReader& inout_stream);
  };

  WSkeletonResourceHandle m_hDefaultSkeleton;   ///< Default skeleton for skinned meshes.
  WHashTable<WHashedString, BoneData> m_Bones; ///< Bone data indexed by bone name.

  /// Maximum distance between any vertex and its influencing bones.
  ///
  /// Can be used for adjusting the bounding box of an animated pose.
  float m_fMaxBoneVertexOffset = 0.0f;

private:
  WHybridArray<Material, 8> m_Materials;
  WHybridArray<SubMesh, 8> m_SubMeshes;
  WMeshBufferResourceDescriptor m_MeshBufferDescriptor;
  WMeshBufferResourceHandle m_hMeshBuffer;
  WBoundingBoxSphere m_Bounds;
};
