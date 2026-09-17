#include <Core/CorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Time/Stopwatch.h>

WCVarInt cvar_SpatialQueriesCachingThreshold("Spatial.Queries.CachingThreshold", 100, WCVarFlags::Default, "Number of objects that are tested for a query before it is considered for caching");

struct PlaneData
{
  WSimdVec4f m_x0x1x2x3;
  WSimdVec4f m_y0y1y2y3;
  WSimdVec4f m_z0z1z2z3;
  WSimdVec4f m_w0w1w2w3;

  WSimdVec4f m_x4x5x4x5;
  WSimdVec4f m_y4y5y4y5;
  WSimdVec4f m_z4z5z4z5;
  WSimdVec4f m_w4w5w4w5;
};

namespace
{
  enum
  {
    MAX_CELL_INDEX = (1 << 20) - 1,
    CELL_INDEX_MASK = (1 << 21) - 1
  };

  W_ALWAYS_INLINE WSimdVec4f ToVec3(const WSimdVec4i& v)
  {
    return v.ToFloat();
  }

  W_ALWAYS_INLINE WSimdVec4i ToVec3I32(const WSimdVec4f& v)
  {
    WSimdVec4f vf = v.Floor();
    return WSimdVec4i::Truncate(vf);
  }

  W_ALWAYS_INLINE WUInt64 GetCellKey(WInt32 x, WInt32 y, WInt32 z)
  {
    WUInt64 sx = (x + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    WUInt64 sy = (y + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    WUInt64 sz = (z + MAX_CELL_INDEX) & CELL_INDEX_MASK;

    return (sx << 42) | (sy << 21) | sz;
  }

  W_ALWAYS_INLINE WSimdBBox ComputeCellBoundingBox(const WSimdVec4i& vCellIndex, const WSimdVec4i& vCellSize)
  {
    WSimdVec4i overlapSize = vCellSize >> 2;
    WSimdVec4i minPos = vCellIndex.CompMul(vCellSize);

    WSimdVec4f bmin = ToVec3(minPos - overlapSize);
    WSimdVec4f bmax = ToVec3(minPos + overlapSize + vCellSize);

    return WSimdBBox(bmin, bmax);
  }

  W_ALWAYS_INLINE bool AreTagSetsEqual(const WTagSet& a, const WTagSet* pB)
  {
    if (pB != nullptr)
    {
      return a == *pB;
    }

    return a.IsEmpty();
  }

  W_ALWAYS_INLINE bool FilterByTags(const WTagSet& tags, const WTagSet* pIncludeTags, const WTagSet* pExcludeTags)
  {
    if (pExcludeTags != nullptr && !pExcludeTags->IsEmpty() && pExcludeTags->IsAnySet(tags))
      return true;

    if (pIncludeTags != nullptr && !pIncludeTags->IsEmpty() && !pIncludeTags->IsAnySet(tags))
      return true;

    return false;
  }

  W_ALWAYS_INLINE bool CanBeCached(WSpatialData::Category category)
  {
    return WSpatialData::GetCategoryFlags(category).IsSet(WSpatialData::Flags::FrequentChanges) == false;
  }


#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  void TagsToString(const WTagSet& tags, WStringBuilder& out_sSb)
  {
    out_sSb.Append("{ ");

    bool first = true;
    for (auto it = tags.GetIterator(); it.IsValid(); ++it)
    {
      if (!first)
      {
        out_sSb.Append(", ");
        first = false;
      }
      out_sSb.Append(it->GetTagString().GetView());
    }

    out_sSb.Append(" }");
  }
#endif

  W_FORCE_INLINE bool SphereFrustumIntersect(const WSimdBSphere& sphere, const PlaneData& planeData)
  {
    WSimdVec4f pos_xxxx(sphere.m_CenterAndRadius.x());
    WSimdVec4f pos_yyyy(sphere.m_CenterAndRadius.y());
    WSimdVec4f pos_zzzz(sphere.m_CenterAndRadius.z());
    WSimdVec4f pos_rrrr(sphere.m_CenterAndRadius.w());

    WSimdVec4f dot_0123;
    dot_0123 = WSimdVec4f::MulAdd(pos_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dot_0123 = WSimdVec4f::MulAdd(pos_yyyy, planeData.m_y0y1y2y3, dot_0123);
    dot_0123 = WSimdVec4f::MulAdd(pos_zzzz, planeData.m_z0z1z2z3, dot_0123);

    WSimdVec4f dot_4545;
    dot_4545 = WSimdVec4f::MulAdd(pos_xxxx, planeData.m_x4x5x4x5, planeData.m_w4w5w4w5);
    dot_4545 = WSimdVec4f::MulAdd(pos_yyyy, planeData.m_y4y5y4y5, dot_4545);
    dot_4545 = WSimdVec4f::MulAdd(pos_zzzz, planeData.m_z4z5z4z5, dot_4545);

    WSimdVec4b cmp_0123 = dot_0123 > pos_rrrr;
    WSimdVec4b cmp_4545 = dot_4545 > pos_rrrr;
    return (cmp_0123 || cmp_4545).NoneSet<4>();
  }

  W_FORCE_INLINE WUInt32 SphereFrustumIntersect(const WSimdBSphere& sphereA, const WSimdBSphere& sphereB, const PlaneData& planeData)
  {
    WSimdVec4f posA_xxxx(sphereA.m_CenterAndRadius.x());
    WSimdVec4f posA_yyyy(sphereA.m_CenterAndRadius.y());
    WSimdVec4f posA_zzzz(sphereA.m_CenterAndRadius.z());
    WSimdVec4f posA_rrrr(sphereA.m_CenterAndRadius.w());

    WSimdVec4f dotA_0123;
    dotA_0123 = WSimdVec4f::MulAdd(posA_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dotA_0123 = WSimdVec4f::MulAdd(posA_yyyy, planeData.m_y0y1y2y3, dotA_0123);
    dotA_0123 = WSimdVec4f::MulAdd(posA_zzzz, planeData.m_z0z1z2z3, dotA_0123);

    WSimdVec4f posB_xxxx(sphereB.m_CenterAndRadius.x());
    WSimdVec4f posB_yyyy(sphereB.m_CenterAndRadius.y());
    WSimdVec4f posB_zzzz(sphereB.m_CenterAndRadius.z());
    WSimdVec4f posB_rrrr(sphereB.m_CenterAndRadius.w());

    WSimdVec4f dotB_0123;
    dotB_0123 = WSimdVec4f::MulAdd(posB_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dotB_0123 = WSimdVec4f::MulAdd(posB_yyyy, planeData.m_y0y1y2y3, dotB_0123);
    dotB_0123 = WSimdVec4f::MulAdd(posB_zzzz, planeData.m_z0z1z2z3, dotB_0123);

    WSimdVec4f posAB_xxxx = posA_xxxx.GetCombined<WSwizzle::XXXX>(posB_xxxx);
    WSimdVec4f posAB_yyyy = posA_yyyy.GetCombined<WSwizzle::XXXX>(posB_yyyy);
    WSimdVec4f posAB_zzzz = posA_zzzz.GetCombined<WSwizzle::XXXX>(posB_zzzz);
    WSimdVec4f posAB_rrrr = posA_rrrr.GetCombined<WSwizzle::XXXX>(posB_rrrr);

    WSimdVec4f dot_A45B45;
    dot_A45B45 = WSimdVec4f::MulAdd(posAB_xxxx, planeData.m_x4x5x4x5, planeData.m_w4w5w4w5);
    dot_A45B45 = WSimdVec4f::MulAdd(posAB_yyyy, planeData.m_y4y5y4y5, dot_A45B45);
    dot_A45B45 = WSimdVec4f::MulAdd(posAB_zzzz, planeData.m_z4z5z4z5, dot_A45B45);

    WSimdVec4b cmp_A0123 = dotA_0123 > posA_rrrr;
    WSimdVec4b cmp_B0123 = dotB_0123 > posB_rrrr;
    WSimdVec4b cmp_A45B45 = dot_A45B45 > posAB_rrrr;

    WSimdVec4b cmp_A45 = cmp_A45B45.Get<WSwizzle::XYXY>();
    WSimdVec4b cmp_B45 = cmp_A45B45.Get<WSwizzle::ZWZW>();

    WUInt32 result = (cmp_A0123 || cmp_A45).NoneSet<4>() ? 1 : 0;
    result |= (cmp_B0123 || cmp_B45).NoneSet<4>() ? 2 : 0;

    return result;
  }

  W_ALWAYS_INLINE WUInt64 EncodeLastVisibleFrameIndexAndVisType(WUInt64 uiFrameCounter, WVisibilityState::Enum visType)
  {
    return (uiFrameCounter << 4) | static_cast<WUInt64>(visType);
  }

  W_ALWAYS_INLINE WUInt64 ExtractLastVisibleFrameIndex(WUInt64 uiLastVisibleFrameIdxAndVisType)
  {
    return (uiLastVisibleFrameIdxAndVisType >> 4);
  }

  W_ALWAYS_INLINE WVisibilityState::Enum ExtractVisType(WUInt64 uiLastVisibleFrameIdxAndVisType)
  {
    return static_cast<WVisibilityState::Enum>(uiLastVisibleFrameIdxAndVisType & static_cast<WUInt64>(15));
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

struct CellDataMapping
{
  W_DECLARE_POD_TYPE();

  WUInt32 m_uiCellIndex = WInvalidIndex;
  WUInt32 m_uiCellDataIndex = WInvalidIndex;
};

struct WSpatialSystem_RegularGrid::Cell
{
  Cell(WAllocator* pAlignedAlloctor, WAllocator* pAllocator)
    : m_BoundingSpheres(pAlignedAlloctor)
    , m_BoundingBoxHalfExtents(pAlignedAlloctor)
    , m_TagSets(pAllocator)
    , m_ObjectPointers(pAllocator)
    , m_DataIndices(pAllocator)
  {
  }

  W_FORCE_INLINE WUInt32 AddData(const WSimdBBoxSphere& bounds, const WTagSet& tags, WGameObject* pObject, WUInt64 uiLastVisibleFrameIdxAndVisType, WUInt32 uiDataIndex)
  {
    m_BoundingSpheres.PushBack(bounds.GetSphere());
    m_BoundingBoxHalfExtents.PushBack(bounds.m_BoxHalfExtents);
    m_TagSets.PushBack(tags);
    m_ObjectPointers.PushBack(pObject);
    m_DataIndices.PushBack(uiDataIndex);
    m_LastVisibleFrameIdxAndVisType.PushBack(uiLastVisibleFrameIdxAndVisType);

    return m_BoundingSpheres.GetCount() - 1;
  }

  // Returns the data index of the moved data
  W_FORCE_INLINE WUInt32 RemoveData(WUInt32 uiCellDataIndex)
  {
    WUInt32 uiMovedDataIndex = m_DataIndices.PeekBack();

    m_BoundingSpheres.RemoveAtAndSwap(uiCellDataIndex);
    m_BoundingBoxHalfExtents.RemoveAtAndSwap(uiCellDataIndex);
    m_TagSets.RemoveAtAndSwap(uiCellDataIndex);
    m_ObjectPointers.RemoveAtAndSwap(uiCellDataIndex);
    m_DataIndices.RemoveAtAndSwap(uiCellDataIndex);
    m_LastVisibleFrameIdxAndVisType.RemoveAtAndSwap(uiCellDataIndex);

    W_ASSERT_DEBUG(m_DataIndices.GetCount() == uiCellDataIndex || m_DataIndices[uiCellDataIndex] == uiMovedDataIndex, "Implementation error");

    return uiMovedDataIndex;
  }

  W_ALWAYS_INLINE WBoundingBox GetBoundingBox() const { return WSimdConversion::ToBBoxSphere(m_Bounds).GetBox(); }

  WSimdBBoxSphere m_Bounds;

  WDynamicArray<WSimdBSphere> m_BoundingSpheres;
  WDynamicArray<WSimdVec4f> m_BoundingBoxHalfExtents;
  WDynamicArray<WTagSet> m_TagSets;
  WDynamicArray<WGameObject*> m_ObjectPointers;
  mutable WDynamicArray<WAtomicInteger64> m_LastVisibleFrameIdxAndVisType;
  WDynamicArray<WUInt32> m_DataIndices;
};

//////////////////////////////////////////////////////////////////////////

struct CellKeyHashHelper
{
  W_ALWAYS_INLINE static WUInt32 Hash(WUInt64 value)
  {
    // manually unrolled MurmurHash32
    const WUInt32 m = WInternal::MURMUR_M;
    const WUInt32 r = WInternal::MURMUR_R;

    WUInt32 h = 8;
    {
      WUInt32 k = WUInt32(value);

      k *= m;
      k ^= k >> r;
      k *= m;

      h *= m;
      h ^= k;
    }

    {
      WUInt32 k = WUInt32(value >> 32);

      k *= m;
      k ^= k >> r;
      k *= m;

      h *= m;
      h ^= k;
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  }

  W_ALWAYS_INLINE static bool Equal(WUInt64 a, WUInt64 b) { return a == b; }
};

//////////////////////////////////////////////////////////////////////////

struct WSpatialSystem_RegularGrid::Grid
{
  Grid(WSpatialSystem_RegularGrid& ref_system, WSpatialData::Category category)
    : m_System(ref_system)
    , m_Cells(&ref_system.m_Allocator)
    , m_CellKeyToCellIndex(&ref_system.m_Allocator)
    , m_Category(category)
    , m_bCanBeCached(CanBeCached(category))
  {
    const WSimdBBox overflowBox = WSimdBBox::MakeFromCenterAndHalfExtents(WSimdVec4f::MakeZero(), WSimdVec4f((float)(ref_system.m_vCellSize.x() * MAX_CELL_INDEX)));

    auto pOverflowCell = W_NEW(&m_System.m_AlignedAllocator, Cell, &m_System.m_AlignedAllocator, &m_System.m_Allocator);
    pOverflowCell->m_Bounds = overflowBox;

    m_Cells.PushBack(pOverflowCell);
  }

  WUInt32 GetOrCreateCell(const WSimdBBoxSphere& bounds)
  {
    WSimdVec4i cellIndex = ToVec3I32(bounds.m_CenterAndRadius * m_System.m_fInvCellSize);
    WSimdBBox cellBox = ComputeCellBoundingBox(cellIndex, m_System.m_vCellSize);

    if (cellBox.Contains(bounds.GetBox()))
    {
      WUInt64 cellKey = GetCellKey(cellIndex.x(), cellIndex.y(), cellIndex.z());

      WUInt32 uiCellIndex = 0;
      if (m_CellKeyToCellIndex.TryGetValue(cellKey, uiCellIndex))
      {
        return uiCellIndex;
      }

      uiCellIndex = m_Cells.GetCount();
      m_CellKeyToCellIndex.Insert(cellKey, uiCellIndex);

      auto pNewCell = W_NEW(&m_System.m_AlignedAllocator, Cell, &m_System.m_AlignedAllocator, &m_System.m_Allocator);
      pNewCell->m_Bounds = cellBox;

      m_Cells.PushBack(pNewCell);

      return uiCellIndex;
    }
    else
    {
      return m_uiOverflowCellIndex;
    }
  }

  void AddSpatialData(const WSimdBBoxSphere& bounds, const WTagSet& tags, WGameObject* pObject, WUInt64 uiLastVisibleFrameIdxAndVisType, const WSpatialDataHandle& hData)
  {
    WUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

    WUInt32 uiCellIndex = GetOrCreateCell(bounds);
    WUInt32 uiCellDataIndex = m_Cells[uiCellIndex]->AddData(bounds, tags, pObject, uiLastVisibleFrameIdxAndVisType, uiDataIndex);

    m_CellDataMappings.EnsureCount(uiDataIndex + 1);
    W_ASSERT_DEBUG(m_CellDataMappings[uiDataIndex].m_uiCellIndex == WInvalidIndex, "data has already been added to a cell");
    m_CellDataMappings[uiDataIndex] = {uiCellIndex, uiCellDataIndex};
  }

  void RemoveSpatialData(const WSpatialDataHandle& hData)
  {
    WUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

    auto& mapping = m_CellDataMappings[uiDataIndex];
    WUInt32 uiMovedDataIndex = m_Cells[mapping.m_uiCellIndex]->RemoveData(mapping.m_uiCellDataIndex);
    if (uiMovedDataIndex != uiDataIndex)
    {
      m_CellDataMappings[uiMovedDataIndex].m_uiCellDataIndex = mapping.m_uiCellDataIndex;
    }

    mapping = {};
  }

  bool MigrateSpatialDataFromOtherGrid(WUInt32 uiDataIndex, const Grid& other)
  {
    // Data has already been added
    if (uiDataIndex < m_CellDataMappings.GetCount() && m_CellDataMappings[uiDataIndex].m_uiCellIndex != WInvalidIndex)
      return false;

    auto& mapping = other.m_CellDataMappings[uiDataIndex];
    if (mapping.m_uiCellIndex == WInvalidIndex)
      return false;

    auto& pOtherCell = other.m_Cells[mapping.m_uiCellIndex];

    const WTagSet& tags = pOtherCell->m_TagSets[mapping.m_uiCellDataIndex];
    if (FilterByTags(tags, &m_IncludeTags, &m_ExcludeTags))
      return false;

    WSimdBBoxSphere bounds;
    bounds.m_CenterAndRadius = pOtherCell->m_BoundingSpheres[mapping.m_uiCellDataIndex].m_CenterAndRadius;
    bounds.m_BoxHalfExtents = pOtherCell->m_BoundingBoxHalfExtents[mapping.m_uiCellDataIndex];
    WGameObject* objectPointer = pOtherCell->m_ObjectPointers[mapping.m_uiCellDataIndex];
    const WUInt64 uiLastVisibleFrameIdxAndVisType = pOtherCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex];

    W_ASSERT_DEBUG(pOtherCell->m_DataIndices[mapping.m_uiCellDataIndex] == uiDataIndex, "Implementation error");
    WSpatialDataHandle hData = WSpatialDataHandle(WSpatialDataId(uiDataIndex, 1));

    AddSpatialData(bounds, tags, objectPointer, uiLastVisibleFrameIdxAndVisType, hData);
    return true;
  }

  W_ALWAYS_INLINE bool CachingCompleted() const { return m_uiLastMigrationIndex == WInvalidIndex; }

  template <typename Functor>
  W_FORCE_INLINE void ForEachCellInBox(const WSimdBBox& box, Functor func) const
  {
    WSimdVec4i minIndex = ToVec3I32((box.m_Min - m_System.m_vOverlapSize) * m_System.m_fInvCellSize);
    WSimdVec4i maxIndex = ToVec3I32((box.m_Max + m_System.m_vOverlapSize) * m_System.m_fInvCellSize);

    W_ASSERT_DEBUG((minIndex.Abs() < WSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");
    W_ASSERT_DEBUG((maxIndex.Abs() < WSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");

    const WInt32 iMinX = minIndex.x();
    const WInt32 iMinY = minIndex.y();
    const WInt32 iMinZ = minIndex.z();

    const WSimdVec4i diff = maxIndex - minIndex + WSimdVec4i(1);
    const WInt32 iDiffX = diff.x();
    const WInt32 iDiffY = diff.y();
    const WInt32 iDiffZ = diff.z();
    const WInt32 iNumIterations = iDiffX * iDiffY * iDiffZ;

    // The hash grid approach below is about 10 times slower than simply iterating over all cells
    // and doing an AABB overlap test
    const WUInt64 uiHashGridCost = WUInt64(iNumIterations) * 10;
    if (uiHashGridCost > m_Cells.GetCount())
    {
      for (auto& pCell : m_Cells)
      {
        if (box.Overlaps(pCell->m_Bounds.GetBox()) == false)
          continue;

        if (func(*pCell) == WVisitorExecution::Stop)
          return;
      }
    }
    else
    {
      for (WInt32 i = 0; i < iNumIterations; ++i)
      {
        WInt32 index = i;
        WInt32 z = i / (iDiffX * iDiffY);
        index -= z * iDiffX * iDiffY;
        WInt32 y = index / iDiffX;
        WInt32 x = index - (y * iDiffX);

        x += iMinX;
        y += iMinY;
        z += iMinZ;

        WUInt64 cellKey = GetCellKey(x, y, z);
        WUInt32 cellIndex = 0;
        if (m_CellKeyToCellIndex.TryGetValue(cellKey, cellIndex))
        {
          const Cell& constCell = *m_Cells[cellIndex];
          if (func(constCell) == WVisitorExecution::Stop)
            return;
        }
      }

      const Cell& overflowCell = *m_Cells[m_uiOverflowCellIndex];
      func(overflowCell);
    }
  }

  WSpatialSystem_RegularGrid& m_System;
  WDynamicArray<WUniquePtr<Cell>> m_Cells;

  WHashTable<WUInt64, WUInt32, CellKeyHashHelper> m_CellKeyToCellIndex;
  static constexpr WUInt32 m_uiOverflowCellIndex = 0;

  WDynamicArray<CellDataMapping> m_CellDataMappings;

  const WSpatialData::Category m_Category;
  const bool m_bCanBeCached;

  WTagSet m_IncludeTags;
  WTagSet m_ExcludeTags;

  WUInt32 m_uiLastMigrationIndex = 0;
};

//////////////////////////////////////////////////////////////////////////

struct WSpatialSystem_RegularGrid::Stats
{
  WUInt32 m_uiNumObjectsTested = 0;
  WUInt32 m_uiNumObjectsPassed = 0;
  WUInt32 m_uiNumObjectsFiltered = 0;
};

//////////////////////////////////////////////////////////////////////////

namespace WInternal
{
  struct QueryHelper
  {
    template <typename T>
    struct ShapeQueryData
    {
      T m_Shape;
      WSpatialSystem::QueryCallback m_Callback;
    };

    template <typename T, bool UseTagsFilter>
    static WVisitorExecution::Enum ShapeQueryCallback(const WSpatialSystem_RegularGrid::Cell& cell, const WSpatialSystem::QueryParams& queryParams, WSpatialSystem_RegularGrid::Stats& ref_stats, void* pUserData, WVisibilityState::Enum visType)
    {
      W_IGNORE_UNUSED(visType);

      auto pQueryData = static_cast<const ShapeQueryData<T>*>(pUserData);
      T shape = pQueryData->m_Shape;

      WSimdBBox cellBox = cell.m_Bounds.GetBox();
      if (!cellBox.Overlaps(shape))
        return WVisitorExecution::Continue;

      auto boundingSpheres = cell.m_BoundingSpheres.GetData();
      auto tagSets = cell.m_TagSets.GetData();
      auto objectPointers = cell.m_ObjectPointers.GetData();

      const WUInt32 numSpheres = cell.m_BoundingSpheres.GetCount();
      ref_stats.m_uiNumObjectsTested += numSpheres;

      for (WUInt32 i = 0; i < numSpheres; ++i)
      {
        if (!shape.Overlaps(boundingSpheres[i]))
          continue;

        if constexpr (UseTagsFilter)
        {
          if (FilterByTags(tagSets[i], queryParams.m_pIncludeTags, queryParams.m_pExcludeTags))
          {
            ref_stats.m_uiNumObjectsFiltered++;
            continue;
          }
        }

        ref_stats.m_uiNumObjectsPassed++;

        if (pQueryData->m_Callback(objectPointers[i]) == WVisitorExecution::Stop)
          return WVisitorExecution::Stop;
      }

      return WVisitorExecution::Continue;
    }

    struct FrustumQueryData
    {
      PlaneData m_PlaneData;
      WDynamicArray<const WGameObject*>* m_pOutObjects;
      WUInt64 m_uiFrameCounter;
      WSpatialSystem::IsOccludedFunc m_IsOccludedCB;
    };

    template <bool UseTagsFilter, bool UseOcclusionCallback>
    static WVisitorExecution::Enum FrustumQueryCallback(const WSpatialSystem_RegularGrid::Cell& cell, const WSpatialSystem::QueryParams& queryParams, WSpatialSystem_RegularGrid::Stats& ref_stats, void* pUserData, WVisibilityState::Enum visType)
    {
      auto pQueryData = static_cast<FrustumQueryData*>(pUserData);
      PlaneData planeData = pQueryData->m_PlaneData;

      WSimdBSphere cellSphere = cell.m_Bounds.GetSphere();
      if (!SphereFrustumIntersect(cellSphere, planeData))
        return WVisitorExecution::Continue;

      if constexpr (UseOcclusionCallback)
      {
        if (pQueryData->m_IsOccludedCB(cell.m_Bounds.GetBox()))
        {
          return WVisitorExecution::Continue;
        }
      }

      auto boundingSpheres = cell.m_BoundingSpheres.GetData();
      auto boundingBoxHalfExtents = cell.m_BoundingBoxHalfExtents.GetData();
      auto tagSets = cell.m_TagSets.GetData();
      auto objectPointers = cell.m_ObjectPointers.GetData();
      auto lastVisibleFrameIdxAndVisType = cell.m_LastVisibleFrameIdxAndVisType.GetData();

      const WUInt32 numSpheres = cell.m_BoundingSpheres.GetCount();
      ref_stats.m_uiNumObjectsTested += numSpheres;

      WUInt32 currentIndex = 0;
      const WUInt64 uiFrameIdxAndType = EncodeLastVisibleFrameIndexAndVisType(pQueryData->m_uiFrameCounter, visType);

      while (currentIndex < numSpheres)
      {
        if (numSpheres - currentIndex >= 32)
        {
          WUInt32 mask = 0;

          for (WUInt32 i = 0; i < 32; i += 2)
          {
            auto& objectSphereA = boundingSpheres[currentIndex + i + 0];
            auto& objectSphereB = boundingSpheres[currentIndex + i + 1];

            mask |= SphereFrustumIntersect(objectSphereA, objectSphereB, planeData) << i;
          }

          while (mask > 0)
          {
            WUInt32 i = WMath::FirstBitLow(mask) + currentIndex;
            mask &= mask - 1;

            if constexpr (UseTagsFilter)
            {
              if (FilterByTags(tagSets[i], queryParams.m_pIncludeTags, queryParams.m_pExcludeTags))
              {
                ref_stats.m_uiNumObjectsFiltered++;
                continue;
              }
            }

            if constexpr (UseOcclusionCallback)
            {
              const WSimdBBox bbox = WSimdBBox::MakeFromCenterAndHalfExtents(boundingSpheres[i].GetCenter(), boundingBoxHalfExtents[i]);
              if (pQueryData->m_IsOccludedCB(bbox))
              {
                continue;
              }
            }

            lastVisibleFrameIdxAndVisType[i].Max(uiFrameIdxAndType);
            pQueryData->m_pOutObjects->PushBack(objectPointers[i]);

            ref_stats.m_uiNumObjectsPassed++;
          }

          currentIndex += 32;
        }
        else
        {
          WUInt32 i = currentIndex;
          ++currentIndex;

          if (!SphereFrustumIntersect(boundingSpheres[i], planeData))
            continue;

          if constexpr (UseTagsFilter)
          {
            if (FilterByTags(tagSets[i], queryParams.m_pIncludeTags, queryParams.m_pExcludeTags))
            {
              ref_stats.m_uiNumObjectsFiltered++;
              continue;
            }
          }

          if constexpr (UseOcclusionCallback)
          {
            const WSimdBBox bbox = WSimdBBox::MakeFromCenterAndHalfExtents(boundingSpheres[i].GetCenter(), boundingBoxHalfExtents[i]);

            if (pQueryData->m_IsOccludedCB(bbox))
            {
              continue;
            }
          }

          lastVisibleFrameIdxAndVisType[i].Max(uiFrameIdxAndType);
          pQueryData->m_pOutObjects->PushBack(objectPointers[i]);

          ref_stats.m_uiNumObjectsPassed++;
        }
      }

      return WVisitorExecution::Continue;
    }
  };
} // namespace WInternal

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSpatialSystem_RegularGrid, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WSpatialSystem_RegularGrid::WSpatialSystem_RegularGrid(WUInt32 uiCellSize /*= 128*/)
  : m_AlignedAllocator("Spatial System Aligned", WFoundation::GetAlignedAllocator())
  , m_vCellSize(uiCellSize)
  , m_vOverlapSize(uiCellSize / 4.0f)
  , m_fInvCellSize(1.0f / uiCellSize)
  , m_Grids(&m_Allocator)
  , m_DataTable(&m_Allocator)
{
  static_assert(sizeof(Data) == 8);

  m_Grids.SetCount(MAX_NUM_GRIDS);

  cvar_SpatialQueriesCachingThreshold.m_CVarEvents.AddEventHandler([&](const WCVarEvent& e)
    {
    if (e.m_EventType == WCVarEvent::ValueChanged)
    {
      RemoveAllCachedGrids();
    } });
}

WSpatialSystem_RegularGrid::~WSpatialSystem_RegularGrid() = default;

WResult WSpatialSystem_RegularGrid::GetCellBoxForSpatialData(const WSpatialDataHandle& hData, WBoundingBox& out_boundingBox) const
{
  Data* pData = nullptr;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return W_FAILURE;

  ForEachGrid(*pData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping)
    {
      auto& pCell = ref_grid.m_Cells[mapping.m_uiCellIndex];

      out_boundingBox = pCell->GetBoundingBox();
      return WVisitorExecution::Stop;
    });

  return W_SUCCESS;
}

template <>
struct WHashHelper<WBoundingBox>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WBoundingBox& value) { return WHashingUtils::xxHash32(&value, sizeof(WBoundingBox)); }

  W_ALWAYS_INLINE static bool Equal(const WBoundingBox& a, const WBoundingBox& b) { return a == b; }
};

void WSpatialSystem_RegularGrid::GetAllCellBoxes(WDynamicArray<WBoundingBox>& out_boundingBoxes, WSpatialData::Category filterCategory /*= WInvalidSpatialDataCategory*/) const
{
  if (filterCategory != WInvalidSpatialDataCategory)
  {
    WUInt32 uiGridIndex = filterCategory.m_uiValue;
    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid != nullptr)
    {
      for (auto& pCell : pGrid->m_Cells)
      {
        out_boundingBoxes.ExpandAndGetRef() = pCell->GetBoundingBox();
      }
    }
  }
  else
  {
    WHashSet<WBoundingBox> boundingBoxes;

    for (auto& pGrid : m_Grids)
    {
      if (pGrid != nullptr)
      {
        for (auto& pCell : pGrid->m_Cells)
        {
          boundingBoxes.Insert(pCell->GetBoundingBox());
        }
      }
    }

    for (auto boundingBox : boundingBoxes)
    {
      out_boundingBoxes.PushBack(boundingBox);
    }
  }
}

void WSpatialSystem_RegularGrid::StartNewFrame()
{
  SUPER::StartNewFrame();

  m_SortedCacheCandidates.Clear();

  {
    W_LOCK(m_CacheCandidatesMutex);

    for (WUInt32 i = 0; i < m_CacheCandidates.GetCount(); ++i)
    {
      auto& cacheCandidate = m_CacheCandidates[i];

      const float fScore = cacheCandidate.m_fQueryCount + cacheCandidate.m_fFilteredRatio * 100.0f;
      m_SortedCacheCandidates.PushBack({i, fScore});

      // Query has to be issued at least once every 10 frames to keep a stable value
      cacheCandidate.m_fQueryCount = WMath::Max(cacheCandidate.m_fQueryCount - 0.1f, 0.0f);
    }
  }

  m_SortedCacheCandidates.Sort();

  // First remove all cached grids that don't make it into the top MAX_NUM_CACHED_GRIDS to make space for new grids
  if (m_SortedCacheCandidates.GetCount() > MAX_NUM_CACHED_GRIDS)
  {
    for (WUInt32 i = MAX_NUM_CACHED_GRIDS; i < m_SortedCacheCandidates.GetCount(); ++i)
    {
      RemoveCachedGrid(m_SortedCacheCandidates[i].m_uiIndex);
    }
  }

  // Then take the MAX_NUM_CACHED_GRIDS candidates with the highest score and migrate the data
  for (WUInt32 i = 0; i < WMath::Min<WUInt32>(m_SortedCacheCandidates.GetCount(), MAX_NUM_CACHED_GRIDS); ++i)
  {
    MigrateCachedGrid(m_SortedCacheCandidates[i].m_uiIndex);
  }
}

WSpatialDataHandle WSpatialSystem_RegularGrid::CreateSpatialData(const WSimdBBoxSphere& bounds, WGameObject* pObject, WUInt32 uiCategoryBitmask, const WTagSet& tags)
{
  if (uiCategoryBitmask == 0)
    return WSpatialDataHandle();

  return AddSpatialDataToGrids(bounds, pObject, uiCategoryBitmask, tags, false);
}

WSpatialDataHandle WSpatialSystem_RegularGrid::CreateSpatialDataAlwaysVisible(WGameObject* pObject, WUInt32 uiCategoryBitmask, const WTagSet& tags)
{
  if (uiCategoryBitmask == 0)
    return WSpatialDataHandle();

  const WSimdBBox hugeBox = WSimdBBox::MakeFromCenterAndHalfExtents(WSimdVec4f::MakeZero(), WSimdVec4f((float)(m_vCellSize.x() * MAX_CELL_INDEX)));

  return AddSpatialDataToGrids(hugeBox, pObject, uiCategoryBitmask, tags, true);
}

void WSpatialSystem_RegularGrid::DeleteSpatialData(const WSpatialDataHandle& hData)
{
  Data oldData;
  W_VERIFY(m_DataTable.Remove(hData.GetInternalID(), &oldData), "Invalid spatial data handle");

  ForEachGrid(oldData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping)
    {
      W_IGNORE_UNUSED(mapping);
      ref_grid.RemoveSpatialData(hData);
      return WVisitorExecution::Continue;
    });
}

void WSpatialSystem_RegularGrid::UpdateSpatialDataBounds(const WSpatialDataHandle& hData, const WSimdBBoxSphere& bounds)
{
  Data* pData = nullptr;
  W_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  // No need to update bounds for always visible data
  if (IsAlwaysVisibleData(*pData))
    return;

  ForEachGrid(*pData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping)
    {
      auto& pOldCell = ref_grid.m_Cells[mapping.m_uiCellIndex];

      if (pOldCell->m_Bounds.GetBox().Contains(bounds.GetBox()))
      {
        pOldCell->m_BoundingSpheres[mapping.m_uiCellDataIndex] = bounds.GetSphere();
        pOldCell->m_BoundingBoxHalfExtents[mapping.m_uiCellDataIndex] = bounds.m_BoxHalfExtents;
      }
      else
      {
        const WTagSet tags = pOldCell->m_TagSets[mapping.m_uiCellDataIndex];
        WGameObject* objectPointer = pOldCell->m_ObjectPointers[mapping.m_uiCellDataIndex];

        const WUInt64 uiLastVisibleFrameIdxAndVisType = pOldCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex];

        ref_grid.RemoveSpatialData(hData);

        ref_grid.AddSpatialData(bounds, tags, objectPointer, uiLastVisibleFrameIdxAndVisType, hData);
      }

      return WVisitorExecution::Continue;
    });
}

void WSpatialSystem_RegularGrid::UpdateSpatialDataObject(const WSpatialDataHandle& hData, WGameObject* pObject)
{
  Data* pData = nullptr;
  W_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  ForEachGrid(*pData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping)
    {
      auto& pCell = ref_grid.m_Cells[mapping.m_uiCellIndex];
      pCell->m_ObjectPointers[mapping.m_uiCellDataIndex] = pObject;
      return WVisitorExecution::Continue;
    });
}

void WSpatialSystem_RegularGrid::FindObjectsInSphere(const WBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const
{
  WSimdBSphere simdSphere(WSimdConversion::ToVec3(sphere.m_vCenter), sphere.m_fRadius);

  const WSimdBBox simdBox = WSimdBBox::MakeFromCenterAndHalfExtents(simdSphere.m_CenterAndRadius, simdSphere.m_CenterAndRadius.Get<WSwizzle::WWWW>());

  WInternal::QueryHelper::ShapeQueryData<WSimdBSphere> queryData = {simdSphere, callback};

  ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
    &WInternal::QueryHelper::ShapeQueryCallback<WSimdBSphere, false>,
    &WInternal::QueryHelper::ShapeQueryCallback<WSimdBSphere, true>,
    &queryData, WVisibilityState::Indirect);
}

void WSpatialSystem_RegularGrid::FindObjectsInBox(const WBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const
{
  WSimdBBox simdBox(WSimdConversion::ToVec3(box.m_vMin), WSimdConversion::ToVec3(box.m_vMax));

  WInternal::QueryHelper::ShapeQueryData<WSimdBBox> queryData = {simdBox, callback};

  ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
    &WInternal::QueryHelper::ShapeQueryCallback<WSimdBBox, false>,
    &WInternal::QueryHelper::ShapeQueryCallback<WSimdBBox, true>,
    &queryData, WVisibilityState::Indirect);
}

void WSpatialSystem_RegularGrid::FindVisibleObjects(const WFrustum& frustum, const QueryParams& queryParams, WDynamicArray<const WGameObject*>& out_Objects, WSpatialSystem::IsOccludedFunc IsOccluded, WVisibilityState::Enum visType) const
{
  W_PROFILE_SCOPE("WSpatialSystem_RegularGrid::FindVisibleObjects");

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WStopwatch timer;
#endif

  WVec3 cornerPoints[8];
  frustum.ComputeCornerPoints(cornerPoints).AssertSuccess();

  WSimdVec4f simdCornerPoints[8];
  for (WUInt32 i = 0; i < 8; ++i)
  {
    simdCornerPoints[i] = WSimdConversion::ToVec3(cornerPoints[i]);
  }

  const WSimdBBox simdBox = WSimdBBox::MakeFromPoints(simdCornerPoints, 8);

  WInternal::QueryHelper::FrustumQueryData queryData;
  {
    // Compiler is too stupid to properly unroll a constant loop so we do it by hand
    WSimdVec4f plane0 = WSimdConversion::ToVec4(*reinterpret_cast<const WVec4*>(&(frustum.GetPlane(0).m_vNormal.x)));
    WSimdVec4f plane1 = WSimdConversion::ToVec4(*reinterpret_cast<const WVec4*>(&(frustum.GetPlane(1).m_vNormal.x)));
    WSimdVec4f plane2 = WSimdConversion::ToVec4(*reinterpret_cast<const WVec4*>(&(frustum.GetPlane(2).m_vNormal.x)));
    WSimdVec4f plane3 = WSimdConversion::ToVec4(*reinterpret_cast<const WVec4*>(&(frustum.GetPlane(3).m_vNormal.x)));
    WSimdVec4f plane4 = WSimdConversion::ToVec4(*reinterpret_cast<const WVec4*>(&(frustum.GetPlane(4).m_vNormal.x)));
    WSimdVec4f plane5 = WSimdConversion::ToVec4(*reinterpret_cast<const WVec4*>(&(frustum.GetPlane(5).m_vNormal.x)));

    WSimdMat4f helperMat;
    helperMat.SetRows(plane0, plane1, plane2, plane3);

    queryData.m_PlaneData.m_x0x1x2x3 = helperMat.m_col0;
    queryData.m_PlaneData.m_y0y1y2y3 = helperMat.m_col1;
    queryData.m_PlaneData.m_z0z1z2z3 = helperMat.m_col2;
    queryData.m_PlaneData.m_w0w1w2w3 = helperMat.m_col3;

    helperMat.SetRows(plane4, plane5, plane4, plane5);

    queryData.m_PlaneData.m_x4x5x4x5 = helperMat.m_col0;
    queryData.m_PlaneData.m_y4y5y4y5 = helperMat.m_col1;
    queryData.m_PlaneData.m_z4z5z4z5 = helperMat.m_col2;
    queryData.m_PlaneData.m_w4w5w4w5 = helperMat.m_col3;

    queryData.m_pOutObjects = &out_Objects;
    queryData.m_uiFrameCounter = m_uiFrameCounter;

    queryData.m_IsOccludedCB = IsOccluded;
  }

  if (IsOccluded.IsValid())
  {
    ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
      &WInternal::QueryHelper::FrustumQueryCallback<false, true>,
      &WInternal::QueryHelper::FrustumQueryCallback<true, true>,
      &queryData, visType);
  }
  else
  {
    ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
      &WInternal::QueryHelper::FrustumQueryCallback<false, false>,
      &WInternal::QueryHelper::FrustumQueryCallback<true, false>,
      &queryData, visType);
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (queryParams.m_pStats != nullptr)
  {
    queryParams.m_pStats->m_TimeTaken = timer.GetRunningTotal();
  }
#endif
}

WVisibilityState::Enum WSpatialSystem_RegularGrid::GetVisibilityState(const WSpatialDataHandle& hData, WUInt32 uiNumFramesBeforeInvisible) const
{
  Data* pData = nullptr;
  W_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  if (IsAlwaysVisibleData(*pData))
    return WVisibilityState::Direct;

  WUInt64 uiLastVisibleFrameIdxAndVisType = 0;
  ForEachGrid(*pData, hData,
    [&](const Grid& grid, const CellDataMapping& mapping)
    {
      auto& pCell = grid.m_Cells[mapping.m_uiCellIndex];
      uiLastVisibleFrameIdxAndVisType = WMath::Max<WUInt64>(uiLastVisibleFrameIdxAndVisType, pCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex]);
      return WVisitorExecution::Continue;
    });

  const WUInt64 uiLastVisibleFrameIdx = ExtractLastVisibleFrameIndex(uiLastVisibleFrameIdxAndVisType);

  if (m_uiFrameCounter > uiLastVisibleFrameIdx + uiNumFramesBeforeInvisible)
    return WVisibilityState::Invisible;

  return ExtractVisType(uiLastVisibleFrameIdxAndVisType);
}

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
void WSpatialSystem_RegularGrid::GetInternalStats(WStringBuilder& sb) const
{
  W_LOCK(m_CacheCandidatesMutex);

  WUInt32 uiNumActiveGrids = 0;
  for (auto& pGrid : m_Grids)
  {
    uiNumActiveGrids += (pGrid != nullptr) ? 1 : 0;
  }

  sb.SetFormat("Num Grids: {}\n", uiNumActiveGrids);

  for (auto& pGrid : m_Grids)
  {
    if (pGrid == nullptr)
      continue;

    sb.AppendFormat(" \nCategory: {}, CanBeCached: {}\nIncludeTags: ", WSpatialData::GetCategoryName(pGrid->m_Category), pGrid->m_bCanBeCached);
    TagsToString(pGrid->m_IncludeTags, sb);
    sb.Append(", ExcludeTags: ");
    TagsToString(pGrid->m_ExcludeTags, sb);
    sb.Append("\n");
  }

  sb.Append("\nCache Candidates:\n");

  for (auto& sortedCandidate : m_SortedCacheCandidates)
  {
    auto& candidate = m_CacheCandidates[sortedCandidate.m_uiIndex];
    const WUInt32 uiGridIndex = candidate.m_uiGridIndex;
    Grid* pGrid = nullptr;

    if (uiGridIndex != WInvalidIndex)
    {
      pGrid = m_Grids[uiGridIndex].Borrow();
      if (pGrid->CachingCompleted())
      {
        continue;
      }
    }

    sb.AppendFormat(" \nCategory: {}\nIncludeTags: ", WSpatialData::GetCategoryName(candidate.m_Category));
    TagsToString(candidate.m_IncludeTags, sb);
    sb.Append(", ExcludeTags: ");
    TagsToString(candidate.m_ExcludeTags, sb);
    sb.AppendFormat("\nScore: {}", WArgF(sortedCandidate.m_fScore, 2));

    if (pGrid != nullptr)
    {
      const WUInt32 uiNumObjectsMigrated = pGrid->m_uiLastMigrationIndex;
      sb.AppendFormat("\nMigrationStatus: {}%%\n", WArgF(float(uiNumObjectsMigrated) / m_DataTable.GetCount() * 100.0f, 2));
    }
  }
}
#endif

W_ALWAYS_INLINE bool WSpatialSystem_RegularGrid::IsAlwaysVisibleData(const Data& data) const
{
  return data.m_uiAlwaysVisible != 0;
}

WSpatialDataHandle WSpatialSystem_RegularGrid::AddSpatialDataToGrids(const WSimdBBoxSphere& bounds, WGameObject* pObject, WUInt32 uiCategoryBitmask, const WTagSet& tags, bool bAlwaysVisible)
{
  Data data;
  data.m_uiGridBitmask = uiCategoryBitmask;
  data.m_uiAlwaysVisible = bAlwaysVisible ? 1 : 0;

  // find matching cached grids and add them to data.m_uiGridBitmask
  for (WUInt32 uiCachedGridIndex = m_uiFirstCachedGridIndex; uiCachedGridIndex < m_Grids.GetCount(); ++uiCachedGridIndex)
  {
    auto& pGrid = m_Grids[uiCachedGridIndex];
    if (pGrid == nullptr)
      continue;

    if ((pGrid->m_Category.GetBitmask() & uiCategoryBitmask) == 0 ||
        FilterByTags(tags, &pGrid->m_IncludeTags, &pGrid->m_ExcludeTags))
      continue;

    data.m_uiGridBitmask |= W_BIT(uiCachedGridIndex);
  }

  auto hData = WSpatialDataHandle(m_DataTable.Insert(data));

  WUInt64 uiGridBitmask = data.m_uiGridBitmask;
  while (uiGridBitmask > 0)
  {
    WUInt32 uiGridIndex = WMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid == nullptr)
    {
      pGrid = W_NEW(&m_Allocator, Grid, *this, WSpatialData::Category(static_cast<WUInt16>(uiGridIndex)));
    }

    const WUInt64 uiLastVisibleFrameIdxAndVisType = EncodeLastVisibleFrameIndexAndVisType(m_uiFrameCounter, WVisibilityState::Direct);
    pGrid->AddSpatialData(bounds, tags, pObject, uiLastVisibleFrameIdxAndVisType, hData);
  }

