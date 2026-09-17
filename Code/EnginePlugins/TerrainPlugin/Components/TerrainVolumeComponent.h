#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/TagSet.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <TerrainPlugin/TerrainPluginDLL.h>
#include <TerrainPlugin/TerrainSystem.h>

struct WMsgExtractGeometry;
struct WMsgExtractRenderData;
struct WMsgTransformChanged;

using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;
using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;
using WCpuMeshResourceHandle = WTypedResourceHandle<class WCpuMeshResource>;

using WTerrainVolumeComponentManager = WComponentManager<class WTerrainVolumeComponent, WBlockStorageType::Compact>;

/// Renders a voxel volume built from terrain brushes.
///
/// Brushes with ModifyMode != Ignore fill voxels within their sphere (OuterRadius) to solid.
/// Voxels are baked on the GPU and triangulated on the GPU. The resulting mesh is rendered
/// directly from the GPU-side buffers. Physics collision is baked to disk at export time.
class W_TERRAINPLUGIN_DLL WTerrainVolumeComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WTerrainVolumeComponent, WRenderComponent, WTerrainVolumeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

protected:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;
  void OnMsgTransformChanged(WMsgTransformChanged& msg);

  /// Provides the triangulated voxel surface, for navmesh generation, geometry export and similar.
  ///
  /// The mesh only exists on the GPU, so this reads it back, which is slow. The result is cached.
  /// A volume with the collider disabled provides nothing for a collision mesh, since it is not meant
  /// to be part of the world's physical representation, but it still provides its render geometry.
  void OnMsgExtractGeometry(WMsgExtractGeometry& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // WTerrainVolumeComponent

public:
  WTerrainVolumeComponent();
  ~WTerrainVolumeComponent();

  void SetResolution(WEnum<WTerrainResolution> resolution);  // [ property ]
  WEnum<WTerrainResolution> GetResolution() const { return m_Resolution; }

  void SetSize(float f);                                       // [ property ]
  float GetSize() const { return m_fSize; }

  void SetMaterial(const WMaterialResourceHandle& hMaterial); // [ property ]
  const WMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }

  void SetBaseMaterialIndex(WUInt8 i);                        // [ property ]
  WUInt8 GetBaseMaterialIndex() const { return m_uiBaseMaterialIndex; }

  void SetFillHeight(float f);                                 // [ property ]
  float GetFillHeight() const { return m_fFillHeight; }

  void SetEnableCollider(bool bEnable);                        // [ property ]
  bool GetEnableCollider() const { return m_bEnableCollider; } // [ property ]

  void SetCleanupIterations(WUInt8 n);                        // [ property ]
  WUInt8 GetCleanupIterations() const { return m_uiCleanupIterations; }

  const WTagSet& GetTags() const { return m_Tags; }           // [ property ]
  void Reflection_SetTag(const char* szTagName);               // [ property ]
  void Reflection_RemoveTag(const char* szTagName);            // [ property ]

  /// Per-material-index physics surface handles. Entry i is the surface used when the vertex material index == i and MaterialStrength > 0.5.
  WUInt32 Surfaces_GetCount() const;
  WString Surfaces_GetValue(WUInt32 uiIndex) const;
  void Surfaces_SetValue(WUInt32 uiIndex, WString sValue);
  void Surfaces_Insert(WUInt32 uiIndex, WString sValue);
  void Surfaces_Remove(WUInt32 uiIndex);

  WUInt32 GetVoxelIndex() const { return m_uiVoxelIndex; }

  /// Stable ID derived from the document GUID at object creation. Used to generate a deterministic cache file name.
  WUInt64 GetStableId() const { return m_uiStableId; }

  /// Computes a content hash over all voxel-relevant properties of this component.
  ///
  /// Used to detect whether a baked collision mesh is still up to date.
  WUInt64 ComputeColliderContentHash(WUInt64 uiBrushOverlapHash) const;

private:
  void OnObjectCreated(const WAbstractObjectNode& node);

  /// Builds (or returns the cached) CPU mesh of the voxel surface. Empty handle if unavailable.
  ///
  /// Blocks on a GPU readback, so this is only meant to be called for an explicit user action
  /// (exporting the scene, generating a navmesh), not per frame. It requires the terrain system to
  /// exist already, since it may only take a read lock on the world.
  WCpuMeshResourceHandle GenerateCpuMesh() const;

  mutable WCpuMeshResourceHandle m_hCpuMesh;

  /// ComputeColliderContentHash() of the cached mesh, to detect that the volume changed underneath it.
  mutable WUInt64 m_uiCpuMeshHash = 0;

  WUInt32 m_uiVoxelIndex = WInvalidIndex;
  WUInt64 m_uiStableId = 0;
  WEnum<WTerrainResolution> m_Resolution;
  bool m_bEnableCollider = true;
  WUInt8 m_uiBaseMaterialIndex = 0;
  WUInt8 m_uiCleanupIterations = 2;
  float m_fSize = 64.0f;
  float m_fFillHeight = -1.0f;
  WMaterialResourceHandle m_hMaterial;
  mutable WInstanceDataOffset m_InstanceDataOffset;
  WTagSet m_Tags; ///< Identity tags matched against brush include-tag filters.

  /// Physics surface handles indexed by voxel material index.
  WDynamicArray<WSurfaceResourceHandle> m_Surfaces;
};
