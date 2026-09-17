#include <Utilities/UtilitiesPCH.h>

#include <Utilities/DataStructures/DynamicOctree.h>

const float WDynamicOctree::s_fLooseOctreeFactor = 1.1f;

WDynamicOctree::WDynamicOctree() = default;

void WDynamicOctree::CreateTree(const WVec3& vCenter, const WVec3& vHalfExtents, float fMinNodeSize)
{
  m_uiMultiMapCounter = 1;

  m_NodeMap.Clear();

  // the real bounding box might be long and thing -> bad node-size
  // but still it can be used to reject inserting objects that are entirely outside the world
  m_fRealMinX = vCenter.x - vHalfExtents.x;
  m_fRealMaxX = vCenter.x + vHalfExtents.x;
  m_fRealMinY = vCenter.y - vHalfExtents.y;
  m_fRealMaxY = vCenter.y + vHalfExtents.y;
  m_fRealMinZ = vCenter.z - vHalfExtents.z;
  m_fRealMaxZ = vCenter.z + vHalfExtents.z;

  // the bounding box should be square, so use the maximum of the x, y and z extents
  float fMax = WMath::Max(vHalfExtents.x, WMath::Max(vHalfExtents.y, vHalfExtents.z));

  m_BBox = WBoundingBox::MakeFromCenterAndHalfExtents(vCenter, WVec3(fMax));

  float fLength = fMax * 2.0f;

  m_uiMaxTreeDepth = 0;
  while (fLength > fMinNodeSize)
  {
    ++m_uiMaxTreeDepth;
    fLength = (fLength / 2.0f) * s_fLooseOctreeFactor;
  }

  m_uiAddIDTopLevel = 0;
  for (WUInt32 i = 0; i < m_uiMaxTreeDepth; ++i)
    m_uiAddIDTopLevel += WMath::Pow(8, i);
}

/// The object lies at vCenter and has vHalfExtents as its bounding box.
/// If bOnlyIfInside is false, the object is ALWAYS inserted, even if it is outside the tree.
/// \note In such a case it is inserted at the root-node and thus ALWAYS returned in range/view-frustum queries.
///
/// If bOnlyIfInside is true, the object is discarded, if it is not inside the actual bounding box of the tree.
WResult WDynamicOctree::InsertObject(const WVec3& vCenter, const WVec3& vHalfExtents, WInt32 iObjectType, WInt32 iObjectInstance,
  WDynamicTreeObject* out_pObject, bool bOnlyIfInside)
{
  if (out_pObject)
    *out_pObject = WDynamicTreeObject();

  if (bOnlyIfInside)
  {
    if (vCenter.x + vHalfExtents.x < m_fRealMinX)
      return W_FAILURE;

    if (vCenter.x - vHalfExtents.x > m_fRealMaxX)
      return W_FAILURE;

    if (vCenter.y + vHalfExtents.y < m_fRealMinY)
      return W_FAILURE;

    if (vCenter.y - vHalfExtents.y > m_fRealMaxY)
      return W_FAILURE;

    if (vCenter.z + vHalfExtents.z < m_fRealMinZ)
      return W_FAILURE;

    if (vCenter.z - vHalfExtents.z > m_fRealMaxZ)
      return W_FAILURE;
  }

  WDynamicTree::WObjectData oData;
  oData.m_iObjectType = iObjectType;
  oData.m_iObjectInstance = iObjectInstance;

  // insert the object into the best child
  if (!InsertObject(vCenter, vHalfExtents, oData, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.y, m_BBox.m_vMax.y, m_BBox.m_vMin.z,
        m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, WMath::Pow(8, m_uiMaxTreeDepth - 1), out_pObject))
  {
    if (!bOnlyIfInside)
    {
      WDynamicTree::WMultiMapKey mmk;
      mmk.m_uiKey = 0;
      mmk.m_uiCounter = m_uiMultiMapCounter++;

      auto key = m_NodeMap.Insert(mmk, oData);

      if (out_pObject)
        *out_pObject = key;

      return W_SUCCESS;
    }

    return W_FAILURE;
  }

  return W_SUCCESS;
}

