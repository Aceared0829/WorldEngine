#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

class WShaderTransform;

/// Render data for skinned meshes.
///
/// Extends mesh render data with a GPU buffer handle for bone transformation matrices.
class W_RENDERERCORE_DLL WSkinnedMeshRenderData : public WMeshRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WSkinnedMeshRenderData, WMeshRenderData);

public:
  virtual bool CanBatch(const WRenderData& other) const override;

  WGALDynamicBufferHandle m_hSkinningBuffer;
};

/// Manages the skinning state for an animated mesh.
///
/// Wraps around the WRenderDataManager functions to manage skinning data for skinned meshes.
struct W_RENDERERCORE_DLL WSkinningState
{
  WSkinningState();
  ~WSkinningState();

  void Clear();

  /// Returns a writable array of bone transforms, allocating or reallocating the buffer as needed. Note that existing data is lost on reallocation.
  WArrayPtr<WShaderTransform> GetOrCreateBoneTransformsForWriting(WComponent& ref_ownerComponent, WUInt32 uiNumBones);

  WArrayPtr<const WShaderTransform> GetBoneTransformsForReading() const;

  bool HasBoneTransforms() const { return m_uiNumBones > 0; }

  WCustomInstanceDataOffset m_DataOffset;
  WUInt32 m_uiNumBones = 0;
  WWorld* m_pWorld = nullptr;
};
