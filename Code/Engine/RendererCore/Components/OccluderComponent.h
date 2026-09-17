#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/RendererCoreDLL.h>

struct WMsgTransformChanged;
struct WMsgUpdateLocalBounds;
struct WMsgExtractOccluderData;

class W_RENDERERCORE_DLL WOccluderComponentManager final : public WComponentManager<class WOccluderComponent, WBlockStorageType::FreeList>
{
public:
  WOccluderComponentManager(WWorld* pWorld);
};

struct WOccluderType
{
  using StorageType = WUInt8;

  enum Enum : StorageType
  {
    Box,      ///< The occluder is a box with 6 faces.
    QuadPosX, ///< The occluder is only a single face at the positive X extent of the surrounding box.
    Mesh,

    Default = Box
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WOccluderType);

/// Adds invisible geometry to a scene that is used for occlusion culling.
///
/// The component adds a box occluder to the scene. The renderer uses this geometry
/// to cull other objects which are behind occluder geometry. Use occluder components to optimize levels.
/// Make the shapes conservative, meaning that they shouldn't be bigger than the actual shapes, otherwise
/// they may incorrectly occlude other objects and lead to incorrectly culled objects.
///
/// The WGreyBoxComponent can also create occluder geometry in different shapes.
///
/// Contrary to WGreyBoxComponent, occluder components can be moved around dynamically and thus can be attached to
/// doors and other objects that may dynamically change the visible areas of a level.
class W_RENDERERCORE_DLL WOccluderComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WOccluderComponent, WComponent, WOccluderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WOccluderComponent

public:
  WOccluderComponent();
  ~WOccluderComponent();

  /// Sets the extents of the occluder.
  void SetExtents(const WVec3& vExtents);                // [ property ]
  const WVec3& GetExtents() const { return m_vExtents; } // [ property ]

  /// Sets the type of occluder.
  void SetType(WEnum<WOccluderType> type);                // [ property ]
  WEnum<WOccluderType> GetType() const { return m_Type; } // [ property ]

  // adds SetMeshFile() and GetMeshFile() for convenience
  W_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Mesh, m_hMesh, SetMesh);

  void SetMesh(const WCpuMeshResourceHandle& hCubeMap); // [ property ]
  const WCpuMeshResourceHandle& GetMesh() const;        // [ property ]

private:
  WVec3 m_vExtents = WVec3(5.0f);
  WEnum<WOccluderType> m_Type;
  WCpuMeshResourceHandle m_hMesh;

  mutable WSharedPtr<const WRasterizerObject> m_pOccluderObject;

  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);
  void OnMsgExtractOccluderData(WMsgExtractOccluderData& msg) const;
  void UpdateOccluder();
};
