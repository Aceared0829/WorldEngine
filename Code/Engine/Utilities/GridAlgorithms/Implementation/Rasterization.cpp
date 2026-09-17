#include <Utilities/UtilitiesPCH.h>

#include <Utilities/GridAlgorithms/Rasterization.h>

WRasterizationResult::Enum ez2DGridUtils::ComputePointsOnLine(WInt32 iStartX, WInt32 iStartY, WInt32 iEndX, WInt32 iEndY, W_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  // Implements Bresenham's line algorithm:
  // http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

  WInt32 dx = WMath::Abs(iEndX - iStartX);
  WInt32 dy = WMath::Abs(iEndY - iStartY);

  WInt32 sx = (iStartX < iEndX) ? 1 : -1;
  WInt32 sy = (iStartY < iEndY) ? 1 : -1;

  WInt32 err = dx - dy;

  while (true)
  {
    // The user callback can stop the algorithm at any point, if no further points on the line are required
    if (callback(iStartX, iStartY, pPassThrough) == WCallbackResult::Stop)
      return WRasterizationResult::Aborted;

    if ((iStartX == iEndX) && (iStartY == iEndY))
      return WRasterizationResult::Finished;

    WInt32 e2 = 2 * err;

    if (e2 > -dy)
    {
      err = err - dy;
      iStartX = iStartX + sx;
    }

    if (e2 < dx)
    {
      err = err + dx;
      iStartY = iStartY + sy;
    }
  }
}

WRasterizationResult::Enum ez2DGridUtils::ComputePointsOnLineConservative(WInt32 iStartX, WInt32 iStartY, WInt32 iEndX, WInt32 iEndY,
  W_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */, bool bVisitBothNeighbors /* = false */)
{
  WInt32 dx = WMath::Abs(iEndX - iStartX);
  WInt32 dy = WMath::Abs(iEndY - iStartY);

  WInt32 sx = (iStartX < iEndX) ? 1 : -1;
  WInt32 sy = (iStartY < iEndY) ? 1 : -1;

  WInt32 err = dx - dy;

  WInt32 iLastX = iStartX;
  WInt32 iLastY = iStartY;

  while (true)
  {
    // if this is going to be a diagonal step, make sure to insert horizontal/vertical steps

    if ((WMath::Abs(iLastX - iStartX) + WMath::Abs(iLastY - iStartY)) == 2)
    {
      // This part is the difference to the non-conservative line algorithm

      if (callback(iLastX, iStartY, pPassThrough) == WCallbackResult::Continue)
      {
        // first one succeeded, going to continue

        // if this is true, the user still wants a callback for the alternative, even though it does not change the outcome anymore
        if (bVisitBothNeighbors)
          callback(iStartX, iLastY, pPassThrough);
      }
      else
      {
        // first one failed, try the second
        if (callback(iStartX, iLastY, pPassThrough) == WCallbackResult::Stop)
          return WRasterizationResult::Aborted;
      }
    }

    iLastX = iStartX;
    iLastY = iStartY;

    // The user callback can stop the algorithm at any point, if no further points on the line are required
    if (callback(iStartX, iStartY, pPassThrough) == WCallbackResult::Stop)
      return WRasterizationResult::Aborted;

    if ((iStartX == iEndX) && (iStartY == iEndY))
      return WRasterizationResult::Finished;

    WInt32 e2 = 2 * err;

    if (e2 > -dy)
    {
      err = err - dy;
      iStartX = iStartX + sx;
    }

    if (e2 < dx)
    {
      err = err + dx;
      iStartY = iStartY + sy;
    }
  }
}


