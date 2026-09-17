#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Material/MaterialManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>

W_IMPLEMENT_SINGLETON(WMaterialManager);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, MaterialManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WMaterialManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WMaterialManager* pDummy = WMaterialManager::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    // Required here so that we clean up extracted data before the frame allocator gets destroyed.
    WMaterialManager::GetSingleton()->Cleanup();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WEvent<const WMaterialShaderChanged&, WMutex> WMaterialManager::s_MaterialShaderChangedEvent;

WMaterialManager::MaterialData::~MaterialData()
{
  DeleteBindGroups();
}

void WMaterialManager::MaterialData::DeleteBindGroups()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  for (BindGroupCache& bindGroupCache : m_BindGroups)
  {
    pDevice->DestroyBindGroup(bindGroupCache.m_hGroup);
  }
  m_BindGroups.Clear();
}

void WMaterialManager::MaterialAdded(WMaterialResource* pMaterial)
{
  WMaterialManager* pManager = GetSingleton();
  if (!pManager)
    return;
  pManager->RegisterMaterial(pMaterial);
}

void WMaterialManager::MaterialModified(WMaterialResourceHandle hMaterial)
{
  WMaterialManager* pManager = GetSingleton();
  if (!pManager)
    return;
  W_LOCK(pManager->m_ExtractionMutex);
  pManager->m_ModifiedMaterials.Insert(hMaterial);
}

void WMaterialManager::MaterialRemoved(WMaterialResource* pMaterial)
{
  WMaterialManager* pManager = GetSingleton();
  if (!pManager)
    return;
  if (pMaterial->m_MaterialId.IsInvalidated())
    return;
  pManager->UnregisterMaterial(pMaterial);
}

const WMaterialManager::MaterialData* WMaterialManager::GetMaterialData(const WMaterialResource* pMaterial)
{
  WMaterialManager* pManager = GetSingleton();
  if (!pManager)
    return nullptr;

  auto it = pManager->m_Materials.Find(pMaterial);
  // W_ASSERT_DEV(it.IsValid(), "Loaded materials must always have a valid entry in m_Materials");
  return it.IsValid() ? &it.Value() : nullptr;
}

WGALBindGroupHandle WMaterialManager::GetMaterialBindGroup(const WMaterialResource* pMaterial, WGALBindGroupLayoutHandle hBindGroupLayout)
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WMaterialManager* pManager = GetSingleton();
  if (!pManager)
    return {};

  auto it = pManager->m_Materials.Find(pMaterial);
  if (!it.IsValid())
    return {};

  // First, check the cache for the given layout.
  WMaterialManager::MaterialData& data = it.Value();
  for (WUInt32 i = 0; i < data.m_BindGroups.GetCount(); ++i)
  {
    const MaterialData::BindGroupCache& bindGroupCache = data.m_BindGroups[i];
    if (bindGroupCache.m_hLayout == hBindGroupLayout)
    {
      const WGALBindGroup* pBindGroup = pDevice->GetBindGroup(bindGroupCache.m_hGroup);
      W_ASSERT_DEBUG(pBindGroup != nullptr, "Bind group should be owned by the material manager but it no longer exists.");
      if (pBindGroup->IsInvalidated())
      {
        auto hBindGroup = bindGroupCache.m_hGroup;
        pDevice->DestroyBindGroup(hBindGroup);
        data.m_BindGroups.RemoveAtAndSwap(i);
        break;
      }
      return bindGroupCache.m_hGroup;
    }
  }


  WGALBindGroupCreationDescription desc;
  WBitflags<WGALBindGroupItemFlags> metaFlags;
  {
    // Use builder to create a new bind group for the given layout.
    WBindGroupBuilder bindGroupMaterial;
    bindGroupMaterial.ResetBoundResources(pDevice);
    if (!data.m_hStructuredBuffer.IsInvalidated())
    {
      bindGroupMaterial.BindBuffer("materialData", data.m_hStructuredBuffer);
    }
    else if (!data.m_hConstantBuffer.IsInvalidated())
    {
      bindGroupMaterial.BindBuffer("materialData", data.m_hConstantBuffer);
    }

    for (const WMaterialResourceDescriptor::Texture2DBinding& binding : data.m_Texture2DBindings)
    {
      bindGroupMaterial.BindTexture(binding.m_Name, binding.m_Value);
    }

    for (const WMaterialResourceDescriptor::TextureCubeBinding& binding : data.m_TextureCubeBindings)
    {
      bindGroupMaterial.BindTexture(binding.m_Name, binding.m_Value);
    }
    bindGroupMaterial.CreateBindGroup(hBindGroupLayout, desc, metaFlags);
  }
  WGALBindGroupHandle hBindGroup = pDevice->CreateBindGroup(desc);
  if (hBindGroup.IsInvalidated())
    return {};

  data.m_BindGroups.PushBack({hBindGroupLayout, hBindGroup});
  if (metaFlags.IsAnySet(WGALBindGroupItemFlags::FallbackResource | WGALBindGroupItemFlags::PartiallyLoaded))
  {
    // Bind groups that contain fallback or partially loaded resources will be deleted at the start of each frame.
    pManager->m_DirtyBindGroups.Insert(pMaterial);
  }

  return hBindGroup;
}

