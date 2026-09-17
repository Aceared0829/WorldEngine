#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <JoltPlugin/JoltPluginDLL.h>

using WJoltMeshResourceHandle = WTypedResourceHandle<class WJoltMeshResource>;
using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;
using WCpuMeshResourceHandle = WTypedResourceHandle<class WCpuMeshResource>;

struct WMsgExtractGeometry;
class WJoltMaterial;

namespace JPH
{
  class MeshShape;
  class ConvexHullShape;
  class Shape;
} // namespace JPH

struct W_JOLTPLUGIN_DLL WJoltMeshResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class W_JOLTPLUGIN_DLL WJoltMeshResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WJoltMeshResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WJoltMeshResource);
  W_RESOURCE_DECLARE_CREATEABLE(WJoltMeshResource, WJoltMeshResourceDescriptor);

public:
  WJoltMeshResource();
  ~WJoltMeshResource();

  /// Returns the bounds of the collision mesh
  const WBoundingBoxSphere& GetBounds() const { return m_Bounds; }

  /// Returns the array of default surfaces to be used with this mesh.
  ///
  /// Note the array may contain less surfaces than the mesh does. It may also contain invalid surface handles.
  /// Use the default physics material as a fallback.
  const WDynamicArray<WSurfaceResourceHandle>& GetSurfaces() const { return m_Surfaces; }

  /// Returns whether the mesh resource contains a triangle mesh. Triangle meshes and convex meshes are mutually exclusive.
  bool HasTriangleMesh() const { return m_pTriangleMeshInstance != nullptr || !m_TriangleMeshData.IsEmpty(); }

  /// Creates a new instance (shape) of the triangle mesh.
  JPH::Shape* InstantiateTriangleMesh(WUInt64 uiUserData, const WDynamicArray<const WJoltMaterial*>& materials) const;

  /// Returns the number of convex meshes. Triangle meshes and convex meshes are mutually exclusive.
  WUInt32 GetNumConvexParts() const { return !m_ConvexMeshInstances.IsEmpty() ? m_ConvexMeshInstances.GetCount() : m_ConvexMeshesData.GetCount(); }

  /// Creates a new instance (shape) of the triangle mesh.
  JPH::Shape* InstantiateConvexPart(WUInt32 uiPartIdx, WUInt64 uiUserData, const WJoltMaterial* pMaterial, float fDensity) const;

  /// Converts the geometry of the triangle or convex mesh to a CPU mesh resource
  WCpuMeshResourceHandle ConvertToCpuMesh() const;

  WUInt32 GetNumTriangles() const { return m_uiNumTriangles; }
  WUInt32 GetNumVertices() const { return m_uiNumVertices; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  WBoundingBoxSphere m_Bounds;
  WDynamicArray<WSurfaceResourceHandle> m_Surfaces;
  mutable WHybridArray<WDataBuffer*, 1> m_ConvexMeshesData;
  mutable WDataBuffer m_TriangleMeshData;
  mutable JPH::Shape* m_pTriangleMeshInstance = nullptr;
  mutable WHybridArray<JPH::Shape*, 1> m_ConvexMeshInstances;

  WUInt32 m_uiNumVertices = 0;
  WUInt32 m_uiNumTriangles = 0;
};
