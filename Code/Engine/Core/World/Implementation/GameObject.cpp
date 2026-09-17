#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

namespace
{
  static WVariantArray GetDefaultTags()
  {
    WVariantArray value(WStaticsAllocatorWrapper::GetAllocator());
    value.PushBack("CastShadow");
    return value;
  }
} // namespace

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WTransformPreservation, 1)
  W_ENUM_CONSTANTS(WTransformPreservation::PreserveLocal, WTransformPreservation::PreserveGlobal)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WGameObject, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Name", GetNameInternal, SetNameInternal),
    W_ACCESSOR_PROPERTY("Active", GetActiveFlag, SetActiveFlag)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("GlobalKey", GetGlobalKeyInternal, SetGlobalKeyInternal),
    W_ENUM_ACCESSOR_PROPERTY("Mode", WObjectMode, Reflection_GetMode, Reflection_SetMode),
    W_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    W_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new WDefaultValueAttribute(WVec3(1.0f, 1.0f, 1.0f))),
    W_ACCESSOR_PROPERTY("LocalUniformScaling", GetLocalUniformScaling, SetLocalUniformScaling)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_SET_ACCESSOR_PROPERTY("Tags", GetTags, Reflection_SetTag, Reflection_RemoveTag)->AddAttributes(new WTagSetWidgetAttribute("Default"), new WDefaultValueAttribute(GetDefaultTags())),
    W_SET_ACCESSOR_PROPERTY("Children", Reflection_GetChildren, Reflection_AddChild, Reflection_DetachChild)->AddFlags(WPropertyFlags::PointerOwner | WPropertyFlags::Hidden),
    W_SET_ACCESSOR_PROPERTY("Components", Reflection_GetComponents, Reflection_AddComponent, Reflection_RemoveComponent)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(IsActive),
    W_SCRIPT_FUNCTION_PROPERTY(SetCreatedByPrefab),
    W_SCRIPT_FUNCTION_PROPERTY(WasCreatedByPrefab),
    W_SCRIPT_FUNCTION_PROPERTY(SetHideShapeIcon),
    W_SCRIPT_FUNCTION_PROPERTY(IsShapeIconHidden),

    W_SCRIPT_FUNCTION_PROPERTY(HasName, In, "Name"),

    W_SCRIPT_FUNCTION_PROPERTY(HasTag, In, "TagName"),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_SetTag, In, "TagName"),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_RemoveTag, In, "TagName"),

    W_SCRIPT_FUNCTION_PROPERTY(Reflection_GetParent),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_FindChildByName, In, "Name", In, "Recursive")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_FindChildByPath, In, "Path")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(ActivateChildByName, In, "Name", In, "DeactivateOthers")->AddAttributes(new WFunctionArgumentAttributes(1, new WDefaultValueAttribute(true))),

    W_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalPosition, In, "Position"),
    W_SCRIPT_FUNCTION_PROPERTY(GetGlobalPosition),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalRotation, In, "Rotation"),
    W_SCRIPT_FUNCTION_PROPERTY(GetGlobalRotation),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalScaling, In, "Scaling"),
    W_SCRIPT_FUNCTION_PROPERTY(GetGlobalScaling),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalTransform, In, "Transform"),
    W_SCRIPT_FUNCTION_PROPERTY(GetGlobalTransform),

    W_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirForwards),
    W_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirRight),
    W_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirUp),

    W_SCRIPT_FUNCTION_PROPERTY(SetGlobalRotationToLookAt, In, "TargetPosition", In, "Up")->AddAttributes(new WFunctionArgumentAttributes(1, new WDefaultValueAttribute(WVec3::MakeAxisZ()))),
    W_SCRIPT_FUNCTION_PROPERTY(SetGlobalTransformToLookAt, In, "OwnPosition", In, "TargetPosition", In, "Up")->AddAttributes(new WFunctionArgumentAttributes(2, new WDefaultValueAttribute(WVec3::MakeAxisZ()))),

#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
    W_SCRIPT_FUNCTION_PROPERTY(GetLinearVelocity),
    W_SCRIPT_FUNCTION_PROPERTY(GetAngularVelocity),
#endif

    W_SCRIPT_FUNCTION_PROPERTY(SetTeamID, In, "Id"),
    W_SCRIPT_FUNCTION_PROPERTY(GetTeamID),

    W_SCRIPT_FUNCTION_PROPERTY(GetStableRandomSeed),
  }
  W_END_FUNCTIONS;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

void WGameObject::Reflection_SetTag(const char* szTagName)
{
  if (WStringUtils::IsNullOrEmpty(szTagName))
    return;

  const WTag& tag = WTagRegistry::GetGlobalRegistry().RegisterTag(szTagName);
  SetTag(tag);
}

void WGameObject::Reflection_RemoveTag(const char* szTagName)
{
  if (WStringUtils::IsNullOrEmpty(szTagName))
    return;

  if (const WTag* pTag = WTagRegistry::GetGlobalRegistry().GetTagByName(WTempHashedString(szTagName)))
  {
    RemoveTag(*pTag);
  }
}

