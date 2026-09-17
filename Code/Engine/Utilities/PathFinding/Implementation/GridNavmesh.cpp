#include <Utilities/UtilitiesPCH.h>

#include <Utilities/PathFinding/GridNavmesh.h>

void WGridNavmesh::UpdateRegion(WRectU32 region, CellComparator IsSameCellType, void* pPassThrough1, CellBlocked IsCellBlocked, void* pPassThrough2)
{
  WInt32 iInvalidNode = -(WInt32)m_ConvexAreas.GetCount();

  // initialize with 'invalid'
  for (WUInt32 y = region.y; y < region.y + region.height; ++y)
  {
    for (WUInt32 x = region.x; x < region.x + region.width; ++x)
    {
      --iInvalidNode;
      m_NodesGrid.GetCell(WVec2I32(x, y)) = iInvalidNode;
    }
  }

  Optimize(region, IsSameCellType, pPassThrough1);

  CreateNodes(region, IsCellBlocked, pPassThrough2);
}


WRectU32 WGridNavmesh::GetCellBBox(WInt32 x, WInt32 y) const
{
  const WInt32 iCellNode = m_NodesGrid.GetCell(WVec2I32(x, y));

  WRectU32 r;
  r.x = x;
  r.y = y;
  r.width = 1;
  r.height = 1;

  while (r.x > 0)
  {
    if (iCellNode != m_NodesGrid.GetCell(WVec2I32(r.x - 1, y)))
      break;

    r.x--;
  }

  while (r.y > 0)
  {
    if (iCellNode != m_NodesGrid.GetCell(WVec2I32(x, r.y - 1)))
      break;

    r.y--;
  }

  while (x < m_NodesGrid.GetGridSizeX() - 1)
  {
    if (iCellNode != m_NodesGrid.GetCell(WVec2I32(x + 1, y)))
      break;

    x++;
  }

  while (y < m_NodesGrid.GetGridSizeY() - 1)
  {
    if (iCellNode != m_NodesGrid.GetCell(WVec2I32(x, y + 1)))
      break;

    y++;
  }

  r.width = x - r.x + 1;
  r.height = y - r.y + 1;

  return r;
}

void WGridNavmesh::Merge(const WRectU32& rect)
{
  const WInt32 iCellNode = m_NodesGrid.GetCell(WVec2I32(rect.x, rect.y));

  for (WUInt32 y = rect.y; y < rect.y + rect.height; ++y)
  {
    for (WUInt32 x = rect.x; x < rect.x + rect.width; ++x)
    {
      m_NodesGrid.GetCell(WVec2I32(x, y)) = iCellNode;
    }
  }
}

void WGridNavmesh::CreateNodes(WRectU32 region, CellBlocked IsCellBlocked, void* pPassThrough)
{
  for (WUInt32 y = region.y; y < region.y + region.height; ++y)
  {
    for (WUInt32 x = region.x; x < region.x + region.width; ++x)
    {
      const WInt32 iCellNode = m_NodesGrid.GetCell(WVec2I32(x, y));

      if (iCellNode >= 0)
        continue;

      if (IsCellBlocked(m_NodesGrid.ConvertCellCoordinateToIndex(WVec2I32(x, y)), pPassThrough))
      {
        m_NodesGrid.GetCell(WVec2I32(x, y)) = -1;
        continue;
      }

      ConvexArea a;
      a.m_Rect = GetCellBBox(x, y);

      m_NodesGrid.GetCell(WVec2I32(a.m_Rect.x, a.m_Rect.y)) = m_ConvexAreas.GetCount();
      m_ConvexAreas.PushBack(a);

      Merge(a.m_Rect);
    }
  }
}


void WGridNavmesh::Optimize(WRectU32 region, CellComparator IsSameCellType, void* pPassThrough)
{
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 8, 8, 8, 8))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 4, 4, 4))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 2, 2, 2, 2))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 2, 3, 2))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 2, 4, 2, 3))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 2, 3, 2, 1, 0))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 2, 4, 2, 3, 0, 1))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 2, 4, 2))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 2, 4, 2, 4))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 8, 4, 6, 4))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 8, 4, 6))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 8, 4, 6, 4, 2, 0))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 8, 4, 6, 0, 2))
  {
  }


  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 4, 2))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 2, 4))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 3, 2))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 2, 3))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 2, 2))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 2, 1))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 1, 2))
  {
  }

  while (MergeBestFit(region, IsSameCellType, pPassThrough))
  {
  }
}

