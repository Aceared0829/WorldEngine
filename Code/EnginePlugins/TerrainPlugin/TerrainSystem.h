#pragma once

#include <Core/World/WorldModule.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/TagSet.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Device/Device.h>
#include <TerrainPlugin/TerrainPluginDLL.h>

#include <Shaders/Terrain/Generation/TerrainBrushData.h>
#include <Shaders/Terrain/Generation/VoxelMeshConstants.h>

class WRenderGraph;

struct W_TERRAINPLUGIN_DLL WTerrainModifyMode
{
  using StorageType = WUInt8;
  enum Enum : WUInt8
  {
    Max = 0,         ///< Only raises terrain toward the brush height.
    Min = 1,         ///< Only lowers terrain toward the brush height.
    Set = 2,         ///< Forces terrain to the brush height regardless of current value.
    Carve = 3,
    Add = 4,
    OnlyPaint2D = 5, ///< No geometry change. Paints material using 2D SDF projection (2D brush style).
    OnlyPaint3D = 6, ///< No geometry change. Paints material using 3D SDF (3D brush style).
    Default = Max,
  };
};

struct W_TERRAINPLUGIN_DLL WTerrainResolution
{
  using StorageType = WUInt16;
  enum Enum : WUInt16
  {
    Res32 = 32,
    Res64 = 64,
    Res128 = 128,
    Res256 = 256,
    Res512 = 512,
    Default = Res256,
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_TERRAINPLUGIN_DLL, WTerrainResolution);

/// Full-resolution quads spanned by one baked material cell along each axis.
/// Must match TERRAIN_MATERIAL_CELL_STEP in Shaders/Terrain/Generation/HeightfieldBakeConstants.h,
/// which documents it. Together they fix the size and layout of the material buffers.
inline constexpr WUInt32 WTerrainMaterialCellStep = 4;

/// Vertex sampling stride for the heightfield patch collision mesh.
/// The enumerator value equals the vertex stride (skip factor) used when sub-sampling full-resolution data.
struct W_TERRAINPLUGIN_DLL WTerrainPatchColliderMode
{
  using StorageType = WUInt8;

  enum Enum : WUInt8
  {
    None = 0,              ///< No collision shape is generated.
    FullResolution = 1,    ///< Collision mesh matches the render resolution.
    HalfResolution = 2,    ///< Samples every second vertex — 1/4 the triangles of full resolution.
    QuarterResolution = 4, ///< Samples every fourth vertex — 1/16 the triangles of full resolution.
    EighthResolution = 8,  ///< Samples every eighth vertex — 1/64 the triangles of full resolution.

