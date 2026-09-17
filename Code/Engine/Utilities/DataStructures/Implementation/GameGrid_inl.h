#pragma once

template <class CellData>
WGameGrid<CellData>::WGameGrid()
{
  m_uiGridSizeX = 0;
  m_uiGridSizeY = 0;

  m_mRotateToWorldspace.SetIdentity();
  m_mRotateToGridspace.SetIdentity();

  m_vWorldSpaceOrigin.SetZero();
  m_vLocalSpaceCellSize.Set(1.0f);
  m_vInverseLocalSpaceCellSize.Set(1.0f);
}

template <class CellData>
void WGameGrid<CellData>::CreateGrid(WUInt16 uiSizeX, WUInt16 uiSizeY)
{
  m_Cells.Clear();

  m_uiGridSizeX = uiSizeX;
  m_uiGridSizeY = uiSizeY;

  m_Cells.SetCount(m_uiGridSizeX * m_uiGridSizeY);
}

template <class CellData>
void WGameGrid<CellData>::SetWorldSpaceDimensions(const WVec3& vLowerLeftCorner, const WVec3& vCellSize, Orientation ori)
{
  WMat3 mRot;

  switch (ori)
  {
    case InPlaneXY:
      mRot.SetIdentity();
      break;
    case InPlaneXZ:
      mRot = WMat3::MakeAxisRotation(WVec3(1, 0, 0), WAngle::MakeFromDegree(90.0f));
      break;
    case InPlaneXminusZ:
      mRot = WMat3::MakeAxisRotation(WVec3(1, 0, 0), WAngle::MakeFromDegree(-90.0f));
      break;
  }

  SetWorldSpaceDimensions(vLowerLeftCorner, vCellSize, mRot);
}

template <class CellData>
void WGameGrid<CellData>::SetWorldSpaceDimensions(const WVec3& vLowerLeftCorner, const WVec3& vCellSize, const WMat3& mRotation)
{
  m_vWorldSpaceOrigin = vLowerLeftCorner;
  m_vLocalSpaceCellSize = vCellSize;
  m_vInverseLocalSpaceCellSize = WVec3(1.0f).CompDiv(vCellSize);

  m_mRotateToWorldspace = mRotation;
  m_mRotateToGridspace = mRotation.GetInverse();
}

template <class CellData>
WVec2I32 WGameGrid<CellData>::GetCellAtWorldPosition(const WVec3& vWorldSpacePos) const
{
  const WVec3 vCell = (m_mRotateToGridspace * ((vWorldSpacePos - m_vWorldSpaceOrigin)).CompMul(m_vInverseLocalSpaceCellSize));

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  return WVec2I32((WInt32)WMath::Floor(vCell.x), (WInt32)WMath::Floor(vCell.y));
}

template <class CellData>
WVec3 WGameGrid<CellData>::GetCellWorldSpaceOrigin(const WVec2I32& vCoord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceOrigin(vCoord);
}

template <class CellData>
WVec3 WGameGrid<CellData>::GetCellLocalSpaceOrigin(const WVec2I32& vCoord) const
{
  return m_vLocalSpaceCellSize.CompMul(WVec3((float)vCoord.x, (float)vCoord.y, 0.0f));
}

template <class CellData>
WVec3 WGameGrid<CellData>::GetCellWorldSpaceCenter(const WVec2I32& vCoord, float fHeight) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceCenter(vCoord, fHeight);
}

template <class CellData>
WVec3 WGameGrid<CellData>::GetCellLocalSpaceCenter(const WVec2I32& vCoord, float fHeight) const
{
  return m_vLocalSpaceCellSize.CompMul(WVec3((float)vCoord.x + 0.5f, (float)vCoord.y + 0.5f, fHeight));
}

template <class CellData>
bool WGameGrid<CellData>::IsValidCellCoordinate(const WVec2I32& vCoord) const
{
  return (vCoord.x >= 0 && vCoord.x < m_uiGridSizeX && vCoord.y >= 0 && vCoord.y < m_uiGridSizeY);
}

template <class CellData>
bool WGameGrid<CellData>::PickCell(const WVec3& vRayStartPos, const WVec3& vRayDirNorm, WVec2I32* out_pCellCoord, WVec3* out_pIntersection) const
{
  WPlane p;
  p = WPlane::MakeFromNormalAndPoint(m_mRotateToWorldspace * WVec3(0, 0, -1), m_vWorldSpaceOrigin);

  WVec3 vPos;

  if (!p.GetRayIntersectionBiDirectional(vRayStartPos, vRayDirNorm, nullptr, &vPos))
    return false;

  if (out_pIntersection)
    *out_pIntersection = vPos;

  if (out_pCellCoord)
    *out_pCellCoord = GetCellAtWorldPosition(vPos);

  return true;
}

