#include <Core/CorePCH.h>

#include <Core/Collection/CollectionComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WCollectionComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Collection", GetCollection, SetCollection)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_AssetCollection", WDependencyFlags::Package), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("RegisterNames", m_bRegisterNames),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Utilities"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCollectionComponent::WCollectionComponent() = default;
WCollectionComponent::~WCollectionComponent() = default;

void WCollectionComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hCollection;
  s << m_bRegisterNames;
}

void WCollectionComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hCollection;

  if (uiVersion >= 2)
  {
    s >> m_bRegisterNames;
  }
}

void WCollectionComponent::SetCollection(const WCollectionResourceHandle& hCollection)
{
  m_hCollection = hCollection;

  if (IsActiveAndSimulating())
  {
    InitiatePreload();
  }
}

void WCollectionComponent::OnSimulationStarted()
{
  InitiatePreload();
}

void WCollectionComponent::InitiatePreload()
{
  if (m_hCollection.IsValid())
  {
    WResourceLock<WCollectionResource> pCollection(m_hCollection, WResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pCollection.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      pCollection->PreloadResources();

      if (m_bRegisterNames)
      {
        pCollection->RegisterNames();
      }
    }
  }
}

W_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionComponent);
