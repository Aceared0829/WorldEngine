#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/SpatialAnchorComponent.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSpatialAnchorComponent, 2, WComponentMode::Dynamic)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("XR"),
    new WInDevelopmentAttribute(WInDevelopmentAttribute::Phase::Beta),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WSpatialAnchorComponent::WSpatialAnchorComponent() = default;
WSpatialAnchorComponent::~WSpatialAnchorComponent()
{
  if (WXRSpatialAnchorsInterface* pXR = WSingletonRegistry::GetSingletonInstance<WXRSpatialAnchorsInterface>())
  {
    if (!m_AnchorID.IsInvalidated())
    {
      pXR->DestroyAnchor(m_AnchorID).IgnoreResult();
      m_AnchorID = WXRSpatialAnchorID();
    }
  }
}

void WSpatialAnchorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();
}

void WSpatialAnchorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();
  if (uiVersion == 1)
  {
    WString sAnchorName;
    s >> sAnchorName;
  }
}

WResult WSpatialAnchorComponent::RecreateAnchorAt(const WTransform& position)
{
  if (WXRSpatialAnchorsInterface* pXR = WSingletonRegistry::GetSingletonInstance<WXRSpatialAnchorsInterface>())
  {
    if (!m_AnchorID.IsInvalidated())
    {
      pXR->DestroyAnchor(m_AnchorID).IgnoreResult();
      m_AnchorID = WXRSpatialAnchorID();
    }

    m_AnchorID = pXR->CreateAnchor(position);
    return m_AnchorID.IsInvalidated() ? W_FAILURE : W_SUCCESS;
  }

  return W_SUCCESS;
}

void WSpatialAnchorComponent::Update()
{
  if (IsActiveAndSimulating())
  {
    if (WXRSpatialAnchorsInterface* pXR = WSingletonRegistry::GetSingletonInstance<WXRSpatialAnchorsInterface>())
    {
      if (!m_AnchorID.IsInvalidated())
      {
        WTransform globalTransform;
        if (pXR->TryGetAnchorTransform(m_AnchorID, globalTransform).Succeeded())
        {
          globalTransform.m_vScale = GetOwner()->GetGlobalScaling();
          GetOwner()->SetGlobalTransform(globalTransform);
        }
      }
      else
      {
        m_AnchorID = pXR->CreateAnchor(GetOwner()->GetGlobalTransform());
      }
    }
  }
}

void WSpatialAnchorComponent::OnSimulationStarted()
{
  if (WXRSpatialAnchorsInterface* pXR = WSingletonRegistry::GetSingletonInstance<WXRSpatialAnchorsInterface>())
  {
    m_AnchorID = pXR->CreateAnchor(GetOwner()->GetGlobalTransform());
  }
}

W_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_SpatialAnchorComponent);
