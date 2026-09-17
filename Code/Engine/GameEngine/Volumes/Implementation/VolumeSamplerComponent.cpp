#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Volumes/VolumeSamplerComponent.h>
#include <RendererCore/Components/BlackboardComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WVolumeSamplerValue, WNoBase, 1, WRTTIDefaultAllocator<WVolumeSamplerValue>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardKeysEnum")),
    W_MEMBER_PROPERTY("DefaultValue", m_DefaultValue),
    W_MEMBER_PROPERTY("InterpolationDuration", m_InterpolationDuration),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WResult WVolumeSamplerValue::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  inout_stream << m_DefaultValue;
  inout_stream << m_InterpolationDuration;

  return W_SUCCESS;
}

WResult WVolumeSamplerValue::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_sName;
  inout_stream >> m_DefaultValue;
  inout_stream >> m_InterpolationDuration;

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WVolumeSamplerComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("VolumeType", GetVolumeType, SetVolumeType)->AddAttributes(new WDynamicStringEnumAttribute("SpatialDataCategoryEnum"), new WDefaultValueAttribute("GenericVolume")),
    W_ARRAY_ACCESSOR_PROPERTY("Values", Values_GetCount, Values_GetMapping, Values_SetMapping, Values_Insert, Values_Remove),
    W_ACCESSOR_PROPERTY("AttachToMainCamera", GetAttachToMainCamera, SetAttachToMainCamera),
    W_ACCESSOR_PROPERTY("WriteToBlackboard", GetWriteToBlackboard, SetWriteToBlackboard),
    W_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardNamesEnum")),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(RegisterValue, In, "Name", In, "DefaultValue", In, "InterpolationDuration"),
    W_SCRIPT_FUNCTION_PROPERTY(GetValue, In, "Name"),
    W_SCRIPT_FUNCTION_PROPERTY(GetFloatValue, In, "Name", In, "FallbackValue"),
    W_SCRIPT_FUNCTION_PROPERTY(GetColorValue, In, "Name", In, "FallbackValue"),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WVolumeSamplerComponent::WVolumeSamplerComponent()
{
  m_pSampler = W_DEFAULT_NEW(WVolumeSampler);
}

WVolumeSamplerComponent::WVolumeSamplerComponent(WVolumeSamplerComponent&& other) = default;
WVolumeSamplerComponent::~WVolumeSamplerComponent() = default;
WVolumeSamplerComponent& WVolumeSamplerComponent::operator=(WVolumeSamplerComponent&& other) = default;

void WVolumeSamplerComponent::OnActivated()
{
  m_pBlackboard = WBlackboardComponent::FindBlackboard(*GetOwner(), m_sBlackboardName);
}

void WVolumeSamplerComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  auto& sCategory = WSpatialData::GetCategoryName(m_SpatialCategory);
  s << sCategory;
  s << m_bAttachToMainCamera;
  s << m_bWriteToBlackboard;
  s << m_sBlackboardName;

  s.WriteArray(m_Values).IgnoreResult();
}

void WVolumeSamplerComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  WHashedString sCategory;
  s >> sCategory;
  m_SpatialCategory = WSpatialData::RegisterCategory(sCategory, WSpatialData::Flags::None);

  s >> m_bAttachToMainCamera;

  if (uiVersion >= 2)
  {
    s >> m_bWriteToBlackboard;
    s >> m_sBlackboardName;
  }

  WDynamicArray<WVolumeSamplerValue> values;
  s.ReadArray(values).IgnoreResult();
  RegisterSamplerValues(values);
}

void WVolumeSamplerComponent::SetVolumeType(const char* szType)
{
  m_SpatialCategory = WSpatialData::RegisterCategory(szType, WSpatialData::Flags::None);
}

const char* WVolumeSamplerComponent::GetVolumeType() const
{
  return WSpatialData::GetCategoryName(m_SpatialCategory);
}

void WVolumeSamplerComponent::SetAttachToMainCamera(bool bAttach)
{
  m_bAttachToMainCamera = bAttach;
}

void WVolumeSamplerComponent::SetWriteToBlackboard(bool bWriteToBlackboard)
{
  m_bWriteToBlackboard = bWriteToBlackboard;
}

void WVolumeSamplerComponent::SetBlackboardName(const WHashedString& sName)
{
  if (m_sBlackboardName == sName)
    return;

  m_sBlackboardName = sName;

  if (IsActiveAndInitialized())
  {
    m_pBlackboard = WBlackboardComponent::FindBlackboard(*GetOwner(), m_sBlackboardName);
  }
}

void WVolumeSamplerComponent::RegisterValue(const WHashedString& sName, const WVariant& defaultValue, WTime interpolationDuration)
{
  m_pSampler->RegisterValue(sName, defaultValue, interpolationDuration);
}

