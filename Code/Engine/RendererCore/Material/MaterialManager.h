#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/RenderContext/BindGroupBuilder.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

class WGALCommandEncoder;
struct WGALDeviceEvent;

/// Fired by WMaterialManager::s_MaterialShaderChangedEvent in case a material is reloaded and now occupies another index / shader.
struct W_RENDERERCORE_DLL WMaterialShaderChanged
{
  WMaterialResourceHandle m_hMaterial;
  WShaderResourceHandle m_hOldShader;
  WMaterialResource::WMaterialId m_OldId;
  WShaderResourceHandle m_hNewShader;
  WMaterialResource::WMaterialId m_NewId;
};

class W_RENDERERCORE_DLL WMaterialManager
{
  W_DECLARE_SINGLETON(WMaterialManager);

public:
  /// The material's render data. Accessed via WMaterialManager::GetMaterialData.
  struct W_RENDERERCORE_DLL MaterialData
  {
    ~MaterialData();
    void DeleteBindGroups();

    WShaderResourceHandle m_hShader;
    WMaterialResource::WMaterialId m_MaterialId; ///< Index into m_hStructuredBufferView

    WDynamicArray<WPermutationVar> m_PermutationVars;

    // Depending on the capabilities of the GAL device, materials are either stored in a constant or structured buffer.
    WGALBufferHandle m_hStructuredBuffer;
    WGALBufferHandle m_hConstantBuffer;

    WDynamicArray<WMaterialResourceDescriptor::Parameter> m_Parameters; // Builds constant buffer
    WDynamicArray<WMaterialResourceDescriptor::Texture2DBinding> m_Texture2DBindings;
    WDynamicArray<WMaterialResourceDescriptor::TextureCubeBinding> m_TextureCubeBindings;