bool WGridNavmesh::CanCreateArea(WRectU32 region, CellComparator IsSameCellType, void* pPassThrough) const
{
  if (region.x + region.width > m_NodesGrid.GetGridSizeX())
    return false;
  if (region.y + region.height > m_NodesGrid.GetGridSizeY())
    return false;

  const WUInt32 uiStartNode = m_NodesGrid.ConvertCellCoordinateToIndex(WVec2I32(region.x, region.y));
  // const WInt32 iStartNodeArea = m_NodesGrid.GetCell(uiStartNode);

  for (WUInt32 y = region.y; y < region.y + region.height; ++y)
  {
    for (WUInt32 x = region.x; x < region.x + region.width; ++x)
    {
      const WUInt32 uiCurNode = m_NodesGrid.ConvertCellCoordinateToIndex(WVec2I32(x, y));
      // const WInt32 iCurNodeArea = m_NodesGrid.GetCell(uiCurNode);

      // if (iCurNodeArea == iStartNodeArea)
      // continue;

      if (!IsSameCellType(uiStartNode, uiCurNode, pPassThrough))
        return false;

      const WRectU32 rect = GetCellBBox(x, y);

      if (rect.x < region.x || rect.y < region.y)
        return false;
      if (rect.x + rect.width > region.x + region.width)
        return false;
      if (rect.y + rect.height > region.y + region.height)
        return false;
    }
  }

  return true;
}

bool WGridNavmesh::OptimizeBoxes(WRectU32 region, CellComparator IsSameCellType, void* pPassThrough, WUInt32 uiIntervalX, WUInt32 uiIntervalY,
  WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiOffsetX, WUInt32 uiOffsetY)
{
  bool bMergedAny = false;

  for (WUInt32 y = region.y; y < region.y + region.height; y += uiIntervalY)
  {
    for (WUInt32 x = region.x; x < region.x + region.width; x += uiIntervalX)
    {
      WRectU32 NewArea;
      NewArea.x = x + uiOffsetX;
      NewArea.y = y + uiOffsetY;
      NewArea.width = uiWidth;
      NewArea.height = uiHeight;

      if (CanCreateArea(NewArea, IsSameCellType, pPassThrough))
      {
        bMergedAny = true;

        Merge(NewArea);
      }
    }
  }

  return bMergedAny;
}



bool WGridNavmesh::CanMergeRight(WInt32 x, WInt32 y, CellComparator IsSameCellType, void* pPassThrough, WRectU32& out_Result) const
{
  const WRectU32 r1 = GetCellBBox(x, y);

  if (r1.x + r1.width >= m_NodesGrid.GetGridSizeX())
    return false;

  // const WInt32 iCellNode = m_NodesGrid.GetCell(WVec2I32(x, y));

  if (!IsSameCellType(m_NodesGrid.ConvertCellCoordinateToIndex(WVec2I32(x, y)),
        m_NodesGrid.ConvertCellCoordinateToIndex(WVec2I32(r1.x + r1.width, y)), pPassThrough))
    return false;

  const WRectU32 r2 = GetCellBBox(r1.x + r1.width, y);

  if (r1.y != r2.y || r1.height != r2.height)
    return false;

  out_Result.x = r1.x;
  out_Result.y = r1.y;
  out_Result.width = r1.width + r2.width;
  out_Result.height = r1.height;

  return true;
}

bool WGridNavmesh::CanMergeDown(WInt32 x, WInt32 y, CellComparator IsSameCellType, void* pPassThrough, WRectU32& out_Result) const
{
  const WRectU32 r1 = GetCellBBox(x, y);

  if (r1.y + r1.height >= m_NodesGrid.GetGridSizeY())
    return false;

  // const WInt32 iCellNode = m_NodesGrid.GetCell(WVec2I32(x, y));

  if (!IsSameCellType(m_NodesGrid.ConvertCellCoordinateToIndex(WVec2I32(x, y)),
        m_NodesGrid.ConvertCellCoordinateToIndex(WVec2I32(x, r1.y + r1.height)), pPassThrough))
    return false;

  const WRectU32 r2 = GetCellBBox(x, r1.y + r1.height);

  if (r1.x != r2.x || r1.width != r2.width)
    return false;

  out_Result.x = r1.x;
  out_Result.y = r1.y;
  out_Result.width = r1.width;
  out_Result.height = r1.height + r2.height;

  return true;
}


