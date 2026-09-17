#include <RendererCore/RendererCorePCH.h>

#include <Core/World/World.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

constexpr WUInt32 s_uiSkinningBufferIndex = 2;

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WRenderDataManager);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderDataManager, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WRenderDataManager::WRenderDataManager(WWorld* pWorld)
  : WWorldModule(pWorld)
{
  WRenderWorld::GetExtractionEvent().AddEventHandler(WMakeDelegate(&WRenderDataManager::OnExtractionEvent, this));

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  WGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(WPerInstanceData);
  desc.m_uiTotalSize = 1024 * desc.m_uiStructSize; // TODO: make initial size configurable
  desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, "Static Instance Data"));
  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, "Dynamic Instance Data"));

  // Skinning buffer
  desc.m_uiStructSize = sizeof(WShaderTransform);
  desc.m_uiTotalSize = 1024 * desc.m_uiStructSize; // TODO: make initial size configurable

  W_ASSERT_DEBUG(m_Buffers.GetCount() == s_uiSkinningBufferIndex, "Unexpected buffer index");
  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, "Skinning Data"));
}

WRenderDataManager::~WRenderDataManager()
{
  WRenderWorld::GetExtractionEvent().RemoveEventHandler(WMakeDelegate(&WRenderDataManager::OnExtractionEvent, this));

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  for (auto& hBuffer : m_Buffers)
  {
    pDevice->DestroyDynamicBuffer(hBuffer);
  }
}

void WRenderDataManager::Initialize()
{
  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WRenderDataManager::CompactSkinningDataBuffer, this);
    desc.m_Phase = WWorldUpdatePhase::PostTransform;
    desc.m_fPriority = -1000.0f;

    RegisterUpdateFunction(desc);
  }
}

WArrayPtr<WPerInstanceData> WRenderDataManager::GetOrCreateInstanceData(const WComponent* pOwnerComponent, bool bDynamic, WGALDynamicBufferHandle& out_hBuffer, WInstanceDataOffset& inout_instanceDataOffset, WUInt32 uiCount /*= 1*/) const
{
  W_LOCK(m_Mutex);

  const WUInt32 uiBufferIndex = bDynamic ? 1 : 0;

  if (inout_instanceDataOffset.IsInvalidated() == false && inout_instanceDataOffset.m_uiIsDynamic != uiBufferIndex)
  {
    // The instance data was allocated in a different buffer, need to re-allocate.
    auto pOldInstanceDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[inout_instanceDataOffset.m_uiIsDynamic]);
    pOldInstanceDataBuffer->Deallocate(inout_instanceDataOffset.m_uiOffset);
    inout_instanceDataOffset = {};
  }

  out_hBuffer = m_Buffers[uiBufferIndex];

  auto pInstanceDataBuffer = m_ExtractionData.m_pBuffers.GetCount() > uiBufferIndex ? m_ExtractionData.m_pBuffers[uiBufferIndex] : nullptr;
  if (pInstanceDataBuffer == nullptr)
  {
    pInstanceDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(out_hBuffer);
  }

  if (inout_instanceDataOffset.IsInvalidated())
  {
    WComponentHandle hOwnerComponent = pOwnerComponent != nullptr ? pOwnerComponent->GetHandle() : WComponentHandle();
    inout_instanceDataOffset.m_uiOffset = pInstanceDataBuffer->Allocate(hOwnerComponent, uiCount, WGALDynamicBuffer::AllocateFlags::None, WFrameAllocator::GetCurrentAllocator());
    inout_instanceDataOffset.m_uiIsDynamic = uiBufferIndex;
  }

  return pInstanceDataBuffer->MapForWriting<WPerInstanceData>(inout_instanceDataOffset.m_uiOffset);
}

void WRenderDataManager::DeleteInstanceData(WInstanceDataOffset& inout_instanceDataOffset) const
{
  W_LOCK(m_Mutex);

  if (inout_instanceDataOffset.IsInvalidated() == false)
  {
    const WUInt32 uiBufferIndex = inout_instanceDataOffset.m_uiIsDynamic;

    auto pInstanceDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[uiBufferIndex]);

    pInstanceDataBuffer->Deallocate(inout_instanceDataOffset.m_uiOffset);
    inout_instanceDataOffset = {};
  }
}

WUInt32 WRenderDataManager::RegisterCustomInstanceData(const WGALBufferCreationDescription& desc, WStringView sDebugName, WDelegate<void()> beforeUploadCallback /*= {}*/)
{
  W_LOCK(m_Mutex);

  for (WUInt32 i = 0; i < m_Buffers.GetCount(); ++i)
  {
    auto pBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[i]);
    if (pBuffer->GetDescription() == desc && pBuffer->GetDebugName() == sDebugName)
    {
      return i;
    }
  }

  WUInt32 uiBufferIndex = m_Buffers.GetCount();

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, sDebugName));

  if (beforeUploadCallback.IsValid())
  {
    m_BeforeUploadCallbacks.EnsureCount(uiBufferIndex + 1);
    m_BeforeUploadCallbacks[uiBufferIndex] = beforeUploadCallback;
  }

  return uiBufferIndex;
}

