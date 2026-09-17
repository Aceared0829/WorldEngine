

#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/Stats.h>
#include <RendererCore/Material/MaterialManager.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/RenderContext/BindGroupBuilder.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/State/PipelineCache.h>

WRenderContext* WRenderContext::s_pDefaultInstance = nullptr;
WGALCommandEncoder* WRenderContext::s_pCommandEncoder = nullptr;
WHybridArray<WRenderContext*, 4> WRenderContext::s_Instances;

// 0=Nearest, 1=Bilinear, 2=Trilinear, 3=Aniso2x, 4=Aniso4x, 5=Aniso8x, 6=Aniso16x
WCVarInt cvar_RenderingTextureQuality("Rendering.TextureQuality", 4, WCVarFlags::Save, "Default texture filtering quality. 0=Nearest, 1=Bilinear, 2=Trilinear, 3=Anisotropic2x, 4=Anisotropic4x, 5=Anisotropic8x, 6=Anisotropic16x.");

WMap<WRenderContext::ShaderVertexDecl, WGALVertexDeclarationHandle> WRenderContext::s_GALVertexDeclarations;

WMutex WRenderContext::s_ConstantBufferStorageMutex;
WIdTable<WConstantBufferStorageId, WConstantBufferStorageBase*> WRenderContext::s_ConstantBufferStorageTable;
WMap<WUInt32, WDynamicArray<WConstantBufferStorageBase*>> WRenderContext::s_FreeConstantBufferStorage;
WSet<WConstantBufferStorageBase*> WRenderContext::s_DirtyConstantBuffers;

namespace
{
  WUInt32 GetVertexBufferStride(WGALDevice* pDevice, WGALBufferHandle hBuffer)
  {
    if (!hBuffer.IsInvalidated())
    {
      if (const WGALBuffer* pBuffer = pDevice->GetBuffer(hBuffer))
      {
        return pBuffer->GetDescription().m_uiStructSize;
      }
    }
    return 0;
  }
} // namespace

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RendererContext)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "ImmutableSamplers"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WRenderContext::RegisterImmutableSamplers();
    WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WRenderContext::GALStaticDeviceEventHandler));
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WGALDevice::s_Events.RemoveEventHandler(WMakeDelegate(&WRenderContext::GALStaticDeviceEventHandler));
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WRenderContext::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WRenderContext::OnEngineShutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

WRenderContext::Statistics::Statistics()
{
  Reset();
}

WRenderContext::Statistics WRenderContext::s_LastFrameStatistics;

void WRenderContext::Statistics::Reset()
{
  m_uiFailedDrawcalls = 0;
  m_uiDrawcalls = 0;
  m_uiTriangles = 0;
  for (WUInt32 i = 0; i < W_GAL_MAX_BIND_GROUPS; ++i)
  {
    m_uiModifiedBindGroup[i] = 0;
    m_uiLayoutChanged[i] = 0;
  }
}

//////////////////////////////////////////////////////////////////////////

WRenderContext* WRenderContext::GetDefaultInstance()
{
  if (s_pDefaultInstance == nullptr)
    s_pDefaultInstance = CreateInstance(s_pCommandEncoder);

  W_ASSERT_DEBUG(s_pDefaultInstance != nullptr, "Default instance should have been created during device creation");
  return s_pDefaultInstance;
}

WRenderContext* WRenderContext::CreateInstance(WGALCommandEncoder* pCommandEncoder)
{
  return W_DEFAULT_NEW(WRenderContext, pCommandEncoder);
}

void WRenderContext::DestroyInstance(WRenderContext* pRenderer)
{
  W_DEFAULT_DELETE(pRenderer);
}

WRenderContext::WRenderContext(WGALCommandEncoder* pCommandEncoder)
{
  s_Instances.PushBack(this);

  m_pGALCommandEncoder = pCommandEncoder;

  m_StateFlags = WRenderContextFlags::AllStatesInvalid;
  m_GraphicsPipeline.m_Topology = WGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;
  m_bAllowAsyncShaderLoading = false;

  m_hGlobalConstantBufferStorage = CreateConstantBufferStorage<WGlobalConstants>();

  // If no push constants are supported, they are emulated via constant buffers.
  if (WGALDevice::GetDefaultDevice()->GetCapabilities().m_uiMaxPushConstantsSize == 0)
  {
    m_hPushConstantsStorage = CreateConstantBufferStorage(128);
  }
  ResetContextState();
}

WRenderContext::~WRenderContext()
{
  DeleteConstantBufferStorage(m_hGlobalConstantBufferStorage);
  DeleteConstantBufferStorage(m_hPushConstantsStorage);

  if (s_pDefaultInstance == this)
    s_pDefaultInstance = nullptr;

  s_Instances.RemoveAndSwap(this);
}

WRenderContext::Statistics WRenderContext::GetAndResetStatistics()
{
  WRenderContext::Statistics ret = m_Statistics;
  m_Statistics.Reset();
  return ret;
}

void WRenderContext::BeginRendering(const WGALRenderingSetup& renderingSetup, const WRectFloat& viewport, const char* szName, bool bStereoSupport)
{
  W_ASSERT_DEBUG(m_bRendering == false && m_bCompute == false, "Already in a scope");
  m_bRendering = true;
  m_GraphicsPipeline.m_RenderPass = renderingSetup.GetRenderPass();
  m_StateFlags.Add(WRenderContextFlags::PipelineChanged);
  const WGALMSAASampleCount::Enum msaaSampleCount = renderingSetup.GetRenderPass().m_Msaa;
  if (msaaSampleCount != WGALMSAASampleCount::None)
  {
    SetShaderPermutationVariable("MSAA", "TRUE");
  }
  else
  {
    SetShaderPermutationVariable("MSAA", "FALSE");
  }

  auto& gc = WriteGlobalConstants();
  gc.ViewportSize = WVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
  gc.NumMsaaSamples = msaaSampleCount;

  m_pGALCommandEncoder->BeginRendering(renderingSetup, szName);

  m_pGALCommandEncoder->SetViewport(viewport);

  m_bStereoRendering = bStereoSupport;
}

void WRenderContext::EndRendering()
{
  m_pGALCommandEncoder->EndRendering();

  m_bStereoRendering = false;
  m_bRendering = false;
}

void WRenderContext::BeginCompute(const char* szName /*= ""*/)
{
  W_ASSERT_DEBUG(m_bRendering == false && m_bCompute == false, "Already in a scope");
  m_pGALCommandEncoder->BeginCompute(szName);
  m_bCompute = true;
  m_StateFlags.Add(WRenderContextFlags::PipelineChanged);
}

void WRenderContext::EndCompute()
{
  m_pGALCommandEncoder->EndCompute();
  m_bCompute = false;
  m_StateFlags.Add(WRenderContextFlags::PipelineChanged);
}

