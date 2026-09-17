#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Rect.h>
#include <Utilities/DataStructures/GameGrid.h>

/// Takes an WGameGrid and creates an optimized navmesh structure from it, that is more efficient for path searches.
class W_UTILITIES_DLL WGridNavmesh
{
public:
  struct ConvexArea
  {
    W_DECLARE_POD_TYPE();

    /// The space that is enclosed by this convex area.
    WRectU32 m_Rect;

    /// The first AreaEdge that belongs to this ConvexArea.
    WUInt32 m_uiFirstEdge;

    /// The number of AreaEdge's that belong to this ConvexArea.
    WUInt32 m_uiNumEdges;
  };

  struct AreaEdge
  {
    W_DECLARE_POD_TYPE();

    /// The 'area' of the edge. This is a one cell wide line that is always WITHIN the ConvexArea from where the edge connects to a neighbor
    /// area.
    WRectU16 m_EdgeRect;

    /// The index of the area that can be reached over this edge. This is always a valid index.
    WInt32 m_iNeighborArea;
  };

  /// Callback that determines whether the cell with index \a uiCell1 and the cell with index \a uiCell2 represent the same type of
  /// terrain.
  using CellComparator = bool (*)(WUInt32, WUInt32, void*);

  /// Callback that determines whether the cell with index \a uiCell is blocked entirely (for every type of unit) and therefore can
  /// be optimized away.
  using CellBlocked = bool (*)(WUInt32, void*);

  /// Creates the navmesh from the given WGameGrid.
  template <class CellData>
  void CreateFromGrid(
    const WGameGrid<CellData>& grid, CellComparator isSameCellType, void* pPassThroughSame, CellBlocked isCellBlocked, void* pPassThroughBlocked);

  /// Returns the index of the ConvexArea at the given cell coordinates. Negative, if the cell is blocked.
  WInt32 GetAreaAt(const WVec2I32& vCoord) const { return m_NodesGrid.GetCell(vCoord); }

  /// Returns the number of convex areas that this navmesh consists of.
  WUInt32 GetNumConvexAreas() const { return m_ConvexAreas.GetCount(); }

  /// Returns the given convex area by index.
  const ConvexArea& GetConvexArea(WInt32 iArea) const { return m_ConvexAreas[iArea]; }

  /// Returns the number of edges between convex areas.
  WUInt32 GetNumAreaEdges() const { return m_GraphEdges.GetCount(); }

  /// Returns the given area edge by index.
  const AreaEdge& GetAreaEdge(WInt32 iAreaEdge) const { return m_GraphEdges[iAreaEdge]; }

private:
  void UpdateRegion(WRectU32 region, CellComparator IsSameCellType, void* pPassThrough1, CellBlocked IsCellBlocked, void* pPassThrough2);

  void Optimize(WRectU32 region, CellComparator IsSameCellType, void* pPassThrough);
  bool OptimizeBoxes(WRectU32 region, CellComparator IsSameCellType, void* pPassThrough, WUInt32 uiIntervalX, WUInt32 uiIntervalY,
    WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiOffsetX = 0, WUInt32 uiOffsetY = 0);
  bool CanCreateArea(WRectU32 region, CellComparator IsSameCellType, void* pPassThrough) const;

  bool CanMergeRight(WInt32 x, WInt32 y, CellComparator IsSameCellType, void* pPassThrough, WRectU32& out_Result) const;
  bool CanMergeDown(WInt32 x, WInt32 y, CellComparator IsSameCellType, void* pPassThrough, WRectU32& out_Result) const;
  bool MergeBestFit(WRectU32 region, CellComparator IsSameCellType, void* pPassThrough);

  void CreateGraphEdges();
  void CreateGraphEdges(ConvexArea& Area);

  WRectU32 GetCellBBox(WInt32 x, WInt32 y) const;
  void Merge(const WRectU32& rect);
  void CreateNodes(WRectU32 region, CellBlocked IsCellBlocked, void* pPassThrough);

  WGameGrid<WInt32> m_NodesGrid;
  WDynamicArray<ConvexArea> m_ConvexAreas;
  WDeque<AreaEdge> m_GraphEdges;
};

#include <Utilities/PathFinding/Implementation/GridNavmesh_inl.h>