WVariant WVolumeSamplerComponent::GetValue(const WHashedString& sName) const
{
  return m_pSampler->GetValue(sName);
}

float WVolumeSamplerComponent::GetFloatValue(const WHashedString& sName, float fFallbackValue) const
{
  WVariant varValue = GetValue(sName);
  if (varValue.CanConvertTo<float>())
  {
    return varValue.ConvertTo<float>();
  }

  return fFallbackValue;
}

WColor WVolumeSamplerComponent::GetColorValue(const WHashedString& sName, const WColor& fallbackValue) const
{
  WVariant varValue = GetValue(sName);
  if (varValue.CanConvertTo<WColor>())
  {
    return varValue.ConvertTo<WColor>();
  }

  return fallbackValue;
}

void WVolumeSamplerComponent::Values_SetMapping(WUInt32 i, const WVolumeSamplerValue& mapping)
{
  m_Values.EnsureCount(i + 1);
  m_Values[i] = mapping;

  RegisterSamplerValues(m_Values);
}

void WVolumeSamplerComponent::Values_Insert(WUInt32 uiIndex, const WVolumeSamplerValue& mapping)
{
  m_Values.InsertAt(uiIndex, mapping);

  RegisterSamplerValues(m_Values);
}

void WVolumeSamplerComponent::Values_Remove(WUInt32 uiIndex)
{
  m_Values.RemoveAtAndCopy(uiIndex);

  RegisterSamplerValues(m_Values);
}

void WVolumeSamplerComponent::RegisterSamplerValues(WArrayPtr<const WVolumeSamplerValue> values)
{
  m_pSampler->DeregisterAllValues();

  for (auto& value : values)
  {
    if (value.m_sName.IsEmpty())
      continue;

    m_pSampler->RegisterValue(value.m_sName, value.m_DefaultValue, value.m_InterpolationDuration);
  }
}

void WVolumeSamplerComponent::Update()
{
  WWorld* pWorld = GetWorld();
  WVec3 vSamplePos = GetOwner()->GetGlobalPosition();

  if (GetAttachToMainCamera())
  {
    if (WView* pView = WRenderWorld::GetViewByUsageHint(WCameraUsageHint::MainView, WCameraUsageHint::EditorView, pWorld))
    {
      vSamplePos = pView->GetCullingCamera()->GetCenterPosition();
    }
  }

  WTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = pWorld->GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = WClock::GetGlobalClock()->GetTimeDiff();
  }

  WBlackboard* pBlackboard = m_bWriteToBlackboard ? m_pBlackboard.Borrow() : nullptr;
  m_pSampler->SampleAtPosition(*pWorld, m_SpatialCategory, vSamplePos, deltaTime, pBlackboard);
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

// Migrate post processing component to volume sampler component
class WVolumeSamplerComponent_1_2 : public WGraphPatch
{
public:
  WVolumeSamplerComponent_1_2()
    : WGraphPatch("WPostProcessingComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("WVolumeSamplerComponent");

    // Migrate mappings
    WVariantArray newValues;
    if (auto pMappings = pNode->FindProperty("Mappings"))
    {
      if (pMappings->m_Value.IsA<WVariantArray>())
      {
        auto& mappings = pMappings->m_Value.Get<WVariantArray>();
        for (auto& mapping : mappings)
        {
          if (!mapping.IsA<WUuid>())
            continue;

          auto pMappingNode = pGraph->GetNode(mapping.Get<WUuid>());
          if (!pMappingNode)
            continue;

          WUuid newGuid = WUuid::MakeUuid();
          auto* pNewNode = pGraph->AddNode(newGuid, "WVolumeSamplerValue", 1);

          WStringBuilder name = pMappingNode->FindProperty("RenderPass")->m_Value.Get<WHashedString>().GetView();
          name.Append(".", pMappingNode->FindProperty("Property")->m_Value.Get<WHashedString>());

          pNewNode->AddProperty("Name", name.GetView());
          pNewNode->AddProperty("DefaultValue", pMappingNode->FindProperty("DefaultValue")->m_Value);
          pNewNode->AddProperty("InterpolationDuration", pMappingNode->FindProperty("InterpolationDuration")->m_Value);

          newValues.PushBack(newGuid);
        }
      }
    }

    pNode->AddProperty("Values", newValues);
    pNode->AddProperty("AttachToMainCamera", true);
    pNode->AddProperty("WriteToBlackboard", true);
  }
};

WVolumeSamplerComponent_1_2 g_WVolumeSamplerComponent_1_2;


W_STATICLINK_FILE(GameEngine, GameEngine_Volumes_Implementation_VolumeSamplerComponent);