void WRenderContext::SetShaderPermutationVariable(const char* szName, const WTempHashedString& sTempValue)
{
  WTempHashedString sHashedName(szName);

  WHashedString sName;
  WHashedString sValue;
  if (WShaderManager::IsPermutationValueAllowed(szName, sHashedName, sTempValue, sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void WRenderContext::SetShaderPermutationVariable(const WHashedString& sName, const WHashedString& sValue)
{
  if (WShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}


void WRenderContext::BindMaterial(const WMaterialResourceHandle& hMaterial)
{
  // Don't set m_hMaterial directly since we first need to check whether the material has been modified in the mean time.
  m_hNewMaterial = hMaterial;
  m_StateFlags.Add(WRenderContextFlags::MaterialBindingChanged | WRenderContextFlags::BindGroupChanged);
  m_bDirtyBindGroups[W_GAL_BIND_GROUP_MATERIAL] = true;
}

WBindGroupBuilder& WRenderContext::GetBindGroup(WUInt32 uiBindGroup)
{
  W_ASSERT_DEBUG(uiBindGroup <= W_GAL_MAX_BIND_GROUPS, "Bind group out of range");
  return m_BindGroupBuilders[uiBindGroup];
}

void WRenderContext::ResetBindGroup(WUInt32 uiBindGroup)
{
  m_BindGroupBuilders[uiBindGroup].ResetBoundResources(WGALDevice::GetDefaultDevice());
  m_BindGroups[uiBindGroup].m_hBindGroupLayout.Invalidate();
  m_BindGroups[uiBindGroup].m_BindGroupItems.Clear();
  m_bDirtyBindGroups[uiBindGroup] = false;
}

void WRenderContext::ResetBindGroups()
{
  for (WUInt32 i = 0; i < W_GAL_MAX_BIND_GROUPS; ++i)
  {
    ResetBindGroup(i);
  }
}

void WRenderContext::SetPushConstants(WTempHashedString sSlotName, WArrayPtr<const WUInt8> data)
{

  if (!m_hPushConstantsStorage.IsInvalidated())
  {
    W_ASSERT_DEBUG(data.GetCount() <= 128, "Push constants are not allowed to be bigger than 128 bytes.");
    WConstantBufferStorageBase* pStorage = nullptr;
    bool bResult = TryGetConstantBufferStorage(m_hPushConstantsStorage, pStorage);
    if (bResult)
    {
      WArrayPtr<WUInt8> targetStorage = pStorage->GetRawDataForWriting();
      WMemoryUtils::Copy(targetStorage.GetPtr(), data.GetPtr(), data.GetCount());
      WBindGroupBuilder& bindGroupDraw = WRenderContext::GetDefaultInstance()->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
      bindGroupDraw.BindBuffer(sSlotName, m_hPushConstantsStorage);
    }
  }
  else
  {
    W_ASSERT_DEBUG(data.GetCount() <= WGALDevice::GetDefaultDevice()->GetCapabilities().m_uiMaxPushConstantsSize, "Push constants are not allowed to be bigger than {} bytes.", WGALDevice::GetDefaultDevice()->GetCapabilities().m_uiMaxPushConstantsSize);
    m_pGALCommandEncoder->SetPushConstants(data);
  }
}

void WRenderContext::BindShader(const WShaderResourceHandle& hShader, WBitflags<WShaderBindFlags> flags)
{
  m_hMaterial.Invalidate();
  m_pMaterial = nullptr;
  m_pMaterialBindGroup = nullptr;
  m_bDirtyBindGroups[W_GAL_BIND_GROUP_MATERIAL] = true;
  m_StateFlags.Remove(WRenderContextFlags::MaterialBindingChanged);
  m_StateFlags.Add(WRenderContextFlags::BindGroupChanged);

  BindShaderInternal(hShader, flags);
}

void WRenderContext::SetBlendState(WGALBlendStateHandle hBlendState)
{
  m_GraphicsPipeline.m_hBlendState = hBlendState;
  m_StateFlags.Add(WRenderContextFlags::PipelineChanged);
}

void WRenderContext::SetDepthStencilState(WGALDepthStencilStateHandle hDepthStencilState)
{
  m_GraphicsPipeline.m_hDepthStencilState = hDepthStencilState;
  m_StateFlags.Add(WRenderContextFlags::PipelineChanged);
}

void WRenderContext::SetRasterizerState(WGALRasterizerStateHandle hRasterizerState)
{
  m_GraphicsPipeline.m_hRasterizerState = hRasterizerState;
  m_StateFlags.Add(WRenderContextFlags::PipelineChanged);
}

void WRenderContext::SetStencilRefValue(WUInt8 uiStencilRefValue)
{
  m_uiUserStencilRefValue = uiStencilRefValue;
  m_StateFlags.Add(WRenderContextFlags::NonPipelineStateChanged);
}

void WRenderContext::BindMeshBuffer(const WMeshBufferResourceHandle& hMeshBuffer, WGALBufferHandle hDataOffsetsBuffer /*= {}*/, WUInt32 uiFirstDataOffset /*= 0*/)
{
  WResourceLock<WMeshBufferResource> pMeshBuffer(hMeshBuffer, WResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffers(), pMeshBuffer->GetIndexBuffer(), pMeshBuffer->GetVertexAttributes(), pMeshBuffer->GetTopology(),
    pMeshBuffer->GetPrimitiveCount(), hDataOffsetsBuffer, uiFirstDataOffset);
}

void WRenderContext::BindMeshBuffer(const WDynamicMeshBufferResourceHandle& hDynamicMeshBuffer, WGALBufferHandle hDataOffsetsBuffer /*= {}*/, WUInt32 uiFirstDataOffset /*= 0*/)
{
  WResourceLock<WDynamicMeshBufferResource> pMeshBuffer(hDynamicMeshBuffer, WResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffers(), pMeshBuffer->GetIndexBuffer(), pMeshBuffer->GetVertexAttributes(), pMeshBuffer->GetDescriptor().m_Topology, pMeshBuffer->GetDescriptor().m_uiMaxPrimitives, hDataOffsetsBuffer, uiFirstDataOffset);
}

void WRenderContext::BindMeshBuffer(WArrayPtr<const WGALBufferHandle> vertexBuffers, WGALBufferHandle hIndexBuffer, WArrayPtr<const WGALVertexAttribute> vertexAttributes, WGALPrimitiveTopology::Enum topology, WUInt32 uiPrimitiveCount, WGALBufferHandle hDataOffsetsBuffer /*= {}*/, WUInt32 uiFirstDataOffset /*= 0*/)
{
  constexpr WUInt32 uiMaxNumVertexBuffers = W_ARRAY_SIZE(m_hVertexBuffers);
  constexpr WUInt32 uiDataOffsetsBufferSlot = WMeshVertexStreamType::DataOffsets;
  W_ASSERT_DEBUG(vertexBuffers.GetCount() <= uiDataOffsetsBufferSlot, "Too many vertex buffers");

  // We need to create a new array to ensure that unsused slots are set to invalid
  WGALBufferHandle newVertexBuffers[uiMaxNumVertexBuffers] = {};
  WMemoryUtils::Copy(newVertexBuffers, vertexBuffers.GetPtr(), vertexBuffers.GetCount());
  newVertexBuffers[uiDataOffsetsBufferSlot] = hDataOffsetsBuffer;

  if (WMemoryUtils::IsEqual(m_hVertexBuffers, newVertexBuffers, uiMaxNumVertexBuffers) && m_hIndexBuffer == hIndexBuffer && m_VertexAttributes == vertexAttributes &&
      m_GraphicsPipeline.m_Topology == topology && m_uiMeshBufferPrimitiveCount == uiPrimitiveCount)
  {
    return;
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  for (WUInt32 i1 = 0; i1 < vertexAttributes.GetCount(); ++i1)
  {
    for (WUInt32 i2 = 0; i2 < vertexAttributes.GetCount(); ++i2)
    {
      if (i1 != i2)
      {
        W_ASSERT_DEBUG(vertexAttributes[i1].m_eSemantic != vertexAttributes[i2].m_eSemantic, "Same semantic cannot be used twice in the same vertex declaration");
      }
    }
  }

  W_ASSERT_DEBUG(vertexBuffers.IsEmpty() || !vertexAttributes.IsEmpty(), "Needs vertex attributes if vertex buffers are provided");
#endif

  if (m_GraphicsPipeline.m_Topology != topology)
  {
    m_GraphicsPipeline.m_Topology = topology;
    m_StateFlags.Add(WRenderContextFlags::PipelineChanged);

    WTempHashedString sTopologies[] = {
      WTempHashedString("TOPOLOGY_POINTS"),
      WTempHashedString("TOPOLOGY_LINES"),
      WTempHashedString("TOPOLOGY_TRIANGLES"),
      WTempHashedString("TOPOLOGY_TRIANGLESTRIP"),
    };

    static_assert(W_ARRAY_SIZE(sTopologies) == WGALPrimitiveTopology::ENUM_COUNT);

    SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_GraphicsPipeline.m_Topology]);
  }

  WMemoryUtils::Copy(m_hVertexBuffers, newVertexBuffers, uiMaxNumVertexBuffers);

  m_hIndexBuffer = hIndexBuffer;
  m_VertexAttributes = vertexAttributes;
  m_uiMeshBufferPrimitiveCount = uiPrimitiveCount;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  for (WUInt32 i = 0; i < vertexBuffers.GetCount(); ++i)
  {
    m_VertexBufferStrides[i] = GetVertexBufferStride(pDevice, vertexBuffers[i]);
    m_VertexBufferOffsets[i] = 0;
    m_VertexBufferBindingRates[i] = WGALVertexBindingRate::Vertex;
  }

  if (hDataOffsetsBuffer.IsInvalidated() == false)
  {
    constexpr WUInt32 uiDataOffsetStructSize = sizeof(WInstanceableRenderData::DataOffsets);

    W_ASSERT_DEBUG(GetVertexBufferStride(pDevice, hDataOffsetsBuffer) == uiDataOffsetStructSize, "Wrong buffer stride");
    m_VertexBufferStrides[uiDataOffsetsBufferSlot] = uiDataOffsetStructSize;
    m_VertexBufferOffsets[uiDataOffsetsBufferSlot] = uiFirstDataOffset * uiDataOffsetStructSize;
    m_VertexBufferBindingRates[uiDataOffsetsBufferSlot] = WGALVertexBindingRate::Instance;

    m_VertexAttributes.PushBack(WMeshVertexStreamConfig::GetDataOffsetsVertexAttribute());
  }

  m_StateFlags.Add(WRenderContextFlags::MeshBufferBindingChanged);
}

void WRenderContext::BindVertexBuffer(WGALBufferHandle hVertexBuffer, WUInt32 uiSlot, WEnum<WGALVertexBindingRate> rate, WUInt32 uiOffset)
{
  W_ASSERT_DEBUG(uiSlot < W_GAL_MAX_VERTEX_BUFFER_COUNT, "Vertex buffer slot is out of bounds");
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  m_hVertexBuffers[uiSlot] = hVertexBuffer;
  m_VertexBufferStrides[uiSlot] = GetVertexBufferStride(pDevice, hVertexBuffer);
  m_VertexBufferBindingRates[uiSlot] = rate;
  m_VertexBufferOffsets[uiSlot] = uiOffset;

  m_StateFlags.Add(WRenderContextFlags::MeshBufferBindingChanged);
}

void WRenderContext::SetVertexAttributes(WArrayPtr<WGALVertexAttribute> vertexAttributes)
{
  if (m_VertexAttributes == vertexAttributes)
    return;

  m_VertexAttributes = vertexAttributes;

  m_StateFlags.Add(WRenderContextFlags::MeshBufferBindingChanged);
}

WResult WRenderContext::DrawMeshBuffer(WUInt32 uiPrimitiveCount, WUInt32 uiFirstPrimitive, WUInt32 uiInstanceCount)
{
  if (ApplyContextStates().Failed() || uiPrimitiveCount == 0 || uiInstanceCount == 0)
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return W_FAILURE;
  }

  W_ASSERT_DEV(uiFirstPrimitive < m_uiMeshBufferPrimitiveCount, "Invalid primitive range: first primitive ({0}) can't be larger than number of primitives ({1})", uiFirstPrimitive, uiPrimitiveCount);

  uiPrimitiveCount = WMath::Min(uiPrimitiveCount, m_uiMeshBufferPrimitiveCount - uiFirstPrimitive);
  W_ASSERT_DEV(uiPrimitiveCount > 0, "Invalid primitive range: number of primitives can't be zero.");

  auto pCommandEncoder = GetCommandEncoder();

  const WUInt32 uiIndexCount = WGALPrimitiveTopology::GetIndexCount(m_GraphicsPipeline.m_Topology, uiPrimitiveCount);
  const WUInt32 uiFirstIndex = WGALPrimitiveTopology::GetIndexCount(m_GraphicsPipeline.m_Topology, uiFirstPrimitive);

  if (m_bStereoRendering)
  {
    uiInstanceCount *= 2;
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  m_Statistics.m_uiDrawcalls++;

  if (m_GraphicsPipeline.m_Topology == WGALPrimitiveTopology::Triangles || m_GraphicsPipeline.m_Topology == WGALPrimitiveTopology::TriangleStrip)
  {
    m_Statistics.m_uiTriangles += WUInt64(uiPrimitiveCount) * uiInstanceCount;
  }
#endif

  if (uiInstanceCount > 1)
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      return pCommandEncoder->DrawIndexedInstanced(uiIndexCount, uiInstanceCount, uiFirstIndex);
    }
    else
    {
      return pCommandEncoder->DrawInstanced(uiIndexCount, uiInstanceCount, uiFirstIndex);
    }
  }
  else
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      return pCommandEncoder->DrawIndexed(uiIndexCount, uiFirstIndex);
    }
    else
    {
      return pCommandEncoder->Draw(uiIndexCount, uiFirstIndex);
    }
  }

  return W_SUCCESS;
}

