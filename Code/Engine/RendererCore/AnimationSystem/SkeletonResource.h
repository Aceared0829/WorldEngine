#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/RendererCoreDLL.h>

using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;

/// Defines a collision geometry shape attached to a joint in a skeleton.
///
/// Used for physics collision detection. The transform scales and positions a unit sphere,
/// box, or capsule. For convex meshes, the vertex positions and triangle indices define the shape.
struct WSkeletonResourceGeometry
{
  WTransform m_Transform; ///< Scale is used to resize a unit sphere / box / capsule
  WUInt16 m_uiAttachedToJoint = 0;
  WEnum<WSkeletonJointGeometryType> m_Type;

  WDynamicArray<WVec3> m_VertexPositions;  ///< For convex geometry
  WDynamicArray<WUInt8> m_TriangleIndices; ///< For convex geometry
};

/// Descriptor containing all data needed to create a skeleton resource.
struct W_RENDERERCORE_DLL WSkeletonResourceDescriptor
{
  WSkeletonResourceDescriptor();
  ~WSkeletonResourceDescriptor();
  WSkeletonResourceDescriptor(const WSkeletonResourceDescriptor& rhs) = delete;
  WSkeletonResourceDescriptor(WSkeletonResourceDescriptor&& rhs);
  void operator=(WSkeletonResourceDescriptor&& rhs);
  void operator=(const WSkeletonResourceDescriptor& rhs) = delete;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);

  WUInt64 GetHeapMemoryUsage() const;

  WTransform m_RootTransform = WTransform::MakeIdentity();
  WSkeleton m_Skeleton;
  float m_fMaxImpulse = WMath::HighValue<float>();

  WUInt16 m_uiLeftFootJoint = WInvalidJointIndex;  ///< Used for motion extraction
  WUInt16 m_uiRightFootJoint = WInvalidJointIndex; ///< Used for motion extraction

  WDynamicArray<WSkeletonResourceGeometry> m_Geometry;
};

using WSkeletonResourceHandle = WTypedResourceHandle<class WSkeletonResource>;

/// Runtime resource containing skeleton data used for skeletal animation.
///
/// Stores the joint hierarchy, transforms, collision geometry, and other data needed
/// for animating skeletal meshes. Created from a WSkeletonResourceDescriptor.
/// The skeleton is used by animation components and can be queried for joint indices and transforms.
class W_RENDERERCORE_DLL WSkeletonResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WSkeletonResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WSkeletonResource);
  W_RESOURCE_DECLARE_CREATEABLE(WSkeletonResource, WSkeletonResourceDescriptor);

public:
  WSkeletonResource();
  ~WSkeletonResource();

  const WSkeletonResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WUniquePtr<WSkeletonResourceDescriptor> m_pDescriptor;
};