void WGameObject::Reflection_AddChild(WGameObject* pChild)
{
  if (IsDynamic())
  {
    pChild->MakeDynamic();
  }

  AddChild(pChild->GetHandle(), WTransformPreservation::PreserveLocal);

  // Check whether the child object was only dynamic because of its old parent
  // If that's the case make it static now.
  pChild->ConditionalMakeStatic();
}

void WGameObject::Reflection_DetachChild(WGameObject* pChild)
{
  DetachChild(pChild->GetHandle(), WTransformPreservation::PreserveLocal);

  // The child object is now a top level object, check whether it should be static now.
  pChild->ConditionalMakeStatic();
}

WHybridArray<WGameObject*, 8> WGameObject::Reflection_GetChildren() const
{
  ConstChildIterator it = GetChildren();

  WHybridArray<WGameObject*, 8> all;
  all.Reserve(GetChildCount());

  while (it.IsValid())
  {
    all.PushBack(it.m_pObject);
    ++it;
  }

  return all;
}

void WGameObject::Reflection_AddComponent(WComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  if (pComponent->IsDynamic())
  {
    MakeDynamic();
  }

  AddComponent(pComponent);
}

void WGameObject::Reflection_RemoveComponent(WComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  /*Don't call RemoveComponent here, Component is automatically removed when deleted.*/

  if (pComponent->IsDynamic())
  {
    ConditionalMakeStatic(pComponent);
  }
}

WHybridArray<WComponent*, WGameObject::NUM_INPLACE_COMPONENTS> WGameObject::Reflection_GetComponents() const
{
  return WHybridArray<WComponent*, WGameObject::NUM_INPLACE_COMPONENTS>(m_Components);
}

WObjectMode::Enum WGameObject::Reflection_GetMode() const
{
  return m_Flags.IsSet(WObjectFlags::ForceDynamic) ? WObjectMode::ForceDynamic : WObjectMode::Automatic;
}

void WGameObject::Reflection_SetMode(WObjectMode::Enum mode)
{
  if (Reflection_GetMode() == mode)
  {
    return;
  }

  if (mode == WObjectMode::ForceDynamic)
  {
    m_Flags.Add(WObjectFlags::ForceDynamic);
    MakeDynamic();
  }
  else
  {
    m_Flags.Remove(WObjectFlags::ForceDynamic);
    ConditionalMakeStatic();
  }
}

WGameObject* WGameObject::Reflection_GetParent() const
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

void WGameObject::Reflection_SetGlobalPosition(const WVec3& vPosition)
{
  SetGlobalPosition(vPosition);
}

void WGameObject::Reflection_SetGlobalRotation(const WQuat& qRotation)
{
  SetGlobalRotation(qRotation);
}

void WGameObject::Reflection_SetGlobalScaling(const WVec3& vScaling)
{
  SetGlobalScaling(vScaling);
}

void WGameObject::Reflection_SetGlobalTransform(const WTransform& transform)
{
  SetGlobalTransform(transform);
}

bool WGameObject::DetermineDynamicMode(WComponent* pComponentToIgnore /*= nullptr*/) const
{
  if (m_Flags.IsSet(WObjectFlags::ForceDynamic))
  {
    return true;
  }

  const WGameObject* pParent = GetParent();
  if (pParent != nullptr && pParent->IsDynamic())
  {
    return true;
  }

  for (auto pComponent : m_Components)
  {
    if (pComponent != pComponentToIgnore && pComponent->IsDynamic())
    {
      return true;
    }
  }

  return false;
}

void WGameObject::ConditionalMakeStatic(WComponent* pComponentToIgnore /*= nullptr*/)
{
  if (!DetermineDynamicMode(pComponentToIgnore))
  {
    MakeStaticInternal();

    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      it->ConditionalMakeStatic();
    }
  }
}

void WGameObject::MakeStaticInternal()
{
  if (IsStatic())
  {
    return;
  }

  m_Flags.Remove(WObjectFlags::Dynamic);

  GetWorld()->RecreateHierarchyData(this, true);
}

void WGameObject::UpdateGlobalTransformAndBoundsRecursive()
{
  if (IsStatic() && GetWorld()->ReportErrorWhenStaticObjectMoves())
  {
    WLog::Error("Static object '{0}' was moved during runtime.", GetName());
  }

  WSimdTransform oldGlobalTransform = GetGlobalTransformSimd();

  m_pTransformationData->UpdateGlobalTransformNonRecursive(GetWorld()->GetUpdateCounter());

  if (WSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    m_pTransformationData->UpdateGlobalBoundsAndSpatialData(*pSpatialSystem);
  }
  else
  {
    m_pTransformationData->UpdateGlobalBounds();
  }

  if (IsStatic() && m_Flags.IsSet(WObjectFlags::StaticTransformChangesNotifications) && oldGlobalTransform != GetGlobalTransformSimd())
  {
    WMsgTransformChanged msg;
    msg.m_OldGlobalTransform = WSimdConversion::ToTransform(oldGlobalTransform);
    msg.m_NewGlobalTransform = GetGlobalTransform();

    SendMessage(msg);
  }

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->UpdateGlobalTransformAndBoundsRecursive();
  }
}