WResult WRenderContext::Dispatch(WUInt32 uiThreadGroupCountX, WUInt32 uiThreadGroupCountY, WUInt32 uiThreadGroupCountZ)
{
  if (ApplyContextStates().Failed())
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return W_FAILURE;
  }

  return GetCommandEncoder()->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

WResult WRenderContext::ApplyContextStates(bool bForce)
{
  W_ASSERT_DEBUG(m_bRendering || m_bCompute, "Must be either in a rendering or compute scope");

  // First apply material state since this can modify all other states.
  if (bForce || m_StateFlags.IsSet(WRenderContextFlags::MaterialBindingChanged))
  {
    ApplyMaterialState();
    m_StateFlags.Remove(WRenderContextFlags::MaterialBindingChanged);
    m_StateFlags.Add(WRenderContextFlags::BindGroupChanged);
  }

  for (WUInt32 i = 0; i < W_GAL_MAX_BIND_GROUPS; ++i)
  {
    if (m_BindGroupBuilders[i].IsModified())
    {
      m_StateFlags.Add(WRenderContextFlags::BindGroupChanged);
      break;
    }
  }

  bool bRebuildVertexDeclaration = m_StateFlags.IsAnySet(WRenderContextFlags::ShaderStateChanged | WRenderContextFlags::MeshBufferBindingChanged);

  if (bForce || m_StateFlags.IsSet(WRenderContextFlags::ShaderStateChanged))
  {
    W_SUCCEED_OR_RETURN(ApplyShaderState());
    m_StateFlags.Remove(WRenderContextFlags::ShaderStateChanged);
  }

  if (m_pActiveGALShader)
  {
    const bool bDirty = (bForce || m_StateFlags.IsAnySet(WRenderContextFlags::BindGroupLayoutChanged | WRenderContextFlags::BindGroupChanged));

    WLogBlock applyBindingsBlock("Applying Shader Bindings", m_sActiveShader);
    UploadConstants();
    if (bDirty)
    {
      const WUInt32 uiBindGroups = m_pActiveGALShader->GetBindGroupCount();
      for (WUInt32 uiBindGroup = 0; uiBindGroup < uiBindGroups; uiBindGroup++)
      {
        const bool bForceBindGroupUpdate = bForce || m_bDirtyBindGroups[uiBindGroup];
        const bool bHasMaterialBindGroupResource = uiBindGroup == W_GAL_BIND_GROUP_MATERIAL && m_hMaterial.IsValid();
        const bool bBindGroupModified = m_BindGroupBuilders[uiBindGroup].IsModified() && !bHasMaterialBindGroupResource;
        if (bBindGroupModified)
          m_Statistics.m_uiModifiedBindGroup[uiBindGroup]++;

        if (bForceBindGroupUpdate || bBindGroupModified)
        {
          W_SUCCEED_OR_RETURN(ApplyBindGroup(m_pActiveGALShader, uiBindGroup));
        }
        m_bDirtyBindGroups[uiBindGroup] = false;
      }
      m_StateFlags.Remove(WRenderContextFlags::BindGroupLayoutChanged);
      m_StateFlags.Remove(WRenderContextFlags::BindGroupChanged);
    }
  }

  if ((bForce || bRebuildVertexDeclaration) && !m_bCompute)
  {
    if (m_hActiveGALShader.IsInvalidated())
      return W_FAILURE;

    auto pCommandEncoder = GetCommandEncoder();

    if (bForce || m_StateFlags.IsSet(WRenderContextFlags::MeshBufferBindingChanged))
    {
      for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_hVertexBuffers); ++i)
      {
        pCommandEncoder->SetVertexBuffer(i, m_hVertexBuffers[i], m_VertexBufferOffsets[i]);
      }

      if (!m_hIndexBuffer.IsInvalidated())
        pCommandEncoder->SetIndexBuffer(m_hIndexBuffer);
    }

    WGALVertexDeclarationHandle hVertexDeclaration;
    const bool bHasVertexDeclarations = m_VertexAttributes.GetCount() > 0;
    if (bHasVertexDeclarations && BuildVertexDeclaration(m_hActiveGALShader, m_VertexBufferStrides, m_VertexBufferBindingRates, m_VertexAttributes, hVertexDeclaration).Failed())
      return W_FAILURE;

    // If there is a vertex buffer we need a valid vertex declaration as well.
    if (hVertexDeclaration.IsInvalidated())
    {
      for (auto hVertexBuffer : m_hVertexBuffers)
      {
        if (!hVertexBuffer.IsInvalidated())
        {
          return W_FAILURE;
        }
      }
    }

    m_GraphicsPipeline.m_hVertexDeclaration = hVertexDeclaration;
    m_StateFlags.Add(WRenderContextFlags::PipelineChanged);

    m_StateFlags.Remove(WRenderContextFlags::MeshBufferBindingChanged);
  }

  if (bForce || m_StateFlags.IsSet(WRenderContextFlags::NonPipelineStateChanged))
  {
    if (m_bUseUserStencilRefValue)
    {
      // Set the user provided stencil reference value
      m_pGALCommandEncoder->SetStencilReference(m_uiUserStencilRefValue);
    }
    else
    {
      // Set stencil reference value from shader
      m_pGALCommandEncoder->SetStencilReference(m_uiShaderStencilRefValue);
    }

    m_StateFlags.Remove(WRenderContextFlags::NonPipelineStateChanged);
  }

  if (bForce || m_StateFlags.IsSet(WRenderContextFlags::PipelineChanged))
  {
    m_StateFlags.Remove(WRenderContextFlags::PipelineChanged);

    if (m_bRendering)
    {
      m_pGALCommandEncoder->SetGraphicsPipeline(WGALPipelineCache::GetPipeline(m_GraphicsPipeline));
    }
    else if (m_bCompute)
    {
      m_pGALCommandEncoder->SetComputePipeline(WGALPipelineCache::GetPipeline(m_ComputePipeline));
    }
  }

  if (m_pActiveGALShader)
  {
    for (WUInt32 i = 0; i < m_pActiveGALShader->GetBindGroupCount(); ++i)
    {
      const bool bHasMaterialBindGroupResource = i == W_GAL_BIND_GROUP_MATERIAL && m_hMaterial.IsValid();
      if (!bHasMaterialBindGroupResource)
      {
        W_ASSERT_DEV(m_pActiveGALShader->GetBindGroupLayout(i) == m_BindGroups[i].m_hBindGroupLayout, "Invalid Bind Group Layout");
      }
      else
      {
        W_ASSERT_DEV(m_pActiveGALShader->GetBindGroupLayout(i) == m_pMaterialBindGroup->GetDescription().m_hBindGroupLayout, "Invalid Bind Group Resource Layout");
      }
    }
  }
  return W_SUCCESS;
}

