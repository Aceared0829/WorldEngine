#pragma once

#include <Core/World/SpatialSystem.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Types/UniquePtr.h>

namespace WInternal
{
  struct QueryHelper;
}

/// Spatial system implementation using regular grids for organizing objects.
///
/// Divides space into uniform grid cells to enable efficient spatial queries. Supports
/// multiple grids for different spatial data categories and implements a caching system
/// to optimize frequently used tag-based queries by creating specialized grid views.
class W_CORE_DLL WSpatialSystem_RegularGrid : public WSpatialSystem
{
  W_ADD_DYNAMIC_REFLECTION(WSpatialSystem_RegularGrid, WSpatialSystem);

public:
  /// Creates a regular grid spatial system with the given cell size.
  WSpatialSystem_RegularGrid(WUInt32 uiCellSize = 128);
  ~WSpatialSystem_RegularGrid();

  /// Returns the bounding box of the cell associated with the given spatial data. Useful for debug visualizations.
  WResult GetCellBoxForSpatialData(const WSpatialDataHandle& hData, WBoundingBox& out_boundingBox) const;

  /// Returns bounding boxes of all existing cells.
  void GetAllCellBoxes(WDynamicArray<WBoundingBox>& out_boundingBoxes, WSpatialData::Category filterCategory = WInvalidSpatialDataCategory) const;

private:
  friend WInternal::QueryHelper;

  // WSpatialSystem implementation
  virtual void StartNewFrame() override;

  WSpatialDataHandle CreateSpatialData(const WSimdBBoxSphere& bounds, WGameObject* pObject, WUInt32 uiCategoryBitmask, const WTagSet& tags) override;
  WSpatialDataHandle CreateSpatialDataAlwaysVisible(WGameObject* pObject, WUInt32 uiCategoryBitmask, const WTagSet& tags) override;

  void DeleteSpatialData(const WSpatialDataHandle& hData) override;

  void UpdateSpatialDataBounds(const WSpatialDataHandle& hData, const WSimdBBoxSphere& bounds) override;
  void UpdateSpatialDataObject(const WSpatialDataHandle& hData, WGameObject* pObject) override;

  void FindObjectsInSphere(const WBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const override;
  void FindObjectsInBox(const WBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const override;

  void FindVisibleObjects(const WFrustum& frustum, const QueryParams& queryParams, WDynamicArray<const WGameObject*>& out_Objects, WSpatialSystem::IsOccludedFunc IsOccluded, WVisibilityState::Enum visType) const override;

  WVisibilityState::Enum GetVisibilityState(const WSpatialDataHandle& hData, WUInt32 uiNumFramesBeforeInvisible) const override;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(WStringBuilder& sb) const override;
#endif

  WProxyAllocator m_AlignedAllocator;

  WSimdVec4i m_vCellSize;
  WSimdVec4f m_vOverlapSize;
  WSimdFloat m_fInvCellSize;

  enum
  {
    MAX_NUM_GRIDS = 63,
    MAX_NUM_REGULAR_GRIDS = (sizeof(WSpatialData::Category::m_uiValue) * 8),
    MAX_NUM_CACHED_GRIDS = MAX_NUM_GRIDS - MAX_NUM_REGULAR_GRIDS
  };

  struct Cell;
  struct Grid;
  WDynamicArray<WUniquePtr<Grid>> m_Grids;
  WUInt32 m_uiFirstCachedGridIndex = MAX_NUM_GRIDS;

  /// Internal data structure tracking which grids contain a spatial data object.
  struct Data
  {
    W_DECLARE_POD_TYPE();

    WUInt64 m_uiGridBitmask : MAX_NUM_GRIDS; ///< Bitmask indicating which grids contain this object
    WUInt64 m_uiAlwaysVisible : 1;           ///< Whether this object is always visible (bypasses spatial queries)
  };

  WIdTable<WSpatialDataId, Data, WLocalAllocatorWrapper> m_DataTable;

  bool IsAlwaysVisibleData(const Data& data) const;

  WSpatialDataHandle AddSpatialDataToGrids(const WSimdBBoxSphere& bounds, WGameObject* pObject, WUInt32 uiCategoryBitmask, const WTagSet& tags, bool bAlwaysVisible);

  template <typename Functor>
  void ForEachGrid(const Data& data, const WSpatialDataHandle& hData, Functor func) const;

  struct Stats;
  using CellCallback = WDelegate<WVisitorExecution::Enum(const Cell&, const QueryParams&, Stats&, void*, WVisibilityState::Enum)>;
  void ForEachCellInBoxInMatchingGrids(const WSimdBBox& box, const QueryParams& queryParams, CellCallback noFilterCallback, CellCallback filterByTagsCallback, void* pUserData, WVisibilityState::Enum visType) const;

  /// Candidate for grid caching based on query patterns and filtering efficiency.
  struct CacheCandidate
  {
    WTagSet m_IncludeTags;                  ///< Tags that must be included for this cached grid
    WTagSet m_ExcludeTags;                  ///< Tags that must be excluded for this cached grid
    WSpatialData::Category m_Category;      ///< Spatial data category for this cached grid
    float m_fQueryCount = 0.0f;              ///< How frequently this query pattern is used
    float m_fFilteredRatio = 0.0f;           ///< Ratio of objects that pass the tag filter
    WUInt32 m_uiGridIndex = WInvalidIndex; ///< Index of the associated grid if already cached
  };

  mutable WDynamicArray<CacheCandidate> m_CacheCandidates;
  mutable WMutex m_CacheCandidatesMutex;

  struct SortedCacheCandidate
  {
    WUInt32 m_uiIndex = 0;
    float m_fScore = 0;

    bool operator<(const SortedCacheCandidate& other) const
    {
      if (m_fScore != other.m_fScore)
        return m_fScore > other.m_fScore; // higher score comes first

      return m_uiIndex < other.m_uiIndex;
    }
  };

  WDynamicArray<SortedCacheCandidate> m_SortedCacheCandidates;

  void MigrateCachedGrid(WUInt32 uiCandidateIndex);
  void MigrateSpatialData(WUInt32 uiTargetGridIndex, WUInt32 uiSourceGridIndex);

  void RemoveCachedGrid(WUInt32 uiCandidateIndex);
  void RemoveAllCachedGrids();

  void UpdateCacheCandidate(const WTagSet* pIncludeTags, const WTagSet* pExcludeTags, WSpatialData::Category category, float filteredRatio) const;
};