    struct BindGroupCache
    {
      WGALBindGroupLayoutHandle m_hLayout;
      WGALBindGroupHandle m_hGroup;
    };
    WHybridArray<BindGroupCache, 2> m_BindGroups;
  };

public:
  /// Called by WMaterialResource::UpdateContent and WMaterialResource::CreateResource to add the material to the manager.
  static void MaterialAdded(WMaterialResource* pMaterial);
  /// Called by WMaterialResource::SetModified to inform the material manager of changes.
  static void MaterialModified(WMaterialResourceHandle hMaterial);
  /// Called by WMaterialResource destructor to remove the material from the manager.
  /// Note that we don't call this during unload to maintain the material index during reloading of materials.
  static void MaterialRemoved(WMaterialResource* pMaterial);
  /// Returns the render data for a material resource. Can be nullptr if the material is not loaded yet or extraction + update was not executed yet.
  static const MaterialData* GetMaterialData(const WMaterialResource* pMaterial);
  /// Returns the bind group for the given material resource and layout.
  static WGALBindGroupHandle GetMaterialBindGroup(const WMaterialResource* pMaterial, WGALBindGroupLayoutHandle hBindGroupLayout);
  /// Fired in case a material is reloaded and now occupies another index / shader.
  static WEvent<const WMaterialShaderChanged&, WMutex> s_MaterialShaderChangedEvent;

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, MaterialManager);
  friend class WMemoryUtils;

  /// How MaterialShaderConstants will store the material data.
  enum class MaterialStorageMode
  {
    MultipleConstantBuffer,
    MultipleStructuredBuffer,
    SingleStructuredBuffer,
  };

  /// Manages the constants for all materials of a single shader resource.
  class MaterialShaderConstants
  {
  public:
    MaterialShaderConstants(WShaderResourceHandle hShader, WMaterialManager* pParent);
    ~MaterialShaderConstants();

    WMaterialResource::WMaterialId AddMaterial(WMaterialResourceHandle hMaterial);
    void RemoveMaterial(WMaterialResource::WMaterialId id);
    bool IsEmpty() const;

    void MarkDirty(WMaterialResource::WMaterialId id);
    bool RequiresUpdates() const;
    void UpdateConstantBuffers();

    void DestroyGpuResources();

  private:
    void OnResourceEvent(const WResourceEvent& e);
    void OnShaderChanged(WShaderResource* pShader);
    bool UpdateMaterialLayout();
    void UpdateMaterial(WMaterialResource::WMaterialId id, WMaterialResourceHandle hMaterial);

  private:
    // Material data
    mutable WMutex m_MaterialsMutex;
    WIdTable<WMaterialResource::WMaterialId, WMaterialResourceHandle> m_Materials;

    WDynamicArray<WUInt8> m_MaterialsData;
    // #TODO_MATERIAL Right now, there are individual structured buffers for each material until we have refactored the high level renderer to actually have a place to store the material index.
    WGALBufferHandle m_hStructuredBuffer;
    WDynamicArray<WGALBufferHandle> m_MaterialBuffers;

    // Shader data
    bool m_bShaderDirty = true;
    bool m_bShaderHasNoConstants = false;
    WSet<WMaterialResource::WMaterialId> m_DirtyMaterials;
    WSharedPtr<WShaderConstantBufferLayout> m_pLayout;
    WHashTable<WHashedString, WUInt32> m_ParameterNameToLayoutIndex;

    // Static data
    MaterialStorageMode m_Mode = MaterialStorageMode::MultipleConstantBuffer;
    const WShaderResourceHandle m_hShader;
    WMaterialManager* m_pParent = nullptr;
    WEvent<const WResourceEvent&, WMutex>::Unsubscriber m_ShaderResourceEventSubscriber;
  };

  struct ExtractedMaterial
  {
    // Not using the frame allocator on the member arrays as we std::move these.
    WMaterialResourceHandle m_hMaterial;
    WShaderResourceHandle m_hShader;
    WMaterialResource::WMaterialId m_MaterialId;
    WBitflags<WMaterialResource::DirtyFlags> m_DirtyFlags;
    WDynamicArray<WMaterialResourceDescriptor::Parameter> m_Parameters;
    WDynamicArray<WMaterialResourceDescriptor::Texture2DBinding> m_Texture2DBindings;
    WDynamicArray<WMaterialResourceDescriptor::TextureCubeBinding> m_TextureCubeBindings;
    WDynamicArray<WPermutationVar> m_PermutationVars;
  };

  struct PendingChanges
  {
    PendingChanges();

    WDynamicArray<const void*> m_RemovedMaterials;
    WDynamicArray<ExtractedMaterial> m_AddedOrModifiedMaterials;
  };

private:
  WMaterialManager();
  ~WMaterialManager();

  void OnExtractionEvent(const WRenderWorldExtractionEvent& e);
  void OnRenderEvent(const WGALDeviceEvent& e);

  void ExtractMaterialUpdates();
  void RegisterMaterial(WMaterialResource* pMaterial);
  void UnregisterMaterial(WMaterialResource* pMaterial);
  static void ExtractMaterial(WMaterialResource* pMaterial, ExtractedMaterial& extractedMaterial);
  void ApplyMaterialChanges();
  void Cleanup();

  MaterialShaderConstants& GetShaderConstants(WShaderResourceHandle hShader);

private:
  // Extract these materials during extraction phase.
  WMutex m_ExtractionMutex;
  WHashSet<WMaterialResourceHandle> m_ModifiedMaterials;
  WDynamicArray<const void*> m_RemovedMaterials;

  // Extraction result created by frame allocator
  WUniquePtr<PendingChanges> m_pPendingChanges;

  // Used during material creation, deletion and updates.
  WMutex m_MaterialShaderMutex;
  WMap<WShaderResourceHandle, WUniquePtr<MaterialShaderConstants>> m_MaterialShaders;

  // Used by render thread only to map from WMaterialResource to the MaterialData. No need for locks as only accessed by the render thread.
  // This container will hold dead pointers after material deletion until the next extraction phase + begin render loop.
  // This is better than locking this container on every material draw call.
  WMap<const void*, MaterialData> m_Materials;

  // Contains any materials that has bind groups with fallback or partially loaded resources. These will be deleted at the start of each frame.
  WSet<const void*> m_DirtyBindGroups;
};
