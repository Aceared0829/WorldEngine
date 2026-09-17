#include <Utilities/UtilitiesPCH.h>

#include <Utilities/DataStructures/DynamicQuadtree.h>

const float WDynamicQuadtree::s_fLooseOctreeFactor = 1.1f;

WDynamicQuadtree::WDynamicQuadtree() = default;

void WDynamicQuadtree::CreateTree(const WVec3& vCenter, const WVec3& vHalfExtents, float fMinNodeSize)
{
  m_uiMultiMapCounter = 1;

  m_NodeMap.Clear();

  // the real bounding box might be long and thing -> bad node-size
  // but still it can be used to reject inserting objects that are entirely outside the world
  m_fRealMinX = vCenter.x - vHalfExtents.x;
  m_fRealMaxX = vCenter.x + vHalfExtents.x;
  m_fRealMinZ = vCenter.z - vHalfExtents.z;
  m_fRealMaxZ = vCenter.z + vHalfExtents.z;

  // the bounding box should be square, so use the maximum of the x and z extents
  float fMax = WMath::Max(vHalfExtents.x, vHalfExtents.z);

  m_BBox = WBoundingBox::MakeInvalid();

  m_BBox.m_vMin.x = vCenter.x - fMax;
  m_BBox.m_vMax.x = vCenter.x + fMax;
  m_BBox.m_vMin.z = vCenter.z - fMax;
  m_BBox.m_vMax.z = vCenter.z + fMax;


  // compute the maximum tree-depth
  float fLength = fMax * 2.0f;

  m_uiMaxTreeDepth = 0;
  while (fLength > fMinNodeSize)
  {
    ++m_uiMaxTreeDepth;
    fLength = (fLength / 2.0f) * s_fLooseOctreeFactor;
  }

  // at every tree depth there are pow(4, depth) additional nodes, each node needs a unique ID
  // therefore compute the maximum ID that is used, for later reference
  m_uiAddIDTopLevel = 0;
  for (WUInt32 i = 0; i < m_uiMaxTreeDepth; ++i)
    m_uiAddIDTopLevel += WMath::Pow(4, i);
}

