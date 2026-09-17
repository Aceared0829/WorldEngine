#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <GameComponentsPlugin/Placement/RandomPrefabComponent.h>

WRandomPrefabComponentManager::WRandomPrefabComponentManager(WWorld* pWorld)
  : WComponentManager<WRandomPrefabComponent, WBlockStorageType::Compact>(pWorld)
{
  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WRandomPrefabComponentManager::ResourceEventHandler, this));
}

WRandomPrefabComponentManager::~WRandomPrefabComponentManager()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WRandomPrefabComponentManager::ResourceEventHandler, this));
}

void WRandomPrefabComponentManager::Initialize()
{
  auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WRandomPrefabComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void WRandomPrefabComponentManager::Update(const WWorldModule::UpdateContext& /*context*/)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    WRandomPrefabComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InstantiatePrefabs();
  }

  m_ComponentsToUpdate.Clear();
}

void WRandomPrefabComponentManager::AddToUpdateList(WRandomPrefabComponent* pComponent)
{
  m_ComponentsToUpdate.Insert(pComponent->GetHandle());
}

void WRandomPrefabComponentManager::ResourceEventHandler(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<WPrefabResource>())
  {
    WPrefabResourceHandle hUpdatedPrefab((WPrefabResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      for (auto& entry : it->m_Prefabs)
      {
        if (entry == hUpdatedPrefab)
        {
          AddToUpdateList(it);
          break;
        }
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WRandomPrefabComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Preview", GetPreview, SetPreview)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("Count", GetCount, SetCount)->AddAttributes(new WDefaultValueAttribute(1)),
    W_ACCESSOR_PROPERTY("InstantiateAsChildren", GetInstantiateAsChildren, SetInstantiateAsChildren),
    W_ACCESSOR_PROPERTY("Position", GetPositionDeviation, SetPositionDeviation)->AddAttributes(new WGroupAttribute("Random Transform"), new WClampValueAttribute(WVec3::MakeZero(), WVariant())),
    W_ACCESSOR_PROPERTY("Rotation", GetRotationDeviation, SetRotationDeviation)->AddAttributes(new WGroupAttribute("Random Transform"), new WSuffixAttribute("°"), new WClampValueAttribute(WVec3::MakeZero(), WVec3(180.0f))),
    W_ACCESSOR_PROPERTY("MinScale", GetMinUniformScale, SetMinUniformScale)->AddAttributes(new WGroupAttribute("Random Transform"), new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, 100.0f)),
    W_ACCESSOR_PROPERTY("MaxScale", GetMaxUniformScale, SetMaxUniformScale)->AddAttributes(new WGroupAttribute("Random Transform"), new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, 100.0f)),
    W_ACCESSOR_PROPERTY("Color1", GetColor1, SetColor1)->AddAttributes(new WExposeColorAlphaAttribute(), new WGroupAttribute("Random Color")),
    W_ACCESSOR_PROPERTY("Color2", GetColor2, SetColor2)->AddAttributes(new WExposeColorAlphaAttribute(), new WGroupAttribute("Random Color")),
    W_ARRAY_ACCESSOR_PROPERTY("Prefabs", Prefabs_GetCount, Prefabs_GetValue, Prefabs_SetValue, Prefabs_Insert, Prefabs_Remove)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab"), new WRequiredAttribute()),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Construction"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

WRandomPrefabComponent::WRandomPrefabComponent() = default;
WRandomPrefabComponent::~WRandomPrefabComponent() = default;

void WRandomPrefabComponent::OnActivated()
{
  SUPER::OnActivated();

  // instantiate the prefab right away, such that game play code can access it as soon as possible
  // additionally the manager may update the instance later on, to properly enable editor work flows
  InstantiatePrefabs();
}

void WRandomPrefabComponent::OnDeactivated()
{
  // if this was created procedurally during editor runtime, we do not need to clear specific nodes
  // after simulation, the scene is deleted anyway

  ClearCreatedInstances();

  SUPER::OnDeactivated();
}

enum class RandomPrefabComponentFlags : WUInt8
{
  SelfDeletion = 1
};

void WRandomPrefabComponent::Deinitialize()
{
  if (GetUserFlag((WUInt8)RandomPrefabComponentFlags::SelfDeletion))
  {
    // do nothing, ie do not call OnDeactivated()
    // we do want to keep the created child objects around when this component gets destroyed during simulation
    // that's because the component actually deletes itself when simulation starts
    return;
  }

  // remove the children (through Deactivate)
  OnDeactivated();
}

void WRandomPrefabComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // at runtime, not in editor
  if (GetUniqueID() == WInvalidIndex)
  {
    SetUserFlag((WUInt8)RandomPrefabComponentFlags::SelfDeletion, true);

    // remove the prefab reference component, to prevent issues after another serialization/deserialization
    // and also to save some memory

    if (GetOwner()->GetChildCount() == 0 && GetOwner()->GetComponents().GetCount() == 1)
    {
      // if the owner object is now empty (except for this component)
      // delete the entire object (plus empty parents)
      // this happens when spawned objects are not attached as children
      GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
    }
    else
    {
      DeleteComponent();
    }
  }
  else // in editor
  {
    if (!m_bPreview)
    {
      InstantiatePrefabs();
    }

    if (!m_bInstantiateAsChildren)
    {
      // editor needs to always instantiate as child, so if we don't want that, fix it here

      for (auto child = GetOwner()->GetChildren(); child.IsValid(); ++child)
      {
        GetOwner()->DetachChild(child->GetHandle());
      }
    }
  }
}

void WRandomPrefabComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_uiCount;

  s << m_bInstantiateAsChildren;
  s << m_bPreview;

  s << m_vPositionDeviation;
  s << m_vRotationDeviation;
  s << m_fMinUniformScale;
  s << m_fMaxUniformScale;

  s << m_Color1;
  s << m_Color2;

  s.WriteArray(m_Prefabs).AssertSuccess();
}

void WRandomPrefabComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_uiCount;

  s >> m_bInstantiateAsChildren;
  s >> m_bPreview;

  s >> m_vPositionDeviation;
  s >> m_vRotationDeviation;
  s >> m_fMinUniformScale;
  s >> m_fMaxUniformScale;

  s >> m_Color1;
  s >> m_Color2;

  s.ReadArray(m_Prefabs).AssertSuccess();
}

void WRandomPrefabComponent::SetCount(WUInt16 uiCount)
{
  m_uiCount = uiCount;

  InstantiatePrefabs();
}

WUInt16 WRandomPrefabComponent::GetCount() const
{
  return m_uiCount;
}

void WRandomPrefabComponent::SetPositionDeviation(const WVec3& vValue)
{
  m_vPositionDeviation = vValue;
  InstantiatePrefabs();
}

void WRandomPrefabComponent::SetRotationDeviation(const WVec3& vValue)
{
  m_vRotationDeviation = vValue;
  InstantiatePrefabs();
}

void WRandomPrefabComponent::SetMinUniformScale(float fValue)
{
  m_fMinUniformScale = fValue;
  InstantiatePrefabs();
}

void WRandomPrefabComponent::SetMaxUniformScale(float fValue)
{
  m_fMaxUniformScale = fValue;
  InstantiatePrefabs();
}

void WRandomPrefabComponent::SetColor1(const WColor& value)
{
  m_Color1 = value;

  InstantiatePrefabs();
}

void WRandomPrefabComponent::SetColor2(const WColor& value)
{
  m_Color2 = value;

  InstantiatePrefabs();
}

void WRandomPrefabComponent::SetPreview(bool bValue)
{
  m_bPreview = bValue;

  InstantiatePrefabs();
}

void WRandomPrefabComponent::SetInstantiateAsChildren(bool bValue)
{
  m_bInstantiateAsChildren = bValue;

  InstantiatePrefabs();
}

void WRandomPrefabComponent::ClearCreatedInstances()
{
  // we assume all children are our created instances
  // if the user attaches any other children, there will be collateral damage

  for (auto child = GetOwner()->GetChildren(); child.IsValid(); ++child)
  {
    GetWorld()->DeleteObjectNow(child->GetHandle());
  }
}

