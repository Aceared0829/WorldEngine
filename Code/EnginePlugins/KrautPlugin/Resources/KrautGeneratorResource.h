#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>
#include <KrautPlugin/KrautDeclarations.h>

#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>

struct WKrautTreeResourceDescriptor;

namespace Kraut
{
  struct TreeStructure;
  struct TreeStructureDesc;
}; // namespace Kraut

using WKrautGeneratorResourceHandle = WTypedResourceHandle<class WKrautGeneratorResource>;
using WKrautTreeResourceHandle = WTypedResourceHandle<class WKrautTreeResource>;
using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;

/// Associates a material resource with the branch type and geometry type it is applied to.
struct WKrautMaterialDescriptor
{
  WKrautMaterialType m_MaterialType = WKrautMaterialType::None;
  WKrautBranchType m_BranchType = WKrautBranchType::None;
  WMaterialResourceHandle m_hMaterial;
};

/// Holds all authoring parameters for a Kraut tree asset.
///
/// This descriptor is loaded from disk and shared across all seed instances.
/// It drives both tree structure generation (via Kraut::TreeStructureDesc) and
/// runtime properties such as collision, wind, and LOD switching distances.
struct W_KRAUTPLUGIN_DLL WKrautGeneratorResourceDescriptor : public WRefCounted
{
  Kraut::TreeStructureDesc m_TreeStructureDesc;
  Kraut::LodDesc m_LodDesc[5];

  WHybridArray<WKrautMaterialDescriptor, 4> m_Materials;

  WString m_sSurfaceResource;
  float m_fStaticColliderRadius = 0.5f;
  float m_fUniformScaling = 1.0f;
  float m_fLodDistanceScale = 1.0f;
  float m_fTreeStiffness = 10.0f;
  float m_fMinAmbientOcclusion = 0.7f;

  /// Seed shown in the asset editor when no explicit seed is selected.
  WUInt16 m_uiDefaultDisplaySeed = 0;
  /// Curated list of seeds that produce good-looking tree variations.
  WHybridArray<WUInt16, 16> m_GoodRandomSeeds;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

/// Resource that holds the Kraut tree generator descriptor and manages per-seed WKrautTreeResource instances.
///
/// A single generator resource is shared by all WKrautTreeComponent instances that reference the same
/// tree asset. For each random seed in use, the generator creates one WKrautTreeResource and
/// asynchronously fills it with mesh data, LOD by LOD.
///
/// LOD index 0 is the full-detail preview LOD used in the asset editor and is never auto-selected
/// by distance at runtime. LOD indices 1..N are the runtime LODs selected by camera distance.
class W_KRAUTPLUGIN_DLL WKrautGeneratorResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WKrautGeneratorResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WKrautGeneratorResource);

public:
  WKrautGeneratorResource();

  const WSharedPtr<WKrautGeneratorResourceDescriptor>& GetDescriptor() const
  {
    return m_pGeneratorDesc;
  }

  /// Returns the number of runtime LODs (indices 1..N in the internal representation; excludes LOD0 full-detail).
  WUInt32 GetLodCount() const;

  /// Returns the max distance for a runtime LOD (0-based: 0 = first runtime LOD).
  float GetLodDistance(WUInt32 uiLodIndex) const;

  /// Returns (creating if needed) an empty tree resource skeleton for the given seed.
  /// Also queues async generation of the base data (tree structure + bounds).
  WKrautTreeResourceHandle GetOrCreateTreeResource(WUInt32 uiSeed);

  /// Requests that a specific LOD mesh be generated.
  ///
  /// uiLodIndex 0 = full-detail LOD, 1..N = runtime LODs.
  /// When bImmediate is true, generation runs synchronously on the calling thread,
  /// which will block until the mesh is ready. Returns true if the LOD is already Ready.
  bool RequestLodMesh(WKrautTreeResourceHandle hTree, WUInt32 uiSeed, WUInt32 uiLodIndex, bool bImmediate) const;

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  mutable WMutex m_DataMutex;
  WSharedPtr<WKrautGeneratorResourceDescriptor> m_pGeneratorDesc;

  /// Per-node wind data computed during extra-data initialization.
  struct BranchNodeExtraData
  {
    float m_fSegmentLength = 0.0f;
    float m_fDistanceAlongBranch = 0.0f;
    float m_fBendinessAlongBranch = 0.0f;
  };

  /// Per-branch wind data computed during extra-data initialization.
  struct BranchExtraData
  {
    WInt32 m_iParentBranch = -1;
    WUInt16 m_uiParentBranchNodeID = 0;
    WUInt8 m_uiBranchLevel = 0;
    WDynamicArray<BranchNodeExtraData> m_Nodes;
    float m_fDistanceToAnchor = 0;
    float m_fBendinessToAnchor = 0;
    WUInt32 m_uiRandomNumber = 0;
  };

  /// Extra wind and AO data computed alongside the Kraut tree structure for a single seed.
  struct TreeStructureExtraData
  {
    WDynamicArray<BranchExtraData> m_Branches;
  };

  void InitializeExtraData(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, WUInt32 uiRandomSeed) const;
  void ComputeDistancesAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void ComputeDistancesToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void ComputeBendinessAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, float fWoodBendiness, float fTwigBendiness) const;
  void ComputeBendinessToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void GenerateExtraData(TreeStructureExtraData& treeStructureExtraData, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, WUInt32 uiRandomSeed, float fWoodBendiness, float fTwigBendiness) const;

public:
  // Forward declarations for internal implementation types defined in KrautGeneratorResource.cpp.
  // Public so that helper free functions in the .cpp can reference the type names.
  struct WKrautSharedTreeData;
  class WKrautBaseDataTask;
  class WKrautLodGenerationTask;

private:
  /// Tracks the async generation state for a single seed value.
  ///
  /// Each seed that has been requested via GetOrCreateTreeResource() gets one SeedState entry.
  /// m_pSharedData is null until the base data task completes; LOD tasks must not start before that.
  struct SeedState
  {
    ~SeedState();                                     // defined in KrautGeneratorResource.cpp where task types are complete

    WKrautTreeResourceHandle m_hTree;
    WSharedPtr<WKrautSharedTreeData> m_pSharedData; ///< Null until the base data task completes.
    WSharedPtr<WKrautBaseDataTask> m_pBaseDataTask;
    WSharedPtr<WKrautLodGenerationTask> m_PendingLodTasks[6];
  };

  mutable WMutex m_GenerationMutex;
  mutable WHashTable<WUInt32, SeedState> m_SeedStates;

  void GenerateBaseDataImmediate(SeedState& state, WUInt32 uiSeed, const WSharedPtr<WKrautGeneratorResourceDescriptor>& desc) const;
  void GenerateSingleLodMeshImmediate(const WSharedPtr<WKrautSharedTreeData>& pSharedData, WUInt32 uiLodIndex, WKrautTreeResourceHandle hTree) const;
};
