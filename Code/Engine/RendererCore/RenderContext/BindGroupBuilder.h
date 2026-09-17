#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Shader/BindGroup.h>

/// Creates WGALBindGroupCreationDescription that can be passed into WGALCommandEncoder.
///
/// This class is usually not created manually but instead retrieved via WRenderContext::GetBindGroup.
/// By calling the various Bind* methods resources can be bound to a shader. The evaluation is deferred until the CreateBindGroup call which matches the bound items to the bind group layout's WShaderResourceBinding. Any binding that can't find a matching item will be filled with a fallback resource via WGALRendererFallbackResources. These bind group items will be marked with the flag WGALBindGroupItemFlags::Fallback.
class W_RENDERERCORE_DLL WBindGroupBuilder
{
public:
  WBindGroupBuilder();

  /// Must be called before the builder can be used.
  /// This should be called at the start of each frame to make sure no stale resources are referenced inside the builder.
  /// \param pDevice The device used to validate resources in bind calls.
  void ResetBoundResources(const WGALDevice* pDevice);

  /// Returns whether a Bind* call modified in internal state since the last CreateBindGroup call.
  bool IsModified() const { return m_bModified; }

  /// Binds a sampler to this bind group
  /// @param sSlotName The slot under which the sampler is to be bound.
  /// @param hSampler If valid, it will be bound. If not, bind group item under this slot will be removed and replaced with a fallback resource if required.
  /// @param metaFlags Optional subset of WGALBindGroupItemFlags::MetaFlags to be added to the binding.
  void BindSampler(WTempHashedString sSlotName, WGALSamplerStateHandle hSampler, WBitflags<WGALBindGroupItemFlags> metaFlags = {});

  /// Binds a buffer to this bind group
  /// @param sSlotName The slot under which the buffer is to be bound.
  /// @param hBuffer If valid, it will be bound. If not, bind group item under this slot will be removed and replaced with a fallback resource if required.
  /// @param bufferRange What part of the buffer should be bound. Default is entire buffer.
  /// @param overrideTexelBufferFormat Sets the format of the texel buffer. If invalid, default format of the buffer will be used.
  /// @param metaFlags Optional subset of WGALBindGroupItemFlags::MetaFlags to be added to the binding.
  void BindBuffer(WTempHashedString sSlotName, WGALBufferHandle hBuffer, WGALBufferRange bufferRange = {}, WEnum<WGALResourceFormat> overrideTexelBufferFormat = WGALResourceFormat::Invalid, WBitflags<WGALBindGroupItemFlags> metaFlags = {});

  /// Binds a texture to this bind group
  /// @param sSlotName The slot under which the texture is to be bound.
  /// @param hTexture If valid, it will be bound. If not, bind group item under this slot will be removed and replaced with a fallback resource if required.
  /// @param textureRange What part of the texture should be bound. Default is entire texture. Or as much as the target WShaderResourceBinding allows for.
  /// @param overrideViewFormat If set, re-interprets the format of the texture. This can cause performance penalties. Only use it to e.g. read linear vs gamma space or other formats of same type and width.
  /// @param metaFlags Optional subset of WGALBindGroupItemFlags::MetaFlags to be added to the binding.
  void BindTexture(WTempHashedString sSlotName, WGALTextureHandle hTexture, WGALTextureRange textureRange = {}, WEnum<WGALResourceFormat> overrideViewFormat = WGALResourceFormat::Invalid, WEnum<WGALTextureType> overrideViewType = WGALTextureType::Invalid, WBitflags<WGALBindGroupItemFlags> metaFlags = {});

  // Convenience functions:
  void BindTexture(WTempHashedString sSlotName, const WTexture2DResourceHandle& hTexture, WResourceAcquireMode acquireMode = WResourceAcquireMode::AllowLoadingFallback, WGALTextureRange textureRange = {}, WEnum<WGALResourceFormat> overrideViewFormat = WGALResourceFormat::Invalid, WEnum<WGALTextureType> overrideViewType = WGALTextureType::Invalid);
  void BindTexture(WTempHashedString sSlotName, const WTexture3DResourceHandle& hTexture, WResourceAcquireMode acquireMode = WResourceAcquireMode::AllowLoadingFallback, WGALTextureRange textureRange = {}, WEnum<WGALResourceFormat> overrideViewFormat = WGALResourceFormat::Invalid, WEnum<WGALTextureType> overrideViewType = WGALTextureType::Invalid);
  void BindTexture(WTempHashedString sSlotName, const WTextureCubeResourceHandle& hTexture, WResourceAcquireMode acquireMode = WResourceAcquireMode::AllowLoadingFallback, WGALTextureRange textureRange = {}, WEnum<WGALResourceFormat> overrideViewFormat = WGALResourceFormat::Invalid, WEnum<WGALTextureType> overrideViewType = WGALTextureType::Invalid);
  void BindBuffer(WTempHashedString sSlotName, WConstantBufferStorageHandle hBuffer, WGALBufferRange bufferRange = {}, WGALResourceFormat::Enum overrideTexelBufferFormat = WGALResourceFormat::Invalid);

  /// Create a new bind group for the given layout.
  /// @param hBindGroupLayout The bind group layout for which to create the bind group.
  /// @param out_bindGroup The resulting bind group.
  /// @param out_metaFlags Union of all meta flags across the bindings in the bind group. I.e. a bit will be set here if it is set for any binding.
  void CreateBindGroup(WGALBindGroupLayoutHandle hBindGroupLayout, WGALBindGroupCreationDescription& out_bindGroup, WBitflags<WGALBindGroupItemFlags>& out_metaFlags);

public:
  /// Number of modifications of the hash tables each frame. Used for stats.
  static WUInt32 s_uiWrites;
  /// Number of reads of the hash tables each frame. Used for stats.
  static WUInt32 s_uiReads;

private:
  void RemoveItem(WTempHashedString sSlotName, WHashTable<WUInt64, WGALBindGroupItem>& ref_Container);
  void InsertItem(WTempHashedString sSlotName, const WGALBindGroupItem& item, WHashTable<WUInt64, WGALBindGroupItem>& ref_Container);

private:
  const WGALDevice* m_pDevice = nullptr;
  bool m_bModified = true;
  WGALSamplerStateHandle m_hDefaultSampler;
  WHashTable<WUInt64, WGALBindGroupItem> m_BoundSamplers;
  WHashTable<WUInt64, WGALBindGroupItem> m_BoundBuffers;
  WHashTable<WUInt64, WGALBindGroupItem> m_BoundTextures;
};