WMaterialManager::WMaterialManager()
  : m_SingletonRegistrar(this)
{
  WRenderWorld::GetExtractionEvent().AddEventHandler(WMakeDelegate(&WMaterialManager::OnExtractionEvent, this));
  WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WMaterialManager::OnRenderEvent, this));
}

WMaterialManager::~WMaterialManager()
{
  WRenderWorld::GetExtractionEvent().RemoveEventHandler(WMakeDelegate(&WMaterialManager::OnExtractionEvent, this));
  WGALDevice::s_Events.RemoveEventHandler(WMakeDelegate(&WMaterialManager::OnRenderEvent, this));
  Cleanup();
}

void WMaterialManager::OnExtractionEvent(const WRenderWorldExtractionEvent& e)
{
  // WUInt32 uiDataIndex = WRenderWorld::GetDataIndexForExtraction();
  if (e.m_Type != WRenderWorldExtractionEvent::Type::EndExtraction)
    return;

  ExtractMaterialUpdates();
}

void WMaterialManager::OnRenderEvent(const WGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case WGALDeviceEvent::BeforeShutdown:
    {
      Cleanup();
    }
    break;
    case WGALDeviceEvent::BeforeBeginFrame:
    {
      ApplyMaterialChanges();
    }
    break;
    default:
      break;
  }
}

void WMaterialManager::ExtractMaterialUpdates()
{
  W_PROFILE_SCOPE("ExtractMaterialUpdates");
  W_ASSERT_DEBUG(m_pPendingChanges == nullptr, "OnRenderEvent should have been called to consume pending changes or ExtractMaterialUpdates was called twice.");

  WHashSet<WMaterialResourceHandle> modifiedMaterials;
  WDynamicArray<const void*> removedMaterials;
  {
    W_LOCK(m_ExtractionMutex);
    modifiedMaterials.Swap(m_ModifiedMaterials);
    removedMaterials.Swap(m_RemovedMaterials);
  }

  m_pPendingChanges = W_NEW(WFrameAllocator::GetCurrentAllocator(), PendingChanges);
  m_pPendingChanges->m_AddedOrModifiedMaterials.Reserve(modifiedMaterials.GetCount());

  for (const WMaterialResourceHandle& hMaterial : modifiedMaterials)
  {
    WResourceLock<WMaterialResource> pMaterial(hMaterial, WResourceAcquireMode::BlockTillLoaded);
    if (pMaterial->m_hShader.IsValid())
    {
      ExtractedMaterial& extractedMaterial = m_pPendingChanges->m_AddedOrModifiedMaterials.ExpandAndGetRef();
      ExtractMaterial(pMaterial.GetPointerNonConst(), extractedMaterial);
    }
  }
  m_pPendingChanges->m_RemovedMaterials = removedMaterials;
}

