#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Stats.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SharedTextureSwapChain.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/ReadbackTexture.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererFoundation/Shader/PipelineLayout.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererFoundation/State/ComputePipeline.h>
#include <RendererFoundation/State/GraphicsPipeline.h>
#include <RendererFoundation/State/State.h>

namespace
{
  struct GALObjectType
  {
    enum Enum
    {
      BlendState,
      DepthStencilState,
      RasterizerState,
      SamplerState,
      Shader,
      Buffer,
      DynamicBuffer,
      Texture,
      ReadbackTexture,
      ReadbackBuffer,
      RenderTargetView,
      SwapChain,
      VertexDeclaration,
      BindGroupLayout,
      BindGroup,
      PipelineLayout,
      GraphicsPipeline,
      ComputePipeline,
    };
  };

  static_assert(sizeof(WGALBlendStateHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALDepthStencilStateHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALRasterizerStateHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALSamplerStateHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALShaderHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALBufferHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALTextureHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALRenderTargetViewHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALSwapChainHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALVertexDeclarationHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALBindGroupLayoutHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALBindGroupHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALPipelineLayoutHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALGraphicsPipelineHandle) == sizeof(WUInt32));
  static_assert(sizeof(WGALComputePipelineHandle) == sizeof(WUInt32));
} // namespace

WGALDevice* WGALDevice::s_pDefaultDevice = nullptr;
WEvent<const WGALDeviceEvent&, WMutex> WGALDevice::s_Events;
WEvent<const WGALSwapChain*, WMutex> WGALDevice::s_SwapChainUpdatedEvent;

WGALDevice::WGALDevice(const WGALDeviceCreationDescription& desc)
  : m_Allocator("GALDevice", WFoundation::GetDefaultAllocator())
  , m_AllocatorWrapper(&m_Allocator)
  , m_Description(desc)
{
  m_BindGroupTracker.m_ResourceInvalidatedEvent.AddEventHandler(WMakeDelegate(&WGALDevice::OnBindGroupInvalidatedEventHandler, this));
}

WGALDevice::~WGALDevice()
{
  m_BindGroupTracker.m_ResourceInvalidatedEvent.RemoveEventHandler(WMakeDelegate(&WGALDevice::OnBindGroupInvalidatedEventHandler, this));
  // Check for object leaks
  {
    W_LOG_BLOCK("WGALDevice object leak report");

    if (!m_Shaders.IsEmpty())
      WLog::Warning("{0} shaders have not been cleaned up", m_Shaders.GetCount());

    if (!m_BlendStates.IsEmpty())
      WLog::Warning("{0} blend states have not been cleaned up", m_BlendStates.GetCount());

    if (!m_DepthStencilStates.IsEmpty())
      WLog::Warning("{0} depth stencil states have not been cleaned up", m_DepthStencilStates.GetCount());

    if (!m_RasterizerStates.IsEmpty())
      WLog::Warning("{0} rasterizer states have not been cleaned up", m_RasterizerStates.GetCount());

    if (!m_Buffers.IsEmpty())
      WLog::Warning("{0} buffers have not been cleaned up", m_Buffers.GetCount());

    if (!m_Textures.IsEmpty())
      WLog::Warning("{0} textures have not been cleaned up", m_Textures.GetCount());

    if (!m_RenderTargetViews.IsEmpty())
      WLog::Warning("{0} render target views have not been cleaned up", m_RenderTargetViews.GetCount());

    if (!m_SwapChains.IsEmpty())
      WLog::Warning("{0} swap chains have not been cleaned up", m_SwapChains.GetCount());

    if (!m_VertexDeclarations.IsEmpty())
      WLog::Warning("{0} vertex declarations have not been cleaned up", m_VertexDeclarations.GetCount());

    if (!m_BindGroupLayouts.IsEmpty())
      WLog::Warning("{0} bind group layouts have not been cleaned up", m_BindGroupLayouts.GetCount());

    if (!m_BindGroups.IsEmpty())
      WLog::Warning("{0} bind groups have not been cleaned up", m_BindGroups.GetCount());

    if (!m_PipelineLayouts.IsEmpty())
      WLog::Warning("{0} pipeline layouts have not been cleaned up", m_PipelineLayouts.GetCount());

    if (!m_GraphicsPipelines.IsEmpty())
      WLog::Warning("{0} graphics pipelines have not been cleaned up", m_GraphicsPipelines.GetCount());

    if (!m_ComputePipelines.IsEmpty())
      WLog::Warning("{0} Compute pipelines have not been cleaned up", m_ComputePipelines.GetCount());
  }
}

WResult WGALDevice::Init()
{
  W_LOG_BLOCK("WGALDevice::Init");

  {
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::BeforeInit;
    s_Events.Broadcast(e);
  }

  WResult PlatformInitResult = InitPlatform();

  if (PlatformInitResult == W_FAILURE)
  {
    return W_FAILURE;
  }

  WGALSharedTextureSwapChain::SetFactoryMethod([this](const WGALSharedTextureSwapChainCreationDescription& desc) -> WGALSwapChainHandle
    { return CreateSwapChain([&desc](WAllocator* pAllocator) -> WGALSwapChain*
        { return W_NEW(pAllocator, WGALSharedTextureSwapChain, desc); }); });

  // Fill the capabilities
  FillCapabilitiesPlatform();

  WLog::Info("Adapter: '{}' - {} VRAM, {} Sys RAM, {} Shared RAM", m_Capabilities.m_sAdapterName, WArgFileSize(m_Capabilities.m_uiDedicatedVRAM),
    WArgFileSize(m_Capabilities.m_uiDedicatedSystemRAM), WArgFileSize(m_Capabilities.m_uiSharedSystemRAM));

  if (!m_Capabilities.m_bHardwareAccelerated)
  {
    WLog::Warning("Selected graphics adapter has no hardware acceleration.");
  }

  W_GALDEVICE_LOCK_AND_CHECK();

  WProfilingSystem::InitializeGPUData();

  {
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::AfterInit;
    s_Events.Broadcast(e);
  }

  return W_SUCCESS;
}

WResult WGALDevice::Shutdown()
{
  W_GALDEVICE_LOCK_AND_CHECK();

  W_LOG_BLOCK("WGALDevice::Shutdown");

  {
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::BeforeShutdown;
    s_Events.Broadcast(e);
  }

  DestroyDeadObjects();

  // make sure we are not listed as the default device anymore
  if (WGALDevice::HasDefaultDevice() && WGALDevice::GetDefaultDevice() == this)
  {
    WGALDevice::SetDefaultDevice(nullptr);
  }

  W_ASSERT_DEBUG(m_uiShaders == 0, "Error in counting deduplicated GAL resources");
  W_ASSERT_DEBUG(m_uiVertexDeclarations == 0, "Error in counting deduplicated GAL resources");
  W_ASSERT_DEBUG(m_uiBlendStates == 0, "Error in counting deduplicated GAL resources");
  W_ASSERT_DEBUG(m_uiDepthStencilStates == 0, "Error in counting deduplicated GAL resources");
  W_ASSERT_DEBUG(m_uiRasterizerStates == 0, "Error in counting deduplicated GAL resources");
  W_ASSERT_DEBUG(m_uiSamplerStates == 0, "Error in counting deduplicated GAL resources");
  W_ASSERT_DEBUG(m_uiBindGroupLayouts == 0, "Error in counting deduplicated GAL resources");
  W_ASSERT_DEBUG(m_uiBindGroups == 0, "Error in counting deduplicated GAL resources");
  W_ASSERT_DEBUG(m_uiPipelineLayouts == 0, "Error in counting deduplicated GAL resources");
  W_ASSERT_DEBUG(m_uiGraphicsPipelines == 0, "Error in counting deduplicated GAL resources");
  W_ASSERT_DEBUG(m_uiComputePipelines == 0, "Error in counting deduplicated GAL resources");

  WResult res = ShutdownPlatform();

  {
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::AfterShutdown;
    s_Events.Broadcast(e);
  }

  return res;
}

WStringView WGALDevice::GetRenderer()
{
  return GetRendererPlatform();
}

WGALCommandEncoder* WGALDevice::BeginCommands(const char* szName)
{
  {
    W_PROFILE_SCOPE("BeforeBeginCommands");
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::BeforeBeginCommands;
    s_Events.Broadcast(e, 1);
  }
  {
    W_GALDEVICE_LOCK_AND_CHECK();
    W_ASSERT_DEV(m_bBeginFrameCalled, "BeginCommands is only allowed to be called within a BeginFrame / EndFrame scope");
    W_ASSERT_DEV(m_pCommandEncoder == nullptr, "Nested Passes are not allowed: You must call WGALDevice::EndCommands before you can call WGALDevice::BeginCommands again");
    m_pCommandEncoder = BeginCommandsPlatform(szName);
  }
  {
    W_PROFILE_SCOPE("AfterBeginCommands");
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::AfterBeginCommands;
    e.m_pCommandEncoder = m_pCommandEncoder;
    s_Events.Broadcast(e, 1);
  }
  return m_pCommandEncoder;
}

void WGALDevice::EndCommands(WGALCommandEncoder* pCommandEncoder)
{
  {
    W_PROFILE_SCOPE("BeforeEndCommands");
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::BeforeEndCommands;
    e.m_pCommandEncoder = pCommandEncoder;
    s_Events.Broadcast(e, 1);
  }
  {
    W_GALDEVICE_LOCK_AND_CHECK();
    W_ASSERT_DEV(m_pCommandEncoder != nullptr, "You must have called WGALDevice::BeginCommands before you can call WGALDevice::EndCommands");
    m_EncoderStats += m_pCommandEncoder->GetStats();
    m_pCommandEncoder->ResetStats();

    m_pCommandEncoder = nullptr;
    EndCommandsPlatform(pCommandEncoder);
  }
  {
    W_PROFILE_SCOPE("AfterEndCommands");
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::AfterEndCommands;
    s_Events.Broadcast(e, 1);
  }
}