void WRenderContext::ResetContextState()
{
  W_PROFILE_SCOPE("WRenderContext::ResetContextState");

  m_StateFlags = WRenderContextFlags::AllStatesInvalid;

  m_hMaterial.Invalidate();
  m_hNewMaterial.Invalidate();
  m_pMaterial = nullptr;
  m_pMaterialBindGroup = nullptr;

  m_hActiveShader.Invalidate();
  m_PermutationVariables.Clear();
  m_hActiveShaderPermutation.Invalidate();
  m_sActiveShader.Clear();
  m_hActiveGALShader.Invalidate();
  m_pActiveGALShader = nullptr;

  static_assert(W_ARRAY_SIZE(m_hVertexBuffers) == W_GAL_MAX_VERTEX_BUFFER_COUNT);
  for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
    m_VertexBufferStrides[i] = 0;
    m_VertexBufferBindingRates[i] = WGALVertexBindingRate::Vertex;
    m_VertexBufferOffsets[i] = 0;
  }
  m_hIndexBuffer.Invalidate();
  m_VertexAttributes.Clear();
  m_GraphicsPipeline.m_Topology = WGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;

  ResetBindGroups();
}

WGlobalConstants& WRenderContext::WriteGlobalConstants()
{
  WConstantBufferStorage<WGlobalConstants>* pStorage = nullptr;
  W_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForWriting();
}

