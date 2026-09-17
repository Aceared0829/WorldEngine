#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Volumes/VolumeComponent.h>
#include <RendererCore/Utils/BlackboardTemplateResource.h>

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WVolumeComponent, 1)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Type", GetVolumeType, SetVolumeType)->AddAttributes(new WDynamicStringEnumAttribute("SpatialDataCategoryEnum"), new WDefaultValueAttribute("GenericVolume")),
    W_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new WClampValueAttribute(-64.0f, 64.0f)),
    W_RESOURCE_ACCESSOR_PROPERTY("Template", GetTemplate, SetTemplate)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    W_MAP_ACCESSOR_PROPERTY("Values", Reflection_GetKeys, Reflection_GetValue, Reflection_InsertValue, Reflection_RemoveValue),
  }
  W_END_PROPERTIES;

  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Name", In, "Value"),
    W_SCRIPT_FUNCTION_PROPERTY(GetValue, In, "Name"),
  }
  W_END_FUNCTIONS;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
  }
  W_END_ATTRIBUTES;
}
W_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

WVolumeComponent::WVolumeComponent() = default;
WVolumeComponent::~WVolumeComponent() = default;

void WVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeFromTemplate();

  GetOwner()->UpdateLocalBounds();
}

void WVolumeComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  RemoveReloadFunction();

  GetOwner()->UpdateLocalBounds();
}

void WVolumeComponent::SetTemplate(const WBlackboardTemplateResourceHandle& hResource)
{
  RemoveReloadFunction();

  m_hTemplateResource = hResource;

  if (IsActiveAndInitialized())
  {
    ReloadTemplate();
  }
}

void WVolumeComponent::SetSortOrder(float fOrder)
{
  fOrder = WMath::Clamp(fOrder, -64.0f, 64.0f);
  m_fSortOrder = fOrder;
}

void WVolumeComponent::SetVolumeType(const char* szType)
{
  m_SpatialCategory = WSpatialData::RegisterCategory(szType, WSpatialData::Flags::None);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

const char* WVolumeComponent::GetVolumeType() const
{
  return WSpatialData::GetCategoryName(m_SpatialCategory);
}

void WVolumeComponent::SetValue(const WHashedString& sName, const WVariant& value)
{
  m_Values.Insert(sName, value);
}

void WVolumeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_fSortOrder;

  auto& sCategory = WSpatialData::GetCategoryName(m_SpatialCategory);
  s << sCategory;

  s << m_hTemplateResource;

  // Only serialize overwritten values so a template change doesn't require a re-save of all volumes
  WUInt32 numValues = m_OverwrittenValues.GetCount();
  s << numValues;
  for (auto& sName : m_OverwrittenValues)
  {
    WVariant value;
    m_Values.TryGetValue(sName, value);

    s << sName;
    s << value;
  }
}

void WVolumeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_fSortOrder;

  WHashedString sCategory;
  s >> sCategory;
  m_SpatialCategory = WSpatialData::RegisterCategory(sCategory, WSpatialData::Flags::None);

  s >> m_hTemplateResource;

  // m_OverwrittenValues is only used in editor so we don't write to it here
  WUInt32 numValues = 0;
  s >> numValues;
  for (WUInt32 i = 0; i < numValues; ++i)
  {
    WHashedString sName;
    WVariant value;
    s >> sName;
    s >> value;

    m_Values.Insert(sName, value);
  }
}

const WRangeView<const WString&, WUInt32> WVolumeComponent::Reflection_GetKeys() const
{
  return WRangeView<const WString&, WUInt32>([]() -> WUInt32
    { return 0; },
    [this]() -> WUInt32
    { return m_OverwrittenValues.GetCount(); },
    [](WUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const WUInt32& uiIt) -> const WString&
    { return m_OverwrittenValues[uiIt].GetString(); });
}

bool WVolumeComponent::Reflection_GetValue(const char* szName, WVariant& value) const
{
  return m_Values.TryGetValue(WTempHashedString(szName), value);
}