template <typename Handle, typename Resource, typename Table, typename CacheTable, typename HashType>
Handle WGALDevice::TryGetHashedResource(HashType uiHash, Table& table, CacheTable& cacheTable, WUInt32 galObjectType, WUInt32& ref_uiCounter)
{
  Handle hResource;
  if (cacheTable.TryGetValue(uiHash, hResource))
  {
    Resource* pResource = table[hResource];
    if (pResource->GetRefCount() == 0)
    {
      ReviveDeadObject(galObjectType, hResource);
    }

    pResource->AddRef();
    ref_uiCounter++;
    return hResource;
  }
  return {};
}

template <typename Handle, typename Resource, typename Table, typename CacheTable, typename HashType>
Handle WGALDevice::InsertHashedResource(HashType uiHash, Resource* pResource, Table& table, CacheTable& cacheTable, WUInt32& ref_uiCounter)
{
  if (pResource != nullptr)
  {
    W_ASSERT_DEBUG(pResource->GetDescription().CalculateHash() == uiHash, "Resource hash doesn't match");

    pResource->AddRef();
    ref_uiCounter++;

    Handle hResource(table.Insert(pResource));
    cacheTable.Insert(uiHash, hResource);

    return hResource;
  }

  return Handle();
}

template <typename Resource, typename Handle, typename Table>
void WGALDevice::DestroyHashedResource(Handle& inout_hResource, Table& table, WUInt32 galObjectType, WUInt32& ref_uiCounter)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  Resource* pResource = nullptr;
  if (table.TryGetValue(inout_hResource, pResource))
  {
    pResource->ReleaseRef();
    ref_uiCounter--;

    if (pResource->GetRefCount() == 0)
    {
      AddDeadObject(galObjectType, inout_hResource);
    }
  }

  inout_hResource.Invalidate();
}

void WGALDevice::SetTextureQualityMode(WGALTextureQualitySlot::Enum slot, WGALTextureQuality::Enum quality)
{
  W_ASSERT_DEV(slot < 5, "Invalid WGALTextureQualitySlot value used: {}", (int)slot);
  m_QualityModes[slot] = quality;
}

void WGALDevice::AdjustSamplerStateDescription(WGALSamplerStateCreationDescription& inout_desc)
{
  if (inout_desc.m_useTextureQualitySlot == WGALTextureQualitySlot::None)
    return;

  switch (m_QualityModes[inout_desc.m_useTextureQualitySlot])
  {
    case WGALTextureQuality::Nearest:
      inout_desc.m_MinFilter = WGALTextureFilterMode::Point;
      inout_desc.m_MagFilter = WGALTextureFilterMode::Point;
      inout_desc.m_MipFilter = WGALTextureFilterMode::Point;
      inout_desc.m_uiMaxAnisotropy = 1;
      break;

    case WGALTextureQuality::Bilinear:
      inout_desc.m_MinFilter = WGALTextureFilterMode::Linear;
      inout_desc.m_MagFilter = WGALTextureFilterMode::Linear;
      inout_desc.m_MipFilter = WGALTextureFilterMode::Point;
      inout_desc.m_uiMaxAnisotropy = 1;
      break;

    case WGALTextureQuality::Trilinear:
      inout_desc.m_MinFilter = WGALTextureFilterMode::Linear;
      inout_desc.m_MagFilter = WGALTextureFilterMode::Linear;
      inout_desc.m_MipFilter = WGALTextureFilterMode::Linear;
      inout_desc.m_uiMaxAnisotropy = 1;
      break;

    case WGALTextureQuality::Anisotropic2x:
      inout_desc.m_MinFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_MagFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_MipFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_uiMaxAnisotropy = 2;
      break;

    case WGALTextureQuality::Anisotropic4x:
      inout_desc.m_MinFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_MagFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_MipFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_uiMaxAnisotropy = 4;
      break;

    case WGALTextureQuality::Anisotropic8x:
      inout_desc.m_MinFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_MagFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_MipFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_uiMaxAnisotropy = 8;
      break;

    case WGALTextureQuality::Anisotropic16x:
      inout_desc.m_MinFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_MagFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_MipFilter = WGALTextureFilterMode::Anisotropic;
      inout_desc.m_uiMaxAnisotropy = 16;
      break;
  }
}

void WGALDevice::UpdateTextureQuality()
{
  for (auto it = m_SamplerStates.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetDescription().m_useTextureQualitySlot != WGALTextureQualitySlot::None)
    {
      RecreateSamplerStatePlatform(it.Value());
    }
  }

  // Sampler states are not ref counted and expected to exist forever. If changed, anything referencing samplers must be destroyed, Right now, this is only bind groups as the WGALImmutableSamplers assert that no quality slot is set on registration.
  for (auto it = m_BindGroups.GetIterator(); it.IsValid(); ++it)
  {
    RecreateBindGroupPlatform(it.Value());
  }
}

WGALBlendStateHandle WGALDevice::CreateBlendState(const WGALBlendStateCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const WUInt32 uiHash = desc.CalculateHash();

  if (WGALBlendStateHandle hBlendState = TryGetHashedResource<WGALBlendStateHandle, WGALBlendState>(uiHash, m_BlendStates, m_BlendStateTable, GALObjectType::BlendState, m_uiBlendStates); !hBlendState.IsInvalidated())
    return hBlendState;

  WGALBlendState* pBlendState = CreateBlendStatePlatform(desc);
  return InsertHashedResource<WGALBlendStateHandle>(uiHash, pBlendState, m_BlendStates, m_BlendStateTable, m_uiBlendStates);
}

void WGALDevice::DestroyBlendState(WGALBlendStateHandle& inout_hBlendState)
{
  DestroyHashedResource<WGALBlendState>(inout_hBlendState, m_BlendStates, GALObjectType::BlendState, m_uiBlendStates);
}

WGALDepthStencilStateHandle WGALDevice::CreateDepthStencilState(const WGALDepthStencilStateCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const WUInt32 uiHash = desc.CalculateHash();

  if (WGALDepthStencilStateHandle hDepthStencilState = TryGetHashedResource<WGALDepthStencilStateHandle, WGALDepthStencilState>(uiHash, m_DepthStencilStates, m_DepthStencilStateTable, GALObjectType::DepthStencilState, m_uiDepthStencilStates); !hDepthStencilState.IsInvalidated())
    return hDepthStencilState;

  WGALDepthStencilState* pDepthStencilState = CreateDepthStencilStatePlatform(desc);
  return InsertHashedResource<WGALDepthStencilStateHandle>(uiHash, pDepthStencilState, m_DepthStencilStates, m_DepthStencilStateTable, m_uiDepthStencilStates);
}

void WGALDevice::DestroyDepthStencilState(WGALDepthStencilStateHandle& inout_hDepthStencilState)
{
  DestroyHashedResource<WGALDepthStencilState>(inout_hDepthStencilState, m_DepthStencilStates, GALObjectType::DepthStencilState, m_uiDepthStencilStates);
}

WGALRasterizerStateHandle WGALDevice::CreateRasterizerState(const WGALRasterizerStateCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const WUInt32 uiHash = desc.CalculateHash();

  if (WGALRasterizerStateHandle hRasterizerState = TryGetHashedResource<WGALRasterizerStateHandle, WGALRasterizerState>(uiHash, m_RasterizerStates, m_RasterizerStateTable, GALObjectType::RasterizerState, m_uiRasterizerStates); !hRasterizerState.IsInvalidated())
    return hRasterizerState;

  WGALRasterizerState* pRasterizerState = CreateRasterizerStatePlatform(desc);
  return InsertHashedResource<WGALRasterizerStateHandle>(uiHash, pRasterizerState, m_RasterizerStates, m_RasterizerStateTable, m_uiRasterizerStates);
}

void WGALDevice::DestroyRasterizerState(WGALRasterizerStateHandle& inout_hRasterizerState)
{
  DestroyHashedResource<WGALRasterizerState>(inout_hRasterizerState, m_RasterizerStates, GALObjectType::RasterizerState, m_uiRasterizerStates);
}

WGALSamplerStateHandle WGALDevice::CreateSamplerState(const WGALSamplerStateCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const WUInt32 uiHash = desc.CalculateHash();

  if (WGALSamplerStateHandle hSamplerState = TryGetHashedResource<WGALSamplerStateHandle, WGALSamplerState>(uiHash, m_SamplerStates, m_SamplerStateTable, GALObjectType::SamplerState, m_uiSamplerStates); !hSamplerState.IsInvalidated())
    return hSamplerState;

  WGALSamplerState* pSamplerState = CreateSamplerStatePlatform(desc);
  return InsertHashedResource<WGALSamplerStateHandle>(uiHash, pSamplerState, m_SamplerStates, m_SamplerStateTable, m_uiSamplerStates);
}

void WGALDevice::DestroySamplerState(WGALSamplerStateHandle& inout_hSamplerState)
{
  DestroyHashedResource<WGALSamplerState>(inout_hSamplerState, m_SamplerStates, GALObjectType::SamplerState, m_uiSamplerStates);
}

WGALBindGroupLayoutHandle WGALDevice::CreateBindGroupLayout(const WGALBindGroupLayoutCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const WUInt32 uiHash = desc.CalculateHash();

  if (WGALBindGroupLayoutHandle hBindGroupLayout = TryGetHashedResource<WGALBindGroupLayoutHandle, WGALBindGroupLayout>(uiHash, m_BindGroupLayouts, m_BindGroupLayoutTable, GALObjectType::BindGroupLayout, m_uiBindGroupLayouts); !hBindGroupLayout.IsInvalidated())
    return hBindGroupLayout;

  WGALBindGroupLayout* pBindGroupLayout = CreateBindGroupLayoutPlatform(desc);
  return InsertHashedResource<WGALBindGroupLayoutHandle>(uiHash, pBindGroupLayout, m_BindGroupLayouts, m_BindGroupLayoutTable, m_uiBindGroupLayouts);
}

void WGALDevice::DestroyBindGroupLayout(WGALBindGroupLayoutHandle& inout_hBindGroupLayout)
{
  DestroyHashedResource<WGALBindGroupLayout>(inout_hBindGroupLayout, m_BindGroupLayouts, GALObjectType::BindGroupLayout, m_uiBindGroupLayouts);
}

