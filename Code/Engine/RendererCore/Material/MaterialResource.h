#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;
using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;
using WTextureCubeResourceHandle = WTypedResourceHandle<class WTextureCubeResource>;

/// Descriptor for creating material resources.
///
/// Materials can inherit from a base material and override shader, parameters, and textures.
/// Supports shader permutation variables, numeric/color parameters, and 2D/cube texture bindings.
struct WMaterialResourceDescriptor
{
  /// A shader parameter name-value pair.
  struct Parameter
  {
    WHashedString m_Name; ///< Parameter name (must match shader constant).
    WVariant m_Value;     ///< Parameter value (numeric or color).

    W_FORCE_INLINE bool operator==(const Parameter& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  /// A 2D texture binding.
  struct Texture2DBinding
  {
    WHashedString m_Name;             ///< Texture slot name (must match shader resource).
    WTexture2DResourceHandle m_Value; ///< Texture resource handle.

    W_FORCE_INLINE bool operator==(const Texture2DBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  /// A cube texture binding.
  struct TextureCubeBinding
  {
    WHashedString m_Name;               ///< Texture slot name (must match shader resource).
    WTextureCubeResourceHandle m_Value; ///< Cube map resource handle.

    W_FORCE_INLINE bool operator==(const TextureCubeBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  void Clear();

  bool operator==(const WMaterialResourceDescriptor& other) const;
  W_FORCE_INLINE bool operator!=(const WMaterialResourceDescriptor& other) const { return !(*this == other); }

  WMaterialResourceHandle m_hBaseMaterial;                 ///< Base material to inherit from (optional).
  WHashedString m_sSurface;                                ///< Surface type for physics/collision properties.
  WShaderResourceHandle m_hShader;                         ///< Shader used for rendering.
  WDynamicArray<WPermutationVar> m_PermutationVars;       ///< Shader permutation variable values.
  WDynamicArray<Parameter> m_Parameters;                   ///< Shader constant parameters.
  WDynamicArray<Texture2DBinding> m_Texture2DBindings;     ///< 2D texture bindings.
  WDynamicArray<TextureCubeBinding> m_TextureCubeBindings; ///< Cube texture bindings.
  WRenderData::Category m_RenderDataCategory;              ///< Render data category (opaque, transparent, etc.).
};

/// Resource representing a material with shader, parameters, and textures.
///
/// Materials define the visual appearance of rendered objects. They reference a shader and provide
/// values for shader parameters and texture slots. Supports material inheritance through base materials.
class W_RENDERERCORE_DLL WMaterialResource final : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WMaterialResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WMaterialResource);
  W_RESOURCE_DECLARE_CREATEABLE(WMaterialResource, WMaterialResourceDescriptor);

public:
  /// Default material types with pre-defined shaders.
  ///
  /// Use with GetDefaultMaterialFileName() to get file paths for these materials.
  enum class DefaultMaterialType
  {
    Fullbright,          ///< Unlit material without lighting calculations.
    FullbrightAlphaTest, ///< Unlit material with alpha testing.
    Lit,                 ///< Standard lit material.
    LitAlphaTest,        ///< Lit material with alpha testing.
    Sky,                 ///< Sky material for skyboxes.
    MissingMaterial      ///< Placeholder for missing materials.
  };

  using WMaterialId = WGenericId<24, 8>;

public:
  WMaterialResource();
  ~WMaterialResource();

  WHashedString GetPermutationValue(const WTempHashedString& sName);
  WHashedString GetSurface() const;

  void SetParameter(const WHashedString& sName, const WVariant& value);
  void SetParameter(const char* szName, const WVariant& value);
  WVariant GetParameter(const WTempHashedString& sName);

  void SetTexture2DBinding(const WHashedString& sName, const WTexture2DResourceHandle& value);
  void SetTexture2DBinding(const char* szName, const WTexture2DResourceHandle& value);
  WTexture2DResourceHandle GetTexture2DBinding(const WTempHashedString& sName);

  void SetTextureCubeBinding(const WHashedString& sName, const WTextureCubeResourceHandle& value);
  void SetTextureCubeBinding(const char* szName, const WTextureCubeResourceHandle& value);
  WTextureCubeResourceHandle GetTextureCubeBinding(const WTempHashedString& sName);

  WRenderData::Category GetRenderDataCategory();
  static WRenderData::Category GetRenderDataCategory(const WMaterialResourceHandle& hMaterial, bool* out_pWasFallback = nullptr, WRenderData::Category fallbackCategory = WDefaultRenderDataCategories::LitOpaque);

  /// Copies current desc to original desc so the material is not modified on reset
  void PreserveCurrentDesc();
  virtual void ResetResource() override;

  const WMaterialResourceDescriptor& GetCurrentDesc() const;

  /// In case the renderer uses structured buffers to store materials, this is the index into the buffer returns by WMaterialManager::GetMaterialData.
  ///
  /// You only need to call this if you want to persist the index in some other storage as WMaterialManager::GetMaterialData will return the index as well.
  /// Important: The index can change if the shader changes. This can only be done by unloading and reloading the material in which case the [] event is fired.
  WMaterialId GetMaterialId() const { return m_MaterialId; }

  /// Returns the default material file name for the given type (materials in Data/Base/Materials/BaseMaterials).
  static const char* GetDefaultMaterialFileName(DefaultMaterialType materialType);

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

public:
  struct DirtyFlags
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Parameter = W_BIT(0),
      Texture2D = W_BIT(1),
      TextureCube = W_BIT(2),
      PermutationVar = W_BIT(3),
      ShaderAndId = W_BIT(4),
      ResourceReset = Parameter | Texture2D | TextureCube | PermutationVar,
      ResourceCreation = ResourceReset | ShaderAndId,
      Default = 0
    };

    struct Bits
    {
      StorageType Parameter : 1;
      StorageType Texture2D : 1;
      StorageType TextureCube : 1;
      StorageType PermutationVar : 1;
      StorageType ShaderAndId : 1;
    };
  };

private:
  friend class WRenderContext;
  friend class WMaterialManager;

  WEvent<const WMaterialResource*, WMutex> m_ModifiedEvent;

  void AddPermutationVar(WStringView sName, WStringView sValue);
  void SetModified(DirtyFlags::Enum flag);
  void FlattenOriginalDescHierarchy();
  void ComputeRenderDataCategory();

private:
  WMaterialResourceDescriptor m_mOriginalDesc; // stores the state at loading, such that SetParameter etc. calls can be reset later

  // Dynamic data
  WMaterialResourceDescriptor m_mDesc; // Current desc of the material. Contains any changes done after loading.
  WBitflags<DirtyFlags> m_DirtyFlags;  // Flags indicating what has changed in m_mDesc this frame.

  // WMaterialManager registration
  WShaderResourceHandle m_hShader;
  WMaterialId m_MaterialId;
};

W_DECLARE_FLAGS_OPERATORS(WMaterialResource::DirtyFlags);
