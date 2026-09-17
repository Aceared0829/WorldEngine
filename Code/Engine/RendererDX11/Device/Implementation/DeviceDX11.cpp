#include <RendererDX11/RendererDX11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/System/SystemInformation.h>
#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererDX11/Pools/FencePoolDX11.h>
#include <RendererDX11/Pools/QueryPoolDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/ReadbackBufferDX11.h>
#include <RendererDX11/Resources/ReadbackTextureDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/SharedTextureDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Shader/BindGroupDX11.h>
#include <RendererDX11/Shader/BindGroupLayoutDX11.h>
#include <RendererDX11/Shader/PipelineLayoutDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/ComputePipelineDX11.h>
#include <RendererDX11/State/GraphicsPipelineDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <d3d11.h>
#include <d3d11_3.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

namespace
{
  IDXGIAdapter1* CreateHighPerformanceAdapter()
  {
    IDXGIFactory1* pFactory1 = nullptr;
    IDXGIFactory6* pFactory6 = nullptr;
    IDXGIAdapter1* pAdapter = nullptr;
    W_SCOPE_EXIT(W_GAL_DX11_RELEASE(pFactory1));
    W_SCOPE_EXIT(W_GAL_DX11_RELEASE(pFactory6));

    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory1))))
      return nullptr;

    if (FAILED(pFactory1->QueryInterface(IID_PPV_ARGS(&pFactory6))))
      return nullptr;

    if (pFactory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter)) == DXGI_ERROR_NOT_FOUND)
      return nullptr;

    return pAdapter;
  }
} // namespace

WInternal::NewInstance<WGALDevice> CreateDX11Device(WAllocator* pAllocator, const WGALDeviceCreationDescription& description)
{
  return W_NEW(pAllocator, WGALDeviceDX11, description);
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererDX11, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  WGALDeviceFactory::RegisterCreatorFunc("DX11", &CreateDX11Device, "DX11_SM50", "WShaderCompilerHLSL");
}

ON_CORESYSTEMS_SHUTDOWN
{
  WGALDeviceFactory::UnregisterCreatorFunc("DX11");
}

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WGALDeviceDX11::WGALDeviceDX11(const WGALDeviceCreationDescription& Description)
  : WGALDevice(Description)
  // NOLINTNEXTLINE
  , m_uiFeatureLevel(D3D_FEATURE_LEVEL_9_1)
{
}

WGALDeviceDX11::~WGALDeviceDX11() = default;

// Init & shutdown functions

WResult WGALDeviceDX11::InitPlatform(DWORD dwFlags, IDXGIAdapter* pUsedAdapter)
{
  W_LOG_BLOCK("WGALDeviceDX11::InitPlatform");

retry:

  if (m_Description.m_bDebugDevice)
    dwFlags |= D3D11_CREATE_DEVICE_DEBUG;
  else
    dwFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

  D3D_FEATURE_LEVEL FeatureLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3};
  ID3D11DeviceContext* pImmediateContext = nullptr;

  D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
  // driverType = D3D_DRIVER_TYPE_REFERENCE; // enables the Reference Device

  if (pUsedAdapter != nullptr)
  {
    // required by the specification
    driverType = D3D_DRIVER_TYPE_UNKNOWN;
  }

  // Manually step through feature levels - if a Win 7 system doesn't have the 11.1 runtime installed
  // The create device call will fail even though the 11.0 (or lower) level could've been
  // initialized successfully
  int FeatureLevelIdx = 0;
  for (FeatureLevelIdx = 0; FeatureLevelIdx < W_ARRAY_SIZE(FeatureLevels); FeatureLevelIdx++)
  {
    if (SUCCEEDED(D3D11CreateDevice(pUsedAdapter, driverType, nullptr, dwFlags, &FeatureLevels[FeatureLevelIdx], 1, D3D11_SDK_VERSION, &m_pDevice, (D3D_FEATURE_LEVEL*)&m_uiFeatureLevel, &pImmediateContext)))
    {
      break;
    }
  }

  // Nothing could be initialized:
  if (pImmediateContext == nullptr)
  {
    if (m_Description.m_bDebugDevice)
    {
      WLog::Warning("Couldn't initialize D3D11 debug device!");

      m_Description.m_bDebugDevice = false;
      goto retry;
    }

    WLog::Error("Couldn't initialize D3D11 device!");
    return W_FAILURE;
  }
  else
  {
    m_pImmediateContext = pImmediateContext;

    const char* FeatureLevelNames[] = {"11.1", "11.0", "10.1", "10", "9.3"};

    static_assert(W_ARRAY_SIZE(FeatureLevels) == W_ARRAY_SIZE(FeatureLevelNames));

    // Get the adapter from the device
    IDXGIDevice* pDXGIDevice = nullptr;
    IDXGIAdapter* pAdapter = nullptr;
    DXGI_ADAPTER_DESC desc1 = {};

    if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice)))
    {
      if (SUCCEEDED(pDXGIDevice->GetAdapter(&pAdapter)))
      {
        pAdapter->GetDesc(&desc1);
        pAdapter->Release();
      }
      pDXGIDevice->Release();
    }

    WLog::Success("Initialized D3D11 device '{}' with feature level {}.", desc1.Description, FeatureLevelNames[FeatureLevelIdx]);

    // Validate that we got the minimum required feature level
    if (m_uiFeatureLevel < D3D_FEATURE_LEVEL_11_1)
    {
      W_REPORT_FAILURE("The graphics hardware only supports Direct3D feature level {0}, but WorldEngine requires feature level 11.1 or higher. ", FeatureLevelNames[FeatureLevelIdx]);
      return W_FAILURE;
    }
  }

  if (m_Description.m_bDebugDevice)
  {
    if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&m_pDebug)))
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // only do this when a debugger is attached, otherwise the app would crash on every DX error
        if (WSystemInformation::IsDebuggerAttached())
        {
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
          // pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
        }

        // Ignore list.
        {
          D3D11_MESSAGE_ID hide[] = {
            // Hide messages about abandoned query results. This can easily happen when a GPUStopwatch is suddenly unused.
            D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS, D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS,
            // Don't break on invalid input assembly. This can easily happen when using the wrong mesh-material combination.
            D3D11_MESSAGE_ID_CREATEINPUTLAYOUT_MISSINGELEMENT,
            // Add more message IDs here as needed
          };
          D3D11_INFO_QUEUE_FILTER filter;
          WMemoryUtils::ZeroFill(&filter, 1);
          filter.DenyList.NumIDs = W_ARRAY_SIZE(hide);
          filter.DenyList.pIDList = hide;
          pInfoQueue->AddStorageFilterEntries(&filter);
        }

        pInfoQueue->Release();
      }
    }
  }


  // Create default pass
  m_pCommandEncoderImpl = W_DEFAULT_NEW(WGALCommandEncoderImplDX11, *this);
  m_pCommandEncoder = W_DEFAULT_NEW(WGALCommandEncoder, *this, *m_pCommandEncoderImpl);

  if (FAILED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&m_pDXGIDevice)))
  {
    WLog::Error("Couldn't get the DXGIDevice1 interface of the D3D11 device - this may happen when running on Windows Vista without SP2 "
                 "installed!");
    return W_FAILURE;
  }

  if (FAILED(m_pDevice->QueryInterface(__uuidof(ID3D11Device3), (void**)&m_pDevice3)))
  {
    WLog::Info("D3D device doesn't support ID3D11Device3, some features might be unavailable.");
  }

  if (FAILED(m_pDXGIDevice->SetMaximumFrameLatency(1)))
  {
    WLog::Warning("Failed to set max frames latency");
  }

  if (FAILED(m_pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&m_pDXGIAdapter)))
  {
    return W_FAILURE;
  }

  if (FAILED(m_pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void**)&m_pDXGIFactory)))
  {
    return W_FAILURE;
  }

  WFencePoolDX11::Initialize(this);
  m_pFenceQueue = W_NEW(&m_Allocator, WFenceQueueDX11, &m_Allocator);

  // Fill lookup table
  FillFormatLookupTable();

  WClipSpaceDepthRange::Default = WClipSpaceDepthRange::ZeroToOne;
  WClipSpaceYMode::RenderToTextureDefault = WClipSpaceYMode::Regular;

  m_pQueryPool = W_NEW(&m_Allocator, WQueryPoolDX11, this);
  if (m_pQueryPool->Initialize().Failed())
  {
    return W_FAILURE;
  }

  WGALWindowSwapChain::SetFactoryMethod([this](const WGALWindowSwapChainCreationDescription& desc) -> WGALSwapChainHandle
    { return CreateSwapChain([&desc](WAllocator* pAllocator) -> WGALSwapChain*
        { return W_NEW(pAllocator, WGALSwapChainDX11, desc); }); });

