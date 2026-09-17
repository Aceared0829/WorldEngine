#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>

/// Descriptor for color gradient resources containing the gradient data and serialization methods.
struct W_CORE_DLL WColorGradientResourceDescriptor
{
  WColorGradient m_Gradient;

  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream);
};

using WColorGradientResourceHandle = WTypedResourceHandle<class WColorGradientResource>;

/// A resource that stores a single color gradient for use in rendering and effects.
///
/// Color gradient resources allow artists to define color transitions that can be evaluated
/// at runtime. Commonly used for particle effects, UI elements, and other visual systems
/// that need smooth color transitions.
class W_CORE_DLL WColorGradientResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WColorGradientResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WColorGradientResource);
  W_RESOURCE_DECLARE_CREATEABLE(WColorGradientResource, WColorGradientResourceDescriptor);

public:
  WColorGradientResource();

  /// Returns all the data that is stored in this resource.
  const WColorGradientResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  /// Evaluates the color gradient at the given position and returns the interpolated color.
  inline WColor Evaluate(double x) const
  {
    WColor result;
    m_Descriptor.m_Gradient.Evaluate(x, result);
    return result;
  }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WColorGradientResourceDescriptor m_Descriptor;
};
