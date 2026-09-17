#pragma once

#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;

/// Resource representing a renderable mesh.
///
/// Contains sub-meshes with materials, references a mesh buffer for vertex/index data,
/// and stores bounding information. For skinned meshes, also includes skeleton and bone data.
class W_RENDERERCORE_DLL WMeshResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WMeshResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WMeshResource);
  W_RESOURCE_DECLARE_CREATEABLE(WMeshResource, WMeshResourceDescriptor);

public:
  WMeshResource();

  /// Returns the array of sub-meshes in this mesh.
  WArrayPtr<const WMeshResourceDescriptor::SubMesh> GetSubMeshes() const { return m_SubMeshes; }

  /// Returns the mesh buffer that is used by this resource.
  const WMeshBufferResourceHandle& GetMeshBuffer() const { return m_hMeshBuffer; }

  /// Returns the default materials for this mesh.
  WArrayPtr<const WMaterialResourceHandle> GetMaterials() const { return m_Materials; }

  /// Returns the bounds of this mesh.
  const WBoundingBoxSphere& GetBounds() const { return m_Bounds; }

  /// Returns the asset hash for this mesh. Returns 0 if the mesh was not loaded from an asset file.
  WUInt64 GetAssetHash() const { return m_uiAssetHash; }

  // TODO: clean up
  WSkeletonResourceHandle m_hDefaultSkeleton;
  WHashTable<WHashedString, WMeshResourceDescriptor::BoneData> m_Bones;
  float m_fMaxBoneVertexOffset = 0.0f; // the maximum distance between any vertex and its influencing bones, can be used for adjusting the bounding box of a pose

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WBoundingBoxSphere m_Bounds;

  WDynamicArray<WMeshResourceDescriptor::SubMesh> m_SubMeshes;
  WMeshBufferResourceHandle m_hMeshBuffer;
  WDynamicArray<WMaterialResourceHandle> m_Materials;

  WUInt64 m_uiAssetHash = 0;

  static WUInt32 s_uiMeshBufferNameSuffix;
};

using WMeshResourceHandle = WTypedResourceHandle<class WMeshResource>;