  return hData;
}

template <typename Functor>
W_FORCE_INLINE void WSpatialSystem_RegularGrid::ForEachGrid(const Data& data, const WSpatialDataHandle& hData, Functor func) const
{
  WUInt64 uiGridBitmask = data.m_uiGridBitmask;
  WUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

  while (uiGridBitmask > 0)
  {
    WUInt32 uiGridIndex = WMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& grid = *m_Grids[uiGridIndex];
    auto& mapping = grid.m_CellDataMappings[uiDataIndex];

    if (func(grid, mapping) == WVisitorExecution::Stop)
      break;
  }
}

void WSpatialSystem_RegularGrid::ForEachCellInBoxInMatchingGrids(const WSimdBBox& box, const QueryParams& queryParams, CellCallback noFilterCallback, CellCallback filterByTagsCallback, void* pUserData, WVisibilityState::Enum visType) const
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (queryParams.m_pStats != nullptr)
  {
    queryParams.m_pStats->m_uiTotalNumObjects = m_DataTable.GetCount();
  }
#endif

  WUInt32 uiGridBitmask = queryParams.m_uiCategoryBitmask;

  // search for cached grids that match the exact query params first
  for (WUInt32 uiCachedGridIndex = m_uiFirstCachedGridIndex; uiCachedGridIndex < m_Grids.GetCount(); ++uiCachedGridIndex)
  {
    auto& pGrid = m_Grids[uiCachedGridIndex];
    if (pGrid == nullptr || pGrid->CachingCompleted() == false)
      continue;

    if ((pGrid->m_Category.GetBitmask() & uiGridBitmask) == 0 ||
        AreTagSetsEqual(pGrid->m_IncludeTags, queryParams.m_pIncludeTags) == false ||
        AreTagSetsEqual(pGrid->m_ExcludeTags, queryParams.m_pExcludeTags) == false)
      continue;

    uiGridBitmask &= ~pGrid->m_Category.GetBitmask();

    Stats stats;
    pGrid->ForEachCellInBox(box,
      [&](const Cell& cell)
      {
        return noFilterCallback(cell, queryParams, stats, pUserData, visType);
      });

    UpdateCacheCandidate(queryParams.m_pIncludeTags, queryParams.m_pExcludeTags, pGrid->m_Category, 0.0f);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    if (queryParams.m_pStats != nullptr)
    {
      queryParams.m_pStats->m_uiNumObjectsTested += stats.m_uiNumObjectsTested;
      queryParams.m_pStats->m_uiNumObjectsPassed += stats.m_uiNumObjectsPassed;
    }
#endif
  }

  // then search for the rest
  const bool useTagsFilter = (queryParams.m_pIncludeTags && queryParams.m_pIncludeTags->IsEmpty() == false) || (queryParams.m_pExcludeTags && queryParams.m_pExcludeTags->IsEmpty() == false);
  CellCallback cellCallback = useTagsFilter ? filterByTagsCallback : noFilterCallback;

  while (uiGridBitmask > 0)
  {
    WUInt32 uiGridIndex = WMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid == nullptr)
      continue;

    Stats stats;
    pGrid->ForEachCellInBox(box,
      [&](const Cell& cell)
      {
        return cellCallback(cell, queryParams, stats, pUserData, visType);
      });

    if (pGrid->m_bCanBeCached && useTagsFilter)
    {
      const WUInt32 totalNumObjectsAfterSpatialTest = stats.m_uiNumObjectsFiltered + stats.m_uiNumObjectsPassed;
      const WUInt32 cacheThreshold = WUInt32(WMath::Max(cvar_SpatialQueriesCachingThreshold.GetValue(), 1));

      // 1.0 => all objects filtered, 0.0 => no object filtered by tags
      const float filteredRatio = float(double(stats.m_uiNumObjectsFiltered) / totalNumObjectsAfterSpatialTest);

      // Doesn't make sense to cache if there are only few objects in total or only few objects have been filtered
      if (totalNumObjectsAfterSpatialTest > cacheThreshold && filteredRatio > 0.1f)
      {
        UpdateCacheCandidate(queryParams.m_pIncludeTags, queryParams.m_pExcludeTags, pGrid->m_Category, filteredRatio);
      }
    }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    if (queryParams.m_pStats != nullptr)
    {
      queryParams.m_pStats->m_uiNumObjectsTested += stats.m_uiNumObjectsTested;
      queryParams.m_pStats->m_uiNumObjectsPassed += stats.m_uiNumObjectsPassed;
    }