void WGameObject::UpdateLastGlobalTransform()
{
  m_pTransformationData->UpdateLastGlobalTransform(GetWorld()->GetUpdateCounter());
}

void WGameObject::ConstChildIterator::Next()
{
  m_pObject = m_pWorld->GetObjectUnchecked(m_pObject->m_uiNextSiblingIndex);
}

WGameObject::~WGameObject()
{
  // Since we are using the small array base class for components we have to cleanup ourself with the correct allocator.
  m_Components.Clear();
  m_Components.Compact(GetWorld()->GetAllocator());
}

void WGameObject::operator=(const WGameObject& other)
{
  W_ASSERT_DEV(m_InternalId.m_WorldIndex == other.m_InternalId.m_WorldIndex, "Cannot copy between worlds.");

  m_InternalId = other.m_InternalId;
  m_Flags = other.m_Flags;
  m_sName = other.m_sName;

  m_uiParentIndex = other.m_uiParentIndex;
  m_uiFirstChildIndex = other.m_uiFirstChildIndex;
  m_uiLastChildIndex = other.m_uiLastChildIndex;

  m_uiNextSiblingIndex = other.m_uiNextSiblingIndex;
  m_uiPrevSiblingIndex = other.m_uiPrevSiblingIndex;
  m_uiChildCount = other.m_uiChildCount;

  m_uiTeamID = other.m_uiTeamID;

  m_uiHierarchyLevel = other.m_uiHierarchyLevel;
  m_pTransformationData = other.m_pTransformationData;
  m_pTransformationData->m_pObject = this;

  if (!m_pTransformationData->m_hSpatialData.IsInvalidated())
  {
    WSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
    pSpatialSystem->UpdateSpatialDataObject(m_pTransformationData->m_hSpatialData, this);
  }

  m_Components.CopyFrom(other.m_Components, GetWorld()->GetAllocator());
  for (WComponent* pComponent : m_Components)
  {
    W_ASSERT_DEV(pComponent->m_pOwner == &other, "");
    pComponent->m_pOwner = this;
  }

  m_Tags = other.m_Tags;
}

void WGameObject::MakeDynamic()
{
  if (IsDynamic())
  {
    return;
  }

  m_Flags.Add(WObjectFlags::Dynamic);

  GetWorld()->RecreateHierarchyData(this, false);

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->MakeDynamic();
  }
}

void WGameObject::MakeStatic()
{
  W_ASSERT_DEV(!DetermineDynamicMode(), "This object can't be static because it has a dynamic parent or dynamic component(s) attached.");

  MakeStaticInternal();
}

void WGameObject::SetActiveFlag(bool bEnabled)
{
  if (m_Flags.IsSet(WObjectFlags::ActiveFlag) == bEnabled)
    return;

  m_Flags.AddOrRemove(WObjectFlags::ActiveFlag, bEnabled);

  UpdateActiveState(GetParent() == nullptr ? true : GetParent()->IsActive());
}

void WGameObject::UpdateActiveState(bool bParentActive)
{
  const bool bSelfActive = bParentActive && m_Flags.IsSet(WObjectFlags::ActiveFlag);

  if (bSelfActive != m_Flags.IsSet(WObjectFlags::ActiveState))
  {
    m_Flags.AddOrRemove(WObjectFlags::ActiveState, bSelfActive);

    for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
    {
      m_Components[i]->UpdateActiveState(bSelfActive);
    }

    // recursively update all children
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      it->UpdateActiveState(bSelfActive);
    }
  }
}

void WGameObject::SetGlobalKey(const WHashedString& sName)
{
  GetWorld()->SetObjectGlobalKey(this, sName);
}

WStringView WGameObject::GetGlobalKey() const
{
  return GetWorld()->GetObjectGlobalKey(this);
}

const char* WGameObject::GetGlobalKeyInternal() const
{
  return GetWorld()->GetObjectGlobalKey(this).GetStartPointer(); // we know that it's zero terminated
}

void WGameObject::SetParent(const WGameObjectHandle& hParent, WTransformPreservation::Enum preserve)
{
  WWorld* pWorld = GetWorld();

  WGameObject* pParent = nullptr;
  bool _ = pWorld->TryGetObject(hParent, pParent);
  W_IGNORE_UNUSED(_);
  pWorld->SetParent(this, pParent, preserve);
}

WGameObject* WGameObject::GetParent()
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

const WGameObject* WGameObject::GetParent() const
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

void WGameObject::AddChild(const WGameObjectHandle& hChild, WTransformPreservation::Enum preserve)
{
  WWorld* pWorld = GetWorld();

  WGameObject* pChild = nullptr;
  if (pWorld->TryGetObject(hChild, pChild))
  {
    pWorld->SetParent(pChild, this, preserve);
  }
}

void WGameObject::DetachChild(const WGameObjectHandle& hChild, WTransformPreservation::Enum preserve)
{
  WWorld* pWorld = GetWorld();

  WGameObject* pChild = nullptr;
  if (pWorld->TryGetObject(hChild, pChild))
  {
    if (pChild->GetParent() == this)
    {
      pWorld->SetParent(pChild, nullptr, preserve);
    }
  }
}

