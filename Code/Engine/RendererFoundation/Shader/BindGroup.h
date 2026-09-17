#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Algorithm/HashableStruct.h>
#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Resources/ResourceFormats.h>


/// \file

/// ***** Bind Group Item Notes *****
///
/// The classes in this file define the bindings in a bind group layout. The user should never create these by hand, they are used mainly for easy hashing and comparison of bind groups. Instead, users should use the WBindGroupBuilder to create bind groups and their items.

/// Sampler contents of WGALBindGroupItem
struct WSamplerBindGroupItem
{
  W_DECLARE_POD_TYPE();
  WGALSamplerStateHandle m_hSampler;
};

/// Texture contents of WGALBindGroupItem
struct WTextureBindGroupItem
{
  W_DECLARE_POD_TYPE();
  WGALTextureHandle m_hTexture;
  WGALSamplerStateHandle m_hSampler;               ///< Only used for slots of WGALShaderResourceType::TextureAndSampler.
  WGALTextureRange m_TextureRange;
  WEnum<WGALResourceFormat> m_OverrideViewFormat; ///< Overrides the default view format. E.g. used for converting between linear and gamma space.
  WEnum<WGALTextureType> m_OverrideViewType = WGALTextureType::Invalid;
};

/// Buffer contents of WGALBindGroupItem
struct WGALBufferBindGroupItem
{
  W_DECLARE_POD_TYPE();
  WGALBufferHandle m_hBuffer;
  WGALBufferRange m_BufferRange;
  WEnum<WGALResourceFormat> m_OverrideTexelBufferFormat; ///< Texel buffer only: Overrides the default view format defined in the buffer description.
};

/// Used by WGALBindGroupItem to define its content.
struct WGALBindGroupItemFlags
{
  using StorageType = WUInt8;

  enum Enum : WUInt8
  {
    Sampler = W_BIT(0),          ///< WGALBindGroupItem::m_Sampler is valid
    Texture = W_BIT(1),          ///< WGALBindGroupItem::m_Texture is valid
    Buffer = W_BIT(2),           ///< WGALBindGroupItem::m_Buffer is valid
    EmptyBinding = W_BIT(3),     ///< The binding slot was empty and filled with a fallback resource from WGALRendererFallbackResources.
    FallbackResource = W_BIT(4), ///< The slot was filled with a fallback resource due to WResourceAcquireMode::AllowLoadingFallback.
    PartiallyLoaded = W_BIT(5),  ///< The resource is only partially loaded.
    TypeFlags = Sampler | Texture | Buffer,
    MetaFlags = EmptyBinding | FallbackResource | PartiallyLoaded,
    Default = 0
  };

  struct Bits
  {
    StorageType Sampler : 1;
    StorageType Texture : 1;
    StorageType Buffer : 1;
    StorageType EmptyBinding : 1;
    StorageType FallbackResource : 1;
    StorageType PartiallyLoaded : 1;
  };
};
W_DECLARE_FLAGS_OPERATORS(WGALBindGroupItemFlags);

/// Used by WGALBindGroupCreationDescription to bind resources to a WShaderResourceBinding slot.
struct WGALBindGroupItem : public WHashableStruct<WGALBindGroupItem>
{
  W_DECLARE_POD_TYPE();
  inline WGALBindGroupItem();
  inline WGALBindGroupItem(const WGALBindGroupItem& rhs);
  inline void operator=(const WGALBindGroupItem& rhs);

  WBitflags<WGALBindGroupItemFlags> m_Flags;
  union
  {
    WSamplerBindGroupItem m_Sampler;
    WTextureBindGroupItem m_Texture;
    WGALBufferBindGroupItem m_Buffer;
  };
};

/// Defines a bind group.
/// Can be set to the renderer via WGALCommandEncoder::SetBindGroup.
struct W_RENDERERFOUNDATION_DLL WGALBindGroupCreationDescription
{
  WUInt64 CalculateHash() const;
  void AssertValidDescription(const WGALDevice& galDevice) const;

  WGALBindGroupLayoutHandle m_hBindGroupLayout;       ///< The layout that this bind group was created for.
  WDynamicArray<WGALBindGroupItem> m_BindGroupItems; ///< Contains one item for every WShaderResourceBinding in the WGALBindGroupLayout at matching indices in the arrays.
};

class W_RENDERERFOUNDATION_DLL WGALBindGroup : public WGALResource<WGALBindGroupCreationDescription>
{
public:
  virtual bool IsInvalidated() const = 0;

protected:
  friend class WGALDevice;

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;
  virtual void Invalidate(WGALDevice* pDevice) = 0;

  inline WGALBindGroup(const WGALBindGroupCreationDescription& Description);
  inline virtual ~WGALBindGroup();
};

#include <RendererFoundation/Shader/Implementation/BindGroup_inl.h>