#if W_ENABLED(W_PLATFORM_WINDOWS)
  // RenderDoc cannot handle buffers that are mapped in a different frame than the current one
  m_bSupportsAlwaysMappedTempResources = GetModuleHandleA("renderdoc.dll") == nullptr;
#endif
  return W_SUCCESS;
}

WStringView WGALDeviceDX11::GetRendererPlatform()
{
  return "DX11";
}

WResult WGALDeviceDX11::InitPlatform()
{
  IDXGIAdapter1* pAdapter = CreateHighPerformanceAdapter();
  W_SCOPE_EXIT(W_GAL_DX11_RELEASE(pAdapter));
  return InitPlatform(0, pAdapter);
}

void WGALDeviceDX11::ReportLiveGpuObjects()
{
#if W_ENABLED(W_PLATFORM_WINDOWS)

  const HMODULE hDxgiDebugDLL = LoadLibraryW(L"Dxgidebug.dll");

  if (hDxgiDebugDLL == nullptr)
    return;

  using FnGetDebugInterfacePtr = HRESULT(WINAPI*)(REFIID, void**);
  FnGetDebugInterfacePtr GetDebugInterfacePtr = (FnGetDebugInterfacePtr)GetProcAddress(hDxgiDebugDLL, "DXGIGetDebugInterface");

  if (GetDebugInterfacePtr == nullptr)
    return;

  IDXGIDebug* dxgiDebug = nullptr;
  GetDebugInterfacePtr(IID_PPV_ARGS(&dxgiDebug));

  if (dxgiDebug == nullptr)
    return;

  OutputDebugStringW(L" +++++ Live DX11 Objects: +++++\n");

  // prints to OutputDebugString
  dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

  OutputDebugStringW(L" ----- Live DX11 Objects: -----\n");

  dxgiDebug->Release();

#endif
}

void WGALDeviceDX11::FlushDeadObjects()
{
  DestroyDeadObjects();
}

WResult WGALDeviceDX11::ShutdownPlatform()
{
  WGALWindowSwapChain::SetFactoryMethod({});
  for (WUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    for (auto it = m_FreeTempResources[type].GetIterator(); it.IsValid(); ++it)
    {
      WDynamicArray<TempResource>& tempResources = it.Value();
      for (auto tempResource : tempResources)
      {
        W_GAL_DX11_RELEASE(tempResource.m_pResource);
      }
    }
    m_FreeTempResources[type].Clear();

    for (auto& tempResource : m_UsedTempResources[type])
    {
      W_GAL_DX11_RELEASE(tempResource.m_pResource);
    }
    m_UsedTempResources[type].Clear();
  }

  m_pQueryPool->DeInitialize();
  m_pQueryPool = nullptr;
  m_pFenceQueue = nullptr;
  WFencePoolDX11::DeInitialize();

  m_pCommandEncoder = nullptr;
  m_pCommandEncoderImpl = nullptr;

  W_GAL_DX11_RELEASE(m_pImmediateContext);
  W_GAL_DX11_RELEASE(m_pDevice3);
  W_GAL_DX11_RELEASE(m_pDevice);
  W_GAL_DX11_RELEASE(m_pDebug);
  W_GAL_DX11_RELEASE(m_pDXGIFactory);
  W_GAL_DX11_RELEASE(m_pDXGIAdapter);
  W_GAL_DX11_RELEASE(m_pDXGIDevice);

  ReportLiveGpuObjects();

  return W_SUCCESS;
}

// Command encoder functions

WGALCommandEncoder* WGALDeviceDX11::BeginCommandsPlatform(const char* szName)
{
#if W_ENABLED(W_USE_PROFILING)
  m_pPassTimingScope = WProfilingScopeAndMarker::Start(m_pCommandEncoder.Borrow(), szName);
#else
  W_IGNORE_UNUSED(szName);
#endif

  return m_pCommandEncoder.Borrow();
}

void WGALDeviceDX11::EndCommandsPlatform(WGALCommandEncoder* pPass)
{
  W_ASSERT_DEV(m_pCommandEncoder.Borrow() == pPass, "Invalid pass");
  W_IGNORE_UNUSED(pPass);

#if W_ENABLED(W_USE_PROFILING)
  WProfilingScopeAndMarker::Stop(m_pCommandEncoder.Borrow(), m_pPassTimingScope);
#endif
}

void WGALDeviceDX11::FlushPlatform()
{
  m_pCommandEncoderImpl->FlushPlatform();
}

// State creation functions

WGALBlendState* WGALDeviceDX11::CreateBlendStatePlatform(const WGALBlendStateCreationDescription& Description)
{
  WGALBlendStateDX11* pState = W_NEW(&m_Allocator, WGALBlendStateDX11, Description);

  if (pState->InitPlatform(this).Succeeded())
  {
    return pState;
  }
  else
  {
    W_DELETE(&m_Allocator, pState);
    return nullptr;
  }
}

void WGALDeviceDX11::DestroyBlendStatePlatform(WGALBlendState* pBlendState)
{
  WGALBlendStateDX11* pState = static_cast<WGALBlendStateDX11*>(pBlendState);
  pState->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pState);
}

WGALDepthStencilState* WGALDeviceDX11::CreateDepthStencilStatePlatform(const WGALDepthStencilStateCreationDescription& Description)
{
  WGALDepthStencilStateDX11* pDX11DepthStencilState = W_NEW(&m_Allocator, WGALDepthStencilStateDX11, Description);

  if (pDX11DepthStencilState->InitPlatform(this).Succeeded())
  {
    return pDX11DepthStencilState;
  }
  else
  {
    W_DELETE(&m_Allocator, pDX11DepthStencilState);
    return nullptr;
  }
}

void WGALDeviceDX11::DestroyDepthStencilStatePlatform(WGALDepthStencilState* pDepthStencilState)
{
  WGALDepthStencilStateDX11* pDX11DepthStencilState = static_cast<WGALDepthStencilStateDX11*>(pDepthStencilState);
  pDX11DepthStencilState->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11DepthStencilState);
}

WGALRasterizerState* WGALDeviceDX11::CreateRasterizerStatePlatform(const WGALRasterizerStateCreationDescription& Description)
{
  WGALRasterizerStateDX11* pDX11RasterizerState = W_NEW(&m_Allocator, WGALRasterizerStateDX11, Description);

  if (pDX11RasterizerState->InitPlatform(this).Succeeded())
  {
    return pDX11RasterizerState;
  }
  else
  {
    W_DELETE(&m_Allocator, pDX11RasterizerState);
    return nullptr;
  }
}

void WGALDeviceDX11::DestroyRasterizerStatePlatform(WGALRasterizerState* pRasterizerState)
{
  WGALRasterizerStateDX11* pDX11RasterizerState = static_cast<WGALRasterizerStateDX11*>(pRasterizerState);
  pDX11RasterizerState->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11RasterizerState);
}

WGALSamplerState* WGALDeviceDX11::CreateSamplerStatePlatform(const WGALSamplerStateCreationDescription& Description)
{
  WGALSamplerStateDX11* pDX11SamplerState = W_NEW(&m_Allocator, WGALSamplerStateDX11, Description);

  if (pDX11SamplerState->InitPlatform(this).Succeeded())
  {
    return pDX11SamplerState;
  }
  else
  {
    W_DELETE(&m_Allocator, pDX11SamplerState);
    return nullptr;
  }
}

void WGALDeviceDX11::DestroySamplerStatePlatform(WGALSamplerState* pSamplerState)
{
  WGALSamplerStateDX11* pDX11SamplerState = static_cast<WGALSamplerStateDX11*>(pSamplerState);
  pDX11SamplerState->DeInitPlatform(this).AssertSuccess();
  W_DELETE(&m_Allocator, pDX11SamplerState);
}

void WGALDeviceDX11::RecreateSamplerStatePlatform(WGALSamplerState* pSamplerState)
{
  WGALSamplerStateDX11* pDX11SamplerState = static_cast<WGALSamplerStateDX11*>(pSamplerState);
  pDX11SamplerState->DeInitPlatform(this).AssertSuccess();
  pDX11SamplerState->InitPlatform(this).AssertSuccess();
}

