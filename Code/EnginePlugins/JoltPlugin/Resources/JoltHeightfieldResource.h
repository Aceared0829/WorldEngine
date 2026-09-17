#pragma once

#include <Core/ResourceManager/Resource.h>
#include <JoltPlugin/JoltPluginDLL.h>

using WJoltHeightfieldResourceHandle = WTypedResourceHandle<class WJoltHeightfieldResource>;
using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;

struct W_JOLTPLUGIN_DLL WJoltHeightfieldResourceDescriptor
{
  WVec2 m_vHalfExtents;
  WUInt32 m_uiResolution = 0;

  /// Row-major height samples, m_uiResolution * m_uiResolution entries.
  WDynamicArray<float> m_Heights;

  /// Per-cell material indices, (m_uiResolution-1)^2 entries. May be empty.
  WDynamicArray<WUInt8> m_MaterialIndices;

  /// Surface handles indexed by m_MaterialIndices. May be empty.
  WDynamicArray<WSurfaceResourceHandle> m_Surfaces;

  WUInt8 m_uiCollisionLayer = 0;
};

/// Stores a Jolt heightfield shape.
class W_JOLTPLUGIN_DLL WJoltHeightfieldResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WJoltHeightfieldResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WJoltHeightfieldResource);
  W_RESOURCE_DECLARE_CREATEABLE(WJoltHeightfieldResource, WJoltHeightfieldResourceDescriptor);

public:
  WJoltHeightfieldResource();
  ~WJoltHeightfieldResource();

  /// Returns the content hash that was stored at export time. Used by the export modifier
  /// to detect whether an existing file is still valid (compare against current brush hash).
  WUInt64 GetContentHash() const { return m_uiContentHash; }

  /// Surface resources indexed by material index. May contain invalid handles; use default material as fallback.
  const WDynamicArray<WSurfaceResourceHandle>& GetSurfaces() const { return m_Surfaces; }

  WUInt8 GetCollisionLayer() const { return m_uiCollisionLayer; }

  /// Raw JPH::HeightFieldShape binary state. Pass to JPH::Shape::sRestoreFromBinaryState().
  const WDataBuffer& GetShapeData() const { return m_ShapeData; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WUInt64 m_uiContentHash = 0;
  WUInt8 m_uiCollisionLayer = 0;
  WDynamicArray<WSurfaceResourceHandle> m_Surfaces;
  WDataBuffer m_ShapeData;
};