void WRandomPrefabComponent::InstantiatePrefabs()
{
  if (!IsActiveAndInitialized())
    return;

  ClearCreatedInstances();

  if (m_Prefabs.IsEmpty())
    return;

  const WUInt32 uiUniqueID = GetUniqueID();
  const bool bIsInEditor = (uiUniqueID != WInvalidIndex);

  if (bIsInEditor && !m_bPreview)
  {
    if (!IsActiveAndSimulating())
    {
      return;
    }
  }

  auto MarkAsCreatedByPrefab = [](WGameObject* pChild, WUInt32 uiUniqueID)
  {
    // while exporting a scene all game objects with this flag are ignored and not exported
    // set this flag on all game objects that were created by instantiating this prefab
    // instead it should be instantiated at runtime again
    // only do this at editor time though, at regular runtime we do want to fully serialize the entire sub tree
    pChild->SetCreatedByPrefab();
    pChild->SetHideShapeIcon();

    for (auto pComponent : pChild->GetComponents())
    {
      pComponent->SetUniqueID(uiUniqueID);
      pComponent->SetCreatedByPrefab();
    }
  };

  WTempHybridArray<WGameObject*, 64> allCreatedRootObjects;
  WTempHybridArray<WGameObject*, 8> createdRootObjects;
  WTempHybridArray<WGameObject*, 8> createdChildObjects;

  const bool bAttachAsChildren = m_bInstantiateAsChildren || bIsInEditor;

  WPrefabInstantiationOptions options;
  if (bAttachAsChildren)
  {
    options.m_hParent = GetOwner()->GetHandle();
    options.m_RandomSeedMode = WPrefabInstantiationOptions::RandomSeedMode::DeterministicFromParent;
  }
  else
  {
    options.m_RandomSeedMode = WPrefabInstantiationOptions::RandomSeedMode::CustomRootValue;
    options.m_uiCustomRandomSeedRootValue = GetOwner()->GetStableRandomSeed();
  }
  options.m_pCreatedRootObjectsOut = &createdRootObjects;
  options.m_pCreatedChildObjectsOut = &createdChildObjects;

  const WSimdVec4f half(0.5f);

  const WSimdVec4f minOffset = WSimdConversion::ToVec3(-m_vPositionDeviation);
  const WSimdVec4f maxOffset = WSimdConversion::ToVec3(m_vPositionDeviation);
  // position step currently not exposed
  // const WSimdVec4f offsetStep = WSimdConversion::ToVec3(m_vPositionStep);

  const WSimdVec4f minRotAndIndex = WSimdConversion::ToVec4(-m_vRotationDeviation.GetAsVec4(0.0f));
  const WSimdVec4f maxRotAndIndex = WSimdConversion::ToVec4(m_vRotationDeviation.GetAsVec4(static_cast<float>(m_Prefabs.GetCount())));
  // rotation step currently not exposed
  // const WSimdVec4f rotStep = WSimdConversion::ToVec3(m_vRotationStep);

  const WSimdVec4f minScaleAndColorIndex = WSimdConversion::ToVec4(WVec4(m_fMinUniformScale, 1, 1, 0));
  const WSimdVec4f maxScaleAndColorIndex = WSimdConversion::ToVec4(WVec4(m_fMaxUniformScale, 1, 1, 1));
  // scale step currently not exposed
  // const WSimdVec4f minScaleAndColorIndex = WSimdConversion::ToVec4(m_vMinScale.GetAsVec4(0.0f));
  // const WSimdVec4f maxScaleAndColorIndex = WSimdConversion::ToVec4(m_vMaxScale.GetAsVec4(1.0f));
  // const WSimdVec4f scaleStep = WSimdConversion::ToVec3(m_vScaleStep);

  const bool bRandomColor = (m_Color1 != WColor::White || m_Color2 != WColor::White);

  for (WUInt32 i = 0; i < m_uiCount; ++i)
  {
    WSimdVec4i randPos = WSimdVec4i(0, 1, 2, 3);
    WSimdVec4u seed = WSimdVec4u(GetOwner()->GetStableRandomSeed() + i * 137);

    WSimdVec4f rotAndIndex = WSimdRandom::FloatMinMax(randPos, minRotAndIndex.CompMin(maxRotAndIndex), minRotAndIndex.CompMax(maxRotAndIndex), seed);

    const auto& entry = m_Prefabs[static_cast<int>(rotAndIndex.w())];
    if (!entry.IsValid())
      continue;

    WTransform transform;

    // position
    {
      randPos += WSimdVec4i(13);

      WSimdVec4f offset = WSimdRandom::FloatMinMax(randPos, minOffset.CompMin(maxOffset), minOffset.CompMax(maxOffset), seed);

      // position step currently not exposed
      // WSimdVec4f roundedOffset = (offset.CompDiv(offsetStep) + half).Floor().CompMul(offsetStep);
      // offset = WSimdVec4f::Select(offsetStep == WSimdVec4f::MakeZero(), offset, roundedOffset);

      transform.m_vPosition = WSimdConversion::ToVec3(offset);
    }

    // rotation
    {
      WSimdVec4f rot = rotAndIndex;

      // rotation step currently not exposed
      // WSimdVec4f roundedRot = (rot.CompDiv(rotStep) + half).Floor().CompMul(rotStep);
      // rot = WSimdVec4f::Select(rotStep == WSimdVec4f::MakeZero(), rot, roundedRot);

      transform.m_qRotation = WQuat::MakeFromEulerAngles(WAngle::MakeFromDegree(rot.x()), WAngle::MakeFromDegree(rot.y()), WAngle::MakeFromDegree(rot.z()));
    }

    float colorIndex = 0.0f;

    // scale + color
    {
      randPos += WSimdVec4i(11);

      WSimdVec4f scaleAndColorIndex = WSimdRandom::FloatMinMax(randPos, minScaleAndColorIndex.CompMin(maxScaleAndColorIndex), minScaleAndColorIndex.CompMax(maxScaleAndColorIndex), seed);
      WSimdVec4f scale = scaleAndColorIndex;
      colorIndex = scaleAndColorIndex.w();

      // scale step currently not exposed
      // WSimdVec4f roundedScale = (scale.CompDiv(scaleStep) + half).Floor().CompMul(scaleStep);
      // scale = WSimdVec4f::Select(scaleStep == WSimdVec4f::MakeZero(), scale, roundedScale);

      scale = WSimdVec4f::Select(scale == WSimdVec4f::MakeZero(), WSimdVec4f(1.0f), scale);

      // only use the scale value for uniform scaling
      transform.m_vScale.Set(scale.x());

      // non-uniform scaling not exposed
      // transform.m_vScale = WSimdConversion::ToVec3(scale);
    }

    if (!bAttachAsChildren)
    {
      transform = WTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransform(), transform);
    }

    {
      createdRootObjects.Clear();
      createdChildObjects.Clear();

      WResourceLock<WPrefabResource> pResource(entry, WResourceAcquireMode::AllowLoadingFallback);

      pResource->InstantiatePrefab(*GetWorld(), transform, options);

      allCreatedRootObjects.PushBackRange(createdRootObjects);
    }

    if (bRandomColor)
    {
      WMsgSetColor msg;
      msg.m_Color = WMath::Lerp(m_Color1, m_Color2, colorIndex);

      for (auto pObject : createdRootObjects)
      {
        pObject->PostMessageRecursive(msg, WTime::MakeZero(), WObjectMsgQueueType::AfterInitialized);
      }
    }

    if (bIsInEditor)
    {
      for (WGameObject* pChild : createdRootObjects)
      {
        MarkAsCreatedByPrefab(pChild, uiUniqueID);
      }

      for (WGameObject* pChild : createdChildObjects)
      {
        MarkAsCreatedByPrefab(pChild, uiUniqueID);
      }
    }
  }
}

