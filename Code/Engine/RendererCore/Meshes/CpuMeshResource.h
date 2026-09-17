#pragma once

#include <RendererCore/Meshes/MeshResourceDescriptor.h>

/// CPU-accessible mesh resource that stores mesh data in system memory.
///
/// Unlike regular WMeshResource which stores data on the GPU, this resource keeps
/// the mesh descriptor in CPU memory. Used for scenarios requiring CPU access to
/// mesh data such as collision detection, raycasting, or procedural mesh generation.
class W_RENDERERCORE_DLL WCpuMeshResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WCpuMeshResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WCpuMeshResource);
  W_RESOURCE_DECLARE_CREATEABLE(WCpuMeshResource, WMeshResourceDescriptor);

public:
  WCpuMeshResource();

  /// Returns the mesh descriptor containing vertex and index data.
  const WMeshResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WMeshResourceDescriptor m_Descriptor;
};

using WCpuMeshResourceHandle = WTypedResourceHandle<class WCpuMeshResource>;
