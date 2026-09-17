#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Types/TagSet.h>
#include <RendererCore/RendererCoreDLL.h>

class WWorld;
using WCpuMeshResourceHandle = WTypedResourceHandle<class WCpuMeshResource>;

/// A utility to gather raw geometry from a world
///
/// The utility sends WMsgExtractGeometry to world components and they may fill out the geometry information.
/// \a ExtractionMode defines what the geometry is needed for. This ranges from finding geometry that is used to generate the navmesh from
/// to exporting the geometry to a file for use in another program, e.g. a modeling software.
class W_RENDERERCORE_DLL WWorldGeoExtractionUtil
{
public:
  struct MeshObject
  {
    WTransform m_GlobalTransform;
    WCpuMeshResourceHandle m_hMeshResource;
  };

  using MeshObjectList = WDeque<MeshObject>;

  /// Describes what the geometry is needed for
  enum class ExtractionMode
  {
    RenderMesh,    ///< The render geometry is desired. Typically for exporting it to file.
    CollisionMesh, ///< The collision geometry is desired. Typically for exporting it to file.
  };

  /// Extracts the desired geometry from all objects in a world
  ///
  /// The geometry object is not cleared, so this can be called repeatedly to append more data.
  static void ExtractWorldGeometry(MeshObjectList& ref_objects, const WWorld& world, ExtractionMode mode, WTagSet* pExcludeTags = nullptr);

  /// Extracts the desired geometry from a specified subset of objects in a world
  ///
  /// The geometry object is not cleared, so this can be called repeatedly to append more data.
  static void ExtractWorldGeometry(MeshObjectList& ref_objects, const WWorld& world, ExtractionMode mode, const WDeque<WGameObjectHandle>& selection);

  /// Writes the given geometry in .obj format to file
  static void WriteWorldGeometryToOBJ(const char* szFile, const MeshObjectList& objects, const WMat3& mTransform);
};

/// Sent by WWorldGeoExtractionUtil to gather geometry information about objects in a world
///
/// The mode defines what the geometry is needed for, thus components should decide to participate or not
/// and how detailed the geometry is they return.
struct W_RENDERERCORE_DLL WMsgExtractGeometry : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgExtractGeometry, WMessage);

  /// Specifies what the geometry is extracted for, and thus what the message handler should write back
  WWorldGeoExtractionUtil::ExtractionMode m_Mode = WWorldGeoExtractionUtil::ExtractionMode::RenderMesh;

  /// Append mesh objects to this to describe the requested world geometry
  WWorldGeoExtractionUtil::MeshObjectList* m_pMeshObjects = nullptr;

  void AddMeshObject(const WTransform& transform, WCpuMeshResourceHandle hMeshResource);
  void AddBox(const WTransform& transform, WVec3 vExtents);
};