void WMaterialManager::RegisterMaterial(WMaterialResource* pMaterial)
{
  W_PROFILE_SCOPE("RegisterMaterial");
  W_LOCK(m_MaterialShaderMutex);
  W_LOCK(m_ExtractionMutex);

  const WShaderResourceHandle hOldShader = pMaterial->m_hShader;
  const WMaterialResource::WMaterialId oldId = pMaterial->m_MaterialId;
  const bool bShaderChanged = pMaterial->m_mDesc.m_hShader != hOldShader;

  if (!hOldShader.IsValid())
  {
    if (pMaterial->m_mDesc.m_hShader.IsValid())
    {
      // Newly created material
      pMaterial->m_hShader = pMaterial->m_mDesc.m_hShader;
      MaterialShaderConstants& msc = GetShaderConstants(pMaterial->m_hShader);
      pMaterial->m_MaterialId = msc.AddMaterial(pMaterial->GetResourceHandle());
    }
  }
  else if (bShaderChanged)
  {
    // Material reloaded and changed shader
    UnregisterMaterial(pMaterial);

    // Create new material registration
    if (pMaterial->m_mDesc.m_hShader.IsValid())
    {
      pMaterial->m_hShader = pMaterial->m_mDesc.m_hShader;
      MaterialShaderConstants& msc = GetShaderConstants(pMaterial->m_hShader);
      pMaterial->m_MaterialId = msc.AddMaterial(pMaterial->GetResourceHandle());
    }
    else
    {
      pMaterial->m_hShader = pMaterial->m_mDesc.m_hShader;
      pMaterial->m_MaterialId.Invalidate();
    }

    // Fire event that the shader / id has changed and needs to be invalidated.
    WMaterialShaderChanged change;
    change.m_hMaterial = pMaterial->GetResourceHandle();
    change.m_hOldShader = hOldShader;
    change.m_OldId = oldId;
    change.m_hNewShader = pMaterial->m_hShader;
    change.m_NewId = pMaterial->m_MaterialId;
    s_MaterialShaderChangedEvent.Broadcast(change);
  }
  else
  {
    // Material reloaded with same shader. Nothing to do except marking material dirty below.
  }
  m_ModifiedMaterials.Insert(pMaterial->GetResourceHandle());
}

void WMaterialManager::UnregisterMaterial(WMaterialResource* pMaterial)
{
  W_LOCK(m_MaterialShaderMutex);
  W_LOCK(m_ExtractionMutex);

  if (pMaterial->m_hShader.IsValid())
  {
    MaterialShaderConstants& msc = GetShaderConstants(pMaterial->m_hShader);
    msc.RemoveMaterial(pMaterial->m_MaterialId);
    m_RemovedMaterials.PushBack(pMaterial);
  }
}

void WMaterialManager::ExtractMaterial(WMaterialResource* pMaterial, WMaterialManager::ExtractedMaterial& extractedMaterial)
{
  W_PROFILE_SCOPE("ExtractMaterial");
  extractedMaterial.m_hMaterial = pMaterial->GetResourceHandle();
  extractedMaterial.m_hShader = pMaterial->m_mDesc.m_hShader;
  extractedMaterial.m_MaterialId = pMaterial->m_MaterialId;
  extractedMaterial.m_DirtyFlags = pMaterial->m_DirtyFlags;
  for (WMaterialResource::DirtyFlags::Enum flag : pMaterial->m_DirtyFlags)
  {
    switch (flag)
    {
      case WMaterialResource::DirtyFlags::Parameter:
        extractedMaterial.m_Parameters = pMaterial->m_mDesc.m_Parameters;
        break;
      case WMaterialResource::DirtyFlags::Texture2D:
        extractedMaterial.m_Texture2DBindings = pMaterial->m_mDesc.m_Texture2DBindings;
        break;
      case WMaterialResource::DirtyFlags::TextureCube:
        extractedMaterial.m_TextureCubeBindings = pMaterial->m_mDesc.m_TextureCubeBindings;
        break;
      case WMaterialResource::DirtyFlags::PermutationVar:
        extractedMaterial.m_PermutationVars = pMaterial->m_mDesc.m_PermutationVars;
        break;
      default:
        break;
    }
  }
  pMaterial->m_DirtyFlags.Clear();
}