/// The object lies at vCenter and has vHalfExtents as its bounding box.
/// If bOnlyIfInside is false, the object is ALWAYS inserted, even if it is outside the tree.
/// \note In such a case it is inserted at the root-node and thus ALWAYS returned in range/view-frustum queries.
///
/// If bOnlyIfInside is true, the object is discarded, if it is not inside the actual bounding box of the tree.
///
/// The min and max Y value of the tree's bounding box is updated, if the object lies above/below previously inserted objects.
WResult WDynamicQuadtree::InsertObject(const WVec3& vCenter, const WVec3& vHalfExtents, WInt32 iObjectType, WInt32 iObjectInstance,
  WDynamicTreeObject* out_pObject, bool bOnlyIfInside)
{
  W_ASSERT_DEV(m_uiMaxTreeDepth > 0, "WDynamicQuadtree::InsertObject: You have to first create the tree.");

  if (out_pObject)
    *out_pObject = WDynamicTreeObject();

  if (bOnlyIfInside)
  {
    if (vCenter.x + vHalfExtents.x < m_fRealMinX)
      return W_FAILURE;

    if (vCenter.x - vHalfExtents.x > m_fRealMaxX)
      return W_FAILURE;

    if (vCenter.z + vHalfExtents.z < m_fRealMinZ)
      return W_FAILURE;

    if (vCenter.z - vHalfExtents.z > m_fRealMaxZ)
      return W_FAILURE;
  }

  // update the bounding boxes min/max Y values
  m_BBox.m_vMin.y = WMath::Min(m_BBox.m_vMin.y, vCenter.y - vHalfExtents.y);
  m_BBox.m_vMax.y = WMath::Max(m_BBox.m_vMax.y, vCenter.y + vHalfExtents.y);


  WDynamicTree::WObjectData oData;
  oData.m_iObjectType = iObjectType;
  oData.m_iObjectInstance = iObjectInstance;

  // insert the object into the best child
  if (!InsertObject(vCenter, vHalfExtents, oData, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.z, m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel,
        WMath::Pow(4, m_uiMaxTreeDepth - 1), out_pObject))
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

void WDynamicQuadtree::RemoveObject(WDynamicTreeObject obj)
{
  m_NodeMap.Remove(obj);
}

void WDynamicQuadtree::RemoveObject(WInt32 iObjectType, WInt32 iObjectInstance)
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

void WDynamicQuadtree::RemoveObjectsOfType(WInt32 iObjectType)
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

bool WDynamicQuadtree::InsertObject(const WVec3& vCenter, const WVec3& vHalfExtents, const WDynamicTree::WObjectData& Obj, float minx,
  float maxx, float minz, float maxz, WUInt32 uiNodeID, WUInt32 uiAddID, WUInt32 uiSubAddID, WDynamicTreeObject* out_Object)
{
  // check if object is COMPLETELY inside the node
  // if it is not entirely enclosed by the bounding box, insert it at the parent instead
  if (vCenter.x - vHalfExtents.x < minx)
    return false;
  if (vCenter.x + vHalfExtents.x > maxx)
    return false;
  if (vCenter.z - vHalfExtents.z < minz)
    return false;
  if (vCenter.z + vHalfExtents.z > maxz)
    return false;

  // if there are any child-nodes, try inserting there
  if (uiAddID > 0)
  {
    const float lx = ((maxx - minx) * 0.5f) * s_fLooseOctreeFactor;
    const float lz = ((maxz - minz) * 0.5f) * s_fLooseOctreeFactor;

    // to compute the ID of child node 'n', the number of children in child node 'n-1' has to be considered
    // uiAddID is the number of IDs that need to be skipped to get from the ID of child node 'n' to the ID of child 'n+1'

    // every time one goes down one step in the tree the number of child-nodes in the sub-tree is divided by 4
    // so the number of IDs to skip to get from child 'n' to 'n+1' is reduced to 1/4

    const WUInt32 uiNodeIDBase = uiNodeID + 1;
    const WUInt32 uiAddIDChild = uiAddID - uiSubAddID;
    const WUInt32 uiSubAddIDChild = uiSubAddID / 4;

    if (InsertObject(
          vCenter, vHalfExtents, Obj, minx, minx + lx, minz, minz + lz, uiNodeIDBase + uiAddID * 0, uiAddIDChild, uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(
          vCenter, vHalfExtents, Obj, minx, minx + lx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1, uiAddIDChild, uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(
          vCenter, vHalfExtents, Obj, maxx - lx, maxx, minz, minz + lz, uiNodeIDBase + uiAddID * 2, uiAddIDChild, uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(
          vCenter, vHalfExtents, Obj, maxx - lx, maxx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3, uiAddIDChild, uiSubAddIDChild, out_Object))
      return true;
  }

  // object is inside this node, but not (completely / exclusively) inside any child nodes -> insert here
  WDynamicTree::WMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;
  mmk.m_uiCounter = m_uiMultiMapCounter++;

  auto key = m_NodeMap.Insert(mmk, Obj);

  if (out_Object)
    *out_Object = key;

  return true;
}

void WDynamicQuadtree::FindVisibleObjects(const WFrustum& viewfrustum, W_VISIBLE_OBJ_CALLBACK callback, void* pPassThrough) const
{
  W_ASSERT_DEV(m_uiMaxTreeDepth > 0, "WDynamicQuadtree::FindVisibleObjects: You have to first create the tree.");

  if (m_NodeMap.IsEmpty())
    return;

  FindVisibleObjects(viewfrustum, callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.z, m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel,
    WMath::Pow(4, m_uiMaxTreeDepth - 1), 0xFFFFFFFF);
}

void WDynamicQuadtree::FindVisibleObjects(const WFrustum& Viewfrustum, W_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx,
  float minz, float maxz, WUInt32 uiNodeID, WUInt32 uiAddID, WUInt32 uiSubAddID, WUInt32 uiNextNodeID) const
{
  // build the bounding box of this node
  WVec3 v[8];
  v[0].Set(minx, m_BBox.m_vMin.y, minz);
  v[1].Set(minx, m_BBox.m_vMin.y, maxz);
  v[2].Set(minx, m_BBox.m_vMax.y, minz);
  v[3].Set(minx, m_BBox.m_vMax.y, maxz);
  v[4].Set(maxx, m_BBox.m_vMin.y, minz);
  v[5].Set(maxx, m_BBox.m_vMin.y, maxz);
  v[6].Set(maxx, m_BBox.m_vMax.y, minz);
  v[7].Set(maxx, m_BBox.m_vMax.y, maxz);

  const WVolumePosition::Enum pos = Viewfrustum.GetObjectPosition(&v[0], 8);

  // stop traversal, if node is outside view-frustum
  if (pos == WVolumePosition::Outside)
    return;

  WDynamicTree::WMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  // get the iterator where the objects stored in this sub-tree start
  WDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  // if there are no objects AT ALL in the map after the iterator, OR in this subtree there are no objects stored, stop
  if ((!it1.IsValid()) || (it1.Key().m_uiKey >= uiNextNodeID))
    return;

  // if the node is COMPLETELY inside the frustum -> no need to recurse further, the whole subtree will be visible
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
    // the node is visible, but some parts might be outside, so refine the search

    mmk.m_uiKey = uiNodeID + 1;

    // first return all objects store at this particular node
    while (it1.IsValid())
    {
      // first increase the iterator, the user could erase it in the callback
      WDynamicTreeObjectConst temp = it1;
      ++it1;

      Callback(pPassThrough, temp);
    }

    // if there are additional child nodes
    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_fLooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_fLooseOctreeFactor;

      const WUInt32 uiNodeIDBase = uiNodeID + 1;
      const WUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const WUInt32 uiSubAddIDChild = uiSubAddID >> 2;

      // continue the search at each child node
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, minx, minx + lx, minz, minz + lz, uiNodeIDBase + uiAddID * 0, uiAddIDChild,
        uiSubAddIDChild, uiNodeIDBase + uiAddID * 1);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, minx, minx + lx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1, uiAddIDChild,
        uiSubAddIDChild, uiNodeIDBase + uiAddID * 2);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, minz, minz + lz, uiNodeIDBase + uiAddID * 2, uiAddIDChild,
        uiSubAddIDChild, uiNodeIDBase + uiAddID * 3);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3, uiAddIDChild,
        uiSubAddIDChild, uiNextNodeID);
    }
  }
}