const WGlobalConstants& WRenderContext::ReadGlobalConstants() const
{
  WConstantBufferStorage<WGlobalConstants>* pStorage = nullptr;
  W_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForReading();
}

void WRenderContext::SetGlobalAndWorldTimeConstants(WTime worldTime)
{
  auto& gc = WriteGlobalConstants();

  // Wrap around to prevent floating point issues. A wrap around of 1000 allows all frequencies with 3 digits after the decimal.
  const double fWrapAround = 1000.0;
  gc.DeltaTime = (float)WClock::GetGlobalClock()->GetTimeDiff().GetSeconds();
  gc.GlobalTime = (float)WMath::Mod(WClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), fWrapAround);
  gc.WorldTime = (float)WMath::Mod(worldTime.GetSeconds(), fWrapAround);
}

// static
WConstantBufferStorageHandle WRenderContext::CreateConstantBufferStorage(WUInt32 uiSizeInBytes, WConstantBufferStorageBase*& out_pStorage)
{
  W_ASSERT_DEV(WMemoryUtils::IsSizeAligned(uiSizeInBytes, 16u), "Storage struct for constant buffer is not aligned to 16 bytes");

  W_LOCK(s_ConstantBufferStorageMutex);

  WConstantBufferStorageBase* pStorage = nullptr;

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (it.IsValid())
  {
    WDynamicArray<WConstantBufferStorageBase*>& storageForSize = it.Value();
    if (!storageForSize.IsEmpty())
    {
      pStorage = storageForSize[0];
      storageForSize.RemoveAtAndSwap(0);
    }
  }

  if (pStorage == nullptr)
  {
    pStorage = W_DEFAULT_NEW(WConstantBufferStorageBase, uiSizeInBytes);
  }

  out_pStorage = pStorage;
  s_DirtyConstantBuffers.Insert(pStorage);
  return WConstantBufferStorageHandle(s_ConstantBufferStorageTable.Insert(pStorage));
}

// static
void WRenderContext::DeleteConstantBufferStorage(WConstantBufferStorageHandle& inout_hStorage)
{
  W_LOCK(s_ConstantBufferStorageMutex);

  WConstantBufferStorageBase* pStorage = nullptr;
  if (s_ConstantBufferStorageTable.Remove(inout_hStorage.m_InternalId, &pStorage))
  {
    pStorage->BeforeBeginFrame();
    s_DirtyConstantBuffers.Remove(pStorage);

    WUInt32 uiSizeInBytes = pStorage->m_Data.GetCount();

    auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
    if (!it.IsValid())
    {
      it = s_FreeConstantBufferStorage.Insert(uiSizeInBytes, WDynamicArray<WConstantBufferStorageBase*>());
    }

    it.Value().PushBack(pStorage);
  }

  inout_hStorage.Invalidate();
}

// static
bool WRenderContext::TryGetConstantBufferStorage(WConstantBufferStorageHandle hStorage, WConstantBufferStorageBase*& out_pStorage)
{
  W_LOCK(s_ConstantBufferStorageMutex);

  return s_ConstantBufferStorageTable.TryGetValue(hStorage.m_InternalId, out_pStorage);
}

void WRenderContext::MarktConstantBufferStorageModified(WConstantBufferStorageBase* pDirtyStorage)
{
  W_LOCK(s_ConstantBufferStorageMutex);

  s_DirtyConstantBuffers.Insert(pDirtyStorage);
}

// static
WGALSamplerStateCreationDescription WRenderContext::GetDefaultSamplerState(WBitflags<WDefaultSamplerFlags> flags)
{
  WGALSamplerStateCreationDescription desc;
  desc.m_MinFilter = flags.IsSet(WDefaultSamplerFlags::LinearFiltering) ? WGALTextureFilterMode::Linear : WGALTextureFilterMode::Point;
  desc.m_MagFilter = flags.IsSet(WDefaultSamplerFlags::LinearFiltering) ? WGALTextureFilterMode::Linear : WGALTextureFilterMode::Point;
  desc.m_MipFilter = flags.IsSet(WDefaultSamplerFlags::LinearFiltering) ? WGALTextureFilterMode::Linear : WGALTextureFilterMode::Point;

  desc.m_AddressU = flags.IsSet(WDefaultSamplerFlags::Clamp) ? WImageAddressMode::Clamp : WImageAddressMode::Repeat;
  desc.m_AddressV = flags.IsSet(WDefaultSamplerFlags::Clamp) ? WImageAddressMode::Clamp : WImageAddressMode::Repeat;
  desc.m_AddressW = flags.IsSet(WDefaultSamplerFlags::Clamp) ? WImageAddressMode::Clamp : WImageAddressMode::Repeat;
  return desc;
}

// private functions
//////////////////////////////////////////////////////////////////////////