WGALBindGroupHandle WGALDevice::CreateBindGroup(const WGALBindGroupCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const WUInt64 uiHash = desc.CalculateHash();

  if (WGALBindGroupHandle hBindGroup = TryGetHashedResource<WGALBindGroupHandle, WGALBindGroup>(uiHash, m_BindGroups, m_BindGroupTable, GALObjectType::BindGroup, m_uiBindGroups); !hBindGroup.IsInvalidated())
    return hBindGroup;

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  {
    W_LOG_BLOCK("CreateBindGroup");
    desc.AssertValidDescription(*this);
  }
#endif

  WGALBindGroup* pBindGroup = CreateBindGroupPlatform(desc);

  const WGALBindGroupCreationDescription& desc2 = pBindGroup->GetDescription();
  W_ASSERT_DEBUG(desc.m_hBindGroupLayout == desc2.m_hBindGroupLayout, "");
  W_ASSERT_DEBUG(desc.m_BindGroupItems.GetCount() == desc2.m_BindGroupItems.GetCount(), "");

  W_ASSERT_DEBUG(desc.CalculateHash() == desc2.CalculateHash(), "");
  for (WUInt32 i = 0; i < desc2.m_BindGroupItems.GetCount(); ++i)
  {
    W_ASSERT_DEBUG(desc.m_BindGroupItems[i] == desc2.m_BindGroupItems[i], "");
  }


  {
    WSet<const WGALResourceBase*> dependencies;
    const WGALBindGroupLayout* pLayout = GetBindGroupLayout(desc.m_hBindGroupLayout);
    WArrayPtr<const WShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
    WArrayPtr<const WGALBindGroupItem> items = desc.m_BindGroupItems;
    const WUInt32 uiBindings = bindings.GetCount();
    for (WUInt32 i = 0; i < uiBindings; ++i)
    {
      const WShaderResourceBinding& binding = bindings[i];
      const WGALBindGroupItem& item = items[i];
      switch (binding.m_ResourceType)
      {
        case WGALShaderResourceType::Sampler:
        {
          const WGALSamplerState* pSampler = GetSamplerState(item.m_Sampler.m_hSampler);
          dependencies.Insert(pSampler);
        }
        break;
        case WGALShaderResourceType::ConstantBuffer:
        case WGALShaderResourceType::TexelBuffer:
        case WGALShaderResourceType::StructuredBuffer:
        case WGALShaderResourceType::ByteAddressBuffer:
        case WGALShaderResourceType::TexelBufferRW:
        case WGALShaderResourceType::StructuredBufferRW:
        case WGALShaderResourceType::ByteAddressBufferRW:
        {
          const WGALBuffer* pBuffer = GetBuffer(item.m_Buffer.m_hBuffer);
          dependencies.Insert(pBuffer);
        }
        break;
        case WGALShaderResourceType::TextureAndSampler:
        {
          const WGALSamplerState* pSampler = GetSamplerState(item.m_Texture.m_hSampler);
          dependencies.Insert(pSampler);
        }
          [[fallthrough]];
        case WGALShaderResourceType::Texture:
        case WGALShaderResourceType::TextureRW:
        {
          const WGALTexture* pTexture = GetTexture(item.m_Texture.m_hTexture);
          dependencies.Insert(pTexture);
        }
        break;

        default:
          W_REPORT_FAILURE("Unsupported shader resource type in bind group");
          break;
      }
    }

    m_BindGroupTracker.AddResource(pBindGroup, dependencies);
  }


  return InsertHashedResource<WGALBindGroupHandle>(uiHash, pBindGroup, m_BindGroups, m_BindGroupTable, m_uiBindGroups);
}

void WGALDevice::DestroyBindGroup(WGALBindGroupHandle& inout_hBindGroup)
{
  DestroyHashedResource<WGALBindGroup>(inout_hBindGroup, m_BindGroups, GALObjectType::BindGroup, m_uiBindGroups);
}

WGALPipelineLayoutHandle WGALDevice::CreatePipelineLayout(const WGALPipelineLayoutCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const WUInt32 uiHash = desc.CalculateHash();

  if (WGALPipelineLayoutHandle hPipelineLayout = TryGetHashedResource<WGALPipelineLayoutHandle, WGALPipelineLayout>(uiHash, m_PipelineLayouts, m_PipelineLayoutTable, GALObjectType::PipelineLayout, m_uiPipelineLayouts); !hPipelineLayout.IsInvalidated())
    return hPipelineLayout;

  WGALPipelineLayout* pPipelineLayout = CreatePipelineLayoutPlatform(desc);
  return InsertHashedResource<WGALPipelineLayoutHandle>(uiHash, pPipelineLayout, m_PipelineLayouts, m_PipelineLayoutTable, m_uiPipelineLayouts);
}

void WGALDevice::DestroyPipelineLayout(WGALPipelineLayoutHandle& inout_hPipelineLayout)
{
  DestroyHashedResource<WGALPipelineLayout>(inout_hPipelineLayout, m_PipelineLayouts, GALObjectType::PipelineLayout, m_uiPipelineLayouts);
}

WGALGraphicsPipelineHandle WGALDevice::CreateGraphicsPipeline(const WGALGraphicsPipelineCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  auto IncreaseReference = [&](const auto& idTable, WUInt32& ref_uiCount, auto handle, GALObjectType::Enum type)
  {
    auto pRes = idTable[handle];
    if (pRes->GetRefCount() == 0)
    {
      ReviveDeadObject(type, handle);
    }
    pRes->AddRef();
    ref_uiCount++;
  };

  // Hash desc and return potential existing one (including inc. refcount)
  WUInt32 uiHash = desc.CalculateHash();
  {
    WGALGraphicsPipelineHandle hGraphicsPipeline;
    if (m_GraphicsPipelineTable.TryGetValue(uiHash, hGraphicsPipeline))
    {
      WGALGraphicsPipeline* pGraphicsPipeline = m_GraphicsPipelines[hGraphicsPipeline];
      if (pGraphicsPipeline->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::GraphicsPipeline, hGraphicsPipeline);
        IncreaseReference(m_Shaders, m_uiShaders, desc.m_hShader, GALObjectType::Shader);
        IncreaseReference(m_RasterizerStates, m_uiRasterizerStates, desc.m_hRasterizerState, GALObjectType::RasterizerState);
        IncreaseReference(m_BlendStates, m_uiBlendStates, desc.m_hBlendState, GALObjectType::BlendState);
        IncreaseReference(m_DepthStencilStates, m_uiDepthStencilStates, desc.m_hDepthStencilState, GALObjectType::DepthStencilState);
        if (!desc.m_hVertexDeclaration.IsInvalidated())
        {
          IncreaseReference(m_VertexDeclarations, m_uiVertexDeclarations, desc.m_hVertexDeclaration, GALObjectType::VertexDeclaration);
        }
      }

      pGraphicsPipeline->AddRef();
      m_uiGraphicsPipelines++;
      return hGraphicsPipeline;
    }
  }

  if (desc.m_hBlendState.IsInvalidated() || desc.m_hDepthStencilState.IsInvalidated() || desc.m_hRasterizerState.IsInvalidated() || desc.m_hShader.IsInvalidated())
  {
    WLog::Error("An essential handle was invalid. Only the m_hVertexDeclaration handle can be invalid.");
    return {};
  }

  WGALGraphicsPipeline* pGraphicsPipeline = CreateGraphicsPipelinePlatform(desc);

  if (pGraphicsPipeline != nullptr)
  {
    W_ASSERT_DEBUG(pGraphicsPipeline->GetDescription().CalculateHash() == uiHash, "GraphicsPipeline hash doesn't match");

    pGraphicsPipeline->AddRef();
    m_uiGraphicsPipelines++;

    WGALGraphicsPipelineHandle hGraphicsPipeline(m_GraphicsPipelines.Insert(pGraphicsPipeline));
    m_GraphicsPipelineTable.Insert(uiHash, hGraphicsPipeline);

    IncreaseReference(m_Shaders, m_uiShaders, desc.m_hShader, GALObjectType::Shader);
    IncreaseReference(m_RasterizerStates, m_uiRasterizerStates, desc.m_hRasterizerState, GALObjectType::RasterizerState);
    IncreaseReference(m_BlendStates, m_uiBlendStates, desc.m_hBlendState, GALObjectType::BlendState);
    IncreaseReference(m_DepthStencilStates, m_uiDepthStencilStates, desc.m_hDepthStencilState, GALObjectType::DepthStencilState);
    if (!desc.m_hVertexDeclaration.IsInvalidated())
    {
      IncreaseReference(m_VertexDeclarations, m_uiVertexDeclarations, desc.m_hVertexDeclaration, GALObjectType::VertexDeclaration);
    }

    return hGraphicsPipeline;
  }

  return WGALGraphicsPipelineHandle();
}

void WGALDevice::DestroyGraphicsPipeline(WGALGraphicsPipelineHandle& inout_hGraphicsPipeline)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALGraphicsPipeline* pGraphicsPipeline = nullptr;
  if (m_GraphicsPipelines.TryGetValue(inout_hGraphicsPipeline, pGraphicsPipeline))
  {
    pGraphicsPipeline->ReleaseRef();
    m_uiGraphicsPipelines--;

    if (pGraphicsPipeline->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::GraphicsPipeline, inout_hGraphicsPipeline);
      const WGALGraphicsPipelineCreationDescription& desc = pGraphicsPipeline->GetDescription();

      auto hShader = desc.m_hShader;
      auto hRasterizerState = desc.m_hRasterizerState;
      auto hBlendState = desc.m_hBlendState;
      auto hDepthStencilState = desc.m_hDepthStencilState;
      auto hVertexDeclaration = desc.m_hVertexDeclaration;

      DestroyShader(hShader);
      DestroyRasterizerState(hRasterizerState);
      DestroyBlendState(hBlendState);
      DestroyDepthStencilState(hDepthStencilState);
      DestroyVertexDeclaration(hVertexDeclaration);
    }
  }

  inout_hGraphicsPipeline.Invalidate();
}