WGALBindGroupLayout* WGALDeviceDX11::CreateBindGroupLayoutPlatform(const WGALBindGroupLayoutCreationDescription& Description)
{
  WGALBindGroupLayoutDX11* pDX11BindGroupLayout = W_NEW(&m_Allocator, WGALBindGroupLayoutDX11, Description);

  if (pDX11BindGroupLayout->InitPlatform(this).Succeeded())
  {
    return pDX11BindGroupLayout;
  }
  else
  {
    W_DELETE(&m_Allocator, pDX11BindGroupLayout);
    return nullptr;
  }
}

void WGALDeviceDX11::DestroyBindGroupLayoutPlatform(WGALBindGroupLayout* pBindGroupLayout)
{
  WGALBindGroupLayoutDX11* pDX11BindGroupLayout = static_cast<WGALBindGroupLayoutDX11*>(pBindGroupLayout);
  pDX11BindGroupLayout->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11BindGroupLayout);
}

WGALBindGroup* WGALDeviceDX11::CreateBindGroupPlatform(const WGALBindGroupCreationDescription& Description)
{
  WGALBindGroupDX11* pDX11BindGroup = W_NEW(&m_Allocator, WGALBindGroupDX11, Description);

  if (pDX11BindGroup->InitPlatform(this).Succeeded())
  {
    return pDX11BindGroup;
  }
  else
  {
    W_DELETE(&m_Allocator, pDX11BindGroup);
    return nullptr;
  }
}

void WGALDeviceDX11::RecreateBindGroupPlatform(WGALBindGroup* pBindGroup)
{
  W_IGNORE_UNUSED(pBindGroup);
}

void WGALDeviceDX11::DestroyBindGroupPlatform(WGALBindGroup* pBindGroup)
{
  WGALBindGroupDX11* pDX11BindGroup = static_cast<WGALBindGroupDX11*>(pBindGroup);
  pDX11BindGroup->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11BindGroup);
}


WGALPipelineLayout* WGALDeviceDX11::CreatePipelineLayoutPlatform(const WGALPipelineLayoutCreationDescription& Description)
{
  WGALPipelineLayoutDX11* pDX11PipelineLayout = W_NEW(&m_Allocator, WGALPipelineLayoutDX11, Description);

  if (pDX11PipelineLayout->InitPlatform(this).Succeeded())
  {
    return pDX11PipelineLayout;
  }
  else
  {
    W_DELETE(&m_Allocator, pDX11PipelineLayout);
    return nullptr;
  }
}

void WGALDeviceDX11::DestroyPipelineLayoutPlatform(WGALPipelineLayout* pPipelineLayout)
{
  WGALPipelineLayoutDX11* pDX11PipelineLayout = static_cast<WGALPipelineLayoutDX11*>(pPipelineLayout);
  pDX11PipelineLayout->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11PipelineLayout);
}

WGALGraphicsPipeline* WGALDeviceDX11::CreateGraphicsPipelinePlatform(const WGALGraphicsPipelineCreationDescription& Description)
{
  WGALGraphicsPipelineDX11* pGraphicsPipeline = W_NEW(&m_Allocator, WGALGraphicsPipelineDX11, Description);

  if (pGraphicsPipeline->InitPlatform(this).Succeeded())
  {
    return pGraphicsPipeline;
  }
  else
  {
    W_DELETE(&m_Allocator, pGraphicsPipeline);
    return nullptr;
  }
}

void WGALDeviceDX11::DestroyGraphicsPipelinePlatform(WGALGraphicsPipeline* pGraphicsPipeline)
{
  WGALGraphicsPipelineDX11* pGraphicsPipelineDX11 = static_cast<WGALGraphicsPipelineDX11*>(pGraphicsPipeline);
  pGraphicsPipelineDX11->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pGraphicsPipelineDX11);
}

WGALComputePipeline* WGALDeviceDX11::CreateComputePipelinePlatform(const WGALComputePipelineCreationDescription& Description)
{
  WGALComputePipelineDX11* pComputePipeline = W_NEW(&m_Allocator, WGALComputePipelineDX11, Description);

  if (pComputePipeline->InitPlatform(this).Succeeded())
  {
    return pComputePipeline;
  }
  else
  {
    W_DELETE(&m_Allocator, pComputePipeline);
    return nullptr;
  }
}

void WGALDeviceDX11::DestroyComputePipelinePlatform(WGALComputePipeline* pComputePipeline)
{
  WGALComputePipelineDX11* pComputePipelineDX11 = static_cast<WGALComputePipelineDX11*>(pComputePipeline);
  pComputePipelineDX11->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pComputePipelineDX11);
}

// Resource creation functions

WGALShader* WGALDeviceDX11::CreateShaderPlatform(const WGALShaderCreationDescription& Description)
{
  WGALShaderDX11* pShader = W_NEW(&m_Allocator, WGALShaderDX11, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    W_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void WGALDeviceDX11::DestroyShaderPlatform(WGALShader* pShader)
{
  WGALShaderDX11* pDX11Shader = static_cast<WGALShaderDX11*>(pShader);
  pDX11Shader->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11Shader);
}

WGALBuffer* WGALDeviceDX11::CreateBufferPlatform(const WGALBufferCreationDescription& Description, WArrayPtr<const WUInt8> pInitialData)
{
  if (Description.m_BufferFlags.AreAllSet(WGALBufferUsageFlags::DrawIndirect | WGALBufferUsageFlags::StructuredBuffer))
  {
    WLog::Error("DX11 does not support creating buffers with both DrawIndirect and StructuredBuffer set");
    return nullptr;
  }
  if (Description.m_BufferFlags.AreAllSet(WGALBufferUsageFlags::DrawIndirect | WGALBufferUsageFlags::ConstantBuffer))
  {
    WLog::Error("DX11 does not support creating buffers with both DrawIndirect and ConstantBuffer set");
    return nullptr;
  }

  WGALBufferDX11* pBuffer = W_NEW(&m_Allocator, WGALBufferDX11, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    W_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void WGALDeviceDX11::DestroyBufferPlatform(WGALBuffer* pBuffer)
{
  WGALBufferDX11* pDX11Buffer = static_cast<WGALBufferDX11*>(pBuffer);

  for (WUInt32 i = 0; i < m_PendingCopies.GetCount(); ++i)
  {
    auto& copy = m_PendingCopies[i];

    if (copy.m_pDestResource == pDX11Buffer->GetDXBuffer())
    {
      UnmapTempResource(copy.m_SourceResource);
      m_PendingCopies.RemoveAtAndSwap(i);
      --i;
    }
  }

  pDX11Buffer->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11Buffer);
}

WGALTexture* WGALDeviceDX11::CreateTexturePlatform(const WGALTextureCreationDescription& Description, WArrayPtr<WGALSystemMemoryDescription> pInitialData)
{
  WGALTextureDX11* pTexture = W_NEW(&m_Allocator, WGALTextureDX11, Description);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    W_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void WGALDeviceDX11::DestroyTexturePlatform(WGALTexture* pTexture)
{
  WGALTextureDX11* pDX11Texture = static_cast<WGALTextureDX11*>(pTexture);

  for (WUInt32 i = 0; i < m_PendingCopies.GetCount(); ++i)
  {
    auto& copy = m_PendingCopies[i];

    if (copy.m_pDestResource == pDX11Texture->GetDXTexture())
    {
      UnmapTempResource(copy.m_SourceResource);
      m_PendingCopies.RemoveAtAndSwap(i);
      --i;
    }
  }

  pDX11Texture->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11Texture);
}

WGALTexture* WGALDeviceDX11::CreateSharedTexturePlatform(const WGALTextureCreationDescription& Description, WArrayPtr<WGALSystemMemoryDescription> pInitialData, WEnum<WGALSharedTextureType> sharedType, WGALPlatformSharedHandle handle)
{
  WGALSharedTextureDX11* pTexture = W_NEW(&m_Allocator, WGALSharedTextureDX11, Description, sharedType, handle);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    W_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void WGALDeviceDX11::DestroySharedTexturePlatform(WGALTexture* pTexture)
{
  WGALSharedTextureDX11* pDX11Texture = static_cast<WGALSharedTextureDX11*>(pTexture);
  pDX11Texture->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11Texture);
}

WGALReadbackBuffer* WGALDeviceDX11::CreateReadbackBufferPlatform(const WGALBufferCreationDescription& Description)
{
  WGALReadbackBufferDX11* pReadbackBuffer = W_NEW(&m_Allocator, WGALReadbackBufferDX11, Description);

  if (!pReadbackBuffer->InitPlatform(this).Succeeded())
  {
    W_DELETE(&m_Allocator, pReadbackBuffer);
    return nullptr;
  }

  return pReadbackBuffer;
}

void WGALDeviceDX11::DestroyReadbackBufferPlatform(WGALReadbackBuffer* pReadbackBuffer)
{
  WGALReadbackBufferDX11* pDX11ReadbackBuffer = static_cast<WGALReadbackBufferDX11*>(pReadbackBuffer);

  pDX11ReadbackBuffer->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11ReadbackBuffer);
}

WGALReadbackTexture* WGALDeviceDX11::CreateReadbackTexturePlatform(const WGALTextureCreationDescription& Description)
{
  WGALReadbackTextureDX11* pReadbackTexture = W_NEW(&m_Allocator, WGALReadbackTextureDX11, Description);

  if (!pReadbackTexture->InitPlatform(this).Succeeded())
  {
    W_DELETE(&m_Allocator, pReadbackTexture);
    return nullptr;
  }

  return pReadbackTexture;
}

void WGALDeviceDX11::DestroyReadbackTexturePlatform(WGALReadbackTexture* pReadbackTexture)
{
  WGALReadbackTextureDX11* pDX11ReadbackTexture = static_cast<WGALReadbackTextureDX11*>(pReadbackTexture);

  pDX11ReadbackTexture->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11ReadbackTexture);
}

