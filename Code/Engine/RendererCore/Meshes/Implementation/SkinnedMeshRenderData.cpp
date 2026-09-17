#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/SkinnedMeshRenderData.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSkinnedMeshRenderData, 1, WRTTIDefaultAllocator<WSkinnedMeshRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool WSkinnedMeshRenderData::CanBatch(const WRenderData& other0) const
{
  const auto& other = WStaticCast<const WSkinnedMeshRenderData&>(other0);

  return m_hSkinningBuffer == other.m_hSkinningBuffer && SUPER::CanBatch(other0);
}

//////////////////////////////////////////////////////////////////////////

WSkinningState::WSkinningState() = default;

WSkinningState::~WSkinningState()
{
  Clear();
}

void WSkinningState::Clear()
{
  if (m_pWorld == nullptr)
    return;

  if (auto pRenderDataManager = m_pWorld->GetModule<WRenderDataManager>())
  {
    pRenderDataManager->DeleteSkinningData(m_DataOffset);
  }

  m_uiNumBones = 0;
  m_pWorld = nullptr;
}

WArrayPtr<WShaderTransform> WSkinningState::GetOrCreateBoneTransformsForWriting(WComponent& ref_ownerComponent, WUInt32 uiNumBones)
{
  W_ASSERT_DEV(ref_ownerComponent.HandlesMessage(WMsgCustomInstanceDataOffsetChanged()), "Owner component must handle WMsgCustomInstanceDataOffsetChanged.");

  auto pWorld = ref_ownerComponent.GetWorld();
  W_ASSERT_DEV(m_pWorld == nullptr || m_pWorld == pWorld, "WSkinningState used with different worlds simultaneously, which is not supported.");
  m_pWorld = pWorld;

  auto pRenderDataManager = m_pWorld->GetModuleReadOnly<WRenderDataManager>();

  if (m_uiNumBones > 0 && m_uiNumBones != uiNumBones)
  {
    pRenderDataManager->DeleteSkinningData(m_DataOffset);
  }
  m_uiNumBones = uiNumBones;

  return pRenderDataManager->GetOrCreateSkinningData(&ref_ownerComponent, m_DataOffset, uiNumBones);
}

WArrayPtr<const WShaderTransform> WSkinningState::GetBoneTransformsForReading() const
{
  if (m_pWorld != nullptr && m_uiNumBones > 0)
  {
    if (auto pRenderDataManager = m_pWorld->GetModuleReadOnly<WRenderDataManager>())
    {
      return pRenderDataManager->GetSkinningData(m_DataOffset);
    }
  }

  return WArrayPtr<const WShaderTransform>();
}

W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshRenderData);
