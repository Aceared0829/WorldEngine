#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/TimedDeathComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WTimedDeathComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MinDelay", m_MinDelay)->AddAttributes(new WClampValueAttribute(WTime(), WVariant()), new WDefaultValueAttribute(WTime::MakeFromSeconds(1.0))),
    W_MEMBER_PROPERTY("DelayRange", m_DelayRange)->AddAttributes(new WClampValueAttribute(WTime(), WVariant())),
    W_RESOURCE_MEMBER_PROPERTY("TimeoutPrefab", m_hTimeoutPrefab)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab", WDependencyFlags::Package)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnTriggered),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WTimedDeathComponent::WTimedDeathComponent() = default;
WTimedDeathComponent::~WTimedDeathComponent() = default;

void WTimedDeathComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_MinDelay;
  s << m_DelayRange;
  s << m_hTimeoutPrefab;
}

void WTimedDeathComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_hTimeoutPrefab;
}

void WTimedDeathComponent::OnSimulationStarted()
{
  WMsgComponentInternalTrigger msg;
  msg.m_sMessage.Assign("Suicide");

  WWorld* pWorld = GetWorld();

  const WTime tKill = WTime::MakeFromSeconds(pWorld->GetRandomNumberGenerator().DoubleMinMax(m_MinDelay.GetSeconds(), m_MinDelay.GetSeconds() + m_DelayRange.GetSeconds()));

  PostMessage(msg, tKill);

  // make sure the prefab is available when the component dies
  if (m_hTimeoutPrefab.IsValid())
  {
    WResourceManager::PreloadResource(m_hTimeoutPrefab);
  }
}

void WTimedDeathComponent::OnTriggered(WMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != WTempHashedString("Suicide"))
    return;

  if (m_hTimeoutPrefab.IsValid())
  {
    WResourceLock<WPrefabResource> pPrefab(m_hTimeoutPrefab, WResourceAcquireMode::AllowLoadingFallback);

    WPrefabInstantiationOptions options;
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), options);
  }

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WTimedDeathComponentPatch_1_2 : public WGraphPatch
{
public:
  WTimedDeathComponentPatch_1_2()
    : WGraphPatch("WTimedDeathComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Min Delay", "MinDelay");
    pNode->RenameProperty("Delay Range", "DelayRange");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
  }
};

WTimedDeathComponentPatch_1_2 g_WTimedDeathComponentPatch_1_2;



W_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_TimedDeathComponent);