WGameObject::ChildIterator WGameObject::GetChildren()
{
  WWorld* pWorld = GetWorld();
  return ChildIterator(pWorld->GetObjectUnchecked(m_uiFirstChildIndex), pWorld);
}

WGameObject::ConstChildIterator WGameObject::GetChildren() const
{
  const WWorld* pWorld = GetWorld();
  return ConstChildIterator(pWorld->GetObjectUnchecked(m_uiFirstChildIndex), pWorld);
}

WGameObject* WGameObject::FindChildByName(const WTempHashedString& sName, bool bRecursive /*= true*/)
{
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == sName)
    {
      return &(*it);
    }
  }

  if (bRecursive)
  {
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      WGameObject* pChild = it->FindChildByName(sName, bRecursive);

      if (pChild != nullptr)
        return pChild;
    }
  }

  return nullptr;
}

const WGameObject* WGameObject::FindChildByName(const WTempHashedString& sName, bool bRecursive /*= true*/) const
{
  WGameObject* pThis = const_cast<WGameObject*>(this);
  return pThis->FindChildByName(sName, bRecursive);
}

WGameObject* WGameObject::FindChildByPath(WStringView sPath)
{
  if (sPath.IsEmpty())
    return this;

  const char* szSep = sPath.FindSubString("/");
  WUInt64 uiNameHash = 0;

  if (szSep == nullptr)
    uiNameHash = WHashingUtils::StringHash(sPath);
  else
    uiNameHash = WHashingUtils::StringHash(WStringView(sPath.GetStartPointer(), szSep));

  WGameObject* pNextChild = FindChildByName(WTempHashedString(uiNameHash), false);

  if (szSep == nullptr || pNextChild == nullptr)
    return pNextChild;

  return pNextChild->FindChildByPath(WStringView(szSep + 1, sPath.GetEndPointer()));
}

const WGameObject* WGameObject::FindChildByPath(WStringView sPath) const
{
  WGameObject* pThis = const_cast<WGameObject*>(this);
  return pThis->FindChildByPath(sPath);
}

WGameObject* WGameObject::SearchForChildByNameSequence(WStringView sObjectSequence, const WRTTI* pExpectedComponent /*= nullptr*/)
{
  if (sObjectSequence.IsEmpty())
  {
    // in case we are searching for a specific component type, verify that it exists on this object
    if (pExpectedComponent != nullptr)
    {
      const WComponent* pComp = nullptr;
      if (!TryGetComponentOfBaseType(pExpectedComponent, pComp))
        return nullptr;
    }

    return this;
  }

  const char* szSep = sObjectSequence.FindSubString("/");
  WStringView sNextSequence;
  WUInt64 uiNameHash = 0;

  if (szSep == nullptr)
  {
    uiNameHash = WHashingUtils::StringHash(sObjectSequence);
  }
  else
  {
    uiNameHash = WHashingUtils::StringHash(WStringView(sObjectSequence.GetStartPointer(), szSep));
    sNextSequence = WStringView(szSep + 1, sObjectSequence.GetEndPointer());
  }

  const WTempHashedString name(uiNameHash);

  // first go through all direct children an see if any of them actually matches the current name
  // if so, continue the recursion from there and give them the remaining search path to continue
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      WGameObject* res = it->SearchForChildByNameSequence(sNextSequence, pExpectedComponent);
      if (res != nullptr)
        return res;
    }
  }

  // if no direct child fulfilled the requirements, just recurse with the full name sequence
  // however, we can skip any child that already fulfilled the next sequence name,
  // because that's definitely a lost cause
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName != name)
    {
      WGameObject* res = it->SearchForChildByNameSequence(sObjectSequence, pExpectedComponent);
      if (res != nullptr)
        return res;
    }
  }

  return nullptr;
}

const WGameObject* WGameObject::SearchForChildByNameSequence(WStringView sObjectSequence, const WRTTI* pExpectedComponent /*= nullptr*/) const
{
  WGameObject* pThis = const_cast<WGameObject*>(this);
  return pThis->SearchForChildByNameSequence(sObjectSequence, pExpectedComponent);
}

void WGameObject::SearchForChildrenByNameSequence(WStringView sObjectSequence, const WRTTI* pExpectedComponent, WDynamicArray<WGameObject*>& out_objects)
{
  if (sObjectSequence.IsEmpty())
  {
    // in case we are searching for a specific component type, verify that it exists on this object
    if (pExpectedComponent != nullptr)
    {
      WComponent* pComp = nullptr;
      if (!TryGetComponentOfBaseType(pExpectedComponent, pComp))
        return;
    }

    out_objects.PushBack(this);
    return;
  }

  const char* szSep = sObjectSequence.FindSubString("/");
  WStringView sNextSequence;
  WUInt64 uiNameHash = 0;

  if (szSep == nullptr)
  {
    uiNameHash = WHashingUtils::StringHash(sObjectSequence);
  }
  else
  {
    uiNameHash = WHashingUtils::StringHash(WStringView(sObjectSequence.GetStartPointer(), szSep));
    sNextSequence = WStringView(szSep + 1, sObjectSequence.GetEndPointer());
  }

  const WTempHashedString name(uiNameHash);

  // first go through all direct children an see if any of them actually matches the current name
  // if so, continue the recursion from there and give them the remaining search path to continue
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      it->SearchForChildrenByNameSequence(sNextSequence, pExpectedComponent, out_objects);
    }
  }

  // if no direct child fulfilled the requirements, just recurse with the full name sequence
  // however, we can skip any child that already fulfilled the next sequence name,
  // because that's definitely a lost cause
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName != name) // TODO: in this function it is actually debatable whether to skip these or not
    {
      it->SearchForChildrenByNameSequence(sObjectSequence, pExpectedComponent, out_objects);
    }
  }
}