WGALComputePipelineHandle WGALDevice::CreateComputePipeline(const WGALComputePipelineCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  auto IncreaseReference = [&](const auto& idTable, WUInt32& ref_uiCount, auto handle, GALObjectType::Enum type)
  {
    auto pRes = idTable[handle];
    if (pRes->GetRefCount() == 0)
    {
      ReviveDeadObject(type, handle);
    }
    pRes->AddRef();
    ref_uiCount++;
  };

  // Hash desc and return potential existing one (including inc. refcount)
  WUInt32 uiHash = desc.CalculateHash();
  {
    WGALComputePipelineHandle hComputePipeline;
    if (m_ComputePipelineTable.TryGetValue(uiHash, hComputePipeline))
    {
      WGALComputePipeline* pComputePipeline = m_ComputePipelines[hComputePipeline];
      if (pComputePipeline->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::ComputePipeline, hComputePipeline);
        IncreaseReference(m_Shaders, m_uiShaders, desc.m_hShader, GALObjectType::Shader);
      }

      pComputePipeline->AddRef();
      m_uiComputePipelines++;
      return hComputePipeline;
    }
  }

  if (desc.m_hShader.IsInvalidated())
  {
    WLog::Error("Shader handle must be valid.");
    return {};
  }

  WGALComputePipeline* pComputePipeline = CreateComputePipelinePlatform(desc);

  if (pComputePipeline != nullptr)
  {
    W_ASSERT_DEBUG(pComputePipeline->GetDescription().CalculateHash() == uiHash, "ComputePipeline hash doesn't match");

    pComputePipeline->AddRef();
    m_uiComputePipelines++;

    WGALComputePipelineHandle hComputePipeline(m_ComputePipelines.Insert(pComputePipeline));
    m_ComputePipelineTable.Insert(uiHash, hComputePipeline);
    IncreaseReference(m_Shaders, m_uiShaders, desc.m_hShader, GALObjectType::Shader);

    return hComputePipeline;
  }

  return WGALComputePipelineHandle();
}

void WGALDevice::DestroyComputePipeline(WGALComputePipelineHandle& inout_hComputePipeline)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALComputePipeline* pComputePipeline = nullptr;
  if (m_ComputePipelines.TryGetValue(inout_hComputePipeline, pComputePipeline))
  {
    pComputePipeline->ReleaseRef();
    m_uiComputePipelines--;

    if (pComputePipeline->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::ComputePipeline, inout_hComputePipeline);
      const WGALComputePipelineCreationDescription& desc = pComputePipeline->GetDescription();

      auto hShader = desc.m_hShader;
      DestroyShader(hShader);
    }
  }
  else
  {
    WLog::Warning("DestroyComputePipeline called on invalid handle (double free?)");
  }
}

WGALShaderHandle WGALDevice::CreateShader(const WGALShaderCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  bool bHasByteCodes = false;

  for (WUInt32 uiStage = 0; uiStage < WGALShaderStage::ENUM_COUNT; uiStage++)
  {
    if (desc.HasByteCodeForStage((WGALShaderStage::Enum)uiStage))
    {
      bHasByteCodes = true;
      break;
    }
  }

  if (!bHasByteCodes)
  {
    WLog::Error("Can't create a shader which supplies no bytecodes at all!");
    return WGALShaderHandle();
  }

  // Hash desc and return potential existing one (including inc. refcount)
  WUInt32 uiHash = desc.CalculateHash();

  {
    WGALShaderHandle hShader;
    if (m_ShaderTable.TryGetValue(uiHash, hShader))
    {
      WGALShader* pShader = m_Shaders[hShader];
      if (pShader->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::Shader, hShader);
      }

      pShader->AddRef();
      m_uiShaders++;
      return hShader;
    }
  }

  WGALShader* pShader = CreateShaderPlatform(desc);

  if (pShader != nullptr)
  {
    W_ASSERT_DEBUG(pShader->GetDescription().CalculateHash() == uiHash, "Shader hash doesn't match");

    pShader->AddRef();
    m_uiShaders++;

    WGALShaderHandle hShader(m_Shaders.Insert(pShader));
    m_ShaderTable.Insert(uiHash, hShader);

    return hShader;
  }

  return WGALShaderHandle();
}

void WGALDevice::DestroyShader(WGALShaderHandle& inout_hShader)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALShader* pShader = nullptr;
  if (m_Shaders.TryGetValue(inout_hShader, pShader))
  {
    pShader->ReleaseRef();
    m_uiShaders--;

    if (pShader->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::Shader, inout_hShader);
    }
  }

  inout_hShader.Invalidate();
}


WGALBufferHandle WGALDevice::CreateBuffer(const WGALBufferCreationDescription& desc, WArrayPtr<const WUInt8> initialData)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  if (desc.m_uiTotalSize == 0)
  {
    WLog::Error("Trying to create a buffer with size of 0 is not possible!");
    return WGALBufferHandle();
  }

  if (desc.m_ResourceAccess.IsImmutable() && initialData.IsEmpty())
  {
    WLog::Error("Trying to create an immutable buffer but not supplying initial data is not possible!");
    return WGALBufferHandle();
  }

  WUInt32 uiBufferSize = desc.m_uiTotalSize;
  if (initialData.GetCount() > 0 && uiBufferSize != initialData.GetCount())
  {
    WLog::Error("Trying to create a buffer with invalid initial data.");
    return {};
  }

  if (desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::Transient) && !initialData.IsEmpty())
  {
    WLog::Error("Transient buffers cannot have initial data.");
    return {};
  }
  if (desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::IndexBuffer) && desc.m_uiStructSize != 2 && desc.m_uiStructSize != 4)
  {
    WLog::Error("IndexBuffer struct size must be either 2 or 4 but {} is set.", desc.m_uiStructSize);
  }
  if (desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::TexelBuffer) && desc.m_Format == WGALResourceFormat::Invalid)
  {
    WLog::Error("Texel buffers must have a valid m_Format set.");
    return {};
  }
  if (!desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::TexelBuffer) && desc.m_Format != WGALResourceFormat::Invalid)
  {
    WLog::Error("m_Format is only allowed if TexelBuffer flag is set.");
    return {};
  }
  if (desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::TexelBuffer) && (desc.m_uiTotalSize % (WGALResourceFormat::GetBitsPerElement(desc.m_Format) / 8)) != 0)
  {
    WLog::Error("TexelBuffer with format {} must have a size multiple of {}, but size is {}.", (WUInt32)desc.m_Format, WGALResourceFormat::GetBitsPerElement(desc.m_Format) / 8, desc.m_uiTotalSize);
    return {};
  }

  if (desc.m_BufferFlags.IsAnySet(WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::VertexBuffer | WGALBufferUsageFlags::IndexBuffer) && desc.m_uiStructSize == 0)
  {
    WLog::Error("m_uiStructSize must be != 0 if StructuredBuffer, IndexBuffer or VertexBuffer flag is set.");
    return {};
  }
  if (desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::StructuredBuffer) && (desc.m_uiTotalSize % desc.m_uiStructSize) != 0)
  {
    WLog::Error("StructuredBuffer must have a size multiple of m_uiStructSize {}, but size is {}.", desc.m_uiStructSize, desc.m_uiTotalSize);
    return {};
  }
  if (!m_Capabilities.m_bSupportsTexelBuffer && desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::TexelBuffer))
  {
    WLog::Error("TexelBuffer flag is not supported on this platform.");
    return {};
  }

  /// \todo Platform independent validation (buffer type supported)

  WGALBuffer* pBuffer = CreateBufferPlatform(desc, initialData);

  WGALBufferHandle hBuffer(m_Buffers.Insert(pBuffer));
  return hBuffer;
}

void WGALDevice::DestroyBuffer(WGALBufferHandle& inout_hBuffer)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALBuffer* pBuffer = nullptr;
  if (m_Buffers.TryGetValue(inout_hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::Buffer, inout_hBuffer);
  }

  inout_hBuffer.Invalidate();
}

WGALDynamicBufferHandle WGALDevice::CreateDynamicBuffer(const WGALBufferCreationDescription& description, WStringView sDebugName)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  auto pBuffer = W_NEW(&m_Allocator, WGALDynamicBuffer);
  pBuffer->Initialize(description, sDebugName);

  return WGALDynamicBufferHandle(m_DynamicBuffers.Insert(pBuffer));
}

void WGALDevice::DestroyDynamicBuffer(WGALDynamicBufferHandle& inout_hBuffer)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALDynamicBuffer* pBuffer = nullptr;
  if (m_DynamicBuffers.TryGetValue(inout_hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::DynamicBuffer, inout_hBuffer);
  }

  inout_hBuffer.Invalidate();
}

// Helper functions for buffers (for common, simple use cases)
WGALBufferHandle WGALDevice::CreateVertexBuffer(WUInt32 uiVertexSize, WUInt32 uiVertexCount, WArrayPtr<const WUInt8> initialData, bool bDataIsMutable /*= false */)
{
  WGALBufferCreationDescription desc;
  desc.m_uiStructSize = uiVertexSize;
  desc.m_uiTotalSize = uiVertexSize * WMath::Max(1u, uiVertexCount);
  desc.m_BufferFlags = WGALBufferUsageFlags::VertexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !initialData.IsEmpty() && !bDataIsMutable;

  return CreateBuffer(desc, initialData);
}

WGALBufferHandle WGALDevice::CreateIndexBuffer(WGALIndexType::Enum indexType, WUInt32 uiIndexCount, WArrayPtr<const WUInt8> initialData, bool bDataIsMutable /*= false*/)
{
  WGALBufferCreationDescription desc;
  desc.m_uiStructSize = WGALIndexType::GetSize(indexType);
  desc.m_uiTotalSize = desc.m_uiStructSize * WMath::Max(1u, uiIndexCount);
  desc.m_BufferFlags = WGALBufferUsageFlags::IndexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !bDataIsMutable && !initialData.IsEmpty();

  return CreateBuffer(desc, initialData);
}