#endif
  }
}

void WSpatialSystem_RegularGrid::MigrateCachedGrid(WUInt32 uiCandidateIndex)
{
  WUInt32 uiTargetGridIndex = WInvalidIndex;
  WUInt32 uiSourceGridIndex = WInvalidIndex;

  {
    W_LOCK(m_CacheCandidatesMutex);

    auto& cacheCandidate = m_CacheCandidates[uiCandidateIndex];
    uiTargetGridIndex = cacheCandidate.m_uiGridIndex;
    uiSourceGridIndex = cacheCandidate.m_Category.m_uiValue;

    if (uiTargetGridIndex == WInvalidIndex)
    {
      for (WUInt32 i = m_Grids.GetCount() - 1; i >= MAX_NUM_REGULAR_GRIDS; --i)
      {
        if (m_Grids[i] == nullptr)
        {
          uiTargetGridIndex = i;
          break;
        }
      }

      W_ASSERT_DEBUG(uiTargetGridIndex != WInvalidIndex, "No free cached grid");
      cacheCandidate.m_uiGridIndex = uiTargetGridIndex;

      auto pGrid = W_NEW(&m_Allocator, Grid, *this, cacheCandidate.m_Category);
      pGrid->m_IncludeTags = cacheCandidate.m_IncludeTags;
      pGrid->m_ExcludeTags = cacheCandidate.m_ExcludeTags;

      m_Grids[uiTargetGridIndex] = pGrid;

      m_uiFirstCachedGridIndex = WMath::Min(m_uiFirstCachedGridIndex, uiTargetGridIndex);
    }
  }

  MigrateSpatialData(uiTargetGridIndex, uiSourceGridIndex);
}

