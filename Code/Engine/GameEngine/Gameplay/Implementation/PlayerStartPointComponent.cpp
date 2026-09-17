#include <GameEngine/GameEnginePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WPlayerStartPointComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("PlayerPrefab", m_hPlayerPrefab)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab", WDependencyFlags::Package)),
    W_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new WExposedParametersAttribute("PlayerPrefab")),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 0.5f, WColor::DarkSlateBlue),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WPlayerStartPointComponent::WPlayerStartPointComponent() = default;
WPlayerStartPointComponent::~WPlayerStartPointComponent() = default;

void WPlayerStartPointComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hPlayerPrefab;

  WPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), inout_stream, m_Parameters);
}

void WPlayerStartPointComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hPlayerPrefab;

  if (uiVersion >= 2)
  {
    WPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, inout_stream);
  }
}

void WPlayerStartPointComponent::SetPlayerPrefab(const WPrefabResourceHandle& hPrefab)
{
  m_hPlayerPrefab = hPrefab;
}

const WPrefabResourceHandle& WPlayerStartPointComponent::GetPlayerPrefab() const
{
  return m_hPlayerPrefab;
}

const WRangeView<const char*, WUInt32> WPlayerStartPointComponent::GetParameters() const
{
  return WRangeView<const char*, WUInt32>([]() -> WUInt32
    { return 0; },
    [this]() -> WUInt32
    { return m_Parameters.GetCount(); },
    [](WUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const WUInt32& uiIt) -> const char*
    { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void WPlayerStartPointComponent::SetParameter(const char* szKey, const WVariant& value)
{
  WHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != WInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void WPlayerStartPointComponent::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(WTempHashedString(szKey));
}

bool WPlayerStartPointComponent::GetParameter(const char* szKey, WVariant& out_value) const
{
  WUInt32 it = m_Parameters.Find(szKey);

  if (it == WInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}


W_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_PlayerStartPointComponent);