bool WDynamicOctree::InsertObject(const WVec3& vCenter, const WVec3& vHalfExtents, const WDynamicTree::WObjectData& Obj, float minx, float maxx,
  float miny, float maxy, float minz, float maxz, WUInt32 uiNodeID, WUInt32 uiAddID, WUInt32 uiSubAddID, WDynamicTreeObject* out_Object)
{
  if (vCenter.x - vHalfExtents.x < minx)
    return false;
  if (vCenter.x + vHalfExtents.x > maxx)
    return false;
  if (vCenter.y - vHalfExtents.y < miny)
    return false;
  if (vCenter.y + vHalfExtents.y > maxy)
    return false;
  if (vCenter.z - vHalfExtents.z < minz)
    return false;
  if (vCenter.z + vHalfExtents.z > maxz)
    return false;

  if (uiAddID > 0)
  {
    const float lx = ((maxx - minx) * 0.5f) * s_fLooseOctreeFactor;
    const float ly = ((maxy - miny) * 0.5f) * s_fLooseOctreeFactor;
    const float lz = ((maxz - minz) * 0.5f) * s_fLooseOctreeFactor;

    const WUInt32 uiNodeIDBase = uiNodeID + 1;
    const WUInt32 uiAddIDChild = uiAddID - uiSubAddID;
    const WUInt32 uiSubAddIDChild = uiSubAddID >> 3;

    if (InsertObject(vCenter, vHalfExtents, Obj, minx, minx + lx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 0, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, minx, minx + lx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, minx, minx + lx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 2, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, minx, minx + lx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, maxx - lx, maxx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 4, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, maxx - lx, maxx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 5, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, maxx - lx, maxx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 6, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, maxx - lx, maxx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 7, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
  }

  WDynamicTree::WMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;
  mmk.m_uiCounter = m_uiMultiMapCounter++;

  auto key = m_NodeMap.Insert(mmk, Obj);

  if (out_Object)
    *out_Object = key;

  return true;
}

void WDynamicOctree::FindObjectsInRange(const WVec3& vPoint, W_VISIBLE_OBJ_CALLBACK callback, void* pPassThrough) const
{
  if (m_NodeMap.IsEmpty())
    return;

  if (!m_BBox.Contains(vPoint))
    return;

  FindObjectsInRange(vPoint, callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.y, m_BBox.m_vMax.y, m_BBox.m_vMin.z,
    m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, WMath::Pow(8, m_uiMaxTreeDepth - 1), 0xFFFFFFFF);
}

void WDynamicOctree::FindVisibleObjects(const WFrustum& viewfrustum, W_VISIBLE_OBJ_CALLBACK callback, void* pPassThrough) const
{
  W_ASSERT_DEV(m_uiMaxTreeDepth > 0, "WDynamicOctree::FindVisibleObjects: You have to first create the tree.");

  if (m_NodeMap.IsEmpty())
    return;

  FindVisibleObjects(viewfrustum, callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.y, m_BBox.m_vMax.y, m_BBox.m_vMin.z,
    m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, WMath::Pow(4, m_uiMaxTreeDepth - 1), 0xFFFFFFFF);
}

void WDynamicOctree::FindVisibleObjects(const WFrustum& Viewfrustum, W_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx,
  float miny, float maxy, float minz, float maxz, WUInt32 uiNodeID, WUInt32 uiAddID, WUInt32 uiSubAddID, WUInt32 uiNextNodeID) const
{
  WVec3 v[8];
  v[0].Set(minx, miny, minz);
  v[1].Set(minx, miny, maxz);
  v[2].Set(minx, maxy, minz);
  v[3].Set(minx, maxy, maxz);
  v[4].Set(maxx, miny, minz);
  v[5].Set(maxx, miny, maxz);
  v[6].Set(maxx, maxy, minz);
  v[7].Set(maxx, maxy, maxz);

  WVolumePosition::Enum pos = Viewfrustum.GetObjectPosition(&v[0], 8);

  if (pos == WVolumePosition::Outside)
    return;

  WDynamicTree::WMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  WDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  if ((!it1.IsValid()) || (it1.Key().m_uiKey >= uiNextNodeID))
    return;

  if (pos == WVolumePosition::Inside)
  {
    mmk.m_uiKey = uiNextNodeID;

    while (it1.IsValid())
    {
      // first increase the iterator, the user could erase it in the callback
      WDynamicTreeObjectConst temp = it1;
      ++it1;

      Callback(pPassThrough, temp);
    }

    return;
  }
  else if (pos == WVolumePosition::Intersecting)
  {
    mmk.m_uiKey = uiNodeID + 1;

    while (it1.IsValid())
    {
      // first increase the iterator, the user could erase it in the callback
      WDynamicTreeObjectConst temp = it1;
      ++it1;

      Callback(pPassThrough, temp);
    }

    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_fLooseOctreeFactor;
      const float ly = ((maxy - miny) * 0.5f) * s_fLooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_fLooseOctreeFactor;

      const WUInt32 uiNodeIDBase = uiNodeID + 1;
      const WUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const WUInt32 uiSubAddIDChild = uiSubAddID >> 3;

      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 0,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 1);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 2);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 2,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 3);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 4);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 4,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 5);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 5,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 6);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 6,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 7);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 7,
        uiAddIDChild, uiSubAddIDChild, uiNextNodeID);
    }
  }
}

void WDynamicOctree::RemoveObject(WDynamicTreeObject obj)
{
  m_NodeMap.Remove(obj);
}