WGALRenderTargetView* WGALDeviceDX11::CreateRenderTargetViewPlatform(WGALTexture* pTexture, const WGALRenderTargetViewCreationDescription& Description)
{
  WGALRenderTargetViewDX11* pRTView = W_NEW(&m_Allocator, WGALRenderTargetViewDX11, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    W_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void WGALDeviceDX11::DestroyRenderTargetViewPlatform(WGALRenderTargetView* pRenderTargetView)
{
  WGALRenderTargetViewDX11* pDX11RenderTargetView = static_cast<WGALRenderTargetViewDX11*>(pRenderTargetView);
  pDX11RenderTargetView->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pDX11RenderTargetView);
}

// Other rendering creation functions

WGALVertexDeclaration* WGALDeviceDX11::CreateVertexDeclarationPlatform(const WGALVertexDeclarationCreationDescription& Description)
{
  WGALVertexDeclarationDX11* pVertexDeclaration = W_NEW(&m_Allocator, WGALVertexDeclarationDX11, Description);

  if (pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    W_DELETE(&m_Allocator, pVertexDeclaration);
    return nullptr;
  }
}

void WGALDeviceDX11::DestroyVertexDeclarationPlatform(WGALVertexDeclaration* pVertexDeclaration)
{
  WGALVertexDeclarationDX11* pVertexDeclarationDX11 = static_cast<WGALVertexDeclarationDX11*>(pVertexDeclaration);
  pVertexDeclarationDX11->DeInitPlatform(this).IgnoreResult();
  W_DELETE(&m_Allocator, pVertexDeclarationDX11);
}

void WGALDeviceDX11::UpdateBufferForNextFramePlatform(const WGALBuffer* pBuffer, WConstByteArrayPtr sourceData, WUInt32 uiDestOffset)
{
  const WGALBufferDX11* pBufferDX11 = static_cast<const WGALBufferDX11*>(pBuffer);

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  for (auto& copy : m_PendingCopies)
  {
    if (copy.m_pDestResource == pBufferDX11->GetDXBuffer())
    {
      const bool bWholeBuffer = copy.m_vSourceSize.x == WInvalidIndex;
      const bool bOverlapping = uiDestOffset < copy.m_vDestPoint.x + copy.m_vSourceSize.x && copy.m_vDestPoint.x < uiDestOffset + sourceData.GetCount();
      if (bWholeBuffer || bOverlapping)
      {
        WLog::Error("Buffer range is already updated for next frame.");
        return;
      }
    }
  }
#endif

  const WUInt32 uiDestBufferSize = pBuffer->GetDescription().m_uiTotalSize;

  auto& copy = m_PendingCopies.ExpandAndGetRef();
  if (m_bSupportsAlwaysMappedTempResources)
  {
    copy.m_SourceResource = CopyToTempBuffer(sourceData, m_uiFrameCounter + 1);
  }
  else
  {
    auto sourceDataCopy = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WUInt8, sourceData.GetCount());
    WMemoryUtils::Copy(sourceDataCopy.GetPtr(), sourceData.GetPtr(), sourceData.GetCount());
    copy.m_SourceData.m_pData = sourceDataCopy;
  }
  copy.m_pDestResource = pBufferDX11->GetDXBuffer();
  copy.m_vDestPoint.Set(uiDestOffset, 0, 0);
  copy.m_vSourceSize = WVec3U32(sourceData.GetCount(), 1, 1);
  copy.m_bCopySubresource = uiDestOffset != 0 || copy.m_SourceResource.m_uiRowPitch != uiDestBufferSize;
}

void WGALDeviceDX11::UpdateTextureForNextFramePlatform(const WGALTexture* pTexture, const WGALSystemMemoryDescription& sourceData, const WGALTextureSubresource& destinationSubResource, const WBoundingBoxu32& destinationBox)
{
  const WGALTextureDX11* pTextureDX11 = static_cast<const WGALTextureDX11*>(pTexture);

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  for (auto& copy : m_PendingCopies)
  {
    if (copy.m_pDestResource == pTextureDX11->GetDXTexture())
    {
      WLog::Error("Texture is already updated for next frame.");
      return;
    }
  }
#endif

  auto& desc = pTexture->GetDescription();

  const WUInt32 uiWidth = destinationBox.m_vMax.x - destinationBox.m_vMin.x;
  const WUInt32 uiHeight = destinationBox.m_vMax.y - destinationBox.m_vMin.y;
  const WUInt32 uiDepth = destinationBox.m_vMax.z - destinationBox.m_vMin.z;

  auto& copy = m_PendingCopies.ExpandAndGetRef();
  if (m_bSupportsAlwaysMappedTempResources)
  {
    copy.m_SourceResource = CopyToTempTexture(sourceData, uiWidth, uiHeight, uiDepth, desc.m_Format, m_uiFrameCounter + 1);
  }
  else
  {
    const WUInt32 uiSourceDataCount = static_cast<WUInt32>(sourceData.m_pData.GetCount());

    auto sourceDataCopy = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WUInt8, uiSourceDataCount);
    WMemoryUtils::Copy(sourceDataCopy.GetPtr(), sourceData.m_pData.GetPtr(), uiSourceDataCount);
    copy.m_SourceData.m_pData = sourceDataCopy;
    copy.m_SourceData.m_uiRowPitch = sourceData.m_uiRowPitch;
    copy.m_SourceData.m_uiSlicePitch = sourceData.m_uiSlicePitch;
    copy.m_SourceFormat = desc.m_Format;
  }
  copy.m_pDestResource = pTextureDX11->GetDXTexture();
  copy.m_uiDestSubResource = D3D11CalcSubresource(destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, desc.m_uiMipLevelCount);
  copy.m_vDestPoint = destinationBox.m_vMin;
  copy.m_vSourceSize = WVec3U32(uiWidth, uiHeight, uiDepth);
  copy.m_bCopySubresource = !destinationBox.m_vMin.IsZero() || destinationBox.m_vMax != WVec3U32(desc.m_uiWidth, desc.m_uiHeight, desc.m_uiDepth);
}

WEnum<WGALAsyncResult> WGALDeviceDX11::GetTimestampResultPlatform(WGALTimestampHandle hTimestamp, WTime& out_result)
{
  return m_pQueryPool->GetTimestampResult(hTimestamp, out_result);
}

WEnum<WGALAsyncResult> WGALDeviceDX11::GetOcclusionResultPlatform(WGALOcclusionHandle hOcclusion, WUInt64& out_uiResult)
{
  return m_pQueryPool->GetOcclusionQueryResult(hOcclusion, out_uiResult);
}