void WGameObject::ActivateChildByName(const WTempHashedString& sName, bool bDeactivateOthers)
{
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == sName)
    {
      it->SetActiveFlag(true);
    }
    else if (bDeactivateOthers)
    {
      it->SetActiveFlag(false);
    }
  }
}

WWorld* WGameObject::GetWorld()
{
  return WWorld::GetWorld(m_InternalId.m_WorldIndex);
}

const WWorld* WGameObject::GetWorld() const
{
  return WWorld::GetWorld(m_InternalId.m_WorldIndex);
}

void WGameObject::SetGlobalRotationToLookAt(const WVec3& vTargetPosition, const WVec3& vUp /*= WVec3::MakeAxisZ()*/)
{
  const WVec3 vDir = vTargetPosition - GetGlobalPosition();
  W_ASSERT_DEV(!vDir.IsZero(0.0001f), "Own position and target position must differ.");

  const WVec3 vFwd = vDir.GetNormalized();
  const WVec3 vRight = vUp.CrossRH(vFwd).GetNormalized();
  const WVec3 vUp2 = vFwd.CrossRH(vRight).GetNormalized();

  WMat3 mLook;
  mLook.SetColumn(0, vFwd);
  mLook.SetColumn(1, vRight);
  mLook.SetColumn(2, vUp2);

  SetGlobalRotation(WQuat::MakeFromMat3(mLook));
}

void WGameObject::SetGlobalTransformToLookAt(const WVec3& vOwnPosition, const WVec3& vTargetPosition, const WVec3& vUp /*= WVec3::MakeAxisZ()*/)
{
  const WVec3 vDir = vTargetPosition - vOwnPosition;
  W_ASSERT_DEV(!vDir.IsZero(0.0001f), "Own position and target position must differ.");

  const WVec3 vFwd = vDir.GetNormalized();
  const WVec3 vRight = vUp.CrossRH(vFwd).GetNormalized();
  const WVec3 vUp2 = vFwd.CrossRH(vRight).GetNormalized();

  WMat3 mLook;
  mLook.SetColumn(0, vFwd);
  mLook.SetColumn(1, vRight);
  mLook.SetColumn(2, vUp2);

  SetGlobalTransform(WTransform(vOwnPosition, WQuat::MakeFromMat3(mLook)));
}

WVec3 WGameObject::GetGlobalDirForwards() const
{
  WCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vForwardDir;
}

WVec3 WGameObject::GetGlobalDirRight() const
{
  WCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vRightDir;
}

WVec3 WGameObject::GetGlobalDirUp() const
{
  WCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vUpDir;
}

#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
void WGameObject::SetLastGlobalTransform(const WSimdTransform& transform)
{
  m_pTransformationData->m_lastGlobalTransform = transform;
  m_pTransformationData->m_uiLastGlobalTransformUpdateCounter = GetWorld()->GetUpdateCounter();
}

WVec3 WGameObject::GetLinearVelocity() const
{
  const WSimdFloat invDeltaSeconds = GetWorld()->GetInvDeltaSeconds();
  const WSimdVec4f linearVelocity = (m_pTransformationData->m_globalTransform.m_Position - m_pTransformationData->m_lastGlobalTransform.m_Position) * invDeltaSeconds;
  return WSimdConversion::ToVec3(linearVelocity);
}

WVec3 WGameObject::GetAngularVelocity() const
{
  const WSimdFloat invDeltaSeconds = GetWorld()->GetInvDeltaSeconds();
  const WSimdQuat q = m_pTransformationData->m_globalTransform.m_Rotation * -m_pTransformationData->m_lastGlobalTransform.m_Rotation;
  WSimdVec4f angularVelocity = WSimdVec4f::MakeZero();

  WSimdVec4f axis;
  WSimdFloat angle;
  if (q.GetRotationAxisAndAngle(axis, angle).Succeeded())
  {
    angularVelocity = axis * (angle * invDeltaSeconds);
  }
  return WSimdConversion::ToVec3(angularVelocity);
}
#endif

void WGameObject::UpdateGlobalTransform()
{
  m_pTransformationData->UpdateGlobalTransformRecursive(GetWorld()->GetUpdateCounter());
}