void WSpatialSystem_RegularGrid::MigrateSpatialData(WUInt32 uiTargetGridIndex, WUInt32 uiSourceGridIndex)
{
  auto& pTargetGrid = m_Grids[uiTargetGridIndex];
  if (pTargetGrid->CachingCompleted())
    return;

  auto& pSourceGrid = m_Grids[uiSourceGridIndex];

  constexpr WUInt32 uiNumObjectsPerStep = 64;
  WUInt32& uiLastMigrationIndex = pTargetGrid->m_uiLastMigrationIndex;
  const WUInt32 uiSourceCount = pSourceGrid->m_CellDataMappings.GetCount();
  const WUInt32 uiEndIndex = WMath::Min(uiLastMigrationIndex + uiNumObjectsPerStep, uiSourceCount);

  for (WUInt32 i = uiLastMigrationIndex; i < uiEndIndex; ++i)
  {
    if (pTargetGrid->MigrateSpatialDataFromOtherGrid(i, *pSourceGrid))
    {
      m_DataTable.GetValueUnchecked(i).m_uiGridBitmask |= W_BIT(uiTargetGridIndex);
    }
  }

  uiLastMigrationIndex = (uiEndIndex == uiSourceCount) ? WInvalidIndex : uiEndIndex;
}

void WSpatialSystem_RegularGrid::RemoveCachedGrid(WUInt32 uiCandidateIndex)
{
  WUInt32 uiGridIndex;

  {
    W_LOCK(m_CacheCandidatesMutex);

    auto& cacheCandidate = m_CacheCandidates[uiCandidateIndex];
    uiGridIndex = cacheCandidate.m_uiGridIndex;

    if (uiGridIndex == WInvalidIndex)
      return;

    cacheCandidate.m_fQueryCount = 0.0f;
    cacheCandidate.m_fFilteredRatio = 0.0f;
    cacheCandidate.m_uiGridIndex = WInvalidIndex;
  }

  m_Grids[uiGridIndex] = nullptr;
}