WByteArrayPtr WRenderDataManager::GetOrCreateCustomInstanceData(WUInt32 uiCustomDataIndex, WUInt32 uiStructByteSize, const WComponent* pOwnerComponent, WGALDynamicBufferHandle& out_hBuffer, WCustomInstanceDataOffset& inout_instanceDataOffset, WUInt32 uiCount) const
{
  W_LOCK(m_Mutex);

  out_hBuffer = m_Buffers[uiCustomDataIndex];

  auto pInstanceDataBuffer = m_ExtractionData.m_pBuffers.GetCount() > uiCustomDataIndex ? m_ExtractionData.m_pBuffers[uiCustomDataIndex] : nullptr;
  if (pInstanceDataBuffer == nullptr)
  {
    pInstanceDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(out_hBuffer);
  }

  W_ASSERT_DEV(pInstanceDataBuffer->GetDescription().m_uiStructSize == uiStructByteSize, "Requested struct size {} does not match the registered size {}.", uiStructByteSize, pInstanceDataBuffer->GetDescription().m_uiStructSize);

  if (inout_instanceDataOffset.IsInvalidated())
  {
    inout_instanceDataOffset.m_uiOffset = pInstanceDataBuffer->Allocate(pOwnerComponent->GetHandle(), uiCount, WGALDynamicBuffer::AllocateFlags::None, WFrameAllocator::GetCurrentAllocator());
  }

  return pInstanceDataBuffer->MapBytesForWriting(inout_instanceDataOffset.m_uiOffset);
}

void WRenderDataManager::DeleteCustomInstanceData(WUInt32 uiCustomDataIndex, WCustomInstanceDataOffset& inout_instanceDataOffset) const
{
  W_LOCK(m_Mutex);

  if (inout_instanceDataOffset.IsInvalidated() == false)
  {
    auto pInstanceDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[uiCustomDataIndex]);

    pInstanceDataBuffer->Deallocate(inout_instanceDataOffset.m_uiOffset);
    inout_instanceDataOffset = {};
  }
}

void WRenderDataManager::CompactCustomInstanceDataBuffer(WUInt32 uiCustomDataIndex, WUInt32 uiMaxSteps)
{
  W_LOCK(m_Mutex);

  auto pInstanceDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[uiCustomDataIndex]);

  WTempHybridArray<WGALDynamicBuffer::ChangedAllocation, 16> changedAllocations;
  pInstanceDataBuffer->RunCompactionSteps(changedAllocations, uiMaxSteps);

  for (const auto& changedAllocation : changedAllocations)
  {
    WComponentHandle hComponent(WComponentId(changedAllocation.m_uiUserData));
    WComponent* pComponent = nullptr;
    W_VERIFY(GetWorld()->TryGetComponent(hComponent, pComponent), "Invalid component handle");

    WMsgCustomInstanceDataOffsetChanged msg;
    msg.m_NewOffset.m_uiOffset = changedAllocation.m_uiNewOffset;
    W_VERIFY(pComponent->SendMessage(msg), "Component of type '{}' did not handle WMsgCustomInstanceDataOffsetChanged.", pComponent->GetDynamicRTTI()->GetTypeName());
  }
}

WArrayPtr<WShaderTransform> WRenderDataManager::GetOrCreateSkinningData(const WComponent* pOwnerComponent, WCustomInstanceDataOffset& inout_instanceDataOffset, WUInt32 uiNumTransforms) const
{
  WGALDynamicBufferHandle hDummy;
  return GetOrCreateCustomInstanceData<WShaderTransform>(s_uiSkinningBufferIndex, pOwnerComponent, hDummy, inout_instanceDataOffset, uiNumTransforms);
}

WArrayPtr<const WShaderTransform> WRenderDataManager::GetSkinningData(const WCustomInstanceDataOffset& instanceDataOffset) const
{
  W_LOCK(m_Mutex);

  auto pInstanceDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[s_uiSkinningBufferIndex]);

  return pInstanceDataBuffer->MapForReading<WShaderTransform>(instanceDataOffset.m_uiOffset);
}

void WRenderDataManager::DeleteSkinningData(WCustomInstanceDataOffset& inout_instanceDataOffset) const
{
  DeleteCustomInstanceData(s_uiSkinningBufferIndex, inout_instanceDataOffset);
}

WGALDynamicBufferHandle WRenderDataManager::GetSkinningDataBuffer() const
{
  return GetCustomInstanceDataBuffer(s_uiSkinningBufferIndex);
}

void WRenderDataManager::CompactSkinningDataBuffer(const UpdateContext& context)
{
  CompactCustomInstanceDataBuffer(s_uiSkinningBufferIndex);
}

void WRenderDataManager::OnExtractionEvent(const WRenderWorldExtractionEvent& e)
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  if (e.m_Type == WRenderWorldExtractionEvent::Type::BeginExtraction)
  {
    m_ExtractionData.m_pBuffers.SetCount(m_Buffers.GetCount());

    for (WUInt32 i = 0; i < m_Buffers.GetCount(); ++i)
    {
      m_ExtractionData.m_pBuffers[i] = pDevice->GetDynamicBuffer(m_Buffers[i]);
    }
  }
  else if (e.m_Type == WRenderWorldExtractionEvent::Type::EndExtraction)
  {
    for (WUInt32 i = 0; i < m_Buffers.GetCount(); ++i)
    {
      if (m_BeforeUploadCallbacks.GetCount() > i && m_BeforeUploadCallbacks[i].IsValid())
      {
        m_BeforeUploadCallbacks[i]();
      }

      m_ExtractionData.m_pBuffers[i]->UploadChangesForNextFrame();
    }

    m_ExtractionData.m_pBuffers.Clear();
  }
}


W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderDataManager);