bool WGridNavmesh::MergeBestFit(WRectU32 region, CellComparator IsSameCellType, void* pPassThrough)
{
  bool bMergedAny = false;

  for (WUInt32 y = region.y; y < region.y + region.height; y += 1)
  {
    for (WUInt32 x = region.x; x < region.x + region.width; x += 1)
    {
      const WInt32 iCellNode = m_NodesGrid.GetCell(WVec2I32(x, y));

      if (iCellNode >= 0)
        continue;

      WRectU32 rd;
      bool bRD = false;
      if (CanMergeDown(x, y, IsSameCellType, pPassThrough, rd))
      {
        if (rd.x == x && rd.y == y)
          bRD = true;
      }

      WRectU32 rr;
      bool bRR = false;
      if (CanMergeRight(x, y, IsSameCellType, pPassThrough, rr))
      {
        if (rr.x == x && rr.y == y)
          bRR = true;
      }

      if (bRR && bRD)
      {
        const float fRatioRR = (float)WMath::Max(rr.width, rr.height) / (float)WMath::Min(rr.width, rr.height);
        const float fRatioRD = (float)WMath::Max(rd.width, rd.height) / (float)WMath::Min(rd.width, rd.height);

        if (fRatioRR < fRatioRD)
          bRD = false;
        else
          bRR = false;
      }

      if (bRR)
      {
        // const float fRatio = (float) WMath::Max(rr.width,  rr.height) / (float) WMath::Min(rr.width,  rr.height);

        bMergedAny = true;
        Merge(rr);
      }

      if (bRD)
      {
        // const float fRatio = (float) WMath::Max(rd.width,  rd.height) / (float) WMath::Min(rd.width,  rd.height);

        Merge(rd);
        bMergedAny = true;
      }
    }
  }

  return bMergedAny;
}

void WGridNavmesh::CreateGraphEdges()
{
  m_GraphEdges.Clear();

  for (WUInt32 i = 0; i < m_ConvexAreas.GetCount(); ++i)
    CreateGraphEdges(m_ConvexAreas[i]);
}