WRasterizationResult::Enum ez2DGridUtils::ComputePointsOnCircle(WInt32 iStartX, WInt32 iStartY, WUInt32 uiRadius, W_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  int f = 1 - uiRadius;
  int ddF_x = 1;
  int ddF_y = -2 * uiRadius;
  int x = 0;
  int y = uiRadius;

  // report the four extremes
  if (callback(iStartX, iStartY + uiRadius, pPassThrough) == WCallbackResult::Stop)
    return WRasterizationResult::Aborted;
  if (callback(iStartX, iStartY - uiRadius, pPassThrough) == WCallbackResult::Stop)
    return WRasterizationResult::Aborted;
  if (callback(iStartX + uiRadius, iStartY, pPassThrough) == WCallbackResult::Stop)
    return WRasterizationResult::Aborted;
  if (callback(iStartX - uiRadius, iStartY, pPassThrough) == WCallbackResult::Stop)
    return WRasterizationResult::Aborted;

  // the loop iterates over an eighth of the circle (a 45 degree segment) and then mirrors each point 8 times to fill the entire circle
  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    if (callback(iStartX + x, iStartY + y, pPassThrough) == WCallbackResult::Stop)
      return WRasterizationResult::Aborted;
    if (callback(iStartX - x, iStartY + y, pPassThrough) == WCallbackResult::Stop)
      return WRasterizationResult::Aborted;
    if (callback(iStartX + x, iStartY - y, pPassThrough) == WCallbackResult::Stop)
      return WRasterizationResult::Aborted;
    if (callback(iStartX - x, iStartY - y, pPassThrough) == WCallbackResult::Stop)
      return WRasterizationResult::Aborted;
    if (callback(iStartX + y, iStartY + x, pPassThrough) == WCallbackResult::Stop)
      return WRasterizationResult::Aborted;
    if (callback(iStartX - y, iStartY + x, pPassThrough) == WCallbackResult::Stop)
      return WRasterizationResult::Aborted;
    if (callback(iStartX + y, iStartY - x, pPassThrough) == WCallbackResult::Stop)
      return WRasterizationResult::Aborted;
    if (callback(iStartX - y, iStartY - x, pPassThrough) == WCallbackResult::Stop)
      return WRasterizationResult::Aborted;
  }

  return WRasterizationResult::Finished;
}

WUInt32 ez2DGridUtils::FloodFill(WInt32 iStartX, WInt32 iStartY, W_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */,
  WDeque<WVec2I32>* pTempArray /* = nullptr */)
{
  WUInt32 uiFilled = 0;

  WDeque<WVec2I32> FallbackQueue;

  if (pTempArray == nullptr)
    pTempArray = &FallbackQueue;

  pTempArray->Clear();
  pTempArray->PushBack(WVec2I32(iStartX, iStartY));

  while (!pTempArray->IsEmpty())
  {
    WVec2I32 v = pTempArray->PeekBack();
    pTempArray->PopBack();

    if (callback(v.x, v.y, pPassThrough) == WCallbackResult::Continue)
    {
      ++uiFilled;

      // put the four neighbors into the queue
      pTempArray->PushBack(WVec2I32(v.x - 1, v.y));
      pTempArray->PushBack(WVec2I32(v.x + 1, v.y));
      pTempArray->PushBack(WVec2I32(v.x, v.y - 1));
      pTempArray->PushBack(WVec2I32(v.x, v.y + 1));
    }
  }

  return uiFilled;
}

WUInt32 ez2DGridUtils::FloodFillDiag(WInt32 iStartX, WInt32 iStartY, W_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /*= nullptr*/,
  WDeque<WVec2I32>* pTempArray /*= nullptr*/)
{
  WUInt32 uiFilled = 0;

  WDeque<WVec2I32> FallbackQueue;

  if (pTempArray == nullptr)
    pTempArray = &FallbackQueue;

  pTempArray->Clear();
  pTempArray->PushBack(WVec2I32(iStartX, iStartY));

  while (!pTempArray->IsEmpty())
  {
    WVec2I32 v = pTempArray->PeekBack();
    pTempArray->PopBack();

    if (callback(v.x, v.y, pPassThrough) == WCallbackResult::Continue)
    {
      ++uiFilled;

      // put the eight neighbors into the queue
      pTempArray->PushBack(WVec2I32(v.x - 1, v.y));
      pTempArray->PushBack(WVec2I32(v.x + 1, v.y));
      pTempArray->PushBack(WVec2I32(v.x, v.y - 1));
      pTempArray->PushBack(WVec2I32(v.x, v.y + 1));

      pTempArray->PushBack(WVec2I32(v.x - 1, v.y - 1));
      pTempArray->PushBack(WVec2I32(v.x + 1, v.y - 1));
      pTempArray->PushBack(WVec2I32(v.x + 1, v.y + 1));
      pTempArray->PushBack(WVec2I32(v.x - 1, v.y + 1));
    }
  }

  return uiFilled;
}