void WGameObject::UpdateLocalBounds()
{
  WMsgUpdateLocalBounds msg;
  msg.m_ResultingLocalBounds = WBoundingBoxSphere::MakeInvalid();

  SendMessage(msg);

  const bool bIsAlwaysVisible = m_pTransformationData->m_localBounds.m_BoxHalfExtents.w() != WSimdFloat::MakeZero();
  bool bRecreateSpatialData = false;

  if (m_pTransformationData->m_hSpatialData.IsInvalidated() == false)
  {
    // force spatial data re-creation if categories have changed
    bRecreateSpatialData |= m_pTransformationData->m_uiSpatialDataCategoryBitmask != msg.m_uiSpatialDataCategoryBitmask;

    // force spatial data re-creation if always visible flag has changed
    bRecreateSpatialData |= bIsAlwaysVisible != msg.m_bAlwaysVisible;

    // delete old spatial data if bounds are now invalid
    bRecreateSpatialData |= msg.m_bAlwaysVisible == false && msg.m_ResultingLocalBounds.IsValid() == false;
  }

  m_pTransformationData->m_localBounds = WSimdConversion::ToBBoxSphere(msg.m_ResultingLocalBounds);
  m_pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(msg.m_bAlwaysVisible ? 1.0f : 0.0f);
  m_pTransformationData->m_uiSpatialDataCategoryBitmask = msg.m_uiSpatialDataCategoryBitmask;

  WSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
  if (pSpatialSystem != nullptr && (bRecreateSpatialData || m_pTransformationData->m_hSpatialData.IsInvalidated()))
  {
    // UpdateGlobalBounds is called internally by RecreateSpatialData
    m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
  }
  else if (IsStatic())
  {
    m_pTransformationData->UpdateGlobalBounds(pSpatialSystem);
  }
}

void WGameObject::QueueLocalBoundsUpdate()
{
  GetWorld()->QueueLocalBoundsUpdate(GetHandle());
}

void WGameObject::UpdateGlobalTransformAndBounds()
{
  m_pTransformationData->UpdateGlobalTransformRecursive(GetWorld()->GetUpdateCounter());
  m_pTransformationData->UpdateGlobalBounds(GetWorld()->GetSpatialSystem());
}

void WGameObject::UpdateGlobalBounds()
{
  m_pTransformationData->UpdateGlobalBounds(GetWorld()->GetSpatialSystem());
}

bool WGameObject::TryGetComponentOfBaseType(const WRTTI* pType, WComponent*& out_pComponent)
{
  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    WComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_pComponent = pComponent;
      return true;
    }
  }

  out_pComponent = nullptr;
  return false;
}

bool WGameObject::TryGetComponentOfBaseType(const WRTTI* pType, const WComponent*& out_pComponent) const
{
  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    WComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_pComponent = pComponent;
      return true;
    }
  }

  out_pComponent = nullptr;
  return false;
}


void WGameObject::TryGetComponentsOfBaseType(const WRTTI* pType, WDynamicArray<WComponent*>& out_components)
{
  out_components.Clear();

  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    WComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_components.PushBack(pComponent);
    }
  }
}

void WGameObject::TryGetComponentsOfBaseType(const WRTTI* pType, WDynamicArray<const WComponent*>& out_components) const
{
  out_components.Clear();

  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    WComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_components.PushBack(pComponent);
    }
  }
}

void WGameObject::SetTeamID(WUInt16 uiId)
{
  m_uiTeamID = uiId;

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->SetTeamID(uiId);
  }
}

WVisibilityState::Enum WGameObject::GetVisibilityState(WUInt32 uiNumFramesBeforeInvisible) const
{
  if (!m_pTransformationData->m_hSpatialData.IsInvalidated())
  {
    const WSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
    return pSpatialSystem->GetVisibilityState(m_pTransformationData->m_hSpatialData, uiNumFramesBeforeInvisible);
  }

  return WVisibilityState::Direct;
}

void WGameObject::OnMsgDeleteGameObject(WMsgDeleteGameObject& msg)
{
  GetWorld()->DeleteObjectNow(GetHandle(), msg.m_bDeleteEmptyParents);
}

void WGameObject::AddComponent(WComponent* pComponent)
{
  W_ASSERT_DEV(pComponent->m_pOwner == nullptr, "Component must not be added twice.");
  W_ASSERT_DEV(IsDynamic() || !pComponent->IsDynamic(), "Cannot attach a dynamic component to a static object. Call MakeDynamic() first.");

  pComponent->m_pOwner = this;
  m_Components.PushBack(pComponent, GetWorld()->GetAllocator());
  m_Components.GetUserData<ComponentUserData>().m_uiVersion++;

  pComponent->UpdateActiveState(IsActive());

  if (m_Flags.IsSet(WObjectFlags::ComponentChangesNotifications))
  {
    WMsgComponentsChanged msg;
    msg.m_Type = WMsgComponentsChanged::Type::ComponentAdded;
    msg.m_hOwner = GetHandle();
    msg.m_hComponent = pComponent->GetHandle();

    SendNotificationMessage(msg);
  }
}