WGALBufferHandle WGALDevice::CreateConstantBuffer(WUInt32 uiBufferSize)
{
  WGALBufferCreationDescription desc;
  desc.m_uiStructSize = 0;
  desc.m_uiTotalSize = uiBufferSize;
  desc.m_BufferFlags = WGALBufferUsageFlags::ConstantBuffer | WGALBufferUsageFlags::Transient;
  desc.m_ResourceAccess.m_bImmutable = false;

  return CreateBuffer(desc);
}


WGALTextureHandle WGALDevice::CreateTexture(const WGALTextureCreationDescription& desc, WArrayPtr<WGALSystemMemoryDescription> initialData)
{
  W_GALDEVICE_LOCK_AND_CHECK();
  if (desc.Validate(this, initialData).Failed())
  {
    return {};
  }

  if (desc.m_ResourceAccess.IsImmutable())
  {
    if (desc.m_TextureFlags.IsAnySet(WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::UnorderedAccess))
    {
      WLog::Error("m_uiStructSize must be != 0 if StructuredBuffer, IndexBuffer or VertexBuffer flag is set.");
      return {};
    }
  }

  WGALTexture* pTexture = CreateTexturePlatform(desc, initialData);
  return FinalizeTextureInternal(desc, pTexture);
}

WGALTextureHandle WGALDevice::FinalizeTextureInternal(const WGALTextureCreationDescription& desc, WGALTexture* pTexture)
{
  if (pTexture != nullptr)
  {
    WGALTextureHandle hTexture(m_Textures.Insert(pTexture));

    // Create default render target view
    if (desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
    {
      WGALRenderTargetViewCreationDescription rtDesc;
      rtDesc.m_hTexture = hTexture;
      rtDesc.m_uiFirstSlice = 0;
      rtDesc.m_uiSliceCount = desc.m_uiArraySize;
      if (desc.m_Type == WGALTextureType::TextureCube || desc.m_Type == WGALTextureType::TextureCubeArray)
      {
        rtDesc.m_OverrideViewType = WGALTextureType::Texture2DArray;
      }

      pTexture->m_hDefaultRenderTargetView = GetRenderTargetView(rtDesc);
    }

    return hTexture;
  }

  return WGALTextureHandle();
}

void WGALDevice::DestroyTexture(WGALTextureHandle& inout_hTexture)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(inout_hTexture, pTexture))
  {
    AddDeadObject(GALObjectType::Texture, inout_hTexture);
  }

  inout_hTexture.Invalidate();
}

WGALTextureHandle WGALDevice::CreateProxyTexture(WGALTextureHandle hParentTexture, WUInt32 uiSlice)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALTexture* pParentTexture = nullptr;

  if (!hParentTexture.IsInvalidated())
  {
    pParentTexture = Get<TextureTable, WGALTexture>(hParentTexture, m_Textures);
  }

  if (pParentTexture == nullptr)
  {
    WLog::Error("No valid texture handle given for proxy texture creation!");
    return WGALTextureHandle();
  }

  const auto& parentDesc = pParentTexture->GetDescription();
  W_IGNORE_UNUSED(parentDesc);
  W_ASSERT_DEV(parentDesc.m_Type == WGALTextureType::TextureCube || parentDesc.m_Type == WGALTextureType::Texture2DArray || parentDesc.m_Type == WGALTextureType::TextureCubeArray, "Proxy textures can only be created for cubemaps or array textures.");
  [[maybe_unused]] const WUInt32 uiSliceCount = parentDesc.m_Type == WGALTextureType::TextureCube || parentDesc.m_Type == WGALTextureType::TextureCubeArray ? parentDesc.m_uiArraySize * 6 : parentDesc.m_uiArraySize;
  W_ASSERT_DEV(uiSlice < uiSliceCount, "Proxy slice {} is out of bounds. Parent texture only has {} slices", uiSlice, uiSliceCount);
  WGALProxyTexture* pProxyTexture = W_NEW(&m_Allocator, WGALProxyTexture, hParentTexture, *pParentTexture, (WUInt16)uiSlice);
  WGALTextureHandle hProxyTexture(m_Textures.Insert(pProxyTexture));

  // Create default render target view
  // if (desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
  {
    WGALRenderTargetViewCreationDescription rtDesc;
    rtDesc.m_hTexture = hProxyTexture;
    rtDesc.m_uiFirstSlice = uiSlice;
    rtDesc.m_uiSliceCount = 1;

    pProxyTexture->m_hDefaultRenderTargetView = GetRenderTargetView(rtDesc);
  }

  return hProxyTexture;
}

void WGALDevice::DestroyProxyTexture(WGALTextureHandle& inout_hProxyTexture)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(inout_hProxyTexture, pTexture))
  {
    W_ASSERT_DEV(pTexture->GetDescription().m_Type == WGALTextureType::Texture2DProxy, "Given texture is not a proxy texture");

    AddDeadObject(GALObjectType::Texture, inout_hProxyTexture);
  }

  inout_hProxyTexture.Invalidate();
}

WGALTextureHandle WGALDevice::CreateSharedTexture(const WGALTextureCreationDescription& desc, WArrayPtr<WGALSystemMemoryDescription> initialData)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (desc.m_ResourceAccess.IsImmutable() && (initialData.IsEmpty() || initialData.GetCount() < desc.m_uiMipLevelCount) &&
      !desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
  {
    WLog::Error("Trying to create an immutable texture but not supplying initial data (or not enough data pointers) is not possible!");
    return WGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    WLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return WGALTextureHandle();
  }

  if (desc.m_pExisitingNativeObject != nullptr)
  {
    WLog::Error("Shared textures cannot be created on exiting native objects!");
    return WGALTextureHandle();
  }

  if (desc.m_Type != WGALTextureType::Texture2DShared)
  {
    WLog::Error("Only WGALTextureType::Texture2DShared is supported for shared textures!");
    return WGALTextureHandle();
  }

  WGALTexture* pTexture = CreateSharedTexturePlatform(desc, initialData, WGALSharedTextureType::Exported, {});

  return FinalizeTextureInternal(desc, pTexture);
}

WGALTextureHandle WGALDevice::OpenSharedTexture(const WGALTextureCreationDescription& desc, WGALPlatformSharedHandle hSharedHandle)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  if (desc.m_pExisitingNativeObject != nullptr)
  {
    WLog::Error("Shared textures cannot be created on exiting native objects!");
    return WGALTextureHandle();
  }

  if (desc.m_Type != WGALTextureType::Texture2DShared)
  {
    WLog::Error("Only WGALTextureType::Texture2DShared is supported for shared textures!");
    return WGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    WLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return WGALTextureHandle();
  }

  WGALTexture* pTexture = CreateSharedTexturePlatform(desc, {}, WGALSharedTextureType::Imported, hSharedHandle);

  return FinalizeTextureInternal(desc, pTexture);
}

void WGALDevice::DestroySharedTexture(WGALTextureHandle& inout_hSharedTexture)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(inout_hSharedTexture, pTexture))
  {
    W_ASSERT_DEV(pTexture->GetDescription().m_Type == WGALTextureType::Texture2DShared, "Given texture is not a shared texture texture");

    AddDeadObject(GALObjectType::Texture, inout_hSharedTexture);
  }

  inout_hSharedTexture.Invalidate();
}

WGALReadbackTextureHandle WGALDevice::CreateReadbackTexture(const WGALTextureCreationDescription& description)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  if (description.m_uiWidth == 0 || description.m_uiHeight == 0)
  {
    WLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return WGALReadbackTextureHandle();
  }

  WGALReadbackTexture* pReadbackTexture = CreateReadbackTexturePlatform(description);
  WGALReadbackTextureHandle hTexture(m_ReadbackTextures.Insert(pReadbackTexture));
  return hTexture;
}

void WGALDevice::DestroyReadbackTexture(WGALReadbackTextureHandle& inout_hTexture)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALReadbackTexture* pTexture = nullptr;
  if (m_ReadbackTextures.TryGetValue(inout_hTexture, pTexture))
  {
    AddDeadObject(GALObjectType::ReadbackTexture, inout_hTexture);
  }

  inout_hTexture.Invalidate();
}

WGALReadbackBufferHandle WGALDevice::CreateReadbackBuffer(const WGALBufferCreationDescription& description)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALReadbackBuffer* pReadbackBuffer = CreateReadbackBufferPlatform(description);
  WGALReadbackBufferHandle hBuffer(m_ReadbackBuffers.Insert(pReadbackBuffer));
  return hBuffer;
}

void WGALDevice::DestroyReadbackBuffer(WGALReadbackBufferHandle& inout_hBuffer)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALReadbackBuffer* pBuffer = nullptr;
  if (m_ReadbackBuffers.TryGetValue(inout_hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::ReadbackBuffer, inout_hBuffer);
  }

  inout_hBuffer.Invalidate();
}

void WGALDevice::UpdateBufferForNextFrame(WGALBufferHandle hBuffer, WConstByteArrayPtr sourceData, WUInt32 uiDestOffset /*= 0*/)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  if (const WGALBuffer* pBuffer = GetBuffer(hBuffer))
  {
    W_ASSERT_DEBUG(!pBuffer->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable buffers");
    if (uiDestOffset + sourceData.GetCount() > pBuffer->GetDescription().m_uiTotalSize)
    {
      WLog::Error("Trying to update buffer outside of its bounds!");
      return;
    }

    UpdateBufferForNextFramePlatform(pBuffer, sourceData, uiDestOffset);
  }
  else
  {
    WLog::Error("No valid buffer handle given to update!");
  }
}