WEnum<WGALAsyncResult> WGALDeviceDX11::GetFenceResultPlatform(WGALFenceHandle hFence, WTime timeout)
{
  if (m_pFenceQueue->GetCurrentFenceHandle() == hFence && timeout.IsPositive())
  {
    // Fence has not been submitted yet, force submit of the command buffer or we would deadlock here.
    Flush();
  }

  return m_pFenceQueue->GetFenceResult(hFence, timeout);
}

WResult WGALDeviceDX11::LockBufferPlatform(const WGALReadbackBuffer* pBuffer, WArrayPtr<const WUInt8>& out_Memory) const
{
  const WGALReadbackBufferDX11* pDXBuffer = static_cast<const WGALReadbackBufferDX11*>(pBuffer);

  D3D11_MAPPED_SUBRESOURCE Mapped;
  HRESULT hr = GetDXImmediateContext()->Map(pDXBuffer->GetDXBuffer(), 0, D3D11_MAP_READ, 0, &Mapped);
  if (FAILED(hr))
  {
    return W_FAILURE;
  }
  out_Memory = WArrayPtr<const WUInt8>(reinterpret_cast<const WUInt8*>(Mapped.pData), pBuffer->GetDescription().m_uiTotalSize);
  return W_SUCCESS;
}

void WGALDeviceDX11::UnlockBufferPlatform(const WGALReadbackBuffer* pBuffer) const
{
  const WGALReadbackBufferDX11* pDXBuffer = static_cast<const WGALReadbackBufferDX11*>(pBuffer);
  GetDXImmediateContext()->Unmap(pDXBuffer->GetDXBuffer(), 0);
}

WResult WGALDeviceDX11::LockTexturePlatform(const WGALReadbackTexture* pTexture, const WArrayPtr<const WGALTextureSubresource>& subResources, WDynamicArray<WGALSystemMemoryDescription>& out_Memory) const
{
  out_Memory.Clear();
  const WGALReadbackTextureDX11* pDXTexture = static_cast<const WGALReadbackTextureDX11*>(pTexture);

  const WUInt32 uiSubResources = subResources.GetCount();
  for (WUInt32 i = 0; i < uiSubResources; i++)
  {
    const WGALTextureSubresource& subRes = subResources[i];
    WGALSystemMemoryDescription& memDesc = out_Memory.ExpandAndGetRef();
    const WUInt32 uiSubResourceIndex = D3D11CalcSubresource(subRes.m_uiMipLevel, subRes.m_uiArraySlice, pTexture->GetDescription().m_uiMipLevelCount);

    D3D11_MAPPED_SUBRESOURCE Mapped;
    if (FAILED(GetDXImmediateContext()->Map(pDXTexture->GetDXTexture(), uiSubResourceIndex, D3D11_MAP_READ, 0, &Mapped)))
    {
      WLog::Error("Failed to map sub resource with miplevel {} and array slice {}.", subRes.m_uiMipLevel, subRes.m_uiArraySlice);
    }
    else
    {
      switch (pTexture->GetDescription().m_Type)
      {
        case WGALTextureType::Texture2D:
        case WGALTextureType::Texture2DArray:
        case WGALTextureType::Texture2DProxy:
        case WGALTextureType::Texture2DShared:
        case WGALTextureType::TextureCube:
        case WGALTextureType::TextureCubeArray:
          memDesc.m_pData = WMakeByteBlobPtr(Mapped.pData, Mapped.RowPitch * pTexture->GetDescription().m_uiHeight);
          memDesc.m_uiRowPitch = Mapped.RowPitch;
          memDesc.m_uiSlicePitch = 0;
        case WGALTextureType::Texture3D:
          memDesc.m_pData = WMakeByteBlobPtr(Mapped.pData, Mapped.DepthPitch * pTexture->GetDescription().m_uiDepth);
          memDesc.m_uiRowPitch = Mapped.RowPitch;
          memDesc.m_uiSlicePitch = Mapped.DepthPitch;
          break;
        default:
          W_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }
  }
  return W_SUCCESS;
}

void WGALDeviceDX11::UnlockTexturePlatform(const WGALReadbackTexture* pTexture, const WArrayPtr<const WGALTextureSubresource>& subResources) const
{
  const WGALReadbackTextureDX11* pDXTexture = static_cast<const WGALReadbackTextureDX11*>(pTexture);

  const WUInt32 uiSubResources = subResources.GetCount();
  for (WUInt32 i = 0; i < uiSubResources; i++)
  {
    const WGALTextureSubresource& subRes = subResources[i];
    const WUInt32 uiSubResourceIndex = D3D11CalcSubresource(subRes.m_uiMipLevel, subRes.m_uiArraySlice, pTexture->GetDescription().m_uiMipLevelCount);

    GetDXImmediateContext()->Unmap(pDXTexture->GetDXTexture(), uiSubResourceIndex);
  }
}

// Swap chain functions

void WGALDeviceDX11::PresentPlatform(const WGALSwapChain* pSwapChain, bool bVSync)
{
  W_IGNORE_UNUSED(pSwapChain);
  W_IGNORE_UNUSED(bVSync);
}

// Misc functions

void WGALDeviceDX11::BeginFramePlatform(WArrayPtr<WGALSwapChain*> swapchains, const WUInt64 uiAppFrame)
{
  // check if fence is reached
  for (WUInt64 uiFrame = m_uiSafeFrame + 1; uiFrame < m_uiFrameCounter; uiFrame++)
  {
    auto& perFrameData = m_PerFrameData[uiFrame % FRAMES];

    // if we accumulate more frames than we can hold in the ring buffer, force waiting for fences.
    const bool bForce = uiFrame % FRAMES == m_uiFrameCounter % FRAMES;
    if (perFrameData.m_uiFrame != ((WUInt64)-1))
    {
      W_ASSERT_DEBUG(uiFrame == perFrameData.m_uiFrame, "Frame data was likely overwritten and no longer matches the expected previous frame index. This should have been prevented by bForce above.");
      bool bFenceReached = m_pFenceQueue->GetFenceResult(perFrameData.m_hFence) == WGALAsyncResult::Ready;
      if (!bFenceReached && bForce)
      {
        m_pFenceQueue->GetFenceResult(perFrameData.m_hFence, WTime::MakeFromHours(1));
        bFenceReached = true;
      }
      if (bFenceReached)
      {
        FreeTempResources(perFrameData.m_uiFrame);
        perFrameData.m_hFence = {};
        m_uiSafeFrame = uiFrame;
      }
      else
        break;
    }
  }

  W_ASSERT_DEBUG((m_uiFrameCounter % FRAMES) == m_uiCurrentPerFrameData, "");
  m_PerFrameData[m_uiCurrentPerFrameData].m_uiFrame = m_uiFrameCounter;

  m_pQueryPool->BeginFrame();

#if W_ENABLED(W_USE_PROFILING)
  WStringBuilder sb;
  sb.SetFormat("RENDER FRAME {}", uiAppFrame);
  m_pFrameTimingScope = WProfilingScopeAndMarker::Start(m_pCommandEncoder.Borrow(), sb);
#else
  W_IGNORE_UNUSED(uiAppFrame);
#endif

  ProcessPendingCopies();

  for (WGALSwapChain* pSwapChain : swapchains)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void WGALDeviceDX11::EndFramePlatform(WArrayPtr<WGALSwapChain*> swapchains)
{
  for (WGALSwapChain* pSwapChain : swapchains)
  {
    pSwapChain->PresentRenderTarget(this);
  }

#if W_ENABLED(W_USE_PROFILING)
  WProfilingScopeAndMarker::Stop(m_pCommandEncoder.Borrow(), m_pFrameTimingScope);
#endif

  m_pCommandEncoderImpl->EndFrame();
  m_pQueryPool->EndFrame();

  auto& currentFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
  currentFrameData.m_hFence = m_pFenceQueue->SubmitCurrentFence();

  ++m_uiFrameCounter;
  m_uiCurrentPerFrameData = (m_uiFrameCounter) % FRAMES;
}

WUInt64 WGALDeviceDX11::GetCurrentFramePlatform() const
{
  return m_uiFrameCounter;
}

WUInt64 WGALDeviceDX11::GetSafeFramePlatform() const
{
  return m_uiSafeFrame;
}

void WGALDeviceDX11::FillCapabilitiesPlatform()
{
  {
    DXGI_ADAPTER_DESC1 adapterDesc;
    m_pDXGIAdapter->GetDesc1(&adapterDesc);

    m_Capabilities.m_sAdapterName = WStringUtf8(adapterDesc.Description).GetData();
    m_Capabilities.m_uiDedicatedVRAM = static_cast<WUInt64>(adapterDesc.DedicatedVideoMemory);
    m_Capabilities.m_uiDedicatedSystemRAM = static_cast<WUInt64>(adapterDesc.DedicatedSystemMemory);
    m_Capabilities.m_uiSharedSystemRAM = static_cast<WUInt64>(adapterDesc.SharedSystemMemory);
    m_Capabilities.m_bHardwareAccelerated = (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0;
    m_Capabilities.m_bSupportsTexelBuffer = true;
    m_Capabilities.m_bSupportsMultipleSRVTypes = false;
    m_Capabilities.m_bSupportsMultiSampledArrays = true;
  }

  m_Capabilities.m_bSupportsMultithreadedResourceCreation = true;

  switch (m_uiFeatureLevel)
  {
    case D3D_FEATURE_LEVEL_11_1:
      m_Capabilities.m_bShaderStageSupported[WGALShaderStage::VertexShader] = true;
      m_Capabilities.m_bShaderStageSupported[WGALShaderStage::HullShader] = true;
      m_Capabilities.m_bShaderStageSupported[WGALShaderStage::DomainShader] = true;
      m_Capabilities.m_bShaderStageSupported[WGALShaderStage::GeometryShader] = true;
      m_Capabilities.m_bShaderStageSupported[WGALShaderStage::PixelShader] = true;
      m_Capabilities.m_bShaderStageSupported[WGALShaderStage::ComputeShader] = true;
      m_Capabilities.m_bSupportsIndirectDraw = true;
      m_Capabilities.m_bSupportsSharedTextures = true;
      m_Capabilities.m_bSupportsDepthBiasClamp = true;
      m_Capabilities.m_bSupportsWireframe = true;
      break;

      // not supported any longer
      // case D3D_FEATURE_LEVEL_11_0:
      // case D3D_FEATURE_LEVEL_10_1:
      // case D3D_FEATURE_LEVEL_10_0:
      // case D3D_FEATURE_LEVEL_9_3:

    default:
      W_ASSERT_ALWAYS(false, "Unsupported Direct3D feature level. This should have been caught during device initialization.");
      break;
  }

  if (m_pDevice3)
  {
    D3D11_FEATURE_DATA_D3D11_OPTIONS2 featureOpts2;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &featureOpts2, sizeof(featureOpts2))))
    {
      m_Capabilities.m_bSupportsConservativeRasterization = (featureOpts2.ConservativeRasterizationTier != D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED);
    }

    D3D11_FEATURE_DATA_D3D11_OPTIONS3 featureOpts3;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &featureOpts3, sizeof(featureOpts3))))
    {
      m_Capabilities.m_bSupportsVSRenderTargetArrayIndex = featureOpts3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer != 0;
    }
  }

  m_Capabilities.m_FormatSupport.SetCount(WGALResourceFormat::ENUM_COUNT);
  for (WUInt32 i = 0; i < WGALResourceFormat::ENUM_COUNT; i++)
  {
    WGALResourceFormat::Enum format = (WGALResourceFormat::Enum)i;
    const WGALFormatLookupEntryDX11& entry = m_FormatLookupTable.GetFormatInfo(format);
    const bool bIsDepth = WGALResourceFormat::IsDepthFormat(format);
    if (bIsDepth)
    {
      UINT uiSampleSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eDepthOnlyType, &uiSampleSupport)))
      {
        if (uiSampleSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_SHADER_SAMPLE)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::Texture);

        if (uiSampleSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::TextureRW);
      }

      UINT uiRenderSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eDepthStencilType, &uiRenderSupport)))
      {
        if (uiRenderSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_DEPTH_STENCIL)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::RenderTarget);
      }

      UINT uiMSAALevels;
      if (SUCCEEDED(m_pDevice3->CheckMultisampleQualityLevels(entry.m_eDepthStencilType, 2, &uiMSAALevels)))
      {
        if (uiMSAALevels > 0)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::MSAA2x);
      }
      if (SUCCEEDED(m_pDevice3->CheckMultisampleQualityLevels(entry.m_eDepthStencilType, 4, &uiMSAALevels)))
      {
        if (uiMSAALevels > 0)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::MSAA4x);
      }
      if (SUCCEEDED(m_pDevice3->CheckMultisampleQualityLevels(entry.m_eDepthStencilType, 8, &uiMSAALevels)))
      {
        if (uiMSAALevels > 0)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::MSAA8x);
      }
    }
    else
    {
      UINT uiSampleSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eResourceViewType, &uiSampleSupport)))
      {
        UINT uiSampleFlag = WGALResourceFormat::IsIntegerFormat(format) ? D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_SHADER_LOAD : D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
        if (uiSampleSupport & uiSampleFlag)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::Texture);

        if (uiSampleSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::TextureRW);
      }

      UINT uiVertexSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eVertexAttributeType, &uiVertexSupport)))
      {
        if (uiVertexSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::VertexAttribute);
      }

      UINT uiRenderSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eRenderTarget, &uiRenderSupport)))
      {
        if (uiRenderSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_RENDER_TARGET)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::RenderTarget);
      }

      UINT uiMSAALevels;
      if (SUCCEEDED(m_pDevice3->CheckMultisampleQualityLevels(entry.m_eRenderTarget, 2, &uiMSAALevels)))
      {
        if (uiMSAALevels > 0)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::MSAA2x);
      }
      if (SUCCEEDED(m_pDevice3->CheckMultisampleQualityLevels(entry.m_eRenderTarget, 4, &uiMSAALevels)))
      {
        if (uiMSAALevels > 0)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::MSAA4x);
      }
      if (SUCCEEDED(m_pDevice3->CheckMultisampleQualityLevels(entry.m_eRenderTarget, 8, &uiMSAALevels)))
      {
        if (uiMSAALevels > 0)
          m_Capabilities.m_FormatSupport[i].Add(WGALResourceFormatSupport::MSAA8x);
      }
    }
  }
}

