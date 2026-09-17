#pragma once

#include <Core/World/SpatialData.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/Types/TagSet.h>

/// Abstract base class for spatial systems that organize objects for efficient spatial queries.
///
/// Spatial systems manage spatial data for objects in a world, enabling efficient queries like
/// finding objects in a sphere or box, frustum culling, and visibility testing. Concrete
/// implementations use different spatial data structures (octrees, grids, etc.) for optimization.
class W_CORE_DLL WSpatialSystem : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSpatialSystem, WReflectedClass);

public:
  WSpatialSystem();
  ~WSpatialSystem();

  virtual void StartNewFrame();

  /// \name Spatial Data Functions
  ///@{

  virtual WSpatialDataHandle CreateSpatialData(const WSimdBBoxSphere& bounds, WGameObject* pObject, WUInt32 uiCategoryBitmask, const WTagSet& tags) = 0;
  virtual WSpatialDataHandle CreateSpatialDataAlwaysVisible(WGameObject* pObject, WUInt32 uiCategoryBitmask, const WTagSet& tags) = 0;

  virtual void DeleteSpatialData(const WSpatialDataHandle& hData) = 0;

  virtual void UpdateSpatialDataBounds(const WSpatialDataHandle& hData, const WSimdBBoxSphere& bounds) = 0;
  virtual void UpdateSpatialDataObject(const WSpatialDataHandle& hData, WGameObject* pObject) = 0;

  ///@}
  /// \name Simple Queries
  ///@{

  using QueryCallback = WDelegate<WVisitorExecution::Enum(WGameObject*)>;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  struct QueryStats
  {
    WUInt32 m_uiTotalNumObjects = 0;  ///< The total number of spatial objects in this system.
    WUInt32 m_uiNumObjectsTested = 0; ///< Number of objects tested for the query condition.
    WUInt32 m_uiNumObjectsPassed = 0; ///< Number of objects that passed the query condition.
    WTime m_TimeTaken;                ///< Time taken to execute the query
  };
#endif

  /// Parameters for spatial queries to filter and track results.
  struct QueryParams
  {
    WUInt32 m_uiCategoryBitmask = 0;         ///< Bitmask of spatial data categories to include in the query
    const WTagSet* m_pIncludeTags = nullptr; ///< Only include objects that have all of these tags
    const WTagSet* m_pExcludeTags = nullptr; ///< Exclude objects that have any of these tags
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    QueryStats* m_pStats = nullptr;           ///< Optional stats tracking for development builds
#endif
  };

  virtual void FindObjectsInSphere(const WBoundingSphere& sphere, const QueryParams& queryParams, WDynamicArray<WGameObject*>& out_objects) const;
  virtual void FindObjectsInSphere(const WBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const = 0;

  virtual void FindObjectsInBox(const WBoundingBox& box, const QueryParams& queryParams, WDynamicArray<WGameObject*>& out_objects) const;
  virtual void FindObjectsInBox(const WBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const = 0;

  ///@}
  /// \name Visibility Queries
  ///@{

  using IsOccludedFunc = WDelegate<bool(const WSimdBBox&)>;

  virtual void FindVisibleObjects(const WFrustum& frustum, const QueryParams& queryParams, WDynamicArray<const WGameObject*>& out_objects, IsOccludedFunc isOccluded, WVisibilityState::Enum visType) const = 0;

  /// Retrieves a state describing how visible the object is.
  ///
  /// An object may be invisible, fully visible, or indirectly visible (through shadows or reflections).
  ///
  /// \param uiNumFramesBeforeInvisible Used to treat an object that was visible and just became invisible as visible for a few more frames.
  virtual WVisibilityState::Enum GetVisibilityState(const WSpatialDataHandle& hData, WUInt32 uiNumFramesBeforeInvisible) const = 0;

  ///@}

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(WStringBuilder& ref_sSb) const;
#endif

protected:
  WProxyAllocator m_Allocator;

  WUInt64 m_uiFrameCounter = 0;
};

/// Script extension class providing spatial query functions for scripting languages.
class W_CORE_DLL WScriptExtensionClass_Spatial
{
public:
  /// Finds the closest object in a sphere within the given category.
  static WGameObject* FindClosestObjectInSphere(WWorld* pWorld, WStringView sCategory, const WVec3& vCenter, float fRadius);
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptExtensionClass_Spatial);
