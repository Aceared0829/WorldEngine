#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/TagSet.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <TerrainPlugin/TerrainPluginDLL.h>
#include <TerrainPlugin/TerrainSystem.h>

struct WMsgExtractGeometry;
struct WMsgExtractRenderData;
struct WMsgExtractOccluderData;
struct WMsgTransformChanged;
struct WResourceEvent;
class WAbstractObjectNode;

using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;
using WCpuMeshResourceHandle = WTypedResourceHandle<class WCpuMeshResource>;

class W_TERRAINPLUGIN_DLL WTerrainPatchComponentManager : public WComponentManager<class WTerrainPatchComponent, WBlockStorageType::Compact>
{
  using SUPER = WComponentManager<WTerrainPatchComponent, WBlockStorageType::Compact>;

public:
  WTerrainPatchComponentManager(WWorld* pWorld);
  ~WTerrainPatchComponentManager();

  virtual void Initialize() override;

private:
  void Update(const WWorldModule::UpdateContext& context);
  void ResourceEventHandler(const WResourceEvent& e);
};

/// Renders a single terrain patch as a procedural grid.
///
/// The vertex shader generates grid geometry from SV_VertexID; no vertex buffer is needed.
/// Heights are stored in a GPU structured buffer baked by a compute shader via WTerrainSystem.
class W_TERRAINPLUGIN_DLL WTerrainPatchComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WTerrainPatchComponent, WRenderComponent, WTerrainPatchComponentManager);

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
  void OnMsgExtractOccluderData(WMsgExtractOccluderData& msg) const;

  void OnMsgTransformChanged(WMsgTransformChanged& msg);

  /// Provides the terrain surface as a triangle mesh, for navmesh generation, geometry export and similar.
  ///
  /// The heights only exist on the GPU, so this reads them back, which is slow and only possible on the
  /// main thread. The mesh is therefore built on demand and cached.
  ///
  /// Render geometry is provided at the full render resolution, collision geometry at the resolution the
  /// collider setting asks for. Neither includes the skirt, which only exists to hide LOD seams. A patch
  /// with the collider disabled provides nothing for a collision mesh, since it is not meant to be part
  /// of the world's physical representation, but it still provides its render geometry.
  void OnMsgExtractGeometry(WMsgExtractGeometry& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // WTerrainPatchComponent

public:
  WTerrainPatchComponent();
  ~WTerrainPatchComponent();

  void SetResolution(WEnum<WTerrainResolution> resolution);
  WEnum<WTerrainResolution> GetResolution() const { return m_Resolution; }

  void SetSize(float fSize);
  float GetSize() const { return m_fSize; }

  void SetMaterial(const WMaterialResourceHandle& hMaterial);                       // [ property ]
  const WMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }        // [ property ]

  void SetHeightImage(const WImageDataResourceHandle& hImage);                      // [ property ]
  const WImageDataResourceHandle& GetHeightImage() const { return m_hHeightImage; } // [ property ]

  /// UV offset into the height image; selects the top-left corner of the sampled rectangle.
  WVec2 GetHeightImageOffset() const { return m_vImageOffset; } // [ property ]
  void SetHeightImageOffset(WVec2 vOffset);                     // [ property ]

  /// UV size of the rectangle sampled from the height image. Values < 1 select a sub-region.
  WVec2 GetHeightImageSize() const { return m_vImageSize; } // [ property ]
  void SetHeightImageSize(WVec2 vSize);                     // [ property ]

  /// Multiplier applied to the [0, 1] greyscale sample to produce a world-space height.
  float GetHeightImageScale() const { return m_fHeightScale; }                      // [ property ]
  void SetHeightImageScale(float fScale);                                           // [ property ]

  void SetCollider(WEnum<WTerrainPatchColliderMode> mode);                        // [ property ]
  WEnum<WTerrainPatchColliderMode> GetCollider() const { return m_ColliderMode; } // [ property ]

  WUInt8 GetBaseMaterialIndex() const { return m_uiBaseMaterialIndex; }            // [ property ]
  void SetBaseMaterialIndex(WUInt8 uiIndex);                                       // [ property ]

  /// On-screen height, in pixels, that one grid cell has to fall below before the patch switches to
  /// the next-coarser LOD. Larger values keep more detail (switching later, at a greater distance),
  /// smaller values coarsen sooner.
  ///
  /// The value is measured against a 1080-pixel-high view, not the actual viewport, so the LOD a
  /// patch picks does not change with the window size or display resolution. Terrain.LodQuality
  /// scales it globally to drive a quality setting.
  ///
  /// 0 ("LOD Disabled") turns automatic LOD off entirely: the patch always renders at full
  /// resolution and the skirt (only needed to hide seams against a coarser LOD neighbor) is off too.
  float GetLodCellPixelSize() const { return m_fLodCellPixelSize; } // [ property ]
  void SetLodCellPixelSize(float fPixels);                          // [ property ]

  /// Size in meters of one cell of the CPU occlusion culling geometry. 0 disables the occluder.
  ///
  /// The occluder is a coarse mesh, baked during scene export and when the editor starts the simulation, so a
  /// patch has no occluder in the plain editor viewport. The value is only read during that bake, changing it
  /// afterwards has no effect until the next one. A patch whose Collider mode is 'None' gets no occluder,
  /// since both are baked from the same data, and the value is clamped to the collider's grid spacing.
  float GetOcclusionCellSize() const { return m_fOcclusionCellSize; } // [ property ]
  void SetOcclusionCellSize(float fCellSize);                         // [ property ]

  /// Replaces the baked occluder mesh (local space). Called by the scene export modifier, empty arrays
  /// remove the occluder.
  void SetBakedOccluder(WArrayPtr<const WVec3> vertices, WArrayPtr<const WUInt32> indices);

  /// Draws the occluder geometry as solid, single sided triangles. Called for all patches while the CVar
  /// 'Terrain.VisOccluder' is enabled.
  void DebugDrawOccluder() const;

  const WTagSet& GetTags() const { return m_Tags; }                // [ property ]
  void Reflection_SetTag(const char* szTagName);                    // [ property ]
  void Reflection_RemoveTag(const char* szTagName);                 // [ property ]

  /// Per-material-index physics surface handles. Entry i is the surface used when dominant material index == i.
  WUInt32 Surfaces_GetCount() const;
  WString Surfaces_GetValue(WUInt32 uiIndex) const;
  void Surfaces_SetValue(WUInt32 uiIndex, WString sValue);
  void Surfaces_Insert(WUInt32 uiIndex, WString sValue);
  void Surfaces_Remove(WUInt32 uiIndex);

  /// Returns the index into WTerrainSystem for this patch, or WInvalidIndex if not activated.
  WUInt32 GetHeightfieldIndex() const { return m_uiHeightfieldIndex; }

  /// Returns the stable identifier derived from the component's editor UUID.
  /// Used to construct deterministic file paths for baked collider assets.
  WUInt64 GetStableId() const { return m_uiStableId; }

  /// Computes the content hash used to detect whether the baked collider file is up to date.
  /// Pass the brush overlap hash from WTerrainSystem::GetHeightfieldBrushOverlapHash().
  WUInt64 ComputeColliderContentHash(WUInt64 uiBrushOverlapHash) const;