// Lookup table that describes the shape of the circle
// When rasterizing circles with few pixels algorithms usually don't give nice shapes
// so this lookup table is handcrafted for better results
static const WUInt8 OverlapCircle[15][15] = {{9, 9, 9, 9, 9, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9}, {9, 9, 9, 8, 8, 7, 7, 7, 7, 7, 8, 8, 9, 9, 9},
  {9, 9, 8, 8, 7, 6, 6, 6, 6, 6, 7, 8, 8, 9, 9}, {9, 8, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 8, 9}, {9, 8, 7, 6, 6, 5, 4, 4, 4, 5, 6, 6, 7, 8, 9},
  {8, 7, 6, 6, 5, 4, 3, 3, 3, 4, 5, 6, 6, 7, 8}, {8, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 8}, {8, 7, 6, 5, 4, 3, 1, 0, 1, 3, 4, 5, 6, 7, 8},
  {8, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 8}, {8, 7, 6, 6, 5, 4, 3, 3, 3, 4, 5, 6, 6, 7, 8}, {9, 8, 7, 6, 6, 5, 4, 4, 4, 5, 6, 6, 7, 8, 9},
  {9, 8, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 8, 9}, {9, 9, 8, 8, 7, 6, 6, 6, 6, 6, 7, 8, 8, 9, 9}, {9, 9, 9, 8, 8, 7, 7, 7, 7, 7, 8, 8, 9, 9, 9},
  {9, 9, 9, 9, 9, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9}};

static const WInt32 CircleCenter = 7;
static const WUInt8 CircleAreaMin[9] = {7, 6, 6, 5, 4, 3, 2, 1, 0};
static const WUInt8 CircleAreaMax[9] = {7, 8, 8, 9, 10, 11, 12, 13, 14};

WRasterizationResult::Enum ez2DGridUtils::RasterizeBlob(WInt32 iPosX, WInt32 iPosY, WBlobType type, W_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  const WUInt8 uiCircleType = WMath::Clamp<WUInt8>(type, 0, 8);

  const WInt32 iAreaMin = CircleAreaMin[uiCircleType];
  const WInt32 iAreaMax = CircleAreaMax[uiCircleType];

  iPosX -= CircleCenter;
  iPosY -= CircleCenter;

  for (WInt32 y = iAreaMin; y <= iAreaMax; ++y)
  {
    for (WInt32 x = iAreaMin; x <= iAreaMax; ++x)
    {
      if (OverlapCircle[y][x] <= uiCircleType)
      {
        if (callback(iPosX + x, iPosY + y, pPassThrough) == WCallbackResult::Stop)
          return WRasterizationResult::Aborted;
      }
    }
  }

  return WRasterizationResult::Finished;
}

WRasterizationResult::Enum ez2DGridUtils::RasterizeBlobWithDistance(WInt32 iPosX, WInt32 iPosY, WBlobType type, W_RASTERIZED_BLOB_CALLBACK callback, void* pPassThrough /*= nullptr*/)
{
  const WUInt8 uiCircleType = WMath::Clamp<WUInt8>(type, 0, 8);

  const WInt32 iAreaMin = CircleAreaMin[uiCircleType];
  const WInt32 iAreaMax = CircleAreaMax[uiCircleType];

  iPosX -= CircleCenter;
  iPosY -= CircleCenter;

  for (WInt32 y = iAreaMin; y <= iAreaMax; ++y)
  {
    for (WInt32 x = iAreaMin; x <= iAreaMax; ++x)
    {
      const WUInt8 uiDistance = OverlapCircle[y][x];

      if (uiDistance <= uiCircleType)
      {
        if (callback(iPosX + x, iPosY + y, pPassThrough, uiDistance) == WCallbackResult::Stop)
          return WRasterizationResult::Aborted;
      }
    }
  }

  return WRasterizationResult::Finished;
}