    Default = QuarterResolution,
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_TERRAINPLUGIN_DLL, WTerrainPatchColliderMode);

/// CPU-side description of one terrain modification brush.
///
/// Components write to this each frame; the terrain system uploads the data to GPU before each bake.
struct WTerrainData_Brush
{
  bool m_bInUse = false;
  bool m_bAffectHeightfields = true;
  bool m_bAffectVoxels = true;
  WInt8 m_iPriority = 0;                      ///< Sort order. Higher = applied later = wins over lower-priority brushes. Equal priorities use mode-based ordering.
  WUInt8 m_uiMaterialIndex = 0;               ///< Material (layer) index to paint: 0-3.
  WEnum<WTerrainModifyMode> m_ModifyMode;
  WVec3 m_vPosition = WVec3::MakeZero();
  WQuat m_qRotation = WQuat::MakeIdentity(); ///< World-space rotation of the brush.
  WVec2 m_vHalfExtents = WVec2::MakeZero();  ///< Half-size of the straight rectangular region along each brush axis.
  float m_fHalfExtentZ = 0.0f;                 ///< Half-size along the brush Z axis. Used only by Carve/Add mode for a 3D rounded-box volume.
  float m_fHalfExtentYTop = 0.0f;              ///< Upper Y half-size for asymmetric Carve/Add volumes (e.g. flat-floor tunnels). 0 = same as m_vHalfExtents.y (symmetric).
  float m_fInnerRadius = 0.0f;                 ///< Corner rounding radius of the inner (full-weight) region.
  float m_fOuterRadius = 5.0f;                 ///< Corner rounding radius; also the outer edge of the falloff zone.
  float m_fFalloff = 1.0f;                     ///< Exponent applied after smoothstep in the transition zone.
  float m_fMaterialStrength = 0.0f;            ///< Blend weight for material painting in [0, 1]. 0 = disabled (no material write).
  float m_fNoiseStrength = 0.0f;
  float m_fNoiseFrequency = 1.0f;
  WTagSet m_Tags;                             ///< If non-empty, the brush only affects terrain objects that have at least one matching tag.
};

/// CPU-side state for one heightfield patch managed by WTerrainSystem.
struct WTerrainData_Heightfield
{
  bool m_bInUse = false;
  bool m_bDirty = true;
  WUInt16 m_uiCellsPerSide = 128;            ///< Number of rendered quads per side (= resolution enum value).
  WTransform m_GlobalTransform = WTransform::MakeIdentity();
  WGALBufferHandle m_hBakedHeights;          ///< Persistent, bound as UAV in heights CS and SRV in VS.
  WGALBufferHandle m_hBakedNormals;          ///< Persistent, bound as UAV in normals CS and SRV in VS.
  WGALBufferHandle m_hCellMaterials;         ///< Per-cell top-4 material indices (uint/cell), baked by Step3 CS over the stored grid including the border ring. SRV in VS.
  WGALBufferHandle m_hVertexWeights;         ///< Per-cell-corner blend weights relative to that cell's top-4 (4 uint/cell), baked by Step3 CS over the same grid. SRV in VS.
  WGALBufferHandle m_hCarveMask;             ///< One bit per stored-grid vertex, set when carved away. Written by Step2 CS, SRV in VS. Always full resolution.
  WUInt8 m_uiDefaultMaterialIndex = 0;
  WImageDataResourceHandle m_hHeightImage;   ///< Optional greyscale image used as baseline height source; sampled each bake.
  WVec2 m_vImageOffset = WVec2::MakeZero(); ///< UV offset into m_hHeightImage; selects the top-left corner of the sampled rect.
  WVec2 m_vImageSize = WVec2(1.0f);         ///< UV extent of the sampled rect within m_hHeightImage. Values < 1 select a sub-region.
  float m_fHeightScale = 0.0f;                ///< Multiplier applied to the [0, 1] greyscale sample to produce a world-space height.
  float m_fGridSpacing = 1.0f;
  WTagSet m_Tags;                            ///< Identity tags for this heightfield; matched against brush include-tag filters.
  /// Hash of the indices of brushes that spatially overlap this heightfield.
  /// Updated each time brush state is re-evaluated; compared against the previous value to skip
  /// re-bakes when the set of relevant brushes (and whether any are present) has not changed.
  WUInt64 m_uiBrushOverlapHash = 0;
};

/// CPU-side state for one voxel terrain volume managed by WTerrainSystem.
struct WTerrainData_Voxel
{
  bool m_bInUse = false;
  bool m_bDirty = true;
  bool m_bInitialSolid = false;      ///< false = all cells start as air; true = all cells start as solid. Applied before brushes each bake.
  WUInt8 m_uiCleanupIterations = 1; ///< Number of topology-cleanup iterations to run after baking (0–4). Higher values remove more spike voxels at the cost of slightly more GPU work.

  WUInt16 m_uiResolution = 32;      ///< User-selected inner voxels per axis (no border, no alignment). Equals the triangulated cell count.
  WUInt16 m_uiPackPitch = 5;        ///< Packed X row count = BufX / 8, where BufX = ((m_uiResolution + 2*border + 7) & ~7). Stride for BakedVoxels.
  float m_fVoxelSize = 1.0f;         ///< World-space size of one voxel.
  WTransform m_GlobalTransform = WTransform::MakeIdentity();

  /// Final render buffers — written from the shared compact scratch by VoxelCompactCopyCS each rebuild.
  /// Allocated at worst-case capacity in CreateVoxelTerrain; the copy is GPU-driven (DispatchIndirect)
  /// so only the used entries are written, and DrawArgs controls the actual draw count.
  WGALBufferHandle m_hFinalVertices; ///< StructuredBuffer<VoxelGpuVertex> for rendering.
  WGALBufferHandle m_hFinalIndices;  ///< StructuredBuffer<uint> for rendering.
  /// DrawArgs buffer: 4 uints {IndexCount, 1, 0, 0}. Written by VoxelCompactCopyCS thread 0.
  WGALBufferHandle m_hFinalDrawArgs;