void WDynamicOctree::RemoveObject(WInt32 iObjectType, WInt32 iObjectInstance)
{
  for (WDynamicTreeObject it = m_NodeMap.GetIterator(); it.IsValid(); ++it)
  {
    if ((it.Value().m_iObjectInstance == iObjectInstance) && (it.Value().m_iObjectType == iObjectType))
    {
      m_NodeMap.Remove(it);
      return;
    }
  }
}

void WDynamicOctree::RemoveObjectsOfType(WInt32 iObjectType)
{
  for (WDynamicTreeObject it = m_NodeMap.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_iObjectType == iObjectType)
    {
      WDynamicTreeObject itold = it;
      ++it;

      m_NodeMap.Remove(itold);
    }
    else
      ++it;
  }
}



bool WDynamicOctree::FindObjectsInRange(const WVec3& vPoint, W_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx,
  float miny, float maxy, float minz, float maxz, WUInt32 uiNodeID, WUInt32 uiAddID, WUInt32 uiSubAddID, WUInt32 uiNextNodeID) const
{
  if (vPoint.x < minx)
    return true;
  if (vPoint.x > maxx)
    return true;
  if (vPoint.y < miny)
    return true;
  if (vPoint.y > maxy)
    return true;
  if (vPoint.z < minz)
    return true;
  if (vPoint.z > maxz)
    return true;

  WDynamicTree::WMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  WDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  if ((!it1.IsValid()) || (it1.Key().m_uiKey >= uiNextNodeID))
    return true;

  {
    {
      WDynamicTree::WMultiMapKey mmk2;
      mmk2.m_uiKey = uiNodeID + 1;

      const WDynamicTreeObjectConst itlast = m_NodeMap.LowerBound(mmk2);

      while (it1 != itlast)
      {
        // first increase the iterator, the user could erase it in the callback
        WDynamicTreeObjectConst temp = it1;
        ++it1;

        if (!Callback(pPassThrough, temp))
          return false;
      }
    }

    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_fLooseOctreeFactor;
      const float ly = ((maxy - miny) * 0.5f) * s_fLooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_fLooseOctreeFactor;

      const WUInt32 uiNodeIDBase = uiNodeID + 1;
      const WUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const WUInt32 uiSubAddIDChild = uiSubAddID >> 3;

      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 0,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 1))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 2))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 2,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 3))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 4))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 4,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 5))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 5,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 6))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 6,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 7))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 7,
            uiAddIDChild, uiSubAddIDChild, uiNextNodeID))
        return false;
    }
  }

  return true;
}

void WDynamicOctree::FindObjectsInRange(const WVec3& vPoint, float fRadius, W_VISIBLE_OBJ_CALLBACK callback, void* pPassThrough) const
{
  W_ASSERT_DEV(m_uiMaxTreeDepth > 0, "WDynamicOctree::FindObjectsInRange: You have to first create the tree.");

  if (m_NodeMap.IsEmpty())
    return;

  FindObjectsInRange(vPoint, fRadius, callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.y, m_BBox.m_vMax.y, m_BBox.m_vMin.z,
    m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, WMath::Pow(8, m_uiMaxTreeDepth - 1), 0xFFFFFFFF);
}

bool WDynamicOctree::FindObjectsInRange(const WVec3& vPoint, float fRadius, W_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx,
  float maxx, float miny, float maxy, float minz, float maxz, WUInt32 uiNodeID, WUInt32 uiAddID, WUInt32 uiSubAddID, WUInt32 uiNextNodeID) const
{
  if (vPoint.x + fRadius < minx)
    return true;
  if (vPoint.x - fRadius > maxx)
    return true;
  if (vPoint.y + fRadius < miny)
    return true;
  if (vPoint.y - fRadius > maxy)
    return true;
  if (vPoint.z + fRadius < minz)
    return true;
  if (vPoint.z - fRadius > maxz)
    return true;

  WDynamicTree::WMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  WDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  // if the whole sub-tree doesn't contain any data, no need to check further
  if ((!it1.IsValid()) || (it1.Key().m_uiKey >= uiNextNodeID))
    return true;

  {

    // return all objects stored at this node
    {
      while (it1.IsValid() && (it1.Key().m_uiKey == uiNodeID))
      {
        // first increase the iterator, the user could erase it in the callback
        WDynamicTreeObjectConst temp = it1;
        ++it1;

        if (!Callback(pPassThrough, temp))
          return false;
      }
    }

    // if the node has children
    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_fLooseOctreeFactor;
      const float ly = ((maxy - miny) * 0.5f) * s_fLooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_fLooseOctreeFactor;

      const WUInt32 uiNodeIDBase = uiNodeID + 1;
      const WUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const WUInt32 uiSubAddIDChild = uiSubAddID >> 3;

      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 0,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 1))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 2))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 2,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 3))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 4))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 4,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 5))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 5,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 6))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 6,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 7))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 7,
            uiAddIDChild, uiSubAddIDChild, uiNextNodeID))
        return false;
    }
  }

  return true;
}