void WGALDevice::UpdateTextureForNextFrame(WGALTextureHandle hTexture, const WGALSystemMemoryDescription& sourceData, const WGALTextureSubresource& destinationSubResource /*= {}*/, const WBoundingBoxu32& destinationBox /*= WBoundingBoxu32::MakeInvalid()*/)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  if (const WGALTexture* pTexture = GetTexture(hTexture))
  {
    auto& desc = pTexture->GetDescription();
    W_ASSERT_DEBUG(!desc.m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
    const WVec3U32 vMipSize = pTexture->GetMipMapSize(destinationSubResource.m_uiMipLevel);
    const bool bDestBoxIsValid = destinationBox.IsValid() && destinationBox.GetExtents().IsZero() == false;
    if (bDestBoxIsValid && (destinationBox.m_vMax.x > vMipSize.x || destinationBox.m_vMax.y > vMipSize.y || destinationBox.m_vMax.z > vMipSize.z))
    {
      WLog::Error("Trying to update texture outside of its bounds!");
      return;
    }

    const WUInt32 uiWidth = bDestBoxIsValid ? WMath::Max(destinationBox.m_vMax.x - destinationBox.m_vMin.x, 1u) : vMipSize.x;
    const WUInt32 uiHeight = bDestBoxIsValid ? WMath::Max(destinationBox.m_vMax.y - destinationBox.m_vMin.y, 1u) : vMipSize.y;
    const WUInt32 uiDepth = bDestBoxIsValid ? WMath::Max(destinationBox.m_vMax.z - destinationBox.m_vMin.z, 1u) : vMipSize.z;

    const bool bBlockCompressed = WGALResourceFormat::IsBlockCompressed(desc.m_Format);
    const WUInt32 uiBlockWidth = bBlockCompressed ? 4u : 1u;
    const WUInt32 uiBlockHeight = bBlockCompressed ? 4u : 1u;
    const WUInt32 uiBlockDepth = 1u;
    const WUInt32 uiBlockCountX = (uiWidth + uiBlockWidth - 1) / uiBlockWidth;
    const WUInt32 uiBlockCountY = (uiHeight + uiBlockHeight - 1) / uiBlockHeight;
    const WUInt32 uiBlockCountZ = (uiDepth + uiBlockDepth - 1) / uiBlockDepth;
    const WUInt32 uiBytesPerBlock = WGALResourceFormat::GetBitsPerElement(desc.m_Format) * uiBlockWidth * uiBlockHeight * uiBlockDepth / 8;

    const WUInt32 uiTightRowPitch = uiBlockCountX * uiBytesPerBlock;
    if (sourceData.m_uiRowPitch < uiTightRowPitch)
    {
      WLog::Error("Invalid row pitch. Expected at least {0} got {1}", uiTightRowPitch, sourceData.m_uiRowPitch);
      return;
    }

    const WUInt32 uiMinSlicePitch = sourceData.m_uiRowPitch * uiBlockCountY;
    if (sourceData.m_uiSlicePitch != 0 && sourceData.m_uiSlicePitch < uiMinSlicePitch)
    {
      WLog::Error("Invalid slice pitch. Expected at least {0} got {1}", uiMinSlicePitch, sourceData.m_uiSlicePitch);
      return;
    }

    const WUInt32 uiSlicePitch = sourceData.m_uiSlicePitch != 0 ? sourceData.m_uiSlicePitch : uiMinSlicePitch;

    if (sourceData.m_pData.GetCount() < uiSlicePitch * uiBlockCountZ)
    {
      WLog::Error("Not enough data provided to update texture");
      return;
    }

    WGALSystemMemoryDescription finalSourceData = sourceData;
    if (finalSourceData.m_uiSlicePitch == 0)
    {
      finalSourceData.m_uiSlicePitch = uiSlicePitch;
    }

    WBoundingBoxu32 finalDestBox = destinationBox;
    if (bDestBoxIsValid)
    {
      finalDestBox.m_vMax = finalDestBox.m_vMin + WVec3U32(uiWidth, uiHeight, uiDepth);
    }
    else
    {
      finalDestBox.m_vMin = WVec3U32(0, 0, 0);
      finalDestBox.m_vMax = vMipSize;
    }

    UpdateTextureForNextFramePlatform(pTexture, finalSourceData, destinationSubResource, finalDestBox);
  }
  else
  {
    WLog::Error("No valid texture handle given to update!");
  }
}

WGALRenderTargetViewHandle WGALDevice::GetDefaultRenderTargetView(WGALTextureHandle hTexture)
{
  if (const WGALTexture* pTexture = GetTexture(hTexture))
  {
    return pTexture->m_hDefaultRenderTargetView;
  }

  return WGALRenderTargetViewHandle();
}

WGALRenderTargetViewHandle WGALDevice::GetRenderTargetView(const WGALRenderTargetViewCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();
  WGALRenderTargetViewCreationDescription rtDesc = desc;

  WGALTexture* pTexture = nullptr;

  if (!rtDesc.m_hTexture.IsInvalidated())
    pTexture = Get<TextureTable, WGALTexture>(rtDesc.m_hTexture, m_Textures);

  if (pTexture == nullptr)
  {
    WLog::Error("No valid texture handle given for render target view creation!");
    return WGALRenderTargetViewHandle();
  }

  const WEnum<WGALTextureType> type = rtDesc.m_OverrideViewType != WGALTextureType::Invalid ? rtDesc.m_OverrideViewType : pTexture->GetDescription().m_Type;
  if (type != WGALTextureType::Texture2DArray && type != WGALTextureType::TextureCubeArray)
  {
    if (rtDesc.m_uiSliceCount != 1)
    {
      W_REPORT_FAILURE("m_uiSliceCount must be 1 for non array textures!");
      return WGALRenderTargetViewHandle();
    }
  }
  if (type == WGALTextureType::TextureCube || type == WGALTextureType::TextureCubeArray)
  {
    W_REPORT_FAILURE("Render targets cannot be created on cube maps, use 2DArrays instead.");
    return WGALRenderTargetViewHandle();
  }
  if (type == WGALTextureType::Texture2DProxy)
  {
    WGALProxyTexture* pProxyTexture = static_cast<WGALProxyTexture*>(pTexture);
    rtDesc.m_uiFirstSlice = pProxyTexture->m_uiSlice;
  }

  // Hash desc and return potential existing one
  const WUInt32 uiHash = rtDesc.CalculateHash();
  {
    WGALRenderTargetViewHandle hRenderTarget;
    if (pTexture->m_RenderTargetViews.TryGetValue(uiHash, hRenderTarget))
      return hRenderTarget;
  }

  WGALRenderTargetView* pRenderTargetView = CreateRenderTargetViewPlatform(pTexture, rtDesc);
  if (pRenderTargetView != nullptr)
  {
    WGALRenderTargetViewHandle hView(m_RenderTargetViews.Insert(pRenderTargetView));
    pTexture->m_RenderTargetViews.Insert(uiHash, hView);
    return hView;
  }

  return WGALRenderTargetViewHandle();
}

WGALSwapChainHandle WGALDevice::CreateSwapChain(const SwapChainFactoryFunction& func)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  ///// \todo Platform independent validation
  // if (desc.m_pWindow == nullptr)
  //{
  //   WLog::Error("The desc for the swap chain creation contained an invalid (nullptr) window handle!");
  //   return WGALSwapChainHandle();
  // }

  WGALSwapChain* pSwapChain = func(&m_Allocator);
  // WGALSwapChainDX11* pSwapChain = W_NEW(&m_Allocator, WGALSwapChainDX11, Description);

  if (!pSwapChain->InitPlatform(this).Succeeded())
  {
    W_DELETE(&m_Allocator, pSwapChain);
    return WGALSwapChainHandle();
  }

  return WGALSwapChainHandle(m_SwapChains.Insert(pSwapChain));
}

WResult WGALDevice::UpdateSwapChain(WGALSwapChainHandle hSwapChain, WEnum<WGALPresentMode> newPresentMode)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  Flush();
  WGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    return pSwapChain->UpdateSwapChain(this, newPresentMode);
  }
  else
  {
    WLog::Warning("UpdateSwapChain called on invalid handle.");
    return W_FAILURE;
  }
}

void WGALDevice::DestroySwapChain(WGALSwapChainHandle& inout_hSwapChain)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALSwapChain* pSwapChain = nullptr;
  if (m_SwapChains.TryGetValue(inout_hSwapChain, pSwapChain))
  {
    AddDeadObject(GALObjectType::SwapChain, inout_hSwapChain);
  }

  inout_hSwapChain.Invalidate();
}

WGALVertexDeclarationHandle WGALDevice::CreateVertexDeclaration(const WGALVertexDeclarationCreationDescription& desc)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WInt32 iHighestUsedBinding = -1;
  for (WUInt32 slot = 0; slot < desc.m_VertexAttributes.GetCount(); ++slot)
  {
    iHighestUsedBinding = WMath::Max(iHighestUsedBinding, static_cast<WInt32>(desc.m_VertexAttributes[slot].m_uiVertexBufferSlot));
  }
  if (desc.m_VertexBindings.GetCount() != static_cast<WUInt32>(iHighestUsedBinding + 1))
  {
    WLog::Error("Not enough vertex bindings ({}) to support the maximum used vertex buffer index ({}) used by the vertex attributes.", desc.m_VertexBindings.GetCount(), iHighestUsedBinding);
    return {};
  }
  if (desc.m_VertexBindings.GetCount() > W_GAL_MAX_VERTEX_BUFFER_COUNT)
  {
    WLog::Error("Too many vertex bindings ({}), only up to {} are supported.", desc.m_VertexBindings.GetCount(), W_GAL_MAX_VERTEX_BUFFER_COUNT);
    return {};
  }

  /// \todo Platform independent validation

  // Hash desc and return potential existing one (including inc. refcount)
  WUInt32 uiHash = desc.CalculateHash();

  {
    WGALVertexDeclarationHandle hVertexDeclaration;
    if (m_VertexDeclarationTable.TryGetValue(uiHash, hVertexDeclaration))
    {
      WGALVertexDeclaration* pVertexDeclaration = m_VertexDeclarations[hVertexDeclaration];
      if (pVertexDeclaration->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::VertexDeclaration, hVertexDeclaration);
      }

      pVertexDeclaration->AddRef();
      m_uiVertexDeclarations++;
      return hVertexDeclaration;
    }
  }

  WGALVertexDeclaration* pVertexDeclaration = CreateVertexDeclarationPlatform(desc);

  if (pVertexDeclaration != nullptr)
  {
    pVertexDeclaration->AddRef();
    m_uiVertexDeclarations++;

    WGALVertexDeclarationHandle hVertexDeclaration(m_VertexDeclarations.Insert(pVertexDeclaration));
    m_VertexDeclarationTable.Insert(uiHash, hVertexDeclaration);

    return hVertexDeclaration;
  }

  return WGALVertexDeclarationHandle();
}