  float m_fFillHeight = 0.0f; ///< World-space Z height below which voxels start as solid (when m_bInitialSolid is true).
  WTagSet m_Tags;            ///< Identity tags for this voxel volume; matched against brush include-tag filters.
  WUInt64 m_uiBrushOverlapHash = 0;
};

/// Manages GPU resources and compute shader dispatch for all terrain patches in a world.
///
/// Terrain components register here on activation and receive a slot index.
/// On each BeginRender event the system re-bakes all dirty patches before the main pipeline renders.
/// Baking is also triggered when the set of overlapping brushes changes for a given patch.
class W_TERRAINPLUGIN_DLL WTerrainSystem : public WWorldModule
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WTerrainSystem, WWorldModule);

public:
  WTerrainSystem(WWorld* pWorld);
  ~WTerrainSystem();

private:
  virtual void Initialize() override;
  virtual void Deinitialize() override;

  /// BeginRender handler; drives UpdateTerrain on every registered WTerrainSystem instance.
  static void OnRenderEvent(const WRenderWorldRenderEvent& e);

  /// Processes the deferred deletion queues.
  /// Called at the start of UpdateTerrain and from readback functions before they submit a sync graph.
  void FrameCleanup();

  WMutex m_Mutex;
  WSharedPtr<WRenderGraph> m_pRenderGraph;

  static WDeque<WTerrainSystem*> s_TerrainSystems;

  //////////////////////////////////////////////////////////////////////////
  // Brushes
  //////////////////////////////////////////////////////////////////////////

public:
  /// Allocates a brush slot and returns its index. Reuses freed slots before growing the array.
  WUInt32 CreateBrushData();

  /// Releases the brush slot and sets uiIdx to WInvalidIndex. Marks all brushes dirty for rebake.
  void RemoveBrushData(WUInt32& ref_uiIdx);

  const WTerrainData_Brush& ReadBrushData(WUInt32 uiIdx) const;

  /// Returns a mutable reference and marks the brush set dirty so all patches rebake next frame.
  WTerrainData_Brush& ModifyBrushData(WUInt32 uiIdx);

private:
  /// Creates a transient GPU structured buffer from the provided brush array.
  /// Always allocates at least one element so shader bindings stay valid when the brush list is empty.
  WGALBufferHandle CreateBrushBuffer(WDynamicArray<TerrainBrushData>& brushes, WGALDevice* pDevice) const;

  bool m_bBrushesDirty = true;
  WDeque<WTerrainData_Brush> m_Brushes;

  //////////////////////////////////////////////////////////////////////////
  // Heightfields
  //////////////////////////////////////////////////////////////////////////

public:
  /// Allocates GPU buffers for a new heightfield patch of the given resolution and returns a slot index.
  WUInt32 CreateHeightfieldTerrain(WUInt32 uiCellsPerSide);

  /// Thread-safe: queues the patch for GPU buffer destruction at the start of the next frame.
  void RemoveHeightfieldTerrain(WUInt32& ref_uiPatchIndex);

  /// Returns a mutable reference and marks the patch dirty so it rebakes next frame.
  WTerrainData_Heightfield& ModifyHeightfieldTerrain(WUInt32 uiIdx);

  /// Returns the baked GPU height buffer handle (SRV in VS, UAV in heights CS).
  WGALBufferHandle GetHeightfieldHeightBuffer(WUInt32 uiPatchIndex) const;

  /// Returns the baked GPU normal buffer handle (SRV in VS, UAV in normals CS).
  WGALBufferHandle GetHeightfieldNormalBuffer(WUInt32 uiPatchIndex) const;

  /// Returns the per-cell material index buffer (uint/cell) baked by Step3. Bound as SRV in VS.
  WGALBufferHandle GetHeightfieldCellMaterialBuffer(WUInt32 uiPatchIndex) const;

  /// Returns the per-vertex f16 weight buffer (uint/vertex) baked by Step3. Bound as SRV in VS.
  WGALBufferHandle GetHeightfieldMaterialVertexWeightBuffer(WUInt32 uiPatchIndex) const;

  /// Returns the per-vertex carve bitmask buffer (32 vertices per uint) written by Step2. SRV in VS.
  WGALBufferHandle GetHeightfieldCarveMaskBuffer(WUInt32 uiPatchIndex) const;


  /// Returns the number of quads per side (= resolution enum value) for the given patch.
  WUInt32 GetHeightfieldCellsPerSide(WUInt32 uiPatchIndex) const;

  /// Returns a hash over all brushes whose footprint overlaps this patch's XY extent.
  /// Changes when the set of relevant brushes changes. Returns 0 for invalid indices.
  WUInt64 GetHeightfieldBrushOverlapHash(WUInt32 uiPatchIndex) const;