WRasterizationResult::Enum ez2DGridUtils::RasterizeCircle(WInt32 iPosX, WInt32 iPosY, float fRadius, W_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  const WVec2 vCenter((float)iPosX, (float)iPosY);

  const WInt32 iRadius = (WInt32)fRadius;
  const float fRadiusSqr = WMath::Square(fRadius);

  for (WInt32 y = iPosY - iRadius; y <= iPosY + iRadius; ++y)
  {
    for (WInt32 x = iPosX - iRadius; x <= iPosX + iRadius; ++x)
    {
      const WVec2 v((float)x, (float)y);

      if ((v - vCenter).GetLengthSquared() > fRadiusSqr)
        continue;

      if (callback(x, y, pPassThrough) == WCallbackResult::Stop)
        return WRasterizationResult::Aborted;
    }
  }

  return WRasterizationResult::Finished;
}


struct VisibilityLine
{
  WDynamicArray<WUInt8>* m_pVisible;
  WUInt32 m_uiSize;
  WUInt32 m_uiRadius;
  WInt32 m_iCenterX;
  WInt32 m_iCenterY;
  ez2DGridUtils::W_RASTERIZED_POINT_CALLBACK m_VisCallback;
  void* m_pUserPassThrough;
  WUInt32 m_uiWidth;
  WUInt32 m_uiHeight;
  WVec2 m_vDirection;
  WAngle m_ConeAngle;
};

struct CellFlags
{
  enum Enum
  {
    NotVisited = 0,
    Visited = W_BIT(0),
    Visible = Visited | W_BIT(1),
    Invisible = Visited,
  };
};

static WCallbackResult::Enum MarkPointsOnLineVisible(WInt32 x, WInt32 y, void* pPassThrough)
{
  VisibilityLine* VisLine = (VisibilityLine*)pPassThrough;

  // if the reported point is outside the playing field, don't continue
  if (x < 0 || y < 0 || x >= (WInt32)VisLine->m_uiWidth || y >= (WInt32)VisLine->m_uiHeight)
    return WCallbackResult::Stop;

  // compute the point position inside our virtual grid (where the start position is at the center)
  const WUInt32 VisX = x - VisLine->m_iCenterX + VisLine->m_uiRadius;
  const WUInt32 VisY = y - VisLine->m_iCenterY + VisLine->m_uiRadius;

  // if we are outside our virtual grid, stop
  if (VisX >= (WInt32)VisLine->m_uiSize || VisY >= (WInt32)VisLine->m_uiSize)
    return WCallbackResult::Stop;

  // We actually only need two bits for each cell (visited + visible)
  // so we pack the information for four cells into one byte
  const WUInt32 uiCellIndex = VisY * VisLine->m_uiSize + VisX;
  const WUInt32 uiBitfieldByte = uiCellIndex >> 2;                   // division by four
  const WUInt32 uiBitfieldBiteOff = uiBitfieldByte << 2;             // modulo to determine where in the byte this cell is stored
  const WUInt32 uiMaskShift = (uiCellIndex - uiBitfieldBiteOff) * 2; // times two because we use two bits

  WUInt8& CellFlagsRef = (*VisLine->m_pVisible)[uiBitfieldByte];     // for writing into the byte later
  const WUInt8 ThisCellsFlags = (CellFlagsRef >> uiMaskShift) & 3U;  // the decoded flags value for reading (3U == lower two bits)

  // if this point on the line was already visited and determined to be invisible, don't continue
  if (ThisCellsFlags == CellFlags::Invisible)
    return WCallbackResult::Stop;

  // this point has been visited already and the point was determined to be visible, so just continue
  if (ThisCellsFlags == CellFlags::Visible)
    return WCallbackResult::Continue;

  // apparently this cell has not been visited yet, so ask the user callback what to do
  if (VisLine->m_VisCallback(x, y, VisLine->m_pUserPassThrough) == WCallbackResult::Continue)
  {
    // the callback reported this cell as visible, so flag it and continue
    CellFlagsRef |= ((WUInt8)CellFlags::Visible) << uiMaskShift;
    return WCallbackResult::Continue;
  }

  // the callback reported this flag as invisible, flag it and stop the line
  CellFlagsRef |= ((WUInt8)CellFlags::Invisible) << uiMaskShift;
  return WCallbackResult::Stop;
}