void WGALDevice::DestroyVertexDeclaration(WGALVertexDeclarationHandle& inout_hVertexDeclaration)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WGALVertexDeclaration* pVertexDeclaration = nullptr;
  if (m_VertexDeclarations.TryGetValue(inout_hVertexDeclaration, pVertexDeclaration))
  {
    pVertexDeclaration->ReleaseRef();
    m_uiVertexDeclarations--;

    if (pVertexDeclaration->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::VertexDeclaration, inout_hVertexDeclaration);
    }
  }

  inout_hVertexDeclaration.Invalidate();
}

WEnum<WGALAsyncResult> WGALDevice::GetFenceResult(WGALFenceHandle hFence, WTime timeout)
{
  if (hFence == 0)
    return WGALAsyncResult::Expired;

  W_ASSERT_DEBUG(timeout.IsZero() || m_pCommandEncoder == nullptr || !m_pCommandEncoder->IsInRenderingScope(), "Waiting for a fence is only allowed outside of a rendering scope");

  WEnum<WGALAsyncResult> res = GetFenceResultPlatform(hFence, timeout);

  return res;
}

WReadbackBufferLock WGALDevice::LockBuffer(WGALReadbackBufferHandle hReadbackBuffer, WArrayPtr<const WUInt8>& out_memory)
{
  if (hReadbackBuffer.IsInvalidated())
    return {};
  const WGALReadbackBuffer* pReadbackBuffer = GetReadbackBuffer(hReadbackBuffer);
  if (pReadbackBuffer == nullptr)
    return {};

  return WReadbackBufferLock(this, pReadbackBuffer, out_memory);
}

WReadbackTextureLock WGALDevice::LockTexture(WGALReadbackTextureHandle hReadbackTexture, const WArrayPtr<const WGALTextureSubresource>& subResources, WDynamicArray<WGALSystemMemoryDescription>& out_memory)
{
  if (hReadbackTexture.IsInvalidated())
    return {};
  const WGALReadbackTexture* pReadbackTexture = GetReadbackTexture(hReadbackTexture);
  if (pReadbackTexture == nullptr)
    return {};

  return WReadbackTextureLock(this, pReadbackTexture, subResources, out_memory);
}

WGALTextureHandle WGALDevice::GetBackBufferTextureFromSwapChain(WGALSwapChainHandle hSwapChain) const
{
  WGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    return pSwapChain->GetBackBufferTexture();
  }
  else
  {
    W_REPORT_FAILURE("Swap chain handle invalid");
    return WGALTextureHandle();
  }
}

void WGALDevice::GetAllSwapChains(WDynamicArray<WGALSwapChainHandle>& out_swapChains) const
{
  out_swapChains.Reserve(m_SwapChains.GetCount());
  for (auto it = m_SwapChains.GetIterator(); it.IsValid(); ++it)
  {
    out_swapChains.PushBack(WGALSwapChainHandle(it.Id()));
  }
}

// Misc functions

void WGALDevice::EnqueueFrameSwapChain(WGALSwapChainHandle hSwapChain)
{
  W_ASSERT_DEV(!m_bBeginFrameCalled, "EnqueueFrameSwapChain must be called before or during WGALDeviceEvent::BeforeBeginFrame");

  WGALSwapChain* pSwapChain = nullptr;
  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    // Enqueuing the same swap-chain twice would acquire and present it twice.
    // Several independent systems may want to render into the same swap-chain (e.g. a render pipeline
    // and something that draws on top of everything), so silently ignore duplicates.
    if (!m_FrameSwapChains.Contains(pSwapChain))
    {
      m_FrameSwapChains.PushBack(pSwapChain);
    }
  }
}

void WGALDevice::BeginFrame(const WUInt64 uiAppFrame)
{
  {
    W_PROFILE_SCOPE("BeforeBeginFrame");
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::BeforeBeginFrame;
    s_Events.Broadcast(e);
  }

  {
    W_GALDEVICE_LOCK_AND_CHECK();
    W_ASSERT_DEV(!m_bBeginFrameCalled, "You must call WGALDevice::EndFrame before you can call WGALDevice::BeginFrame again");
    m_bBeginFrameCalled = true;
    BeginFramePlatform(m_FrameSwapChains, uiAppFrame);
  }

  for (auto it = m_DynamicBuffers.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->SwapBuffers();
  }

  {
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::AfterBeginFrame;
    s_Events.Broadcast(e);
  }
}

void WGALDevice::EndFrame()
{
  W_PROFILE_SCOPE("WGALDevice::EndFrame");

  {
    W_PROFILE_SCOPE("WGALDevice::BeforeEndFrame");

    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::BeforeEndFrame;
    s_Events.Broadcast(e);
  }

  {
    W_GALDEVICE_LOCK_AND_CHECK();
    W_ASSERT_DEV(m_bBeginFrameCalled, "You must have called WGALDevice::Begin before you can call WGALDevice::EndFrame");

    DestroyDeadObjects();

    EndFramePlatform(m_FrameSwapChains);
    m_FrameSwapChains.Clear();
    m_bBeginFrameCalled = false;
    if (m_pCommandEncoder)
      m_pCommandEncoder->ResetStats();
  }

  {
    WGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = WGALDeviceEvent::AfterEndFrame;
    s_Events.Broadcast(e);
  }

  {
    m_EncoderStats.SetStatistics();
    m_EncoderStats.Reset();
    WStats::SetStat("GalDevice/ShaderReferences", m_uiShaders);
    WStats::SetStat("GalDevice/VertexDeclarationReferences", m_uiVertexDeclarations);
    WStats::SetStat("GalDevice/BlendStateReferences", m_uiBlendStates);
    WStats::SetStat("GalDevice/DepthStencilStateReferences", m_uiDepthStencilStates);
    WStats::SetStat("GalDevice/RasterizerStateReferences", m_uiRasterizerStates);
    WStats::SetStat("GalDevice/SamplerStateReferences", m_uiSamplerStates);
    WStats::SetStat("GalDevice/BindGroupLayoutReferences", m_uiBindGroupLayouts);
    WStats::SetStat("GalDevice/BindGroupReferences", m_uiBindGroups);
    WStats::SetStat("GalDevice/PipelineLayoutReferences", m_uiPipelineLayouts);
    WStats::SetStat("GalDevice/GraphicsPipelineReferences", m_uiGraphicsPipelines);
    WStats::SetStat("GalDevice/ComputePipelineReferences", m_uiComputePipelines);

    WStats::SetStat("GalDevice/Shaders", m_Shaders.GetCount());
    WStats::SetStat("GalDevice/VertexDeclarations", m_VertexDeclarations.GetCount());
    WStats::SetStat("GalDevice/BlendStates", m_BlendStates.GetCount());
    WStats::SetStat("GalDevice/DepthStencilStates", m_DepthStencilStates.GetCount());
    WStats::SetStat("GalDevice/RasterizerStates", m_RasterizerStates.GetCount());
    WStats::SetStat("GalDevice/SamplerStates", m_SamplerStates.GetCount());
    WStats::SetStat("GalDevice/BindGroupLayouts", m_BindGroupLayouts.GetCount());
    WStats::SetStat("GalDevice/BindGroups", m_BindGroups.GetCount());
    WStats::SetStat("GalDevice/PipelineLayouts", m_PipelineLayouts.GetCount());
    WStats::SetStat("GalDevice/GraphicsPipelines", m_GraphicsPipelines.GetCount());
    WStats::SetStat("GalDevice/ComputePipelines", m_ComputePipelines.GetCount());

    WStats::SetStat("GalDevice/Buffers", m_Buffers.GetCount());
    WStats::SetStat("GalDevice/DynamicBuffers", m_DynamicBuffers.GetCount());
    WStats::SetStat("GalDevice/Textures", m_Textures.GetCount());
    WStats::SetStat("GalDevice/ReadbackBuffers", m_ReadbackBuffers.GetCount());
    WStats::SetStat("GalDevice/ReadbackTextures", m_ReadbackTextures.GetCount());
    WStats::SetStat("GalDevice/RenderTargetViews", m_RenderTargetViews.GetCount());
    WStats::SetStat("GalDevice/SwapChains", m_SwapChains.GetCount());
  }
}

const WGALDeviceCapabilities& WGALDevice::GetCapabilities() const
{
  return m_Capabilities;
}

WUInt64 WGALDevice::GetMemoryConsumptionForTexture(const WGALTextureCreationDescription& desc) const
{
  // This generic implementation is only an approximation, but it can be overridden by specific devices
  // to give an accurate memory consumption figure.
  WUInt64 uiMemory = WUInt64(desc.m_uiWidth) * WUInt64(desc.m_uiHeight) * WUInt64(desc.m_uiDepth);
  uiMemory *= desc.m_uiArraySize;
  uiMemory *= WGALResourceFormat::GetBitsPerElement(desc.m_Format);
  uiMemory /= 8; // Bits per pixel
  uiMemory *= desc.m_SampleCount;

  // Also account for mip maps
  if (desc.m_uiMipLevelCount > 1)
  {
    uiMemory += static_cast<WUInt64>((1.0 / 3.0) * uiMemory);
  }

  return uiMemory;
}


WUInt64 WGALDevice::GetMemoryConsumptionForBuffer(const WGALBufferCreationDescription& desc) const
{
  return desc.m_uiTotalSize;
}

void WGALDevice::Flush()
{
  W_GALDEVICE_LOCK_AND_CHECK();

  FlushPlatform();
}

void WGALDevice::WaitIdle()
{
  WaitIdlePlatform();
}

