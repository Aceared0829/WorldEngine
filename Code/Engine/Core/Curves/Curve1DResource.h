#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/Curve1D.h>

/// Descriptor for 1D curve resources containing multiple curves and serialization methods.
///
/// A curve resource can contain more than one curve, but all curves are of the same type.
/// This allows grouping related curves together for efficiency and logical organization.
struct W_CORE_DLL WCurve1DResourceDescriptor
{
  WDynamicArray<WCurve1D> m_Curves;

  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream);
};

using WCurve1DResourceHandle = WTypedResourceHandle<class WCurve1DResource>;

/// A resource that stores multiple 1D curves for animation and value interpolation.
///
/// 1D curve resources contain mathematical curves that map time or other input values to
/// output values. Commonly used for animations, easing functions, and procedural value
/// generation where smooth interpolation is needed.
class W_CORE_DLL WCurve1DResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WCurve1DResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WCurve1DResource);
  W_RESOURCE_DECLARE_CREATEABLE(WCurve1DResource, WCurve1DResourceDescriptor);

public:
  WCurve1DResource();

  /// Returns all the data that is stored in this resource.
  const WCurve1DResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WCurve1DResourceDescriptor m_Descriptor;
};