static WCallbackResult::Enum MarkPointsInCircleVisible(WInt32 x, WInt32 y, void* pPassThrough)
{
  VisibilityLine* ld = (VisibilityLine*)pPassThrough;

  ez2DGridUtils::ComputePointsOnLineConservative(ld->m_iCenterX, ld->m_iCenterY, x, y, MarkPointsOnLineVisible, pPassThrough, false);

  return WCallbackResult::Continue;
}

void ez2DGridUtils::ComputeVisibleArea(WInt32 iPosX, WInt32 iPosY, WUInt16 uiRadius, WUInt32 uiWidth, WUInt32 uiHeight,
  W_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */, WDynamicArray<WUInt8>* pTempArray /* = nullptr */)
{
  const WUInt32 uiSize = uiRadius * 2 + 1;

  WDynamicArray<WUInt8> VisiblityFlags;

  // if we don't get a temp array, use our own array, with blackjack etc.
  if (pTempArray == nullptr)
    pTempArray = &VisiblityFlags;

  pTempArray->Clear();
  pTempArray->SetCount(WMath::Square(uiSize) / 4); // we store only two bits per cell, so we can pack four values into each byte

  VisibilityLine ld;
  ld.m_uiSize = uiSize;
  ld.m_uiRadius = uiRadius;
  ld.m_pVisible = pTempArray;
  ld.m_iCenterX = iPosX;
  ld.m_iCenterY = iPosY;
  ld.m_VisCallback = callback;
  ld.m_pUserPassThrough = pPassThrough;
  ld.m_uiWidth = uiWidth;
  ld.m_uiHeight = uiHeight;

  // from the center, trace lines to all points on the circle around it
  // each line determines for each cell whether it is visible
  // once an invisible cell is encountered, a line will stop further tracing
  // no cell is ever reported twice to the user callback
  ez2DGridUtils::ComputePointsOnCircle(iPosX, iPosY, uiRadius, MarkPointsInCircleVisible, &ld);
}

static WCallbackResult::Enum MarkPointsInConeVisible(WInt32 x, WInt32 y, void* pPassThrough)
{
  VisibilityLine* ld = (VisibilityLine*)pPassThrough;

  const WVec2 vPos((float)x, (float)y);
  const WVec2 vDirToPos = (vPos - WVec2((float)ld->m_iCenterX, (float)ld->m_iCenterY)).GetNormalized();

  const WAngle angle = WMath::ACos(vDirToPos.Dot(ld->m_vDirection));

  if (angle.GetRadian() < ld->m_ConeAngle.GetRadian())
    ez2DGridUtils::ComputePointsOnLineConservative(ld->m_iCenterX, ld->m_iCenterY, x, y, MarkPointsOnLineVisible, pPassThrough, false);

  return WCallbackResult::Continue;
}

void ez2DGridUtils::ComputeVisibleAreaInCone(WInt32 iPosX, WInt32 iPosY, WUInt16 uiRadius, const WVec2& vDirection, WAngle coneAngle,
  WUInt32 uiWidth, WUInt32 uiHeight, W_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */,
  WDynamicArray<WUInt8>* pTempArray /* = nullptr */)
{
  const WUInt32 uiSize = uiRadius * 2 + 1;

  WDynamicArray<WUInt8> VisiblityFlags;

  // if we don't get a temp array, use our own array, with blackjack etc.
  if (pTempArray == nullptr)
    pTempArray = &VisiblityFlags;

  pTempArray->Clear();
  pTempArray->SetCount(WMath::Square(uiSize) / 4); // we store only two bits per cell, so we can pack four values into each byte


  VisibilityLine ld;
  ld.m_uiSize = uiSize;
  ld.m_uiRadius = uiRadius;
  ld.m_pVisible = pTempArray;
  ld.m_iCenterX = iPosX;
  ld.m_iCenterY = iPosY;
  ld.m_VisCallback = callback;
  ld.m_pUserPassThrough = pPassThrough;
  ld.m_uiWidth = uiWidth;
  ld.m_uiHeight = uiHeight;
  ld.m_vDirection = vDirection;
  ld.m_ConeAngle = coneAngle;

  ez2DGridUtils::ComputePointsOnCircle(iPosX, iPosY, uiRadius, MarkPointsInConeVisible, &ld);
}