void WVolumeComponent::Reflection_InsertValue(const char* szName, const WVariant& value)
{
  WHashedString sName;
  sName.Assign(szName);

  // Only needed in editor
  if (GetUniqueID() != WInvalidIndex && m_OverwrittenValues.Contains(sName) == false)
  {
    m_OverwrittenValues.PushBack(sName);
  }

  m_Values.Insert(sName, value);
}

void WVolumeComponent::Reflection_RemoveValue(const char* szName)
{
  WHashedString sName;
  sName.Assign(szName);

  m_OverwrittenValues.RemoveAndCopy(sName);

  m_Values.Remove(sName);
}

void WVolumeComponent::InitializeFromTemplate()
{
  if (!m_hTemplateResource.IsValid())
    return;

  WResourceLock<WBlackboardTemplateResource> pTemplate(m_hTemplateResource, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pTemplate.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    if (m_Values.Contains(entry.m_sName) == false)
    {
      m_Values.Insert(entry.m_sName, entry.m_InitialValue);
    }
  }

  if (m_bReloadFunctionAdded == false)
  {
    GetWorld()->AddResourceReloadFunction(m_hTemplateResource, GetHandle(), nullptr,
      [](const WWorld::ResourceReloadContext& context)
      {
        WStaticCast<WVolumeComponent*>(context.m_pComponent)->ReloadTemplate();
      });

    m_bReloadFunctionAdded = true;
  }
}

void WVolumeComponent::ReloadTemplate()
{
  // Remove all values that are not overwritten
  WHashTable<WHashedString, WVariant> overwrittenValues;
  for (auto& sName : m_OverwrittenValues)
  {
    overwrittenValues.Insert(sName, m_Values[sName]);
  }
  m_Values.Swap(overwrittenValues);

  InitializeFromTemplate();
}

void WVolumeComponent::RemoveReloadFunction()
{
  if (m_bReloadFunctionAdded)
  {
    GetWorld()->RemoveResourceReloadFunction(m_hTemplateResource, GetHandle(), nullptr);

    m_bReloadFunctionAdded = false;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WVolumeSphereComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WSphereManipulatorAttribute("Radius"),
    new WSphereVisualizerAttribute("Radius", WColorScheme::LightUI(WColorScheme::Cyan)),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WVolumeSphereComponent::WVolumeSphereComponent() = default;
WVolumeSphereComponent::~WVolumeSphereComponent() = default;

void WVolumeSphereComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void WVolumeSphereComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = WMath::Max(fFalloff, 0.0001f);
}

void WVolumeSphereComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
}

void WVolumeSphereComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
}

void WVolumeSphereComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fRadius), m_SpatialCategory);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WVolumeBoxComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new WDefaultValueAttribute(WVec3(10.0f)), new WClampValueAttribute(WVec3(0), WVariant())),
    W_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new WDefaultValueAttribute(WVec3(0.5f)), new WClampValueAttribute(WVec3(0.0f), WVec3(1.0f))),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WBoxManipulatorAttribute("Extents", 1.0f, true),
    new WBoxVisualizerAttribute("Extents", 1.0f, WColorScheme::LightUI(WColorScheme::Cyan)),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WVolumeBoxComponent::WVolumeBoxComponent() = default;
WVolumeBoxComponent::~WVolumeBoxComponent() = default;

void WVolumeBoxComponent::SetExtents(const WVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void WVolumeBoxComponent::SetFalloff(const WVec3& vFalloff)
{
  m_vFalloff = vFalloff.CompMax(WVec3(0.0001f));
}

void WVolumeBoxComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vFalloff;
}

void WVolumeBoxComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFalloff;
}

void WVolumeBoxComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), m_SpatialCategory);
}


W_STATICLINK_FILE(GameEngine, GameEngine_Volumes_Implementation_VolumeComponent);