WUInt32 WRandomPrefabComponent::Prefabs_GetCount() const
{
  return m_Prefabs.GetCount();
}

WString WRandomPrefabComponent::Prefabs_GetValue(WUInt32 uiIndex) const
{
  if (uiIndex >= m_Prefabs.GetCount())
  {
    return "";
  }

  return m_Prefabs[uiIndex].GetResourceID();
}

void WRandomPrefabComponent::Prefabs_SetValue(WUInt32 uiIndex, WString sValue)
{
  m_Prefabs.EnsureCount(uiIndex + 1);

  if (!sValue.IsEmpty())
  {
    m_Prefabs[uiIndex] = WResourceManager::LoadResource<WPrefabResource>(sValue);
    WResourceManager::PreloadResource(m_Prefabs[uiIndex]);
  }
  else
  {
    m_Prefabs[uiIndex].Invalidate();
  }

  InstantiatePrefabs();
}

void WRandomPrefabComponent::Prefabs_Insert(WUInt32 uiIndex, WString sValue)
{
  WPrefabResourceHandle hResource;

  if (!sValue.IsEmpty())
  {
    hResource = WResourceManager::LoadResource<WPrefabResource>(sValue);
    WResourceManager::PreloadResource(hResource);
  }

  m_Prefabs.InsertAt(uiIndex, hResource);

  InstantiatePrefabs();
}

void WRandomPrefabComponent::Prefabs_Remove(WUInt32 uiIndex)
{
  m_Prefabs.RemoveAtAndCopy(uiIndex);

  InstantiatePrefabs();
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Placement_Implementation_RandomPrefabComponent);