template <class CellData>
WBoundingBox WGameGrid<CellData>::GetWorldBoundingBox() const
{
  WVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  vGridBox = m_mRotateToWorldspace * m_vLocalSpaceCellSize.CompMul(vGridBox);

  return WBoundingBox(m_vWorldSpaceOrigin, m_vWorldSpaceOrigin + vGridBox);
}

template <class CellData>
bool WGameGrid<CellData>::GetRayIntersection(const WVec3& vRayStartWorldSpace, const WVec3& vRayDirNormalizedWorldSpace, float fMaxLength,
  float& out_fIntersection, WVec2I32& out_vCellCoord) const
{
  const WVec3 vRayStart = m_mRotateToGridspace * (vRayStartWorldSpace - m_vWorldSpaceOrigin);
  const WVec3 vRayDir = m_mRotateToGridspace * vRayDirNormalizedWorldSpace;

  WVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  const WBoundingBox localBox(WVec3(0.0f), m_vLocalSpaceCellSize.CompMul(vGridBox));

  if (localBox.Contains(vRayStart))
  {
    // if the ray is already inside the box, we know that a cell is hit
    out_fIntersection = 0.0f;
  }
  else
  {
    if (!localBox.GetRayIntersection(vRayStart, vRayDir, &out_fIntersection, nullptr))
      return false;

    if (out_fIntersection > fMaxLength)
      return false;
  }

  const WVec3 vEnterPos = vRayStart + vRayDir * out_fIntersection;

  const WVec3 vCell = vEnterPos.CompMul(m_vInverseLocalSpaceCellSize);

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  out_vCellCoord = WVec2I32((WInt32)WMath::Floor(vCell.x), (WInt32)WMath::Floor(vCell.y));
  out_vCellCoord.x = WMath::Clamp(out_vCellCoord.x, 0, m_uiGridSizeX - 1);
  out_vCellCoord.y = WMath::Clamp(out_vCellCoord.y, 0, m_uiGridSizeY - 1);

  return true;
}

template <class CellData>
bool WGameGrid<CellData>::GetRayIntersectionExpandedBBox(const WVec3& vRayStartWorldSpace, const WVec3& vRayDirNormalizedWorldSpace,
  float fMaxLength, float& out_fIntersection, const WVec3& vExpandBBoxByThis) const
{
  const WVec3 vRayStart = m_mRotateToGridspace * (vRayStartWorldSpace - m_vWorldSpaceOrigin);
  const WVec3 vRayDir = m_mRotateToGridspace * vRayDirNormalizedWorldSpace;

  WVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  WBoundingBox localBox(WVec3(0.0f), m_vLocalSpaceCellSize.CompMul(vGridBox));
  localBox.Grow(vExpandBBoxByThis);

  if (localBox.Contains(vRayStart))
  {
    // if the ray is already inside the box, we know that a cell is hit
    out_fIntersection = 0.0f;
  }
  else
  {
    if (!localBox.GetRayIntersection(vRayStart, vRayDir, &out_fIntersection, nullptr))
      return false;

    if (out_fIntersection > fMaxLength)
      return false;
  }

  return true;
}

template <class CellData>
void WGameGrid<CellData>::ComputeWorldSpaceCorners(WVec3* pCorners) const
{
  pCorners[0] = m_vWorldSpaceOrigin;
  pCorners[1] = m_vWorldSpaceOrigin + m_mRotateToWorldspace * WVec3(m_uiGridSizeX * m_vLocalSpaceCellSize.x, 0, 0);
  pCorners[2] = m_vWorldSpaceOrigin + m_mRotateToWorldspace * WVec3(0, m_uiGridSizeY * m_vLocalSpaceCellSize.y, 0);
  pCorners[3] = m_vWorldSpaceOrigin + m_mRotateToWorldspace * WVec3(m_uiGridSizeX * m_vLocalSpaceCellSize.x, m_uiGridSizeY * m_vLocalSpaceCellSize.y, 0);
}