void WGameObject::RemoveComponent(WComponent* pComponent)
{
  WUInt32 uiIndex = m_Components.IndexOf(pComponent);
  W_ASSERT_DEV(uiIndex != WInvalidIndex, "Component not found");

  pComponent->m_pOwner = nullptr;
  m_Components.RemoveAtAndSwap(uiIndex);
  m_Components.GetUserData<ComponentUserData>().m_uiVersion++;

  if (m_Flags.IsSet(WObjectFlags::ComponentChangesNotifications))
  {
    WMsgComponentsChanged msg;
    msg.m_Type = WMsgComponentsChanged::Type::ComponentRemoved;
    msg.m_hOwner = GetHandle();
    msg.m_hComponent = pComponent->GetHandle();

    SendNotificationMessage(msg);
  }
}

bool WGameObject::SendMessageInternal(WMessage& msg, bool bWasPostedMsg)
{
  bool bSentToAny = false;

  const WRTTI* pRtti = WGetStaticRTTI<WGameObject>();
  if (pRtti->DispatchMessage(this, msg))
  {
    bSentToAny = true;

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
    {
      WLog::Success("WGameObject::SendMessage: Messages of type {0} was delivered to WGameObject.", msg.GetId());
    }
#endif
  }

  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    WComponent* pComponent = m_Components[i];
    if (pComponent->SendMessageInternal(msg, bWasPostedMsg))
    {
      bSentToAny = true;

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
      if (msg.GetDebugMessageRouting())
      {
        WLog::Success("WGameObject::SendMessage: Messages of type {0} was delivered to '{}'.", msg.GetId(), pComponent->GetDynamicRTTI()->GetTypeName());
      }
#endif
    }
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
  {
    if (!bSentToAny)
    {
      WLog::Warning("WGameObject::SendMessage: None of the target object's components had a handler for messages of type {0}.", msg.GetId());
    }
  }
#endif

  return bSentToAny;
}

bool WGameObject::SendMessageInternal(WMessage& msg, bool bWasPostedMsg) const
{
  bool bSentToAny = false;

  const WRTTI* pRtti = WGetStaticRTTI<WGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    // forward only to 'const' message handlers
    const WComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  if (!bSentToAny && msg.GetDebugMessageRouting())
  {
    WLog::Warning("WGameObject::SendMessage (const): None of the target object's components had a handler for messages of type {0}.", msg.GetId());
  }
#endif

  return bSentToAny;
}

bool WGameObject::SendMessageRecursiveInternal(WMessage& msg, bool bWasPostedMsg)
{
  bool bSentToAny = false;

  const WRTTI* pRtti = WGetStaticRTTI<WGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    WComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

  for (auto childIt = GetChildren(); childIt.IsValid(); ++childIt)
  {
    bSentToAny |= childIt->SendMessageRecursiveInternal(msg, bWasPostedMsg);
  }

  // should only be evaluated at the top function call
  // #if W_ENABLED(W_COMPILE_FOR_DEBUG)
  //  if (!bSentToAny && msg.GetDebugMessageRouting())
  //  {
  //    WLog::Warning("WGameObject::SendMessageRecursive: None of the target object's components had a handler for messages of type {0}.",
  //    msg.GetId());
  //  }
  // #endif
  // #
  return bSentToAny;
}

bool WGameObject::SendMessageRecursiveInternal(WMessage& msg, bool bWasPostedMsg) const
{
  bool bSentToAny = false;

  const WRTTI* pRtti = WGetStaticRTTI<WGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    // forward only to 'const' message handlers
    const WComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

  for (auto childIt = GetChildren(); childIt.IsValid(); ++childIt)
  {
    bSentToAny |= childIt->SendMessageRecursiveInternal(msg, bWasPostedMsg);
  }

  // should only be evaluated at the top function call
  // #if W_ENABLED(W_COMPILE_FOR_DEBUG)
  //  if (!bSentToAny && msg.GetDebugMessageRouting())
  //  {
  //    WLog::Warning("WGameObject::SendMessageRecursive(const): None of the target object's components had a handler for messages of type
  //    {0}.", msg.GetId());
  //  }
  // #endif
  // #
  return bSentToAny;
}

void WGameObject::PostMessage(const WMessage& msg, WTime delay, WObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessage(GetHandle(), msg, delay, queueType);
}

void WGameObject::PostMessageRecursive(const WMessage& msg, WTime delay, WObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessageRecursive(GetHandle(), msg, delay, queueType);
}

bool WGameObject::SendEventMessage(WMessage& ref_msg, const WComponent* pSenderComponent)
{
  WTempHybridArray<WComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, pSenderComponent, this, eventMsgHandlers);

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  if (ref_msg.GetDebugMessageRouting())
  {
    if (eventMsgHandlers.IsEmpty())
    {
      WLog::Warning("WGameObject::SendEventMessage: None of the target object's components had a handler for messages of type {0}.", ref_msg.GetId());
    }
  }
#endif

  bool bResult = false;
  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    bResult |= pEventMsgHandler->SendMessage(ref_msg);
  }
  return bResult;
}