void WSpatialSystem_RegularGrid::RemoveAllCachedGrids()
{
  W_LOCK(m_CacheCandidatesMutex);

  for (WUInt32 i = 0; i < m_CacheCandidates.GetCount(); ++i)
  {
    RemoveCachedGrid(i);
  }
}

void WSpatialSystem_RegularGrid::UpdateCacheCandidate(const WTagSet* pIncludeTags, const WTagSet* pExcludeTags, WSpatialData::Category category, float filteredRatio) const
{
  W_LOCK(m_CacheCandidatesMutex);

  CacheCandidate* pCacheCandiate = nullptr;
  for (auto& cacheCandidate : m_CacheCandidates)
  {
    if (cacheCandidate.m_Category == category &&
        AreTagSetsEqual(cacheCandidate.m_IncludeTags, pIncludeTags) &&
        AreTagSetsEqual(cacheCandidate.m_ExcludeTags, pExcludeTags))
    {
      pCacheCandiate = &cacheCandidate;
      break;
    }
  }

  if (pCacheCandiate != nullptr)
  {
    pCacheCandiate->m_fQueryCount = WMath::Min(pCacheCandiate->m_fQueryCount + 1.0f, 100.0f);
    pCacheCandiate->m_fFilteredRatio = WMath::Max(pCacheCandiate->m_fFilteredRatio, filteredRatio);
  }
  else
  {
    auto& cacheCandidate = m_CacheCandidates.ExpandAndGetRef();
    cacheCandidate.m_Category = category;
    cacheCandidate.m_fQueryCount = 1;
    cacheCandidate.m_fFilteredRatio = filteredRatio;

    if (pIncludeTags != nullptr)
    {
      cacheCandidate.m_IncludeTags = *pIncludeTags;
    }
    if (pExcludeTags != nullptr)
    {
      cacheCandidate.m_ExcludeTags = *pExcludeTags;
    }
  }
}

W_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem_RegularGrid);