void WMaterialManager::ApplyMaterialChanges()
{
  if (m_pPendingChanges == nullptr)
  {
    // If no pending changes are present, we will start extracting here.
    // This is usually the case for applications that don't use extraction like tests and basic sample apps.
    ExtractMaterialUpdates();
  }
  W_PROFILE_SCOPE("ApplyMaterialChanges");
  // Remove dead pointers first
  for (const void* pRemovedMaterial : m_pPendingChanges->m_RemovedMaterials)
  {
    m_Materials.Remove(pRemovedMaterial);
  }

  // Execute updates and additions
  for (const auto& extractedMaterial : m_pPendingChanges->m_AddedOrModifiedMaterials)
  {
    WResourceLock<WMaterialResource> pMaterial(extractedMaterial.m_hMaterial, WResourceAcquireMode::PointerOnly);
    auto matIt = m_Materials.FindOrAdd(pMaterial.GetPointer());
    MaterialData& md = matIt.Value();
    bool bDeleteBindGroups = false;
    for (WMaterialResource::DirtyFlags::Enum flag : extractedMaterial.m_DirtyFlags)
    {
      switch (flag)
      {
        case WMaterialResource::DirtyFlags::Parameter:
          md.m_Parameters = std::move(extractedMaterial.m_Parameters);
          break;
        case WMaterialResource::DirtyFlags::Texture2D:
          md.m_Texture2DBindings = std::move(extractedMaterial.m_Texture2DBindings);
          bDeleteBindGroups = true;
          break;
        case WMaterialResource::DirtyFlags::TextureCube:
          md.m_TextureCubeBindings = std::move(extractedMaterial.m_TextureCubeBindings);
          bDeleteBindGroups = true;
          break;
        case WMaterialResource::DirtyFlags::PermutationVar:
          md.m_PermutationVars = std::move(extractedMaterial.m_PermutationVars);
          bDeleteBindGroups = true;
          break;
        case WMaterialResource::DirtyFlags::ShaderAndId:
          md.m_MaterialId = extractedMaterial.m_MaterialId;
          md.m_hShader = extractedMaterial.m_hShader;
          break;
        default:
          break;
      }
    }

    if (bDeleteBindGroups && !md.m_BindGroups.IsEmpty())
    {
      md.DeleteBindGroups();
    }

    W_LOCK(m_MaterialShaderMutex);
    const WMaterialResource::WMaterialId mid = md.m_MaterialId;
    auto matShaderIt = m_MaterialShaders.Find(md.m_hShader);
    W_ASSERT_DEBUG(matIt.IsValid(), "Material shader must exist if dirty");
    MaterialShaderConstants& ms = *matShaderIt.Value();
    ms.MarkDirty(mid);
  }

  for (const void* pMaterial : m_DirtyBindGroups)
  {
    MaterialData* pMD = nullptr;
    if (m_Materials.TryGetValue(pMaterial, pMD))
    {
      pMD->DeleteBindGroups();
    }
  }
  m_DirtyBindGroups.Clear();

  // We need to make sure that we are not holding the m_MaterialShaderMutex when calling MaterialShaderConstants::UpdateConstantBuffers or we may deadlock. See comment in that function.
  WTempHybridArray<MaterialShaderConstants*, 8> requireUpdates;
  {
    W_LOCK(m_MaterialShaderMutex);
    for (auto it = m_MaterialShaders.GetIterator(); it.IsValid();)
    {
      if (it.Value()->IsEmpty())
      {
        // Delete empty
        it = m_MaterialShaders.Remove(it);
      }
      else
      {
        if (it.Value()->RequiresUpdates())
        {
          // Enqueue for updates
          requireUpdates.PushBack(it.Value().Borrow());
        }
        ++it;
      }
    }
  }

  for (MaterialShaderConstants* materialShader : requireUpdates)
  {
    // Load resources outside of lock
    materialShader->UpdateConstantBuffers();
  }

  m_pPendingChanges.Clear();
}

void WMaterialManager::Cleanup()
{
  m_ModifiedMaterials.Clear();
  m_RemovedMaterials.Clear();
  m_pPendingChanges.Clear();
  m_MaterialShaders.Clear();
  m_Materials.Clear();
}

WMaterialManager::MaterialShaderConstants& WMaterialManager::GetShaderConstants(WShaderResourceHandle hShader)
{
  W_ASSERT_DEBUG(hShader.IsValid(), "Invalid materials should not be handled by the material manager");
  W_ASSERT_DEBUG(m_MaterialShaderMutex.IsLocked(), "m_MaterialShaderMutex must be locked before accessing m_MaterialShaders");
  auto it = m_MaterialShaders.FindOrAdd(hShader);
  if (it.Value() == nullptr)
  {
    it.Value() = W_DEFAULT_NEW(MaterialShaderConstants, hShader, this);
  }
  return *it.Value();
}