template <class CellData>
WResult WGameGrid<CellData>::Serialize(WStreamWriter& ref_stream) const
{
  auto& stream = ref_stream;

  stream.WriteVersion(1);

  stream << m_uiGridSizeX;
  stream << m_uiGridSizeY;
  stream << m_mRotateToWorldspace;
  stream << m_mRotateToGridspace;
  stream << m_vWorldSpaceOrigin;
  stream << m_vLocalSpaceCellSize;
  stream << m_vInverseLocalSpaceCellSize;
  W_SUCCEED_OR_RETURN(stream.WriteArray(m_Cells));

  return W_SUCCESS;
}

template <class CellData>
WResult WGameGrid<CellData>::Deserialize(WStreamReader& ref_stream)
{
  auto& stream = ref_stream;

  const WTypeVersion version = stream.ReadVersion(1);
  W_IGNORE_UNUSED(version);

  stream >> m_uiGridSizeX;
  stream >> m_uiGridSizeY;
  stream >> m_mRotateToWorldspace;
  stream >> m_mRotateToGridspace;
  stream >> m_vWorldSpaceOrigin;
  stream >> m_vLocalSpaceCellSize;
  stream >> m_vInverseLocalSpaceCellSize;
  W_SUCCEED_OR_RETURN(stream.ReadArray(m_Cells));

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <class CellData, class EdgeData>
void WGameGridWithEdges<CellData, EdgeData>::ConvertEdgeIndexToCellCoords(WUInt32 uiEdgeIndex, WVec2I32& out_vCell1, WVec2I32& out_vCell2) const
{
  const WUInt32 uiOffsetY = (this->m_uiGridSizeX + 1) * this->m_uiGridSizeY;


  if (uiEdgeIndex < uiOffsetY)
  {
    // this index is the NegX edge of out_vCell2, so the cell before it is on the other side
    out_vCell2.y = uiEdgeIndex / (this->m_uiGridSizeX + 1);
    out_vCell2.x = uiEdgeIndex - out_vCell2.y * (this->m_uiGridSizeX + 1);

    out_vCell1 = out_vCell2;
    out_vCell1.x -= 1;
  }
  else
  {
    uiEdgeIndex -= uiOffsetY;

    // this index is the NegY edge of out_vCell2
    out_vCell2.x = uiEdgeIndex / (this->m_uiGridSizeY + 1);
    out_vCell2.y = uiEdgeIndex - out_vCell2.x * (this->m_uiGridSizeY + 1);

    out_vCell1 = out_vCell2;
    out_vCell1.y -= 1;
  }
}

template <class CellData, class EdgeData>
WUInt32 WGameGridWithEdges<CellData, EdgeData>::ConvertCellCoordinateToEdgeIndex(const WVec2I32& vCoord, WGameGridCellEdge edge) const
{
  const WUInt32 uiOffsetY = (this->m_uiGridSizeX + 1) * this->m_uiGridSizeY;

  switch (edge)
  {
    case WGameGridCellEdge::NegX:
      return vCoord.y * (this->m_uiGridSizeX + 1) + vCoord.x;

    case WGameGridCellEdge::PosX:
      return vCoord.y * (this->m_uiGridSizeX + 1) + vCoord.x + 1;

    case WGameGridCellEdge::NegY:
      return uiOffsetY + vCoord.x * (this->m_uiGridSizeY + 1) + vCoord.y;

    case WGameGridCellEdge::PosY:
      return uiOffsetY + vCoord.x * (this->m_uiGridSizeY + 1) + vCoord.y + 1;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

template <class CellData, class EdgeData>
void WGameGridWithEdges<CellData, EdgeData>::CreateGrid(WUInt16 uiSizeX, WUInt16 uiSizeY)
{
  WGameGrid<CellData>::CreateGrid(uiSizeX, uiSizeY);

  m_Edges.Clear();
  m_Edges.SetCount(uiSizeX * uiSizeY * 2 + uiSizeX + uiSizeY);
}

template <class CellData, class EdgeData>
WResult WGameGridWithEdges<CellData, EdgeData>::Serialize(WStreamWriter& ref_stream) const
{
  auto& stream = ref_stream;

  WGameGrid<CellData>::Serialize(stream);

  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(stream.WriteArray(m_Edges));

  return W_SUCCESS;
}

template <class CellData, class EdgeData>
WResult WGameGridWithEdges<CellData, EdgeData>::Deserialize(WStreamReader& ref_stream)
{
  auto& stream = ref_stream;

  WGameGrid<CellData>::Deserialize(stream);

  const WTypeVersion version = stream.ReadVersion(1);
  W_IGNORE_UNUSED(version);

  W_SUCCEED_OR_RETURN(stream.ReadArray(m_Edges));

  return W_SUCCESS;
}


template <class CellData, class EdgeData>
WVec3 WGameGridWithEdges<CellData, EdgeData>::GetEdgeWorldSpaceCenter(WUInt32 uiEdgeIndex, float fHeight) const
{
  return this->m_vWorldSpaceOrigin + this->m_mRotateToWorldspace * GetEdgeLocalSpaceCenter(uiEdgeIndex, fHeight);
}

template <class CellData, class EdgeData>
WVec3 WGameGridWithEdges<CellData, EdgeData>::GetEdgeLocalSpaceCenter(WUInt32 uiEdgeIndex, float fHeight) const
{
  const WUInt32 uiOffsetY = (this->m_uiGridSizeX + 1) * this->m_uiGridSizeY;

  if (uiEdgeIndex < uiOffsetY)
  {
    // Horizontal edge (parallel to X axis)
    const WUInt32 y = uiEdgeIndex / (this->m_uiGridSizeX + 1);
    const WUInt32 x = uiEdgeIndex - y * (this->m_uiGridSizeX + 1);

    return this->m_vLocalSpaceCellSize.CompMul(WVec3((float)x, (float)y + 0.5f, fHeight));
  }
  else
  {
    // Vertical edge (parallel to Y axis)
    const WUInt32 uiAdjustedIndex = uiEdgeIndex - uiOffsetY;
    const WUInt32 x = uiAdjustedIndex / (this->m_uiGridSizeY + 1);
    const WUInt32 y = uiAdjustedIndex - x * (this->m_uiGridSizeY + 1);

    return this->m_vLocalSpaceCellSize.CompMul(WVec3((float)x + 0.5f, (float)y, fHeight));
  }
}

template <class CellData, class EdgeData>
bool WGameGridWithEdges<CellData, EdgeData>::IsValidEdgeIndex(WUInt32 uiEdgeIndex) const
{
  return uiEdgeIndex < m_Edges.GetCount();
}

template <class CellData, class EdgeData>
bool WGameGridWithEdges<CellData, EdgeData>::IsEdgeHorizontal(WUInt32 uiEdgeIndex) const
{
  const WUInt32 uiOffsetY = (this->m_uiGridSizeX + 1) * this->m_uiGridSizeY;
  return uiEdgeIndex < uiOffsetY;
}

template <class CellData, class EdgeData>
bool WGameGridWithEdges<CellData, EdgeData>::GetRayIntersectionWithEdge(const WVec3& vRayStartWorldSpace,
  const WVec3& vRayDirNormalizedWorldSpace, float fMaxLength, float& out_fIntersection, WVec2I32& out_vCellCoord, WGameGridCellEdge& out_edge) const
{
  // First find which cell is hit
  if (!this->GetRayIntersection(vRayStartWorldSpace, vRayDirNormalizedWorldSpace, fMaxLength, out_fIntersection, out_vCellCoord))
    return false;

  // Calculate the intersection point in world space
  const WVec3 vIntersectionWorld = vRayStartWorldSpace + vRayDirNormalizedWorldSpace * out_fIntersection;

  // Transform intersection point to local space
  const WVec3 vIntersectionLocal = this->m_mRotateToGridspace * (vIntersectionWorld - this->m_vWorldSpaceOrigin);

  // Get the cell center in local space
  const WVec3 vCellCenter = this->GetCellLocalSpaceCenter(out_vCellCoord);

  // Calculate the offset from cell center to intersection point
  const WVec3 vOffset = vIntersectionLocal - vCellCenter;

  // Determine which edge is closest based on the offset direction
  // Compare absolute values to find the dominant axis
  const float fAbsX = WMath::Abs(vOffset.x);
  const float fAbsY = WMath::Abs(vOffset.y);

  if (fAbsX > fAbsY)
  {
    // Closer to a vertical edge (NegX or PosX)
    out_edge = (vOffset.x < 0.0f) ? WGameGridCellEdge::NegX : WGameGridCellEdge::PosX;
  }
  else
  {
    // Closer to a horizontal edge (NegY or PosY)
    out_edge = (vOffset.y < 0.0f) ? WGameGridCellEdge::NegY : WGameGridCellEdge::PosY;
  }

  return true;
}
