#pragma once

#include <Core/Interfaces/PhysicsQuery.h>
#include <Core/World/WorldModule.h>

/// A triangle used for navmesh generation.
struct WNavmeshTriangle
{
  WVec3 m_Vertices[3];
  const WSurfaceResource* m_pSurface = nullptr;
};

/// A world module that retrieves triangle data that should be used for building navmeshes at runtime.
///
/// If a physics engine is active, it usually automatically provides such a world module to retrieve the triangle data
/// through physics queries.
///
/// In other types of games, a custom world module can be implemented, to generate this data in a different way.
/// If a physics engine is active, but a custom method should be used, you can write a custom world module
/// and then use WWorldModuleFactory::RegisterInterfaceImplementation() to specify which module to use.
/// Also see WWorldModuleConfig.
class W_CORE_DLL WNavmeshGeoWorldModuleInterface : public WWorldModule
{
  W_ADD_DYNAMIC_REFLECTION(WNavmeshGeoWorldModuleInterface, WWorldModule);

protected:
  WNavmeshGeoWorldModuleInterface(WWorld* pWorld)
    : WWorldModule(pWorld)
  {
  }

public:
  /// Retrieves triangles within a specified area for navmesh generation.
  ///
  /// \param uiCollisionLayer The collision layer to query for geometry
  /// \param box The bounding box defining the area to retrieve geometry from
  /// \param out_triangles Array to fill with triangles found in the specified area
  virtual void RetrieveGeometryInArea(WUInt32 uiCollisionLayer, const WBoundingBox& box, WDynamicArray<WNavmeshTriangle>& out_triangles) const = 0;
};