/////////////////////////////////////////////////
// MaterialShaderConstants
WMaterialManager::MaterialShaderConstants::MaterialShaderConstants(WShaderResourceHandle hShader, WMaterialManager* pParent)
  : m_hShader(hShader)
  , m_pParent(pParent)
{
  // BEGIN-DOCS-CODE-SNIPPET: resource-management-listen
  // Subscribe to resource changes of the shader
  WResourceLock<WShaderResource> pShader(m_hShader, WResourceAcquireMode::PointerOnly);
  pShader->m_ResourceEvents.AddEventHandler(WMakeDelegate(&WMaterialManager::MaterialShaderConstants::OnResourceEvent, this), m_ShaderResourceEventSubscriber);
  // END-DOCS-CODE-SNIPPET

  WEnum<WGALBufferLayout> layout = WGALDevice::GetDefaultDevice()->GetCapabilities().m_materialBufferLayout;
  switch (layout)
  {
    case WGALBufferLayout::Vulkan_Std140_relaxed:
    case WGALBufferLayout::DirectX_ConstantButter:
      m_Mode = MaterialStorageMode::MultipleConstantBuffer;
      break;
    case WGALBufferLayout::Vulkan_Std430_relaxed:
    case WGALBufferLayout::DirectX_StructuredButter:
      m_Mode = MaterialStorageMode::MultipleStructuredBuffer;
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  OnShaderChanged(pShader.GetPointerNonConst());
}

WMaterialManager::MaterialShaderConstants::~MaterialShaderConstants()
{
  DestroyGpuResources();
}

WMaterialResource::WMaterialId WMaterialManager::MaterialShaderConstants::AddMaterial(WMaterialResourceHandle hMaterial)
{
  W_LOCK(m_MaterialsMutex);
  return m_Materials.Insert(hMaterial);
}

void WMaterialManager::MaterialShaderConstants::RemoveMaterial(WMaterialResource::WMaterialId id)
{
  W_LOCK(m_MaterialsMutex);
  m_Materials.Remove(id);
  // We ignore the state of the buffers as the next added material will reuse it.
}

void WMaterialManager::MaterialShaderConstants::MarkDirty(WMaterialResource::WMaterialId id)
{
  m_DirtyMaterials.Insert(id);
}

bool WMaterialManager::MaterialShaderConstants::RequiresUpdates() const
{
  if (m_bShaderDirty || !m_DirtyMaterials.IsEmpty())
    return true;

  if (m_bShaderHasNoConstants)
    return false;

  W_LOCK(m_MaterialsMutex);
  const WUInt32 uiCapacity = m_Materials.GetCapacity();
  const WUInt64 uiNewSize = m_pLayout != nullptr ? m_pLayout->m_uiTotalSize * uiCapacity : 0;
  return uiNewSize != m_MaterialsData.GetCount();
}

void WMaterialManager::MaterialShaderConstants::UpdateConstantBuffers()
{
  W_PROFILE_SCOPE("UpdateConstantBuffers");
  bool bLayoutChanged = false;
  if (m_bShaderDirty)
  {
    m_bShaderDirty = false;
    bLayoutChanged = UpdateMaterialLayout();
  }

  if (m_bShaderHasNoConstants)
  {
    m_DirtyMaterials.Clear();
    return;
  }

  // Resize data array
  W_LOCK(m_MaterialsMutex);
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  const WUInt32 uiCapacity = m_Materials.GetCapacity();
  const WUInt32 uiNewSize = m_pLayout->m_uiTotalSize * uiCapacity;
  const bool bBufferResized = uiNewSize != m_MaterialsData.GetCount();
  if (bBufferResized || bLayoutChanged)
  {
    m_MaterialsData.SetCount(uiNewSize);
    switch (m_Mode)
    {
      case WMaterialManager::MaterialStorageMode::MultipleConstantBuffer:
        m_MaterialBuffers.SetCount(uiCapacity);
        break;
      case WMaterialManager::MaterialStorageMode::MultipleStructuredBuffer:
        m_MaterialBuffers.SetCount(uiCapacity);
        break;
      case WMaterialManager::MaterialStorageMode::SingleStructuredBuffer:
      {
        pDevice->DestroyBuffer(m_hStructuredBuffer);

        WGALBufferCreationDescription bufferDesc;
        bufferDesc.m_uiStructSize = m_pLayout->m_uiTotalSize;
        bufferDesc.m_uiTotalSize = uiNewSize;
        bufferDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
        bufferDesc.m_ResourceAccess.m_bImmutable = false;
        m_hStructuredBuffer = WGALDevice::GetDefaultDevice()->CreateBuffer(bufferDesc);
      }
      break;
      default:
        break;
    }
  }

  if (!bLayoutChanged && m_DirtyMaterials.IsEmpty() && !bBufferResized)
    return;

  // #TODO_MATERIAL On resize of the array, this is recomputing the CPU work which is not necessary but was easy to implement.
  const bool bRecreateAll = bBufferResized && m_Mode == MaterialStorageMode::SingleStructuredBuffer;
  if (bLayoutChanged || bRecreateAll)
  {
    m_DirtyMaterials.Clear();
    for (auto it = m_Materials.GetIterator(); it.IsValid(); ++it)
    {
      WMaterialResource::WMaterialId id = it.Id();
      WMaterialResourceHandle hMaterial = it.Value();
      UpdateMaterial(id, hMaterial);
    }
  }
  else
  {
    for (WMaterialResource::WMaterialId id : m_DirtyMaterials)
    {
      WMaterialResourceHandle hMaterial;
      if (m_Materials.TryGetValue(id, hMaterial))
      {
        UpdateMaterial(id, hMaterial);
      }
    }
    m_DirtyMaterials.Clear();
  }
}

void WMaterialManager::MaterialShaderConstants::DestroyGpuResources()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hStructuredBuffer);

  for (WGALBufferHandle hBuffer : m_MaterialBuffers)
  {
    pDevice->DestroyBuffer(hBuffer);
  }
  m_MaterialBuffers.Clear();
}

