#include <Core/CorePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WPrefabReferenceComponent, 4, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Prefab", GetPrefab, SetPrefab)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("ShowShapeIcons", GetShowShapeIcons, SetShowShapeIcons),
    W_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new WExposedParametersAttribute("Prefab")),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Prefabs"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum class PrefabComponentFlags : WUInt8
{
  SelfDeletion = 1,   ///< the prefab component is currently deleting itself but does not want to remove the instantiated objects
  ShowShapeIcons = 2, ///< the prefab component should show shape icons for the instantiated objects
  InUpdateList = 3,   ///< the prefab component is currently in the update list of the prefab manager
};

WPrefabReferenceComponent::WPrefabReferenceComponent() = default;
WPrefabReferenceComponent::~WPrefabReferenceComponent() = default;

void WPrefabReferenceComponent::SerializePrefabParameters(const WWorld& world, WWorldWriter& inout_stream, WArrayMap<WHashedString, WVariant> parameters)
{
  // we need a copy of the parameters here, therefore we don't take it by reference

  auto& s = inout_stream.GetStream();
  const WUInt32 numParams = parameters.GetCount();

  WTempHybridArray<WGameObjectHandle, 8> GoReferences;

  // Version 4
  {
    // to support game object references as exposed parameters (which are currently exposed as strings)
    // we need to remap the string from an 'editor uuid' to something that can be interpreted as a proper WGameObjectHandle at runtime

    // so first we get the resolver and try to map any string parameter to a valid WGameObjectHandle
    auto resolver = world.GetGameObjectReferenceResolver();

    if (resolver.IsValid())
    {
      WStringBuilder tmp;

      for (WUInt32 i = 0; i < numParams; ++i)
      {
        // if this is a string parameter
        WVariant& var = parameters.GetValue(i);
        if (var.IsA<WString>())
        {
          // and the resolver CAN map this string to a game object handle
          WGameObjectHandle hObject = resolver(var.Get<WString>().GetData(), WComponentHandle(), nullptr);
          if (!hObject.IsInvalidated())
          {
            // write the handle properly to file (this enables correct remapping during deserialization)
            // and discard the string's value, and instead write a string that specifies the index of the serialized handle to use

            // local game object reference - index into GoReferences
            tmp.SetFormat("#!LGOR-{}", GoReferences.GetCount());
            var = tmp.GetData();

            GoReferences.PushBack(hObject);
          }
        }
      }
    }

    // now write all the WGameObjectHandle's such that during deserialization the WWorldReader will remap it as needed
    const WUInt8 numRefs = static_cast<WUInt8>(GoReferences.GetCount());
    s << numRefs;

    for (WUInt8 i = 0; i < numRefs; ++i)
    {
      inout_stream.WriteGameObjectHandle(GoReferences[i]);
    }
  }

  // Version 2
  s << numParams;
  for (WUInt32 i = 0; i < numParams; ++i)
  {
    s << parameters.GetKey(i);
    s << parameters.GetValue(i); // this may contain modified strings now, to map the game object handle references
  }
}

void WPrefabReferenceComponent::DeserializePrefabParameters(WArrayMap<WHashedString, WVariant>& out_parameters, WWorldReader& inout_stream)
{
  out_parameters.Clear();

  // versioning of this stuff is tied to the version number of WPrefabReferenceComponent
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(WGetStaticRTTI<WPrefabReferenceComponent>());
  auto& s = inout_stream.GetStream();

  // temp array to hold (and remap) the serialized game object handles
  WTempHybridArray<WGameObjectHandle, 8> GoReferences;

  if (uiVersion >= 4)
  {
    WUInt8 numRefs = 0;
    s >> numRefs;
    GoReferences.SetCountUninitialized(numRefs);

    // just read them all, this will remap as necessary to the WWorldReader
    for (WUInt8 i = 0; i < numRefs; ++i)
    {
      GoReferences[i] = inout_stream.ReadGameObjectHandle();
    }
  }

  if (uiVersion >= 2)
  {
    WUInt32 numParams = 0;
    s >> numParams;

    out_parameters.Reserve(numParams);

    WHashedString key;
    WVariant value;
    WStringBuilder tmp;

    for (WUInt32 i = 0; i < numParams; ++i)
    {
      s >> key;
      s >> value;

      if (value.IsA<WString>())
      {
        // if we find a string parameter, check if it is a 'local game object reference'
        const WString& str = value.Get<WString>();
        if (str.StartsWith("#!LGOR-"))
        {
          // if so, extract the index into the GoReferences array
          WInt32 idx;
          if (WConversionUtils::StringToInt(str.GetData() + 7, idx).Succeeded())
          {
            // now we can lookup the remapped WGameObjectHandle from our array
            const WGameObjectHandle hObject = GoReferences[idx];

            // and stringify the handle into a 'global game object reference', ie. one that contains the internal integer data of the handle
            // a regular runtime world has a reference resolver that is capable to reverse this stringified format to a handle again
            // which will happen once 'InstantiatePrefab' passes the m_Parameters list to the newly created objects
            tmp.SetFormat("#!GGOR-{}", hObject.GetInternalID().m_Data);

            // map local game object reference to global game object reference
            value = tmp.GetData();
          }
        }
      }

      out_parameters.Insert(key, value);
    }
  }
}

void WPrefabReferenceComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hPrefab;

  WPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), inout_stream, m_Parameters);
}

void WPrefabReferenceComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hPrefab;

  if (uiVersion < 3)
  {
    bool bDummy;
    s >> bDummy;
  }

  WPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, inout_stream);
}

void WPrefabReferenceComponent::SetPrefab(const WPrefabResourceHandle& hPrefab)
{
  if (m_hPrefab == hPrefab)
    return;

  m_hPrefab = hPrefab;

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway

    GetWorld()->GetComponentManager<WPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

void WPrefabReferenceComponent::SetShowShapeIcons(bool bShow)
{
  SetUserFlag((WUInt8)PrefabComponentFlags::ShowShapeIcons, bShow);

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway

    GetWorld()->GetComponentManager<WPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

bool WPrefabReferenceComponent::GetShowShapeIcons() const
{
  return GetUserFlag((WUInt8)PrefabComponentFlags::ShowShapeIcons);
}

void WPrefabReferenceComponent::InstantiatePrefab()
{
  // now instantiate the prefab
  if (m_hPrefab.IsValid())
  {
    WResourceLock<WPrefabResource> pResource(m_hPrefab, WResourceAcquireMode::AllowLoadingFallback);

    WTransform id;
    id.SetIdentity();

    WPrefabInstantiationOptions options;
    options.m_hParent = GetOwner()->GetHandle();
    options.m_ReplaceNamedRootWithParent = "<Prefab-Root>";
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    // if this ID is valid, this prefab is instantiated at editor runtime
    // replicate the same ID across all instantiated sub components to get correct picking behavior
    if (GetUniqueID() != WInvalidIndex)
    {
      WTempHybridArray<WGameObject*, 8> createdRootObjects;
      WTempHybridArray<WGameObject*, 16> createdChildObjects;

      options.m_pCreatedRootObjectsOut = &createdRootObjects;
      options.m_pCreatedChildObjectsOut = &createdChildObjects;

      WUInt32 uiPrevCompCount = GetOwner()->GetComponents().GetCount();

      pResource->InstantiatePrefab(*GetWorld(), id, options, &m_Parameters);

      auto FixComponent = [](WGameObject* pChild, WUInt32 uiUniqueID, bool bShowShapeIcons)
      {
        // while exporting a scene all game objects with this flag are ignored and not exported
        // set this flag on all game objects that were created by instantiating this prefab
        // instead it should be instantiated at runtime again
        // only do this at editor time though, at regular runtime we do want to fully serialize the entire sub tree
        pChild->SetCreatedByPrefab();

        if (!bShowShapeIcons)
        {
          pChild->SetHideShapeIcon();
        }

        for (auto pComponent : pChild->GetComponents())
        {
          pComponent->SetUniqueID(uiUniqueID);
          pComponent->SetCreatedByPrefab();
        }
      };

      const WUInt32 uiUniqueID = GetUniqueID();
      const bool bShowShapeIcons = GetShowShapeIcons();

      for (WGameObject* pChild : createdRootObjects)
      {
        if (pChild == GetOwner())
          continue;

        FixComponent(pChild, uiUniqueID, bShowShapeIcons);
      }

      for (WGameObject* pChild : createdChildObjects)
      {
        FixComponent(pChild, uiUniqueID, bShowShapeIcons);
      }

      for (; uiPrevCompCount < GetOwner()->GetComponents().GetCount(); ++uiPrevCompCount)
      {
        GetOwner()->GetComponents()[uiPrevCompCount]->SetUniqueID(GetUniqueID());
        GetOwner()->GetComponents()[uiPrevCompCount]->SetCreatedByPrefab();
      }
    }
    else
    {
      pResource->InstantiatePrefab(*GetWorld(), id, options, &m_Parameters);
    }
  }
}

void WPrefabReferenceComponent::OnActivated()
{
  SUPER::OnActivated();

  // instantiate the prefab right away, such that game play code can access it as soon as possible
  // additionally the manager may update the instance later on, to properly enable editor work flows
  InstantiatePrefab();
}

void WPrefabReferenceComponent::OnDeactivated()
{
  // if this was created procedurally during editor runtime, we do not need to clear specific nodes
  // after simulation, the scene is deleted anyway

  ClearPreviousInstances();

  SUPER::OnDeactivated();
}

void WPrefabReferenceComponent::ClearPreviousInstances()
{
  if (GetUniqueID() != WInvalidIndex)
  {
    // if this is in the editor, and the 'activate' flag is toggled,
    // get rid of all our created child objects

    WArrayPtr<WComponent* const> comps = GetOwner()->GetComponents();

    for (WUInt32 ip1 = comps.GetCount(); ip1 > 0; ip1--)
    {
      const WUInt32 i = ip1 - 1;
      WComponent* pComp = comps[i];

      if (pComp == this || // don't try to delete yourself
          pComp->WasCreatedByPrefab() == false)
        continue;

      // Prevent other prefab components from deleting its instances. This might lead to an endless loop of prefab components deleting each other.
      if (pComp->IsInstanceOf<WPrefabReferenceComponent>())
      {
        pComp->SetUserFlag((WUInt8)PrefabComponentFlags::SelfDeletion, true);
      }

      pComp->DeleteComponent();
    }

    for (auto it = GetOwner()->GetChildren(); it.IsValid(); ++it)
    {
      if (it->WasCreatedByPrefab())
      {
        constexpr bool bAlsoDeleteEmptyParents = false;
        GetWorld()->DeleteObjectNow(it->GetHandle(), bAlsoDeleteEmptyParents);
      }
    }
  }
}

void WPrefabReferenceComponent::Deinitialize()
{
  if (GetUserFlag((WUInt8)PrefabComponentFlags::SelfDeletion))
  {
    // do nothing, ie do not call OnDeactivated()
    // we do want to keep the created child objects around when this component gets destroyed during simulation
    // that's because the component actually deletes itself when simulation starts
    return;
  }

  // remove the children (through Deactivate)
  OnDeactivated();
}

void WPrefabReferenceComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetUniqueID() == WInvalidIndex) // when running inside the editor, we don't want to delete the component, otherwise the object is not selectable anymore while simulating
  {
    SetUserFlag((WUInt8)PrefabComponentFlags::SelfDeletion, true);

    // remove the prefab reference component, to prevent issues after another serialization/deserialization
    // and also to save some memory
    DeleteComponent();
  }
}