void WGridNavmesh::CreateGraphEdges(ConvexArea& Area)
{
  Area.m_uiFirstEdge = m_GraphEdges.GetCount();
  Area.m_uiNumEdges = 0;

  if (Area.m_Rect.y > 0)
  {
    AreaEdge e;
    e.m_EdgeRect.x = static_cast<WUInt16>(Area.m_Rect.x);
    e.m_EdgeRect.y = static_cast<WUInt16>(Area.m_Rect.y);
    e.m_EdgeRect.width = 1;
    e.m_EdgeRect.height = 1;
    e.m_iNeighborArea = m_NodesGrid.GetCell(WVec2I32(Area.m_Rect.x, Area.m_Rect.y - 1));

    for (WUInt32 x = Area.m_Rect.x + 1; x < Area.m_Rect.x + Area.m_Rect.width; ++x)
    {
      const WInt32 iThisNeighbor = m_NodesGrid.GetCell(WVec2I32(x, Area.m_Rect.y - 1));

      if (e.m_iNeighborArea != iThisNeighbor)
      {
        if (e.m_iNeighborArea >= 0)
        {
          m_GraphEdges.PushBack(e);
          ++Area.m_uiNumEdges;
        }

        e.m_iNeighborArea = iThisNeighbor;
        e.m_EdgeRect.x = static_cast<WUInt16>(x);
        e.m_EdgeRect.width = 0;
      }

      ++e.m_EdgeRect.width;
    }

    if (e.m_iNeighborArea >= 0)
    {
      m_GraphEdges.PushBack(e);
      ++Area.m_uiNumEdges;
    }
  }

  if (Area.m_Rect.y + Area.m_Rect.height < m_NodesGrid.GetGridSizeY())
  {
    AreaEdge e;
    e.m_EdgeRect.x = static_cast<WUInt16>(Area.m_Rect.x);
    e.m_EdgeRect.y = static_cast<WUInt16>(Area.m_Rect.y + Area.m_Rect.height - 1);
    e.m_EdgeRect.width = 1;
    e.m_EdgeRect.height = 1;
    e.m_iNeighborArea = m_NodesGrid.GetCell(WVec2I32(Area.m_Rect.x, Area.m_Rect.y + Area.m_Rect.height));

    for (WUInt32 x = Area.m_Rect.x + 1; x < Area.m_Rect.x + Area.m_Rect.width; ++x)
    {
      const WInt32 iThisNeighbor = m_NodesGrid.GetCell(WVec2I32(x, Area.m_Rect.y + Area.m_Rect.height));

      if (e.m_iNeighborArea != iThisNeighbor)
      {
        if (e.m_iNeighborArea >= 0)
        {
          m_GraphEdges.PushBack(e);
          ++Area.m_uiNumEdges;
        }

        e.m_iNeighborArea = iThisNeighbor;
        e.m_EdgeRect.x = static_cast<WUInt16>(x);
        e.m_EdgeRect.width = 0;
      }

      ++e.m_EdgeRect.width;
    }

    if (e.m_iNeighborArea >= 0)
    {
      m_GraphEdges.PushBack(e);
      ++Area.m_uiNumEdges;
    }
  }

  if (Area.m_Rect.x > 0)
  {
    AreaEdge e;
    e.m_EdgeRect.x = static_cast<WUInt16>(Area.m_Rect.x);
    e.m_EdgeRect.y = static_cast<WUInt16>(Area.m_Rect.y);
    e.m_EdgeRect.width = 1;
    e.m_EdgeRect.height = 1;
    e.m_iNeighborArea = m_NodesGrid.GetCell(WVec2I32(Area.m_Rect.x - 1, Area.m_Rect.y));

    for (WUInt32 y = Area.m_Rect.y + 1; y < Area.m_Rect.y + Area.m_Rect.height; ++y)
    {
      const WInt32 iThisNeighbor = m_NodesGrid.GetCell(WVec2I32(Area.m_Rect.x - 1, y));

      if (e.m_iNeighborArea != iThisNeighbor)
      {
        if (e.m_iNeighborArea >= 0)
        {
          m_GraphEdges.PushBack(e);
          ++Area.m_uiNumEdges;
        }

        e.m_iNeighborArea = iThisNeighbor;
        e.m_EdgeRect.y = static_cast<WUInt16>(y);
        e.m_EdgeRect.height = 0;
      }

      ++e.m_EdgeRect.height;
    }

    if (e.m_iNeighborArea >= 0)
    {
      m_GraphEdges.PushBack(e);
      ++Area.m_uiNumEdges;
    }
  }

  if (Area.m_Rect.x + Area.m_Rect.width < m_NodesGrid.GetGridSizeX())
  {
    AreaEdge e;
    e.m_EdgeRect.x = static_cast<WUInt16>(Area.m_Rect.x + Area.m_Rect.width - 1);
    e.m_EdgeRect.y = static_cast<WUInt16>(Area.m_Rect.y);
    e.m_EdgeRect.width = 1;
    e.m_EdgeRect.height = 1;
    e.m_iNeighborArea = m_NodesGrid.GetCell(WVec2I32(Area.m_Rect.x + Area.m_Rect.width, Area.m_Rect.y));

    for (WUInt32 y = Area.m_Rect.y + 1; y < Area.m_Rect.y + Area.m_Rect.height; ++y)
    {
      const WInt32 iThisNeighbor = m_NodesGrid.GetCell(WVec2I32(Area.m_Rect.x + Area.m_Rect.width, y));

      if (e.m_iNeighborArea != iThisNeighbor)
      {
        if (e.m_iNeighborArea >= 0)
        {
          m_GraphEdges.PushBack(e);
          ++Area.m_uiNumEdges;
        }

        e.m_iNeighborArea = iThisNeighbor;
        e.m_EdgeRect.y = static_cast<WUInt16>(y);
        e.m_EdgeRect.height = 0;
      }

      ++e.m_EdgeRect.height;
    }

    if (e.m_iNeighborArea >= 0)
    {
      m_GraphEdges.PushBack(e);
      ++Area.m_uiNumEdges;
    }
  }
}