void WDynamicQuadtree::FindObjectsInRange(const WVec3& vPoint, W_VISIBLE_OBJ_CALLBACK callback, void* pPassThrough) const
{
  W_ASSERT_DEV(m_uiMaxTreeDepth > 0, "WDynamicQuadtree::FindObjectsInRange: You have to first create the tree.");

  if (m_NodeMap.IsEmpty())
    return;

  if (!m_BBox.Contains(vPoint))
    return;

  FindObjectsInRange(vPoint, callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.z, m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel,
    WMath::Pow(4, m_uiMaxTreeDepth - 1), 0xFFFFFFFF);
}

bool WDynamicQuadtree::FindObjectsInRange(const WVec3& vPoint, W_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx,
  float minz, float maxz, WUInt32 uiNodeID, WUInt32 uiAddID, WUInt32 uiSubAddID, WUInt32 uiNextNodeID) const
{
  if (vPoint.x < minx)
    return true;
  if (vPoint.x > maxx)
    return true;
  if (vPoint.z < minz)
    return true;
  if (vPoint.z > maxz)
    return true;

  WDynamicTree::WMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  WDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  // if the sub-tree is empty, or even the whole tree is empty for the remaining nodes, stop traversal
  if (!it1.IsValid() || (it1.Key().m_uiKey >= uiNextNodeID))
    return true;

  {
    // return all objects stored at this node
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

    // if this node has children
    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_fLooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_fLooseOctreeFactor;

      const WUInt32 uiNodeIDBase = uiNodeID + 1;
      const WUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const WUInt32 uiSubAddIDChild = uiSubAddID >> 2;

      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, minz, minz + lz, uiNodeIDBase + uiAddID * 0, uiAddIDChild,
            uiSubAddIDChild, uiNodeIDBase + uiAddID * 1))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1, uiAddIDChild,
            uiSubAddIDChild, uiNodeIDBase + uiAddID * 2))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, minz, minz + lz, uiNodeIDBase + uiAddID * 2, uiAddIDChild,
            uiSubAddIDChild, uiNodeIDBase + uiAddID * 3))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3, uiAddIDChild,
            uiSubAddIDChild, uiNextNodeID))
        return false;
    }
  }

  return true;
}



void WDynamicQuadtree::FindObjectsInRange(const WVec3& vPoint, float fRadius, W_VISIBLE_OBJ_CALLBACK callback, void* pPassThrough) const
{
  W_ASSERT_DEV(m_uiMaxTreeDepth > 0, "WDynamicQuadtree::FindObjectsInRange: You have to first create the tree.");

  if (m_NodeMap.IsEmpty())
    return;

  if (!m_BBox.Overlaps(WBoundingBox::MakeFromMinMax(vPoint - WVec3(fRadius), vPoint + WVec3(fRadius))))
    return;

  FindObjectsInRange(vPoint, fRadius, callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.z, m_BBox.m_vMax.z, 0,
    m_uiAddIDTopLevel, WMath::Pow(4, m_uiMaxTreeDepth - 1), 0xFFFFFFFF);
}

bool WDynamicQuadtree::FindObjectsInRange(const WVec3& vPoint, float fRadius, W_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx,
  float maxx, float minz, float maxz, WUInt32 uiNodeID, WUInt32 uiAddID, WUInt32 uiSubAddID, WUInt32 uiNextNodeID) const
{
  if (vPoint.x + fRadius < minx)
    return true;
  if (vPoint.x - fRadius > maxx)
    return true;
  if (vPoint.z + fRadius < minz)
    return true;
  if (vPoint.z - fRadius > maxz)
    return true;

  WDynamicTree::WMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  WDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  // if the whole sub-tree doesn't contain any data, no need to check further
  if (!it1.IsValid() || (it1.Key().m_uiKey >= uiNextNodeID))
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
      const float lz = ((maxz - minz) * 0.5f) * s_fLooseOctreeFactor;

      const WUInt32 uiNodeIDBase = uiNodeID + 1;
      const WUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const WUInt32 uiSubAddIDChild = uiSubAddID >> 2;

      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, minz, minz + lz, uiNodeIDBase + uiAddID * 0, uiAddIDChild,
            uiSubAddIDChild, uiNodeIDBase + uiAddID * 1))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1, uiAddIDChild,
            uiSubAddIDChild, uiNodeIDBase + uiAddID * 2))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, minz, minz + lz, uiNodeIDBase + uiAddID * 2, uiAddIDChild,
            uiSubAddIDChild, uiNodeIDBase + uiAddID * 3))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3, uiAddIDChild,
            uiSubAddIDChild, uiNextNodeID))
        return false;
    }
  }

  return true;
}
