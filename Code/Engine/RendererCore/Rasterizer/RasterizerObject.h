#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/RendererCoreDLL.h>

class WGeometry;

/// Represents a mesh for CPU-based software rasterization and occlusion culling.
///
/// Used for occlusion testing in WRasterizerView. Objects are cached by name and shared across users.
/// Internally uses a software rasterizer to determine if objects are occluded by other geometry.
class W_RENDERERCORE_DLL WRasterizerObject : public WRefCounted
{
  W_DISALLOW_COPY_AND_ASSIGN(WRasterizerObject);

public:
  WRasterizerObject();
  ~WRasterizerObject();

  /// If an object with the given name has been created before, it is returned, otherwise nullptr is returned.
  ///
  /// Use this to quickly query for an existing object. Call CreateMesh() in case the object doesn't exist yet.
  static WSharedPtr<const WRasterizerObject> GetObject(WStringView sUniqueName);

  /// Creates a box object with the specified dimensions. If such a box was created before, the same pointer is returned.
  static WSharedPtr<const WRasterizerObject> CreateBox(const WVec3& vFullExtents);

  /// Creates a quad pointing into the positive X direction with the dimensions along Y and Z. If such a quad was created before, the same pointer is returned.
  static WSharedPtr<const WRasterizerObject> CreateQuadX(const WVec2& vYZExtents);

  /// Creates an object with the given geometry. If an object with the same name was created before, that pointer is returned instead.
  ///
  /// It is assumed that the same name will only be used for identical geometry.
  static WSharedPtr<const WRasterizerObject> CreateMesh(WStringView sUniqueName, const WGeometry& geometry);

private:
  void CreateMesh(const WGeometry& geometry);

  friend class WRasterizerView;
  Occluder m_Occluder;

  static WMutex s_Mutex;
  static WMap<WString, WSharedPtr<WRasterizerObject>> s_Objects;
};