private:
  void OnObjectCreated(const WAbstractObjectNode& node);

  /// Builds (or returns the cached) CPU mesh of the terrain surface. Empty handle if unavailable.
  ///
  /// uiStride is the spacing, in full-resolution cells, between the mesh's vertices. 1 reproduces the
  /// render resolution, higher values sub-sample it. It has to divide the cell count evenly, otherwise
  /// the mesh would not span the whole patch.
  ///
  /// Blocks on a GPU readback of the height data, so this is only meant to be called for an explicit
  /// user action (exporting the scene, generating a navmesh), not per frame. It requires the terrain
  /// system to exist already, since it may only take a read lock on the world.
  WCpuMeshResourceHandle GenerateCpuMesh(WUInt32 uiStride) const;

  /// One cache slot per WWorldGeoExtractionUtil::ExtractionMode, since the two resolutions differ.
  mutable WCpuMeshResourceHandle m_hCpuMesh[2];

  /// ComputeColliderContentHash() of each cached mesh, to detect that the terrain changed underneath it.
  mutable WUInt64 m_uiCpuMeshHash[2] = {0, 0};

  /// (Re)creates m_pOccluderObject and m_OccluderBounds from the baked mesh. Clears both when no mesh
  /// has been baked.
  void UpdateOccluder();

  WUInt32 m_uiHeightfieldIndex = WInvalidIndex;
  WUInt64 m_uiStableId = 0;
  mutable WInstanceDataOffset m_InstanceDataOffset;

  WUInt8 m_uiBaseMaterialIndex = 0;
  WEnum<WTerrainResolution> m_Resolution;
  float m_fSize = 32.0f;
  WMaterialResourceHandle m_hMaterial;
  WEnum<WTerrainPatchColliderMode> m_ColliderMode;
  WTagSet m_Tags;
  WDynamicArray<WSurfaceResourceHandle> m_Surfaces;

  WImageDataResourceHandle m_hHeightImage;
  WVec2 m_vImageOffset = WVec2::MakeZero();
  WVec2 m_vImageSize = WVec2(1.0f);
  float m_fHeightScale = 32.0f;
  float m_fLodCellPixelSize = 16.0f;
  bool m_bHeightImageDirty = false; ///< Set when the image resource reloads

  float m_fOcclusionCellSize = 0.0f;
  WDynamicArray<WVec3> m_OccluderVertices; ///< Local space vertices of the baked occluder mesh.
  WDynamicArray<WUInt32> m_OccluderIndices;
  WBoundingBox m_OccluderBounds;            ///< Local space bounds of the occluder geometry. Invalid when there is none.
  WSharedPtr<const WRasterizerObject> m_pOccluderObject;
};