void WGALDeviceDX11::WaitIdlePlatform()
{
  m_pImmediateContext->Flush();
  DestroyDeadObjects();
}

const WGALSharedTexture* WGALDeviceDX11::GetSharedTexture(WGALTextureHandle hTexture) const
{
  auto pTexture = GetTexture(hTexture);
  if (pTexture == nullptr)
  {
    return nullptr;
  }

  // Resolve proxy texture if any
  return static_cast<const WGALSharedTextureDX11*>(pTexture->GetParentResource());
}

WGALDeviceDX11::TempResource WGALDeviceDX11::CopyToTempBuffer(WConstByteArrayPtr sourceData, WUInt64 uiLastUseFrame /*= WUInt64(-1)*/)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  constexpr WUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  WUInt32 uiSize = WMath::Max(sourceData.GetCount(), 256U);
  if (uiSize < uiExpGrowthLimit)
  {
    uiSize = WMath::PowerOfTwo_Ceil(uiSize);
  }
  else
  {
    uiSize = WMemoryUtils::AlignSize(uiSize, uiExpGrowthLimit);
  }

  TempResource tempResource;
  auto it = m_FreeTempResources[TempResourceType::Buffer].Find(uiSize);
  if (it.IsValid())
  {
    WDynamicArray<TempResource>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      tempResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (tempResource.m_pResource == nullptr)
  {
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = uiSize;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = sourceData.GetPtr();
    initData.SysMemPitch = initData.SysMemSlicePitch = 0;

    if (uiSize > sourceData.GetCount())
    {
      WUInt8* dataCopy = W_NEW_RAW_BUFFER(WFrameAllocator::GetCurrentAllocator(), WUInt8, uiSize);
      memcpy(dataCopy, sourceData.GetPtr(), sourceData.GetCount());
      initData.pSysMem = dataCopy;
    }

    ID3D11Buffer* pBuffer = nullptr;
    if (!SUCCEEDED(m_pDevice->CreateBuffer(&desc, &initData, &pBuffer)))
    {
      return {};
    }

    tempResource.m_pResource = pBuffer;
    tempResource.m_uiRowPitch = uiSize;
  }
  else
  {
    if (!m_bSupportsAlwaysMappedTempResources)
    {
      MapTempResource(tempResource);
    }

    W_ASSERT_DEBUG(tempResource.m_pData != nullptr, "Must be mapped at this point");
    memcpy(tempResource.m_pData, sourceData.GetPtr(), sourceData.GetCount());
  }

  auto& usedTempResource = m_UsedTempResources[TempResourceType::Buffer].ExpandAndGetRef();
  usedTempResource.m_pResource = tempResource.m_pResource;
  usedTempResource.m_uiFrame = uiLastUseFrame != WUInt64(-1) ? uiLastUseFrame : m_uiFrameCounter;
  usedTempResource.m_uiHash = uiSize;

  return tempResource;
}


