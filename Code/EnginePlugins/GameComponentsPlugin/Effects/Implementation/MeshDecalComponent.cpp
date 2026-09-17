#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <GameComponentsPlugin/Effects/MeshDecalComponent.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <RendererCore/Decals/Implementation/DecalManager.h>
#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WMeshDecalDescription, WNoBase, 1, WRTTIDefaultAllocator<WMeshDecalDescription>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Index", m_uiIndex)->AddAttributes(new WClampValueAttribute(0, 7)),
    W_RESOURCE_MEMBER_PROPERTY("BaseColorTexture", m_hBaseColorTexture)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WResult WMeshDecalDescription::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_uiIndex;
  inout_stream << m_hBaseColorTexture;

  return W_SUCCESS;
}

WResult WMeshDecalDescription::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_uiIndex;
  inout_stream >> m_hBaseColorTexture;

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WMeshDecalComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_ACCESSOR_PROPERTY("Decals", Decals_GetCount, Decals_Get, Decals_Set, Decals_Insert, Decals_Remove),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

void WMeshDecalComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s.WriteArray(m_DecalDescs).AssertSuccess();
}

void WMeshDecalComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s.ReadArray(m_DecalDescs).AssertSuccess();
}

void WMeshDecalComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateDecals();
}

void WMeshDecalComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  DeleteDecals();
}

void WMeshDecalComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow)
    return;

  const float fScreenSpaceSize = WLightComponent::CalculateScreenSpaceSize(GetOwner()->GetGlobalBounds().GetSphere(), *msg.m_pView->GetCullingCamera());

  for (WDecalId decalId : m_DecalIds)
  {
    WDecalManager::MarkRuntimeDecalAsUsed(decalId, fScreenSpaceSize, msg.m_pView);
  }
}

WUInt32 WMeshDecalComponent::Decals_GetCount() const
{
  return m_DecalDescs.GetCount();
}

const WMeshDecalDescription& WMeshDecalComponent::Decals_Get(WUInt32 uiIndex) const
{
  return m_DecalDescs[uiIndex];
}

void WMeshDecalComponent::Decals_Set(WUInt32 uiIndex, const WMeshDecalDescription& desc)
{
  m_DecalDescs[uiIndex] = desc;

  UpdateDecals();
}

void WMeshDecalComponent::Decals_Insert(WUInt32 uiIndex, const WMeshDecalDescription& desc)
{
  m_DecalDescs.InsertAt(uiIndex, desc);

  UpdateDecals();
}

void WMeshDecalComponent::Decals_Remove(WUInt32 uiIndex)
{
  m_DecalDescs.RemoveAtAndCopy(uiIndex);

  UpdateDecals();
}

void WMeshDecalComponent::UpdateDecals()
{
  if (!IsActiveAndInitialized())
    return;

  DeleteDecals();

  WArrayMap<WUInt32, WTexture2DResourceHandle, WTempAllocatorWrapper> indexToTexture;
  indexToTexture.Reserve(m_DecalDescs.GetCount());

  for (auto& desc : m_DecalDescs)
  {
    indexToTexture.Insert(desc.m_uiIndex, desc.m_hBaseColorTexture);
  }

  WUInt32 uiRandomSeed = GetOwner()->GetStableRandomSeed();
  int iRandomPos = 0;
  auto RandomIndex = [uiRandomSeed, &iRandomPos](WUInt32 uiMin, WUInt32 uiMax)
  {
    WUInt32 uiIndex = static_cast<WUInt32>(WSimdRandom::FloatMinMax(WSimdVec4i(iRandomPos), WSimdVec4f(float(uiMin)), WSimdVec4f(float(uiMax)), WSimdVec4u(uiRandomSeed)).x());
    iRandomPos++;
    return uiIndex;
  };

  WUInt16 decalIndices[8] = {};
  for (WUInt32 i = 0; i < 8; ++i)
  {
    decalIndices[i] = WSmallInvalidIndex;

    const WUInt32 uiLowerBound = indexToTexture.LowerBound(i);
    const WUInt32 uiUpperBound = WMath::Min(indexToTexture.UpperBound(i), indexToTexture.GetCount());

    if (uiLowerBound != WInvalidIndex && uiUpperBound > uiLowerBound)
    {
      const WUInt32 uiIndex = RandomIndex(uiLowerBound, uiUpperBound);
      auto& hTexture = indexToTexture.GetValue(uiIndex);
      if (hTexture.IsValid())
      {
        WDecalId decalId = WDecalManager::GetOrCreateRuntimeDecal(hTexture);
        decalIndices[i] = decalId.m_InstanceIndex;

        if (!m_DecalIds.Contains(decalId))
          m_DecalIds.PushBack(decalId);
      }
    }
  }

  WMsgSetCustomData msg;
  msg.m_vData = *reinterpret_cast<const WVec4*>(decalIndices);

  GetOwner()->PostMessage(msg, WTime::MakeZero(), WObjectMsgQueueType::AfterInitialized);
}

void WMeshDecalComponent::DeleteDecals()
{
  for (WDecalId decalId : m_DecalIds)
  {
    WDecalManager::DeleteRuntimeDecal(decalId);
  }

  m_DecalIds.Clear();
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Effects_Implementation_MeshDecalComponent);
