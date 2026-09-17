#pragma once

template <class CellData>
void WGridNavmesh::CreateFromGrid(
  const WGameGrid<CellData>& grid, CellComparator isSameCellType, void* pPassThrough, CellBlocked isCellBlocked, void* pPassThrough2)
{
  m_NodesGrid.CreateGrid(grid.GetGridSizeX(), grid.GetGridSizeY());

  UpdateRegion(WRectU32(grid.GetGridSizeX(), grid.GetGridSizeY()), isSameCellType, pPassThrough, isCellBlocked, pPassThrough2);

  CreateGraphEdges();
}
