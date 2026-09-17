#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Threading/Mutex.h>
#include <KrautPlugin/KrautDeclarations.h>

using WMeshResourceHandle = WTypedResourceHandle<class WMeshResource>;
using WKrautTreeResourceHandle = WTypedResourceHandle<class WKrautTreeResource>;
using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;
using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;

/// Tree-level properties that are seed-independent and become available once the base data task completes.
struct W_KRAUTPLUGIN_DLL WKrautTreeResourceDetails
{
  WBoundingBoxSphere m_Bounds;
  WVec3 m_vLeafCenter = WVec3::MakeZero();
  float m_fStaticColliderRadius = 0.0f;
  WString m_sSurfaceResource;
};

/// Full serializable descriptor for a single per-seed tree resource, including all LOD mesh data.
///
/// This descriptor is produced by the Kraut generator and can be saved to and loaded from a stream.
/// It is also the input to WKrautTreeResource::CreateResource().
struct W_KRAUTPLUGIN_DLL WKrautTreeResourceDescriptor
{
  void Save(WStreamWriter& inout_stream) const;
  WResult Load(WStreamReader& inout_stream);

  /// Per-vertex data for a LOD mesh.
  struct VertexData
  {
    W_DECLARE_POD_TYPE();

    WVec3 m_vPosition;
    WVec3 m_vTexCoord; ///< U, V texture coordinates and Q (a third coordinate used for branch-along-UV mapping).
    float m_fAmbientOcclusion = 1.0f;
    WVec3 m_vNormal;
    WVec3 m_vTangent;
    WUInt8 m_uiColorVariation = 0;

    // to compute wind
    WUInt8 m_uiBranchLevel = 0;  ///< 0 = trunk, 1 = main branches, 2 = twigs, etc. Used by the wind shader to scale bend strength per level.
    WUInt8 m_uiFlutterPhase = 0; ///< Phase shift for the per-leaf flutter animation, randomized per leaf.
    /// World-space position of the wind-simulation anchor point for this vertex.
    /// The wind shader divides by the length of (vertex - anchor), so this must never equal the vertex position (i.e. must not be zero after subtraction).
    WVec3 m_vBendAnchor;
    float m_fAnchorBendStrength = 0;     ///< Controls how strongly the global wind force bends the branch toward its anchor.
    float m_fBendAndFlutterStrength = 0; ///< Controls the local bend and flutter magnitude for this vertex.
  };

  struct TriangleData
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_uiVertexIndex[3];
  };

  /// Defines a draw call within a LOD: a contiguous range of triangles sharing a single material.
  struct SubMeshData
  {
    WUInt16 m_uiFirstTriangle = 0;
    WUInt16 m_uiNumTriangles = 0;
    WUInt8 m_uiMaterialIndex = 0xFF; ///< Index into the LOD's material array; 0xFF means no material assigned.
  };

  /// All mesh data for one LOD level.
  struct LodData
  {
    float m_fMinLodDistance = 0;
    float m_fMaxLodDistance = 0;
    WKrautLodType m_LodType = WKrautLodType::None;

    WUInt32 m_uiNumBones = 0;

    WDynamicArray<VertexData> m_Vertices;
    WDynamicArray<TriangleData> m_Triangles;
    WDynamicArray<SubMeshData> m_SubMeshes;
  };

  struct MaterialData
  {
    WKrautMaterialType m_MaterialType;
    WKrautBranchType m_BranchType = WKrautBranchType::None;
    WString m_sMaterial;
    WColorGammaUB m_VariationColor = WColor::White; // currently done through the material
  };

  WKrautTreeResourceDetails m_Details;
  WStaticArray<LodData, 6> m_Lods;
  WHybridArray<MaterialData, 8> m_Materials;
};

/// Holds the runtime mesh data for one Kraut tree instance (a specific random seed).
///
/// Instances are created and owned by WKrautGeneratorResource, one per seed value that is
/// in active use. Mesh data is populated asynchronously: first the bounds and materials are
/// set via SetDetails(), then each LOD slot is filled individually via SetLodMesh() as the
/// corresponding generation task finishes.
///
/// The LOD state machine per slot goes: NotGenerated -> Generating -> Ready.
/// All state-mutating methods are thread-safe.
class W_KRAUTPLUGIN_DLL WKrautTreeResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WKrautTreeResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WKrautTreeResource);
  W_RESOURCE_DECLARE_CREATEABLE(WKrautTreeResource, WKrautTreeResourceDescriptor);

public:
  WKrautTreeResource();

  const WKrautTreeResourceDetails& GetDetails() const { return m_Details; }
  WArrayPtr<const WKrautTreeResourceDescriptor::MaterialData> GetMaterials() const { return m_Materials; }

  /// Runtime representation of a single LOD level, including its mesh handle and generation state.
  struct TreeLod
  {
    WMeshResourceHandle m_hMesh;
    float m_fMinLodDistance = 0;
    float m_fMaxLodDistance = 0;
    WKrautLodType m_LodType = WKrautLodType::None;
    WKrautLodState m_State = WKrautLodState::NotGenerated;

    WUInt32 m_uiNumBones = 0;
    WUInt32 m_uiNumTrianglesBranch = 0;
    WUInt32 m_uiNumTrianglesFrond = 0;
    WUInt32 m_uiNumTrianglesLeaf = 0;

    /// The full material list for this LOD, including branch-type/geometry-type metadata.
    /// Sub-mesh material indices reference this array.
    WHybridArray<WKrautTreeResourceDescriptor::MaterialData, 8> m_Materials;
  };

  WArrayPtr<const TreeLod> GetTreeLODs() const { return m_TreeLODs.GetArrayPtr(); }

  /// Sets bounds and materials. Called by the generator once tree structure is known, before any mesh is generated.
  void SetDetails(const WKrautTreeResourceDetails& details, WArrayPtr<const WKrautTreeResourceDescriptor::MaterialData> materials);

  /// Creates the mesh resource for a LOD slot and sets its state to Ready. Thread-safe.
  void SetLodMesh(WUInt32 uiLodIndex, const WKrautTreeResourceDescriptor::LodData& lodData, WArrayPtr<const WKrautTreeResourceDescriptor::MaterialData> materials);

  /// Sets a LOD slot's state (e.g. NotGenerated -> Generating). Thread-safe.
  void SetLodState(WUInt32 uiLodIndex, WKrautLodState state);

  /// Returns the current state of a LOD slot. Thread-safe.
  WKrautLodState GetLodState(WUInt32 uiLodIndex) const;

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  mutable WMutex m_LodMutex;
  WKrautTreeResourceDetails m_Details;
  WStaticArray<TreeLod, 6> m_TreeLODs;
  WHybridArray<WKrautTreeResourceDescriptor::MaterialData, 8> m_Materials;
};