void WMaterialManager::MaterialShaderConstants::OnResourceEvent(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUpdated)
  {
    OnShaderChanged(static_cast<WShaderResource*>(e.m_pResource));
  }
}

void WMaterialManager::MaterialShaderConstants::OnShaderChanged(WShaderResource* pShader)
{
  WSharedPtr<WShaderConstantBufferLayout> pNewLayout = pShader->GetMaterialLayout();
  const bool bChanged = m_pLayout == nullptr || pNewLayout == nullptr || *m_pLayout != *pNewLayout;
  if (bChanged)
  {
    m_bShaderDirty = true;
  }
}

bool WMaterialManager::MaterialShaderConstants::UpdateMaterialLayout()
{
  // Rebuild mapping
  // IMPORTANT: We must not hold the m_MaterialsMutex or any of the WMaterialManager locks in this scope or the BlockTillLoaded can deadlock if all resource loading threads are occupied with materials which all wait for one of the material manager or the m_MaterialsMutex lock.
  WResourceLock<WShaderResource> pShader(m_hShader, WResourceAcquireMode::BlockTillLoaded);
  WSharedPtr<WShaderConstantBufferLayout> pNewLayout = pShader->GetMaterialLayout();
  bool bLayoutChanged = m_pLayout == nullptr || pNewLayout == nullptr || *m_pLayout != *pNewLayout;
  if (bLayoutChanged)
  {
    m_pLayout = pNewLayout;
    m_ParameterNameToLayoutIndex.Clear();
    // If the layout has changed, we need to re-create all GPU resources.
    DestroyGpuResources();

    // It is valid for materials not not have constants. We still track them in case the shader gets reloaded and suddenly has constants.
    m_bShaderHasNoConstants = m_pLayout == nullptr;
    if (m_pLayout != nullptr)
    {
      m_bShaderHasNoConstants = false;
      // Build map
      for (WUInt32 i = 0; i < m_pLayout->m_Constants.GetCount(); ++i)
      {
        m_ParameterNameToLayoutIndex.Insert(m_pLayout->m_Constants[i].m_sName, i);
      }
      // We still have stale GPU handles in the MaterialData, but these will be overwritten as we return bLayoutChanged == true here which will trigger a complete rebuild.
    }
    else
    {
      W_LOCK(m_MaterialsMutex);
      // If we reloaded the shader and it has now become one without constants, we need to make sure to clear any old buffer handles on the MaterialData.
      for (auto it = m_Materials.GetIterator(); it.IsValid(); ++it)
      {
        WMaterialResource::WMaterialId id = it.Id();
        WMaterialResourceHandle hMaterial = it.Value();
        WResourceLock<WMaterialResource> pMaterial(hMaterial, WResourceAcquireMode::PointerOnly);
        auto itMaterial = m_pParent->m_Materials.Find(pMaterial.GetPointer());
        if (itMaterial.IsValid())
        {
          itMaterial.Value().m_hConstantBuffer.Invalidate();
          itMaterial.Value().m_hStructuredBuffer.Invalidate();
        }
      }
    }
  }
  return bLayoutChanged;
}