WGALDeviceDX11::TempResource WGALDeviceDX11::CopyToTempTexture(const WGALSystemMemoryDescription& sourceData, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiDepth, WGALResourceFormat::Enum format, WUInt64 uiLastUseFrame /*= WUInt64(-1)*/)
{
  W_GALDEVICE_LOCK_AND_CHECK();

  WUInt32 hashData[] = {uiWidth, uiHeight, uiDepth, (WUInt32)format};
  WUInt32 uiHash = WHashingUtils::xxHash32(hashData, sizeof(hashData));

  TempResource tempResource;
  auto it = m_FreeTempResources[TempResourceType::Texture].Find(uiHash);
  if (it.IsValid())
  {
    WDynamicArray<TempResource>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      tempResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (tempResource.m_pResource == nullptr)
  {
    if (uiDepth == 1)
    {
      D3D11_TEXTURE2D_DESC desc;
      desc.Width = uiWidth;
      desc.Height = uiHeight;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = GetFormatLookupTable().GetFormatInfo(format).m_eStorage;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags = 0;

      D3D11_SUBRESOURCE_DATA initData;
      initData.pSysMem = sourceData.m_pData.GetPtr();
      initData.SysMemPitch = sourceData.m_uiRowPitch;
      initData.SysMemSlicePitch = sourceData.m_uiSlicePitch;

      ID3D11Texture2D* pTexture = nullptr;
      if (!SUCCEEDED(m_pDevice->CreateTexture2D(&desc, &initData, &pTexture)))
      {
        return {};
      }

      tempResource.m_pResource = pTexture;
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
      return {};
    }
  }
  else
  {
    W_ASSERT_DEBUG(tempResource.m_pData != nullptr, "Must be mapped at this point");
    if (tempResource.m_uiRowPitch == sourceData.m_uiRowPitch && tempResource.m_uiDepthPitch == sourceData.m_uiSlicePitch)
    {
      memcpy(tempResource.m_pData, sourceData.m_pData.GetPtr(), sourceData.m_uiSlicePitch * uiDepth);
    }
    else
    {
      // Copy row by row
      for (WUInt32 z = 0; z < uiDepth; ++z)
      {
        const void* pSource = WMemoryUtils::AddByteOffset(sourceData.m_pData.GetPtr(), z * sourceData.m_uiSlicePitch);
        void* pDest = WMemoryUtils::AddByteOffset(tempResource.m_pData, z * tempResource.m_uiDepthPitch);

        for (WUInt32 y = 0; y < uiHeight; ++y)
        {
          memcpy(pDest, pSource, sourceData.m_uiRowPitch);

          pSource = WMemoryUtils::AddByteOffset(pSource, sourceData.m_uiRowPitch);
          pDest = WMemoryUtils::AddByteOffset(pDest, tempResource.m_uiRowPitch);
        }
      }
    }
  }

  auto& usedTempResource = m_UsedTempResources[TempResourceType::Buffer].ExpandAndGetRef();
  usedTempResource.m_pResource = tempResource.m_pResource;
  usedTempResource.m_uiFrame = uiLastUseFrame != WUInt64(-1) ? uiLastUseFrame : m_uiFrameCounter;
  usedTempResource.m_uiHash = uiHash;

  return tempResource;
}

void WGALDeviceDX11::MapTempResource(TempResource& tempResource)
{
  W_ASSERT_DEBUG(tempResource.m_pData == nullptr, "Must NOT be mapped at this point");

  D3D11_MAPPED_SUBRESOURCE mapped;
  HRESULT hRes = m_pImmediateContext->Map(tempResource.m_pResource, 0, D3D11_MAP_WRITE, 0, &mapped);
  W_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");
  W_IGNORE_UNUSED(hRes);

  tempResource.m_pData = mapped.pData;
  tempResource.m_uiRowPitch = mapped.RowPitch;
  tempResource.m_uiDepthPitch = mapped.DepthPitch;
}

void WGALDeviceDX11::UnmapTempResource(TempResource& tempResource)
{
  if (tempResource.m_pData != nullptr)
  {
    m_pImmediateContext->Unmap(tempResource.m_pResource, 0);
    tempResource.m_pData = nullptr;
    tempResource.m_uiRowPitch = 0;
    tempResource.m_uiDepthPitch = 0;
  }
}

void WGALDeviceDX11::FreeTempResources(WUInt64 uiFrame)
{
  for (WUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    auto& usedTempResources = m_UsedTempResources[type];

    for (WUInt32 i = 0; i < usedTempResources.GetCount();)
    {
      UsedTempResource& usedTempResource = usedTempResources[i];
      if (usedTempResource.m_uiFrame <= uiFrame)
      {
        auto it = m_FreeTempResources[type].Find(usedTempResource.m_uiHash);
        if (!it.IsValid())
        {
          it = m_FreeTempResources[type].Insert(usedTempResource.m_uiHash, WDynamicArray<TempResource>(&m_Allocator));
        }

        auto& tempResource = it.Value().ExpandAndGetRef();
        tempResource.m_pResource = usedTempResource.m_pResource;

        if (m_bSupportsAlwaysMappedTempResources)
        {
          MapTempResource(tempResource);
        }

        usedTempResources.RemoveAtAndSwap(i);
      }
      else
      {
        ++i;
      }
    }
  }
}

void WGALDeviceDX11::ProcessPendingCopies()
{
  W_PROFILE_AND_MARKER(GetCommandEncoder(), "PendingCopies");

  for (auto& copy : m_PendingCopies)
  {
    TempResource tempResource = copy.m_SourceResource;

    if (!m_bSupportsAlwaysMappedTempResources)
    {
      if (copy.m_SourceData.m_uiRowPitch == 0)
      {
        tempResource = CopyToTempBuffer(WMakeArrayPtr(copy.m_SourceData.m_pData.GetPtr(), static_cast<WUInt32>(copy.m_SourceData.m_pData.GetCount())), m_uiFrameCounter);
      }
      else
      {
        tempResource = CopyToTempTexture(copy.m_SourceData, copy.m_vSourceSize.x, copy.m_vSourceSize.y, copy.m_vSourceSize.z, copy.m_SourceFormat, m_uiFrameCounter);
      }
    }
    UnmapTempResource(tempResource);

    if (copy.m_bCopySubresource)
    {
      D3D11_BOX srcBox = {0, 0, 0, copy.m_vSourceSize.x, copy.m_vSourceSize.y, copy.m_vSourceSize.z};
      m_pImmediateContext->CopySubresourceRegion(copy.m_pDestResource, copy.m_uiDestSubResource, copy.m_vDestPoint.x, copy.m_vDestPoint.y, copy.m_vDestPoint.z, tempResource.m_pResource, 0, &srcBox);
    }
    else
    {
      m_pImmediateContext->CopyResource(copy.m_pDestResource, tempResource.m_pResource);
    }
  }
  m_PendingCopies.Clear();
}

void WGALDeviceDX11::FillFormatLookupTable()
{
  ///       The list below is in the same order as the WGALResourceFormat enum. No format should be missing except the ones that are just
  ///       different names for the same enum value.

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAFloat, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_FLOAT).VA(DXGI_FORMAT_R32G32B32A32_FLOAT).RV(DXGI_FORMAT_R32G32B32A32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUInt, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_UINT).VA(DXGI_FORMAT_R32G32B32A32_UINT).RV(DXGI_FORMAT_R32G32B32A32_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAInt, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_SINT).VA(DXGI_FORMAT_R32G32B32A32_SINT).RV(DXGI_FORMAT_R32G32B32A32_SINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBFloat, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_FLOAT).VA(DXGI_FORMAT_R32G32B32_FLOAT).RV(DXGI_FORMAT_R32G32B32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBUInt, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_UINT).VA(DXGI_FORMAT_R32G32B32_UINT).RV(DXGI_FORMAT_R32G32B32_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBInt, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_SINT).VA(DXGI_FORMAT_R32G32B32_SINT).RV(DXGI_FORMAT_R32G32B32_SINT));

  // Supported with DX 11.1
  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::B5G6R5UNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_B5G6R5_UNORM).RT(DXGI_FORMAT_B5G6R5_UNORM).VA(DXGI_FORMAT_B5G6R5_UNORM).RV(DXGI_FORMAT_B5G6R5_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BGRAUByteNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).RT(DXGI_FORMAT_B8G8R8A8_UNORM).VA(DXGI_FORMAT_B8G8R8A8_UNORM).RV(DXGI_FORMAT_B8G8R8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BGRAUByteNormalizedsRGB, WGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).RT(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB).RV(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAHalf, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_FLOAT).VA(DXGI_FORMAT_R16G16B16A16_FLOAT).RV(DXGI_FORMAT_R16G16B16A16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUShort, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_UINT).VA(DXGI_FORMAT_R16G16B16A16_UINT).RV(DXGI_FORMAT_R16G16B16A16_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUShortNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_UNORM).VA(DXGI_FORMAT_R16G16B16A16_UNORM).RV(DXGI_FORMAT_R16G16B16A16_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAShort, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_SINT).VA(DXGI_FORMAT_R16G16B16A16_SINT).RV(DXGI_FORMAT_R16G16B16A16_SINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAShortNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_SNORM).VA(DXGI_FORMAT_R16G16B16A16_SNORM).RV(DXGI_FORMAT_R16G16B16A16_SNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGFloat, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_FLOAT).VA(DXGI_FORMAT_R32G32_FLOAT).RV(DXGI_FORMAT_R32G32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGUInt, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_UINT).VA(DXGI_FORMAT_R32G32_UINT).RV(DXGI_FORMAT_R32G32_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGInt, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_SINT).VA(DXGI_FORMAT_R32G32_SINT).RV(DXGI_FORMAT_R32G32_SINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGB10A2UInt, WGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).RT(DXGI_FORMAT_R10G10B10A2_UINT).VA(DXGI_FORMAT_R10G10B10A2_UINT).RV(DXGI_FORMAT_R10G10B10A2_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGB10A2UIntNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).RT(DXGI_FORMAT_R10G10B10A2_UNORM).VA(DXGI_FORMAT_R10G10B10A2_UNORM).RV(DXGI_FORMAT_R10G10B10A2_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RG11B10Float, WGALFormatLookupEntryDX11(DXGI_FORMAT_R11G11B10_FLOAT).RT(DXGI_FORMAT_R11G11B10_FLOAT).VA(DXGI_FORMAT_R11G11B10_FLOAT).RV(DXGI_FORMAT_R11G11B10_FLOAT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUByteNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM).VA(DXGI_FORMAT_R8G8B8A8_UNORM).RV(DXGI_FORMAT_R8G8B8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUByteNormalizedsRGB, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).RV(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAUByte, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UINT).VA(DXGI_FORMAT_R8G8B8A8_UINT).RV(DXGI_FORMAT_R8G8B8A8_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAByteNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_SNORM).VA(DXGI_FORMAT_R8G8B8A8_SNORM).RV(DXGI_FORMAT_R8G8B8A8_SNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGBAByte, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_SINT).VA(DXGI_FORMAT_R8G8B8A8_SINT).RV(DXGI_FORMAT_R8G8B8A8_SINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGHalf, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_FLOAT).VA(DXGI_FORMAT_R16G16_FLOAT).RV(DXGI_FORMAT_R16G16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGUShort, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_UINT).VA(DXGI_FORMAT_R16G16_UINT).RV(DXGI_FORMAT_R16G16_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGUShortNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_UNORM).VA(DXGI_FORMAT_R16G16_UNORM).RV(DXGI_FORMAT_R16G16_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGShort, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_SINT).VA(DXGI_FORMAT_R16G16_SINT).RV(DXGI_FORMAT_R16G16_SINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGShortNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_SNORM).VA(DXGI_FORMAT_R16G16_SNORM).RV(DXGI_FORMAT_R16G16_SNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGUByte, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_UINT).VA(DXGI_FORMAT_R8G8_UINT).RV(DXGI_FORMAT_R8G8_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGUByteNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_UNORM).VA(DXGI_FORMAT_R8G8_UNORM).RV(DXGI_FORMAT_R8G8_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGByte, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_SINT).VA(DXGI_FORMAT_R8G8_SINT).RV(DXGI_FORMAT_R8G8_SINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RGByteNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_SNORM).VA(DXGI_FORMAT_R8G8_SNORM).RV(DXGI_FORMAT_R8G8_SNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::DFloat, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RV(DXGI_FORMAT_R32_FLOAT).D(DXGI_FORMAT_R32_FLOAT).DS(DXGI_FORMAT_D32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RFloat, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_FLOAT).VA(DXGI_FORMAT_R32_FLOAT).RV(DXGI_FORMAT_R32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RUInt, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_UINT).VA(DXGI_FORMAT_R32_UINT).RV(DXGI_FORMAT_R32_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RInt, WGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_SINT).VA(DXGI_FORMAT_R32_SINT).RV(DXGI_FORMAT_R32_SINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RHalf, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_FLOAT).VA(DXGI_FORMAT_R16_FLOAT).RV(DXGI_FORMAT_R16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RUShort, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_UINT).VA(DXGI_FORMAT_R16_UINT).RV(DXGI_FORMAT_R16_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RUShortNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_UNORM).VA(DXGI_FORMAT_R16_UNORM).RV(DXGI_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RShort, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_SINT).VA(DXGI_FORMAT_R16_SINT).RV(DXGI_FORMAT_R16_SINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RShortNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_SNORM).VA(DXGI_FORMAT_R16_SNORM).RV(DXGI_FORMAT_R16_SNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RUByte, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_UINT).VA(DXGI_FORMAT_R8_UINT).RV(DXGI_FORMAT_R8_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RUByteNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_UNORM).VA(DXGI_FORMAT_R8_UNORM).RV(DXGI_FORMAT_R8_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RByte, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_SINT).VA(DXGI_FORMAT_R8_SINT).RV(DXGI_FORMAT_R8_SINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::RByteNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_SNORM).VA(DXGI_FORMAT_R8_SNORM).RV(DXGI_FORMAT_R8_SNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::AUByteNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_A8_UNORM).RT(DXGI_FORMAT_A8_UNORM).VA(DXGI_FORMAT_A8_UNORM).RV(DXGI_FORMAT_A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::D16, WGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RV(DXGI_FORMAT_R16_UNORM).DS(DXGI_FORMAT_D16_UNORM).D(DXGI_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::D24S8, WGALFormatLookupEntryDX11(DXGI_FORMAT_R24G8_TYPELESS).DS(DXGI_FORMAT_D24_UNORM_S8_UINT).D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS).S(DXGI_FORMAT_X24_TYPELESS_G8_UINT));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC1, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).RV(DXGI_FORMAT_BC1_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC1sRGB, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).RV(DXGI_FORMAT_BC1_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC2, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).RV(DXGI_FORMAT_BC2_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC2sRGB, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).RV(DXGI_FORMAT_BC2_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC3, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).RV(DXGI_FORMAT_BC3_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC3sRGB, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).RV(DXGI_FORMAT_BC3_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC4UNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).RV(DXGI_FORMAT_BC4_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC4Normalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).RV(DXGI_FORMAT_BC4_SNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC5UNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).RV(DXGI_FORMAT_BC5_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC5Normalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).RV(DXGI_FORMAT_BC5_SNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC6UFloat, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).RV(DXGI_FORMAT_BC6H_UF16));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC6Float, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).RV(DXGI_FORMAT_BC6H_SF16));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC7UNormalized, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).RV(DXGI_FORMAT_BC7_UNORM));

  m_FormatLookupTable.SetFormatInfo(WGALResourceFormat::BC7UNormalizedsRGB, WGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).RV(DXGI_FORMAT_BC7_UNORM_SRGB));
}

WGALCommandEncoder* WGALDeviceDX11::GetCommandEncoder() const
{
  return m_pCommandEncoder.Borrow();
}

WFenceQueueDX11& WGALDeviceDX11::GetFenceQueue() const
{
  return *m_pFenceQueue.Borrow();
}

WQueryPoolDX11& WGALDeviceDX11::GetQueryPool() const
{
  return *m_pQueryPool.Borrow();
}

W_STATICLINK_FILE(RendererDX11, RendererDX11_Device_Implementation_DeviceDX11);