  /// Bakes the given heightfield patch and blocks while reading the result back to the CPU.
  ///
  /// out_heights receives the full stored height grid (CellsPerSide+9)² floats — 4 border rings on each
  /// side beyond the rendered vertices. out_dominantMat receives the dominant material index per stored cell,
  /// which is the material with the largest weight including the patch's implicit base material, or 0xFF for
  /// carved cells.
  /// Must be called from the main/render thread with GPU device access.
  WResult ReadbackHeightfieldData(WUInt32 uiPatchIndex, WDynamicArray<float>& out_heights, WDynamicArray<WUInt8>& out_dominantMat, WTime timeout = WTime::MakeFromSeconds(5.0));

private:
  /// Immediately destroys the GPU buffers for the given slot and marks it free. Not thread-safe; call via RemoveHeightfieldTerrain.
  void DestroyHeightfieldTerrain(WUInt32& uiIndex);

  /// Called each BeginRender. Computes brush overlap hashes, marks patches dirty when the brush set changes,
  /// then enqueues render graph passes for all dirty patches.
  void UpdateTerrain();

  /// Adds a 3-pass render graph sequence for one heightfield bake:
  /// Step1 bakes heights and the material mask, Step2 derives normals, Step3 resolves per-cell materials and vertex weights.
  void UpdateHeightfield(WUInt32 uiIndex, WRenderGraph& graph);

  /// (Re)creates the shared heightfield bake scratch when uiStoredSize exceeds the current capacity, else reuses it.
  /// Content is valid only within a single bake; one buffer set is shared across all heightfield patches.
  void EnsureSharedHeightfieldScratch(WUInt32 uiStoredSize);

  void DestroyHeightfields();

  /// Destroys all heightfield scratch buffers.
  void DestroySharedHeightfieldScratch();

  /// Populates brushes with all active brushes whose footprint overlaps the heightfield's XY extent,
  /// sorted in bake order: priority ascending, Carve last within the same priority.
  void FindHeightfieldOverlappingBrushes(const WTerrainData_Heightfield& heightfield, WDynamicArray<TerrainBrushData>& brushes) const;

  /// Returns a hash over the indices of all brushes whose footprint overlaps the heightfield's XY extent.
  /// Returns 0 when no brush overlaps. Used to detect changes in the set of relevant brushes per patch.
  WUInt64 ComputeHeightfieldBrushOverlapHash(const WTerrainData_Heightfield& heightfield) const;

  /// Shared heightfield bake scratch: intermediate material mask (uint2/vertex), written by Step1/2 and read
  /// by Step3 within one bake. Sized to the largest patch's stored grid; m_uiSharedMaskStoredSize tracks it.
  WGALBufferHandle m_hHeightfieldSharedMask;
  WUInt32 m_uiHeightfieldSharedMaskStoredSize = 0;

  WHybridArray<WUInt32, 16> m_QueuedHeightfieldsToDelete;
  WDynamicArray<WTerrainData_Heightfield> m_Heightfields;
  WConstantBufferStorageHandle m_hHeightfieldBakeConstants;
  WShaderResourceHandle m_hTerrainBakeStep1Shader;
  WShaderResourceHandle m_hTerrainBakeStep2Shader;
  WShaderResourceHandle m_hTerrainBakeStep3Shader;

  //////////////////////////////////////////////////////////////////////////
  // Voxel Volumes
  //////////////////////////////////////////////////////////////////////////

public:
  /// Allocates GPU buffers and readback helpers for a new voxel terrain piece. Returns a slot index.
  WUInt32 CreateVoxelTerrain(WUInt32 uiResolution, float fVoxelSize);
  /// Thread-safe: queues the volume for GPU buffer destruction at the start of the next frame.
  void RemoveVoxelTerrain(WUInt32 uiIndex);

  /// Returns a mutable reference and marks the volume dirty so it rebakes next frame.
  WTerrainData_Voxel& ModifyVoxelTerrain(WUInt32 uiIndex);

  /// Returns the GPU vertex buffer (StructuredBuffer SRV) for the given voxel volume.
  WGALBufferHandle GetVoxelVolumeGpuMeshVertexBuffer(WUInt32 uiIndex) const;

  /// Returns the GPU index buffer (StructuredBuffer SRV) for the given voxel volume.
  WGALBufferHandle GetVoxelVolumeGpuMeshIndexBuffer(WUInt32 uiIndex) const;

  /// Returns the indirect draw arguments buffer for the given voxel volume.
  WGALBufferHandle GetVoxelVolumeGpuMeshDrawArgsBuffer(WUInt32 uiIndex) const;