bool WGameObject::SendEventMessage(WMessage& ref_msg, const WComponent* pSenderComponent) const
{
  WTempHybridArray<const WComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, pSenderComponent, this, eventMsgHandlers);

  bool bResult = false;
  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    bResult |= pEventMsgHandler->SendMessage(ref_msg);
  }
  return bResult;
}

void WGameObject::PostEventMessage(WMessage& ref_msg, const WComponent* pSenderComponent, WTime delay, WObjectMsgQueueType::Enum queueType) const
{
  WTempHybridArray<const WComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, pSenderComponent, this, eventMsgHandlers);

  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    pEventMsgHandler->PostMessage(ref_msg, delay, queueType);
  }
}

void WGameObject::SetTags(const WTagSet& tags)
{
  if (WSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags != tags)
    {
      m_Tags = tags;
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags = tags;
  }
}

void WGameObject::SetTag(const WTag& tag)
{
  if (WSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags.IsSet(tag) == false)
    {
      m_Tags.Set(tag);
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags.Set(tag);
  }
}

void WGameObject::RemoveTag(const WTag& tag)
{
  if (WSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags.IsSet(tag))
    {
      m_Tags.Remove(tag);
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags.Remove(tag);
  }
}

void WGameObject::FixComponentPointer(WComponent* pOldPtr, WComponent* pNewPtr)
{
  WUInt32 uiIndex = m_Components.IndexOf(pOldPtr);
  W_ASSERT_DEV(uiIndex != WInvalidIndex, "Memory corruption?");
  m_Components[uiIndex] = pNewPtr;
}

void WGameObject::SendNotificationMessage(WMessage& msg)
{
  WGameObject* pObject = this;
  while (pObject != nullptr)
  {
    pObject->SendMessage(msg);

    pObject = pObject->GetParent();
  }
}

//////////////////////////////////////////////////////////////////////////

void WGameObject::TransformationData::UpdateLocalTransform()
{
  WSimdTransform tLocal;

  if (m_pParentData != nullptr)
  {
    tLocal = WSimdTransform::MakeLocalTransform(m_pParentData->m_globalTransform, m_globalTransform);
  }
  else
  {
    tLocal = m_globalTransform;
  }

  m_localPosition = tLocal.m_Position;
  m_localRotation = tLocal.m_Rotation;
  m_localScaling = tLocal.m_Scale;
  m_localScaling.SetW(1.0f);
}

void WGameObject::TransformationData::UpdateGlobalTransformNonRecursive(WUInt32 uiUpdateCounter)
{
  if (m_pParentData != nullptr)
  {
    UpdateGlobalTransformWithParent(uiUpdateCounter);
  }
  else
  {
    UpdateGlobalTransformWithoutParent(uiUpdateCounter);
  }
}

void WGameObject::TransformationData::UpdateGlobalTransformRecursive(WUInt32 uiUpdateCounter)
{
  if (m_pParentData != nullptr)
  {
    m_pParentData->UpdateGlobalTransformRecursive(uiUpdateCounter);
    UpdateGlobalTransformWithParent(uiUpdateCounter);
  }
  else
  {
    UpdateGlobalTransformWithoutParent(uiUpdateCounter);
  }
}

void WGameObject::TransformationData::UpdateGlobalBounds(WSpatialSystem* pSpatialSystem)
{
  if (pSpatialSystem == nullptr)
  {
    UpdateGlobalBounds();
  }
  else
  {
    UpdateGlobalBoundsAndSpatialData(*pSpatialSystem);
  }
}

void WGameObject::TransformationData::UpdateGlobalBoundsAndSpatialData(WSpatialSystem& ref_spatialSystem)
{
  WSimdBBoxSphere oldGlobalBounds = m_globalBounds;

  UpdateGlobalBounds();

  const bool bIsAlwaysVisible = m_localBounds.m_BoxHalfExtents.w() != WSimdFloat::MakeZero();
  if (m_hSpatialData.IsInvalidated() == false && bIsAlwaysVisible == false && m_globalBounds != oldGlobalBounds)
  {
    ref_spatialSystem.UpdateSpatialDataBounds(m_hSpatialData, m_globalBounds);
  }
}

void WGameObject::TransformationData::RecreateSpatialData(WSpatialSystem& ref_spatialSystem)
{
  if (m_hSpatialData.IsInvalidated() == false)
  {
    ref_spatialSystem.DeleteSpatialData(m_hSpatialData);
    m_hSpatialData.Invalidate();
  }

  const bool bIsAlwaysVisible = m_localBounds.m_BoxHalfExtents.w() != WSimdFloat::MakeZero();
  if (bIsAlwaysVisible)
  {
    m_hSpatialData = ref_spatialSystem.CreateSpatialDataAlwaysVisible(m_pObject, m_uiSpatialDataCategoryBitmask, m_pObject->m_Tags);
  }
  else if (m_localBounds.IsValid())
  {
    UpdateGlobalBounds();
    m_hSpatialData = ref_spatialSystem.CreateSpatialData(m_globalBounds, m_pObject, m_uiSpatialDataCategoryBitmask, m_pObject->m_Tags);
  }
}

W_STATICLINK_FILE(Core, Core_World_Implementation_GameObject);