void WMaterialManager::MaterialShaderConstants::UpdateMaterial(WMaterialResource::WMaterialId id, WMaterialResourceHandle hMaterial)
{
  WResourceLock<WMaterialResource> pMaterial(hMaterial, WResourceAcquireMode::PointerOnly);
  auto itMaterial = m_pParent->m_Materials.Find(pMaterial.GetPointer());
  if (!itMaterial.IsValid() || !itMaterial.Value().m_hShader.IsValid())
  {
    // As materials can be added at any time, we might have entries that were registered between extraction and rendering and will this be first extracted in the next frame. Ignore these.
    return;
  }

  MaterialData& md = itMaterial.Value();
  WArrayPtr<WUInt8> data = m_MaterialsData.GetArrayPtr().GetSubArray(id.m_InstanceIndex * m_pLayout->m_uiTotalSize, m_pLayout->m_uiTotalSize);
  for (const auto& param : md.m_Parameters)
  {
    auto it = m_ParameterNameToLayoutIndex.Find(param.m_Name);
    if (it.IsValid())
    {
      const WShaderConstant& constant = m_pLayout->m_Constants[it.Value()];
      if (constant.m_uiOffset + WShaderConstant::s_TypeSize[constant.m_Type.GetValue()] <= data.GetCount())
      {
        WUInt8* pDest = &data[constant.m_uiOffset];
        constant.CopyDataFromVariant(pDest, &param.m_Value);
      }
    }
  }

  switch (m_Mode)
  {
    case WMaterialManager::MaterialStorageMode::MultipleConstantBuffer:
    {
      // Create and update constant buffer
      if (m_MaterialBuffers[id.m_InstanceIndex].IsInvalidated())
      {
        WGALBufferCreationDescription bufferDesc;
        bufferDesc.m_uiTotalSize = m_pLayout->m_uiTotalSize;
        bufferDesc.m_BufferFlags = WGALBufferUsageFlags::ConstantBuffer;
        bufferDesc.m_ResourceAccess.m_bImmutable = false;
        m_MaterialBuffers[id.m_InstanceIndex] = WGALDevice::GetDefaultDevice()->CreateBuffer(bufferDesc);
      }

      // We lazily create the buffer so we need to always update the MaterialData as this could be a constant buffer from a previous material that resided in this slot.
      md.m_hConstantBuffer = m_MaterialBuffers[id.m_InstanceIndex];
      WGALDevice::GetDefaultDevice()->UpdateBufferForNextFrame(m_MaterialBuffers[id.m_InstanceIndex], data, 0);
    }
    break;
    case WMaterialManager::MaterialStorageMode::MultipleStructuredBuffer:
    {
      // Create and update structured buffer
      if (m_MaterialBuffers[id.m_InstanceIndex].IsInvalidated())
      {
        WGALBufferCreationDescription bufferDesc;
        bufferDesc.m_uiStructSize = m_pLayout->m_uiTotalSize;
        bufferDesc.m_uiTotalSize = m_pLayout->m_uiTotalSize;
        bufferDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
        bufferDesc.m_ResourceAccess.m_bImmutable = false;
        m_MaterialBuffers[id.m_InstanceIndex] = WGALDevice::GetDefaultDevice()->CreateBuffer(bufferDesc);
      }

      // We lazily create the buffer so we need to always update the MaterialData as this could be a constant buffer from a previous material that resided in this slot.
      md.m_hStructuredBuffer = m_MaterialBuffers[id.m_InstanceIndex];
      WGALDevice::GetDefaultDevice()->UpdateBufferForNextFrame(m_MaterialBuffers[id.m_InstanceIndex], data, 0);
    }
    break;
    case WMaterialManager::MaterialStorageMode::SingleStructuredBuffer:
    {
      md.m_hStructuredBuffer = m_hStructuredBuffer;
      WGALDevice::GetDefaultDevice()->UpdateBufferForNextFrame(m_hStructuredBuffer, data, m_pLayout->m_uiTotalSize * id.m_InstanceIndex);
    }
    break;
    default:
      break;
  }
}

bool WMaterialManager::MaterialShaderConstants::IsEmpty() const
{
  return m_Materials.IsEmpty();
}

WMaterialManager::PendingChanges::PendingChanges()
  : m_RemovedMaterials(WFrameAllocator::GetCurrentAllocator())
  , m_AddedOrModifiedMaterials(WFrameAllocator::GetCurrentAllocator())
{
}


W_STATICLINK_FILE(RendererCore, RendererCore_Material_Implementation_MaterialManager);