  /// Returns the brush overlap hash stored on the voxel volume (updated each bake).
  /// Returns 0 for invalid indices. Used by export modifiers to detect stale baked files.
  WUInt64 GetVoxelBrushOverlapHash(WUInt32 uiIndex) const;

  /// Bakes the given voxel terrain piece and blocks while reading the mesh back to the CPU.
  ///
  /// out_verts / out_indices receive the compacted surface-nets mesh; out_vertexCount and
  /// out_primitiveCount give the valid element counts (out_indices holds out_primitiveCount*3 indices).
  /// Must be called from the main/render thread with GPU device access.
  WResult ReadbackVoxelData(WUInt32 uiIndex, WTempArray<VoxelGpuVertex>& out_verts, WDynamicArray<WUInt32>& out_indices, WUInt32& out_uiVertexCount, WUInt32& out_uiPrimitiveCount, WTime timeout = WTime::MakeFromSeconds(5.0));

private:
  /// Immediately destroys the GPU buffers for the given slot and marks it free. Not thread-safe; call via RemoveVoxelTerrain.
  void DestroyVoxelTerrain(WUInt32& uiIndex);

  /// Adds the full voxel bake pass sequence to graph: solid/SDF bake → SDF blur → topology cleanup iterations
  /// → surface nets pass 1 and 2 → fill indirect dispatch args → GPU-driven compact copy to final render buffers.
  void UpdateVoxels(WUInt32 uiIndex, WRenderGraph& graph);

  /// (Re)creates the shared voxel bake scratch when the requested dimensions exceed the current capacity, else reuses it.
  /// Content is valid only within a single bake sequence; one buffer set is shared across all voxel volumes.
  void EnsureSharedVoxelScratch(WUInt32 uiPackPitch, WUInt32 uiBufYZ, WUInt32 uiResolution);

  void DestroyVoxelVolumes();
  void DestroySharedVoxelScratch();

  /// Populates brushes with all active brushes whose footprint overlaps the voxel volume's 3D extent,
  /// sorted in bake order: priority ascending, Carve last within the same priority.
  void FindVoxelOverlappingBrushes(const WTerrainData_Voxel& vol, WDynamicArray<TerrainBrushData>& brushes) const;
  WUInt64 ComputeVoxelBrushOverlapHash(const WTerrainData_Voxel& vol) const;

  WConstantBufferStorageHandle m_hVoxelBakeConstants;
  WDynamicArray<WTerrainData_Voxel> m_VoxelVolumes;
  WHybridArray<WUInt32, 8> m_QueuedVoxelVolumesToDelete;
  WShaderResourceHandle m_hVoxelBakeShader;
  WShaderResourceHandle m_hVoxelMeshClearShader;
  WShaderResourceHandle m_hVoxelSurfaceNetsPass1Shader;
  WShaderResourceHandle m_hVoxelSurfaceNetsPass2Shader;
  WShaderResourceHandle m_hVoxelBlurDistShader;
  WShaderResourceHandle m_hVoxelCleanupShader;
  WShaderResourceHandle m_hVoxelFillCompactCopyArgsShader;
  WShaderResourceHandle m_hVoxelCompactCopyShader;

  /// Shared voxel bake scratch — one set reused across all voxel volumes; content is valid only within a single bake sequence.
  /// Sized to the largest volume seen so far; m_uiSharedVoxel* track the current capacity dimensions.
  WGALBufferHandle m_hSharedVoxels;                  ///< Packed solidity bits (uint, 8 voxels per uint along X).
  WGALBufferHandle m_hSharedVoxelDist;               ///< Per-voxel SDF (float), unpacked.
  WGALBufferHandle m_hSharedVoxelDistScratch;        ///< Ping-pong partner of m_hSharedVoxelDist for blur/cleanup.
  WGALBufferHandle m_hSharedMeshRemap;               ///< linearCellIndex → compact vertex slot.
  WGALBufferHandle m_hSharedMeshCompactVertices;     ///< Densely packed surface-nets vertices.
  WGALBufferHandle m_hSharedMeshIndices;             ///< Surface-nets indices (worst-case cells*18).
  WGALBufferHandle m_hSharedMeshCounts;              ///< VoxelMeshCounts, 1 entry.
  WGALBufferHandle m_hSharedCompactCopyDispatchArgs; ///< DispatchIndirect args for VoxelCompactCopyCS.
  WUInt32 m_uiSharedVoxelPackPitch = 0;
  WUInt32 m_uiSharedVoxelBufYZ = 0;
  WUInt32 m_uiSharedVoxelResolution = 0;
};