void WGALDevice::DestroyViews(WGALTexture* pResource)
{
  W_ASSERT_DEBUG(pResource != nullptr, "Must provide valid resource");

  W_GALDEVICE_LOCK_AND_CHECK();

  for (auto it = pResource->m_RenderTargetViews.GetIterator(); it.IsValid(); ++it)
  {
    WGALRenderTargetViewHandle hRenderTargetView = it.Value();
    WGALRenderTargetView* pRenderTargetView = m_RenderTargetViews[hRenderTargetView];

    m_RenderTargetViews.Remove(hRenderTargetView);

    DestroyRenderTargetViewPlatform(pRenderTargetView);
  }
  pResource->m_RenderTargetViews.Clear();
  pResource->m_hDefaultRenderTargetView.Invalidate();
}

void WGALDevice::DestroyDeadObjects()
{
  // Can't use range based for here since new objects might be added during iteration
  for (WUInt32 i = 0; i < m_DeadObjects.GetCount(); ++i)
  {
    const auto& deadObject = m_DeadObjects[i];

    switch (deadObject.m_uiType)
    {
      case GALObjectType::BlendState:
      {
        WGALBlendStateHandle hBlendState(WGAL::ez16_16Id(deadObject.m_uiHandle));
        WGALBlendState* pBlendState = nullptr;

        W_VERIFY(m_BlendStates.Remove(hBlendState, &pBlendState), "BlendState not found in idTable");
        W_VERIFY(m_BlendStateTable.Remove(pBlendState->GetDescription().CalculateHash()), "BlendState not found in de-duplication table");

        DestroyBlendStatePlatform(pBlendState);

        break;
      }
      case GALObjectType::DepthStencilState:
      {
        WGALDepthStencilStateHandle hDepthStencilState(WGAL::ez16_16Id(deadObject.m_uiHandle));
        WGALDepthStencilState* pDepthStencilState = nullptr;

        W_VERIFY(m_DepthStencilStates.Remove(hDepthStencilState, &pDepthStencilState), "DepthStencilState not found in idTable");
        W_VERIFY(m_DepthStencilStateTable.Remove(pDepthStencilState->GetDescription().CalculateHash()),
          "DepthStencilState not found in de-duplication table");

        DestroyDepthStencilStatePlatform(pDepthStencilState);

        break;
      }
      case GALObjectType::RasterizerState:
      {
        WGALRasterizerStateHandle hRasterizerState(WGAL::ez16_16Id(deadObject.m_uiHandle));
        WGALRasterizerState* pRasterizerState = nullptr;

        W_VERIFY(m_RasterizerStates.Remove(hRasterizerState, &pRasterizerState), "RasterizerState not found in idTable");
        W_VERIFY(
          m_RasterizerStateTable.Remove(pRasterizerState->GetDescription().CalculateHash()), "RasterizerState not found in de-duplication table");

        DestroyRasterizerStatePlatform(pRasterizerState);

        break;
      }
      case GALObjectType::SamplerState:
      {
        WGALSamplerStateHandle hSamplerState(WGAL::ez16_16Id(deadObject.m_uiHandle));
        WGALSamplerState* pSamplerState = nullptr;

        W_VERIFY(m_SamplerStates.Remove(hSamplerState, &pSamplerState), "SamplerState not found in idTable");
        W_VERIFY(m_SamplerStateTable.Remove(pSamplerState->GetDescription().CalculateHash()), "SamplerState not found in de-duplication table");

        m_BindGroupTracker.DependencyDestroyed(pSamplerState);
        DestroySamplerStatePlatform(pSamplerState);

        break;
      }
      case GALObjectType::Shader:
      {
        WGALShaderHandle hShader(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALShader* pShader = nullptr;

        W_VERIFY(m_Shaders.Remove(hShader, &pShader), "");
        m_ShaderTable.Remove(pShader->GetDescription().CalculateHash());

        DestroyShaderPlatform(pShader);

        break;
      }
      case GALObjectType::Buffer:
      {
        WGALBufferHandle hBuffer(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALBuffer* pBuffer = nullptr;

        W_VERIFY(m_Buffers.Remove(hBuffer, &pBuffer), "");
        m_BindGroupTracker.DependencyDestroyed(pBuffer);
        DestroyBufferPlatform(pBuffer);

        break;
      }
      case GALObjectType::DynamicBuffer:
      {
        WGALDynamicBufferHandle hDynamicBuffer(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALDynamicBuffer* pDynamicBuffer = nullptr;

        W_VERIFY(m_DynamicBuffers.Remove(hDynamicBuffer, &pDynamicBuffer), "");

        W_DELETE(&m_Allocator, pDynamicBuffer);

        break;
      }
      case GALObjectType::Texture:
      {
        WGALTextureHandle hTexture(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALTexture* pTexture = nullptr;

        W_VERIFY(m_Textures.Remove(hTexture, &pTexture), "Unexpected invalild texture handle");

        DestroyViews(pTexture);

        m_BindGroupTracker.DependencyDestroyed(pTexture);
        switch (pTexture->GetDescription().m_Type)
        {
          case WGALTextureType::Texture2DShared:
            DestroySharedTexturePlatform(pTexture);
            break;
          default:
            DestroyTexturePlatform(pTexture);
            break;
        }
        break;
      }
      case GALObjectType::ReadbackBuffer:
      {
        WGALReadbackBufferHandle hBuffer(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALReadbackBuffer* pBuffer = nullptr;
        W_VERIFY(m_ReadbackBuffers.Remove(hBuffer, &pBuffer), "");
        DestroyReadbackBufferPlatform(pBuffer);
        break;
      }
      case GALObjectType::ReadbackTexture:
      {
        WGALReadbackTextureHandle hTexture(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALReadbackTexture* pTexture = nullptr;
        W_VERIFY(m_ReadbackTextures.Remove(hTexture, &pTexture), "");
        DestroyReadbackTexturePlatform(pTexture);
        break;
      }

      case GALObjectType::SwapChain:
      {
        WGALSwapChainHandle hSwapChain(WGAL::ez16_16Id(deadObject.m_uiHandle));
        WGALSwapChain* pSwapChain = nullptr;

        W_VERIFY(m_SwapChains.Remove(hSwapChain, &pSwapChain), "");

        if (pSwapChain != nullptr)
        {
          pSwapChain->DeInitPlatform(this).IgnoreResult();
          W_DELETE(&m_Allocator, pSwapChain);
        }

        break;
      }
      case GALObjectType::VertexDeclaration:
      {
        WGALVertexDeclarationHandle hVertexDeclaration(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALVertexDeclaration* pVertexDeclaration = nullptr;

        W_VERIFY(m_VertexDeclarations.Remove(hVertexDeclaration, &pVertexDeclaration), "Unexpected invalid handle");
        m_VertexDeclarationTable.Remove(pVertexDeclaration->GetDescription().CalculateHash());

        DestroyVertexDeclarationPlatform(pVertexDeclaration);

        break;
      }
      case GALObjectType::BindGroupLayout:
      {
        WGALBindGroupLayoutHandle hBindGroupLayout(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALBindGroupLayout* pBindGroupLayout = nullptr;

        W_VERIFY(m_BindGroupLayouts.Remove(hBindGroupLayout, &pBindGroupLayout), "Unexpected invalid handle");
        m_BindGroupLayoutTable.Remove(pBindGroupLayout->GetDescription().CalculateHash());

        DestroyBindGroupLayoutPlatform(pBindGroupLayout);
        break;
      }
      case GALObjectType::BindGroup:
      {
        WGALBindGroupHandle hBindGroup(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALBindGroup* pBindGroup = nullptr;

        W_VERIFY(m_BindGroups.Remove(hBindGroup, &pBindGroup), "Unexpected invalid handle");
        m_BindGroupTable.Remove(pBindGroup->GetDescription().CalculateHash());

        m_BindGroupTracker.RemoveResource(pBindGroup);
        DestroyBindGroupPlatform(pBindGroup);
        break;
      }
      case GALObjectType::PipelineLayout:
      {
        WGALPipelineLayoutHandle hPipelineLayout(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALPipelineLayout* pPipelineLayout = nullptr;

        W_VERIFY(m_PipelineLayouts.Remove(hPipelineLayout, &pPipelineLayout), "Unexpected invalid handle");
        m_PipelineLayoutTable.Remove(pPipelineLayout->GetDescription().CalculateHash());

        DestroyPipelineLayoutPlatform(pPipelineLayout);
        break;
      }
      case GALObjectType::GraphicsPipeline:
      {
        WGALGraphicsPipelineHandle hGraphicsPipeline(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALGraphicsPipeline* pGraphicsPipeline = nullptr;

        W_VERIFY(m_GraphicsPipelines.Remove(hGraphicsPipeline, &pGraphicsPipeline), "Unexpected invalid handle");
        m_GraphicsPipelineTable.Remove(pGraphicsPipeline->GetDescription().CalculateHash());

        DestroyGraphicsPipelinePlatform(pGraphicsPipeline);
        break;
      }
      case GALObjectType::ComputePipeline:
      {
        WGALComputePipelineHandle hComputePipeline(WGAL::ez18_14Id(deadObject.m_uiHandle));
        WGALComputePipeline* pComputePipeline = nullptr;

        W_VERIFY(m_ComputePipelines.Remove(hComputePipeline, &pComputePipeline), "Unexpected invalid handle");
        m_ComputePipelineTable.Remove(pComputePipeline->GetDescription().CalculateHash());

        DestroyComputePipelinePlatform(pComputePipeline);
        break;
      }
      default:
        W_ASSERT_NOT_IMPLEMENTED;
    }
  }

  m_DeadObjects.Clear();
}

void WGALDevice::OnBindGroupInvalidatedEventHandler(WGALBindGroup* pBindGroup)
{
  W_GALDEVICE_LOCK_AND_CHECK();
  pBindGroup->Invalidate(this);
}

const WGALSwapChain* WGALDevice::GetSwapChainInternal(WGALSwapChainHandle hSwapChain, const WRTTI* pRequestedType) const
{
  const WGALSwapChain* pSwapChain = GetSwapChain(hSwapChain);
  if (pSwapChain)
  {
    if (!pSwapChain->GetDescription().m_pSwapChainType->IsDerivedFrom(pRequestedType))
      return nullptr;
  }
  return pSwapChain;
}