const WRangeView<const char*, WUInt32> WPrefabReferenceComponent::GetParameters() const
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

void WPrefabReferenceComponent::SetParameter(const char* szKey, const WVariant& value)
{
  WHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != WInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway
    GetWorld()->GetComponentManager<WPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

void WPrefabReferenceComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(WTempHashedString(szKey)))
  {
    if (IsActiveAndInitialized())
    {
      // only add to update list, if not yet activated,
      // since OnActivate will do the instantiation anyway
      GetWorld()->GetComponentManager<WPrefabReferenceComponentManager>()->AddToUpdateList(this);
    }
  }
}

bool WPrefabReferenceComponent::GetParameter(const char* szKey, WVariant& out_value) const
{
  WUInt32 it = m_Parameters.Find(szKey);

  if (it == WInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

//////////////////////////////////////////////////////////////////////////

WPrefabReferenceComponentManager::WPrefabReferenceComponentManager(WWorld* pWorld)
  : WComponentManager<ComponentType, WBlockStorageType::Compact>(pWorld)
{
  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WPrefabReferenceComponentManager::ResourceEventHandler, this));
}


WPrefabReferenceComponentManager::~WPrefabReferenceComponentManager()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WPrefabReferenceComponentManager::ResourceEventHandler, this));
}

void WPrefabReferenceComponentManager::Initialize()
{
  auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WPrefabReferenceComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void WPrefabReferenceComponentManager::ResourceEventHandler(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<WPrefabResource>())
  {
    WPrefabResourceHandle hPrefab((WPrefabResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hPrefab == hPrefab)
      {
        AddToUpdateList(it);
      }
    }
  }
}

void WPrefabReferenceComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  W_IGNORE_UNUSED(context);

  for (auto hComp : m_ComponentsToUpdate)
  {
    WPrefabReferenceComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    pComponent->SetUserFlag((WUInt8)PrefabComponentFlags::InUpdateList, false);
    if (!pComponent->IsActive())
      continue;

    pComponent->ClearPreviousInstances();
    pComponent->InstantiatePrefab();
  }

  m_ComponentsToUpdate.Clear();
}

void WPrefabReferenceComponentManager::AddToUpdateList(WPrefabReferenceComponent* pComponent)
{
  if (!pComponent->GetUserFlag((WUInt8)PrefabComponentFlags::InUpdateList))
  {
    m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
    pComponent->SetUserFlag((WUInt8)PrefabComponentFlags::InUpdateList, true);
  }
}



W_STATICLINK_FILE(Core, Core_Prefabs_Implementation_PrefabReferenceComponent);
