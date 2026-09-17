#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class W_RENDERERFOUNDATION_DLL WGALShader : public WGALObject<WGALShaderCreationDescription>
{
public:
  virtual void SetDebugName(WStringView sName) const = 0;

  /// Returns the number of bind groups in the shader. Every bind group must be bound for the shader to be used.
  W_ALWAYS_INLINE WUInt32 GetBindGroupCount() const { return m_BindGroupLayouts.GetCount(); }
  /// Returns the layout of the given bind group.
  /// \param uiBindGroup Must be less than GetBindGroupCount.
  W_ALWAYS_INLINE WGALBindGroupLayoutHandle GetBindGroupLayout(WUInt32 uiBindGroup = 0) const { return m_BindGroupLayouts[uiBindGroup]; }
  /// Returns the pipeline layout for this shader. I.e. the umbrella of all bind group layouts. This can be used to e.g. sort draw calls by to reduce state changes.
  W_ALWAYS_INLINE WGALPipelineLayoutHandle GetPipelineLayout() const { return m_hPipelineLayout; }
  /// Convenience function that returns WGALBindGroupLayoutCreationDescription::m_ResourceBindings of the given bind group layout.
  WArrayPtr<const WShaderResourceBinding> GetBindings(WUInt32 uiBindGroup = 0) const;

  /// Returns the list of vertex input attributes. Compute shaders return an empty array.
  WArrayPtr<const WShaderVertexInputAttribute> GetVertexInputAttributes() const;

protected:
  friend class WGALDevice;

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;

  WResult CreateBindingMapping(bool bAllowMultipleBindingPerName);
  void DestroyBindingMapping();
  WResult CreateLayouts(WGALDevice* pDevice, bool bSupportsImmutableSamplers);
  void DestroyLayouts(WGALDevice* pDevice);

  WGALShader(const WGALShaderCreationDescription& Description);
  virtual ~WGALShader();

protected:
  WGALDevice* m_pDevice = nullptr;
  WDynamicArray<WShaderResourceBinding> m_BindingMapping;

  WHybridArray<WGALBindGroupLayoutHandle, W_GAL_MAX_BIND_GROUPS> m_BindGroupLayouts;
  WGALPipelineLayoutHandle m_hPipelineLayout;
};