// static
void WRenderContext::LoadBuiltinShader(WShaderUtils::WBuiltinShaderType type, WShaderUtils::WBuiltinShader& out_shader)
{
  WShaderResourceHandle hActiveShader;
  bool bStereo = false;
  switch (type)
  {
    case WShaderUtils::WBuiltinShaderType::CopyImageArray:
      bStereo = true;
      [[fallthrough]];
    case WShaderUtils::WBuiltinShaderType::CopyImage:
      hActiveShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/Copy.WShader");
      break;
    case WShaderUtils::WBuiltinShaderType::DownscaleImageArray:
      bStereo = true;
      [[fallthrough]];
    case WShaderUtils::WBuiltinShaderType::DownscaleImage:
      hActiveShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/Downscale.WShader");
      break;
  }

  W_ASSERT_DEV(hActiveShader.IsValid(), "Could not load builtin shader!");

  WHashTable<WHashedString, WHashedString> permutationVariables;
  static WHashedString sTrue = WMakeHashedString("TRUE");
  static WHashedString sFalse = WMakeHashedString("FALSE");
  static WHashedString sCameraMode = WMakeHashedString("CAMERA_MODE");
  static WHashedString sPerspective = WMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static WHashedString sStereo = WMakeHashedString("CAMERA_MODE_STEREO");

  permutationVariables.Insert(sCameraMode, bStereo ? sStereo : sPerspective);
  W_ASSERT_DEV(!bStereo || WGALDevice::GetDefaultDevice()->GetCapabilities().m_bSupportsVSRenderTargetArrayIndex, "Vertex shader render target index must be supported for stereo rendering.");

  WShaderPermutationResourceHandle hActiveShaderPermutation = WShaderManager::PreloadSinglePermutation(hActiveShader, permutationVariables, false);

  W_ASSERT_DEV(hActiveShaderPermutation.IsValid(), "Could not load builtin shader permutation!");

  WResourceLock<WShaderPermutationResource> pShaderPermutation(hActiveShaderPermutation, WResourceAcquireMode::BlockTillLoaded);

  W_ASSERT_DEV(pShaderPermutation->IsShaderValid(), "Builtin shader permutation shader is invalid!");

  out_shader.m_hActiveGALShader = pShaderPermutation->GetGALShader();
  W_ASSERT_DEV(!out_shader.m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

  out_shader.m_hBlendState = pShaderPermutation->GetBlendState();
  out_shader.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
  out_shader.m_hRasterizerState = pShaderPermutation->GetRasterizerState();
}

// static
void WRenderContext::RegisterImmutableSamplers()
{
  WGALImmutableSamplers::RegisterImmutableSampler(WMakeHashedString("LinearSampler"), GetDefaultSamplerState(WDefaultSamplerFlags::LinearFiltering)).AssertSuccess("Failed to register immutable sampler");
  WGALImmutableSamplers::RegisterImmutableSampler(WMakeHashedString("LinearClampSampler"), GetDefaultSamplerState(WDefaultSamplerFlags::LinearFiltering | WDefaultSamplerFlags::Clamp)).AssertSuccess("Failed to register immutable sampler");
  WGALImmutableSamplers::RegisterImmutableSampler(WMakeHashedString("PointSampler"), GetDefaultSamplerState(WDefaultSamplerFlags::PointFiltering)).AssertSuccess("Failed to register immutable sampler");
  WGALImmutableSamplers::RegisterImmutableSampler(WMakeHashedString("PointClampSampler"), GetDefaultSamplerState(WDefaultSamplerFlags::PointFiltering | WDefaultSamplerFlags::Clamp)).AssertSuccess("Failed to register immutable sampler");
}

// static
void WRenderContext::OnEngineStartup()
{
  WShaderUtils::g_RequestBuiltinShaderCallback = WMakeDelegate(WRenderContext::LoadBuiltinShader);
}

// static
void WRenderContext::OnEngineShutdown()
{
  WShaderUtils::g_RequestBuiltinShaderCallback = {};
  WShaderStageBinary::OnEngineShutdown();

  for (auto rc : s_Instances)
    W_DEFAULT_DELETE(rc);

  s_Instances.Clear();

  // Cleanup vertex declarations
  {
    for (auto it = s_GALVertexDeclarations.GetIterator(); it.IsValid(); ++it)
    {
      WGALDevice::GetDefaultDevice()->DestroyVertexDeclaration(it.Value());
    }

    s_GALVertexDeclarations.Clear();
  }

  // Cleanup constant buffer storage
  {
    for (auto it = s_ConstantBufferStorageTable.GetIterator(); it.IsValid(); ++it)
    {
      WConstantBufferStorageBase* pStorage = it.Value();
      W_DEFAULT_DELETE(pStorage);
    }

    s_ConstantBufferStorageTable.Clear();

    for (auto it = s_FreeConstantBufferStorage.GetIterator(); it.IsValid(); ++it)
    {
      WDynamicArray<WConstantBufferStorageBase*>& storageForSize = it.Value();
      for (auto& pStorage : storageForSize)
      {
        W_DEFAULT_DELETE(pStorage);
      }
    }

    s_FreeConstantBufferStorage.Clear();
    s_DirtyConstantBuffers.Clear();
  }
}

// static
void WRenderContext::GALStaticDeviceEventHandler(const WGALDeviceEvent& e)
{
  if (e.m_Type == WGALDeviceEvent::Type::AfterBeginCommands)
  {
    s_pCommandEncoder = e.m_pCommandEncoder;
    if (s_pDefaultInstance)
    {
      s_pDefaultInstance->m_StateFlags = WRenderContextFlags::AllStatesInvalid;
      for (WUInt32 i = 0; i < W_GAL_MAX_BIND_GROUPS; ++i)
      {
        s_pDefaultInstance->m_bDirtyBindGroups[i] = true;
      }
      s_pDefaultInstance->m_pGALCommandEncoder = e.m_pCommandEncoder;

      // this is executed every frame
      const WInt32 iQuality = WMath::Clamp<WInt32>(cvar_RenderingTextureQuality, 0, WGALTextureQuality::Anisotropic16x);
      s_pDefaultInstance->SetDefaultTextureQuality(static_cast<WGALTextureQuality::Enum>(iQuality));
    }
  }
  else if (e.m_Type == WGALDeviceEvent::Type::BeforeEndCommands)
  {
    s_pCommandEncoder = nullptr;
    if (s_pDefaultInstance)
      s_pDefaultInstance->m_pGALCommandEncoder = nullptr;
  }
  else if (e.m_Type == WGALDeviceEvent::Type::BeforeBeginFrame)
  {
    if (s_pDefaultInstance)
    {
      s_pDefaultInstance->ResetContextState();
    }

    W_LOCK(s_ConstantBufferStorageMutex);

    for (auto it = s_ConstantBufferStorageTable.GetIterator(); it.IsValid(); ++it)
    {
      it.Value()->BeforeBeginFrame();
      s_DirtyConstantBuffers.Insert(it.Value());
    }
  }
  else if (e.m_Type == WGALDeviceEvent::Type::BeforeEndFrame)
  {
    WStats::SetStat("RenderContext/BindGroupWrites", WBindGroupBuilder::s_uiWrites);
    WBindGroupBuilder::s_uiWrites = 0;
    WStats::SetStat("RenderContext/BindGroupReads", WBindGroupBuilder::s_uiReads);
    WBindGroupBuilder::s_uiReads = 0;

    if (s_pDefaultInstance)
    {
      WRenderContext::Statistics stats = s_pDefaultInstance->GetAndResetStatistics();
      s_LastFrameStatistics = stats;

      WStats::SetStat("RenderContext/Drawcalls", stats.m_uiDrawcalls);
      WStats::SetStat("RenderContext/Triangles", (double)stats.m_uiTriangles);

      for (WUInt32 i = 0; i < W_GAL_MAX_BIND_GROUPS; ++i)
      {
        WStringBuilder groupName;
        groupName.SetFormat("RenderContext/BindGroup_{}_Modified", i);
        WStats::SetStat(groupName, stats.m_uiModifiedBindGroup[i]);
        groupName.SetFormat("RenderContext/BindGroup_{}_LayoutChanged", i);
        WStats::SetStat(groupName, stats.m_uiLayoutChanged[i]);
      }
    }
  }
}

// static
WResult WRenderContext::BuildVertexDeclaration(WGALShaderHandle hShader, WArrayPtr<WUInt32> vertexBufferStrides, WArrayPtr<WEnum<WGALVertexBindingRate>> vertexBufferBindingRates, WArrayPtr<WGALVertexAttribute> vertexAttributes, WGALVertexDeclarationHandle& out_Declaration)
{
  WInt32 iHighestUsedBinding = -1;
  for (WUInt32 i = 0; i < vertexAttributes.GetCount(); ++i)
  {
    iHighestUsedBinding = WMath::Max(iHighestUsedBinding, static_cast<WInt32>(vertexAttributes[i].m_uiVertexBufferSlot));
  }
  W_ASSERT_DEBUG(iHighestUsedBinding < (WInt32)vertexBufferStrides.GetCount(), "Not enough vertex buffer strides");
  W_ASSERT_DEBUG(iHighestUsedBinding < (WInt32)vertexBufferBindingRates.GetCount(), "Not enough vertex buffer binding rates");

  ShaderVertexDecl svd;
  {
    svd.m_hShader = hShader;

    WHashStreamWriter32 writer;
    writer.WriteBytes(vertexAttributes.GetPtr(), vertexAttributes.ToByteArray().GetCount()).IgnoreResult();
    writer.WriteBytes(vertexBufferStrides.GetPtr(), vertexBufferStrides.GetSubArray(0, iHighestUsedBinding + 1).ToByteArray().GetCount()).IgnoreResult();
    writer.WriteBytes(vertexBufferBindingRates.GetPtr(), vertexBufferBindingRates.GetSubArray(0, iHighestUsedBinding + 1).ToByteArray().GetCount()).IgnoreResult();
    svd.m_uiVertexAttributesHash = writer.GetHashValue();
  }

  bool bExisted = false;
  auto it = s_GALVertexDeclarations.FindOrAdd(svd, &bExisted);

  if (!bExisted)
  {
    WGALVertexDeclarationCreationDescription vd;
    vd.m_hShader = hShader;
    vd.m_VertexAttributes = vertexAttributes;

    for (WInt32 bufferIndex = 0; bufferIndex <= iHighestUsedBinding; ++bufferIndex)
    {
      WGALVertexBinding binding;
      binding.m_uiStride = vertexBufferStrides[bufferIndex];
      binding.m_Rate = vertexBufferBindingRates[bufferIndex];
      vd.m_VertexBindings.PushBack(binding);
    }
    out_Declaration = WGALDevice::GetDefaultDevice()->CreateVertexDeclaration(vd);

    if (out_Declaration.IsInvalidated())
    {
      /* This can happen when the resource system gives you a fallback resource, which then selects a shader that
      does not fit the mesh layout.
      E.g. when a material is not yet loaded and the fallback material is used, that fallback material may
      use another shader, that requires more data streams, than what the mesh provides.
      This problem will go away, once the proper material is loaded.

      This can be fixed by ensuring that the fallback material uses a shader that only requires data that is
      always there, e.g. only position and maybe a texcoord, and of course all meshes must provide at least those
      data streams.

      Otherwise, this is harmless, the renderer will ignore invalid drawcalls and once all the correct stuff is
      available, it will work.
      */
      s_GALVertexDeclarations.Remove(it);
      WLog::Warning("Failed to create vertex declaration");
      return W_FAILURE;
    }

    it.Value() = out_Declaration;
  }

  out_Declaration = it.Value();
  return W_SUCCESS;
}

void WRenderContext::UploadConstants()
{
  WBindGroupBuilder& bindGroup = WRenderContext::GetDefaultInstance()->GetBindGroup();
  bindGroup.BindBuffer("WGlobalConstants", m_hGlobalConstantBufferStorage);

  W_LOCK(s_ConstantBufferStorageMutex);

  for (auto it = s_DirtyConstantBuffers.GetIterator(); it.IsValid(); ++it)
  {
    WConstantBufferStorageBase* pConstantBufferStorage = it.Key();
    pConstantBufferStorage->UploadData(m_pGALCommandEncoder);
  }
  s_DirtyConstantBuffers.Clear();
}

void WRenderContext::SetShaderPermutationVariableInternal(const WHashedString& sName, const WHashedString& sValue)
{
  WHashedString* pOldValue = nullptr;
  m_PermutationVariables.TryGetValue(sName, pOldValue);

  if (pOldValue == nullptr || *pOldValue != sValue)
  {
    m_PermutationVariables.Insert(sName, sValue);
    m_StateFlags.Add(WRenderContextFlags::ShaderStateChanged);
  }
}

void WRenderContext::BindShaderInternal(const WShaderResourceHandle& hShader, WBitflags<WShaderBindFlags> flags)
{
  if (flags.IsAnySet(WShaderBindFlags::ForceRebind) || m_hActiveShader != hShader)
  {
    m_ShaderBindFlags = flags;
    m_hActiveShader = hShader;

    m_StateFlags.Add(WRenderContextFlags::ShaderStateChanged);
  }
}

WResult WRenderContext::ApplyShaderState()
{
  m_hActiveGALShader.Invalidate();

  m_StateFlags.Add(WRenderContextFlags::PipelineChanged);

  if (!m_hActiveShader.IsValid())
    return W_FAILURE;

  m_hActiveShaderPermutation = WShaderManager::PreloadSinglePermutation(m_hActiveShader, m_PermutationVariables, m_bAllowAsyncShaderLoading);

  if (!m_hActiveShaderPermutation.IsValid())
    return W_FAILURE;

  // Non-material shaders are always force-loaded so we don't accidentally miss to render important passes.
  const bool bAsyncShaderLoading = m_bAllowAsyncShaderLoading && m_hMaterial.IsValid();
  WResourceLock<WShaderPermutationResource> pShaderPermutation(m_hActiveShaderPermutation, bAsyncShaderLoading ? WResourceAcquireMode::AllowLoadingFallback : WResourceAcquireMode::BlockTillLoaded);

  if (!pShaderPermutation.IsValid() || !pShaderPermutation->IsShaderValid())
    return W_FAILURE;

  m_sActiveShader = pShaderPermutation->GetResourceDescription();
  m_hActiveGALShader = pShaderPermutation->GetGALShader();
  m_GraphicsPipeline.m_hShader = m_hActiveGALShader;
  m_ComputePipeline.m_hShader = m_hActiveGALShader;
  W_ASSERT_DEV(!m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");
  m_pActiveGALShader = WGALDevice::GetDefaultDevice()->GetShader(m_hActiveGALShader);
  W_ASSERT_DEV(m_pActiveGALShader, "Invalid GAL Shader handle.");
  const WUInt32 uiBindGroups = m_pActiveGALShader->GetBindGroupCount();

  // On DX11 or other platforms that do not support multiple bind groups, we always need to invalidate all bind groups as another bind group could have overwritten the state of a different one as they all write to the same state.
  // E.g. a shader that has only one bind group, can overwrite everything. If we then switch to a shader that uses 4 bind groups, while bind groups 1 to 3 have not been touched, they are still dirty as the old 1 BG layout and the new 4 BG layout can overlap outside of BG0.
  const bool bForceAllBindGroupsDirty = !WGALDevice::GetDefaultDevice()->GetCapabilities().m_bSupportsMultipleBindGroups;
  for (WUInt32 i = 0; i < uiBindGroups; ++i)
  {

    if (bForceAllBindGroupsDirty || m_pActiveGALShader->GetBindGroupLayout(i) != m_BindGroups[i].m_hBindGroupLayout)
    {
      m_Statistics.m_uiLayoutChanged[i]++;
      m_bDirtyBindGroups[i] = true;
      m_StateFlags.Add(WRenderContextFlags::BindGroupLayoutChanged);
    }
  }

  // Set render state from shader
  if (!m_bCompute)
  {
    if (!m_ShaderBindFlags.IsSet(WShaderBindFlags::NoBlendState))
      m_GraphicsPipeline.m_hBlendState = pShaderPermutation->GetBlendState();

    if (!m_ShaderBindFlags.IsSet(WShaderBindFlags::NoRasterizerState))
      m_GraphicsPipeline.m_hRasterizerState = pShaderPermutation->GetRasterizerState();

    if (!m_ShaderBindFlags.IsSet(WShaderBindFlags::NoDepthStencilState))
    {
      m_GraphicsPipeline.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
      m_uiShaderStencilRefValue = pShaderPermutation->GetShaderStencilRefValue();
      m_bUseUserStencilRefValue = pShaderPermutation->GetUseUserStencilRefValue();
    }
    else
    {
      m_bUseUserStencilRefValue = true;
    }

    m_StateFlags.Add(WRenderContextFlags::NonPipelineStateChanged);
  }

  return W_SUCCESS;
}

void WRenderContext::ApplyMaterialState()
{
  if (!m_hNewMaterial.IsValid())
  {
    BindShaderInternal(WShaderResourceHandle(), WShaderBindFlags::Default);
    m_pMaterial = nullptr;
    m_pMaterialBindGroup = nullptr;
    return;
  }

  if (m_hNewMaterial != m_hMaterial)
  {
    // check whether material has been modified
    WResourceLock<WMaterialResource> pMaterial(m_hNewMaterial, WResourceAcquireMode::AllowLoadingFallback);

    const WMaterialManager::MaterialData* data = WMaterialManager::GetMaterialData(pMaterial.GetPointer());
    if (data == nullptr)
    {
      BindShaderInternal(WShaderResourceHandle(), WShaderBindFlags::Default);
      return;
    }

    BindShaderInternal(data->m_hShader, WShaderBindFlags::Default);
    for (const WPermutationVar& perm : data->m_PermutationVars)
    {
      SetShaderPermutationVariableInternal(perm.m_sName, perm.m_sValue);
    }

    m_hMaterial = m_hNewMaterial;
    // We don't know the permutation to use yet and thus also not the correct bind group layout. Therefore, we store the raw address of the material to be able to look up the correct bind group in ApplyBindGroup. We can't acquire the resource lock again as that might result in a different address (fallback vs real) and we would mismatch the material data and bind group from two different materials.
    m_pMaterial = pMaterial.GetPointer();
  }
}

WResult WRenderContext::ApplyBindGroup(const WGALShader* pShader, WUInt32 uiBindGroup)
{
  WGALBindGroupLayoutHandle hLayout = pShader->GetBindGroupLayout(uiBindGroup);
  if (hLayout.IsInvalidated())
    return W_FAILURE;

  const bool bHasMaterialBindGroupResource = uiBindGroup == W_GAL_BIND_GROUP_MATERIAL && m_hMaterial.IsValid();
  if (bHasMaterialBindGroupResource)
  {
    WGALBindGroupHandle hBindGroup = WMaterialManager::GetMaterialBindGroup(m_pMaterial, hLayout);
    if (hBindGroup.IsInvalidated())
      return W_FAILURE;

    m_pMaterialBindGroup = m_pGALCommandEncoder->GetDevice().GetBindGroup(hBindGroup);

    m_pGALCommandEncoder->SetBindGroup(uiBindGroup, hBindGroup);
    return W_SUCCESS;
  }
  WBitflags<WGALBindGroupItemFlags> metaFlags;
  m_BindGroupBuilders[uiBindGroup].CreateBindGroup(hLayout, m_BindGroups[uiBindGroup], metaFlags);
  m_pGALCommandEncoder->SetBindGroup(uiBindGroup, m_BindGroups[uiBindGroup]);
  return W_SUCCESS;
}

void WRenderContext::SetDefaultTextureQuality(WGALTextureQuality::Enum quality, bool bForce)
{
  if (!bForce && m_DefaultTextureQuality == quality)
    return;

  m_DefaultTextureQuality = quality;
  cvar_RenderingTextureQuality = static_cast<WInt32>(quality);

  if (m_pGALCommandEncoder)
  {
    switch (m_DefaultTextureQuality)
    {
      case WGALTextureQuality::Nearest:
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowestQuality, WGALTextureQuality::Nearest);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowQuality, WGALTextureQuality::Nearest);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::DefaultQuality, WGALTextureQuality::Nearest);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighQuality, WGALTextureQuality::Nearest);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighestQuality, WGALTextureQuality::Nearest);
        break;

      case WGALTextureQuality::Bilinear:
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowestQuality, WGALTextureQuality::Bilinear);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowQuality, WGALTextureQuality::Bilinear);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::DefaultQuality, WGALTextureQuality::Bilinear);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighQuality, WGALTextureQuality::Trilinear);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighestQuality, WGALTextureQuality::Anisotropic2x);
        break;

      case WGALTextureQuality::Trilinear:
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowestQuality, WGALTextureQuality::Bilinear);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowQuality, WGALTextureQuality::Bilinear);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::DefaultQuality, WGALTextureQuality::Trilinear);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighQuality, WGALTextureQuality::Anisotropic2x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighestQuality, WGALTextureQuality::Anisotropic4x);
        break;

      case WGALTextureQuality::Anisotropic2x:
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowestQuality, WGALTextureQuality::Bilinear);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowQuality, WGALTextureQuality::Trilinear);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::DefaultQuality, WGALTextureQuality::Anisotropic2x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighQuality, WGALTextureQuality::Anisotropic4x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighestQuality, WGALTextureQuality::Anisotropic8x);
        break;

      case WGALTextureQuality::Anisotropic4x:
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowestQuality, WGALTextureQuality::Trilinear);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowQuality, WGALTextureQuality::Anisotropic2x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::DefaultQuality, WGALTextureQuality::Anisotropic4x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighQuality, WGALTextureQuality::Anisotropic8x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighestQuality, WGALTextureQuality::Anisotropic16x);
        break;

      case WGALTextureQuality::Anisotropic8x:
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowestQuality, WGALTextureQuality::Anisotropic2x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowQuality, WGALTextureQuality::Anisotropic4x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::DefaultQuality, WGALTextureQuality::Anisotropic8x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighQuality, WGALTextureQuality::Anisotropic16x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighestQuality, WGALTextureQuality::Anisotropic16x);
        break;

      case WGALTextureQuality::Anisotropic16x:
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowestQuality, WGALTextureQuality::Anisotropic4x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::LowQuality, WGALTextureQuality::Anisotropic8x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::DefaultQuality, WGALTextureQuality::Anisotropic16x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighQuality, WGALTextureQuality::Anisotropic16x);
        m_pGALCommandEncoder->GetDevice().SetTextureQualityMode(WGALTextureQualitySlot::HighestQuality, WGALTextureQuality::Anisotropic16x);
        break;
    }

    m_pGALCommandEncoder->GetDevice().UpdateTextureQuality();
  }
}

void WRenderContext::SetAllowAsyncShaderLoading(bool bAllow)
{
  m_bAllowAsyncShaderLoading = bAllow;
}

bool WRenderContext::GetAllowAsyncShaderLoading()
{
  return m_bAllowAsyncShaderLoading;
}

W_STATICLINK_FILE(RendererCore, RendererCore_RenderContext_Implementation_RenderContext);