struct PointPair
{
  WVec2I32 ptInner;
  WVec2I32 ptOuter;
};

static void SortTraceLines(WDynamicArray<ez2DGridUtils::TraceLinePoint>& out_result, WVec2I32 vStart, const WDynamicArray<PointPair>& pairs)
{
  bool bFound = false;
  for (const auto& pt : pairs)
  {
    if (pt.ptInner == vStart)
    {
      bFound = true;

      const WUInt32 uiStartIdx = out_result.GetCount();
      auto& newPt = out_result.ExpandAndGetRef();
      newPt.m_vCellCoordOffset = pt.ptOuter;

      SortTraceLines(out_result, pt.ptOuter, pairs);

      newPt.m_uiSkipCount = static_cast<WUInt16>(out_result.GetCount() - uiStartIdx - 1);
    }
    else if (bFound)
    {
      break;
    }
  }
}

void ez2DGridUtils::CalculateVisibilityTraceLines(float fRadius, WDynamicArray<TraceLinePoint>& out_result)
{
  out_result.Clear();

  WDynamicArray<PointPair> pairs;
  pairs.Reserve(static_cast<WUInt32>(fRadius * fRadius * 3.2f));

  ez2DGridUtils::RasterizeCircle(0, 0, fRadius, [](WInt32 x, WInt32 y, void* pPassThrough) -> WCallbackResult::Enum
    {
      // don't insert the center point, that is implicit
      if (x != 0 || y != 0)
      {
        WDynamicArray<PointPair>& result = *(WDynamicArray<PointPair>*)pPassThrough;
        result.ExpandAndGetRef().ptOuter = WVec2I32(x, y);
      }

      return WCallbackResult::Continue;
      //
    },
    &pairs);


  for (auto& point : pairs)
  {
    // find the next point on the line towards the center
    ez2DGridUtils::ComputePointsOnLine(point.ptOuter.x, point.ptOuter.y, 0, 0, [&](WInt32 x, WInt32 y, void* /*pPassThrough*/)
      {
        if (point.ptOuter.x != x || point.ptOuter.y != y)
        {
          point.ptInner.x = x;
          point.ptInner.y = y;
          return WCallbackResult::Stop;
        }

        return WCallbackResult::Continue;
        //
      },
      nullptr);
  }

  // now we have all neighbor points towards the center, so sort them by predecessor (inner)
  // this way we group all points that have the same predecessor
  pairs.Sort([](const PointPair& lhs, const PointPair& rhs) -> bool
    {
      if (lhs.ptInner.x == rhs.ptInner.x)
        return lhs.ptInner.y < rhs.ptInner.y;

      return lhs.ptInner.x < rhs.ptInner.x;
      //
    });

  out_result.Reserve(pairs.GetCount() + 1);
  out_result.ExpandAndGetRef().m_vCellCoordOffset.Set(0, 0);
  SortTraceLines(out_result, WVec2I32(0, 0), pairs);
  out_result[0].m_uiSkipCount = 0xFFFF;
}

void ez2DGridUtils::VisitVisibilityTraceLines(const WDynamicArray<TraceLinePoint>& traces, const WVec2I32& vCenter, W_TRACELINE_CHECK check)
{
  for (WUInt32 i = 0; i < traces.GetCount(); ++i)
  {
    const auto& trace = traces[i];

    WVec2I32 pos = vCenter;
    pos.x += trace.m_vCellCoordOffset.x;
    pos.y += trace.m_vCellCoordOffset.y;

    const WUInt32 uiPopBranchesBefore = i;
    const WUInt32 uiPushBranchUntil = i + trace.m_uiSkipCount;

    if (check(pos.x, pos.y, uiPopBranchesBefore, uiPushBranchUntil) == WCallbackResult::Stop)
    {
      i += trace.m_uiSkipCount;
    }
  }
}
