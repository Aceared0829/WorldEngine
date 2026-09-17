
W_ALWAYS_INLINE WGameObject::ConstChildIterator::ConstChildIterator(WGameObject* pObject, const WWorld* pWorld)
  : m_pObject(pObject)
  , m_pWorld(pWorld)
{
}

W_ALWAYS_INLINE const WGameObject& WGameObject::ConstChildIterator::operator*() const
{
  return *m_pObject;
}

W_ALWAYS_INLINE const WGameObject* WGameObject::ConstChildIterator::operator->() const
{
  return m_pObject;
}

W_ALWAYS_INLINE WGameObject::ConstChildIterator::operator const WGameObject*() const
{
  return m_pObject;
}

W_ALWAYS_INLINE bool WGameObject::ConstChildIterator::IsValid() const
{
  return m_pObject != nullptr;
}

W_ALWAYS_INLINE void WGameObject::ConstChildIterator::operator++()
{
  Next();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE WGameObject::ChildIterator::ChildIterator(WGameObject* pObject, const WWorld* pWorld)
  : ConstChildIterator(pObject, pWorld)
{
}

W_ALWAYS_INLINE WGameObject& WGameObject::ChildIterator::operator*()
{
  return *m_pObject;
}

W_ALWAYS_INLINE WGameObject* WGameObject::ChildIterator::operator->()
{
  return m_pObject;
}

W_ALWAYS_INLINE WGameObject::ChildIterator::operator WGameObject*()
{
  return m_pObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline WGameObject::WGameObject() = default;

W_ALWAYS_INLINE WGameObject::WGameObject(const WGameObject& other)
{
  *this = other;
}

W_ALWAYS_INLINE WGameObjectHandle WGameObject::GetHandle() const
{
  return WGameObjectHandle(m_InternalId);
}

W_ALWAYS_INLINE bool WGameObject::IsDynamic() const
{
  return m_Flags.IsSet(WObjectFlags::Dynamic);
}

W_ALWAYS_INLINE bool WGameObject::IsStatic() const
{
  return !m_Flags.IsSet(WObjectFlags::Dynamic);
}

W_ALWAYS_INLINE bool WGameObject::GetActiveFlag() const
{
  return m_Flags.IsSet(WObjectFlags::ActiveFlag);
}

W_ALWAYS_INLINE bool WGameObject::IsActive() const
{
  return m_Flags.IsSet(WObjectFlags::ActiveState);
}

W_ALWAYS_INLINE void WGameObject::SetName(WStringView sName)
{
  m_sName.Assign(sName);
}

W_ALWAYS_INLINE void WGameObject::SetName(const WHashedString& sName)
{
  m_sName = sName;
}

W_ALWAYS_INLINE void WGameObject::SetGlobalKey(WStringView sKey)
{
  WHashedString sGlobalKey;
  sGlobalKey.Assign(sKey);
  SetGlobalKey(sGlobalKey);
}

W_ALWAYS_INLINE WStringView WGameObject::GetName() const
{
  return m_sName.GetView();
}

W_ALWAYS_INLINE const WHashedString& WGameObject::GetNameHashed() const
{
  return m_sName;
}

W_ALWAYS_INLINE void WGameObject::SetNameInternal(const char* szName)
{
  m_sName.Assign(szName);
}

W_ALWAYS_INLINE const char* WGameObject::GetNameInternal() const
{
  return m_sName;
}

W_ALWAYS_INLINE void WGameObject::SetGlobalKeyInternal(const char* szName)
{
  SetGlobalKey(szName);
}

W_ALWAYS_INLINE bool WGameObject::HasName(const WTempHashedString& sName) const
{
  return m_sName == sName;
}

W_ALWAYS_INLINE void WGameObject::EnableChildChangesNotifications()
{
  m_Flags.Add(WObjectFlags::ChildChangesNotifications);
}

W_ALWAYS_INLINE void WGameObject::DisableChildChangesNotifications()
{
  m_Flags.Remove(WObjectFlags::ChildChangesNotifications);
}

W_ALWAYS_INLINE void WGameObject::EnableParentChangesNotifications()
{
  m_Flags.Add(WObjectFlags::ParentChangesNotifications);
}

W_ALWAYS_INLINE void WGameObject::DisableParentChangesNotifications()
{
  m_Flags.Remove(WObjectFlags::ParentChangesNotifications);
}

W_ALWAYS_INLINE void WGameObject::AddChildren(const WArrayPtr<const WGameObjectHandle>& children, WTransformPreservation::Enum preserve)
{
  for (WUInt32 i = 0; i < children.GetCount(); ++i)
  {
    AddChild(children[i], preserve);
  }
}

W_ALWAYS_INLINE void WGameObject::DetachChildren(const WArrayPtr<const WGameObjectHandle>& children, WTransformPreservation::Enum preserve)
{
  for (WUInt32 i = 0; i < children.GetCount(); ++i)
  {
    DetachChild(children[i], preserve);
  }
}

W_ALWAYS_INLINE WUInt32 WGameObject::GetChildCount() const
{
  return m_uiChildCount;
}


W_ALWAYS_INLINE void WGameObject::SetLocalPosition(WVec3 vPosition)
{
  SetLocalPosition(WSimdConversion::ToVec3(vPosition));
}

W_ALWAYS_INLINE WVec3 WGameObject::GetLocalPosition() const
{
  return WSimdConversion::ToVec3(m_pTransformationData->m_localPosition);
}


W_ALWAYS_INLINE void WGameObject::SetLocalRotation(WQuat qRotation)
{
  SetLocalRotation(WSimdConversion::ToQuat(qRotation));
}

W_ALWAYS_INLINE WQuat WGameObject::GetLocalRotation() const
{
  return WSimdConversion::ToQuat(m_pTransformationData->m_localRotation);
}


W_ALWAYS_INLINE void WGameObject::SetLocalScaling(WVec3 vScaling)
{
  SetLocalScaling(WSimdConversion::ToVec3(vScaling));
}

W_ALWAYS_INLINE WVec3 WGameObject::GetLocalScaling() const
{
  return WSimdConversion::ToVec3(m_pTransformationData->m_localScaling);
}


W_ALWAYS_INLINE void WGameObject::SetLocalUniformScaling(float fScaling)
{
  SetLocalUniformScaling(WSimdFloat(fScaling));
}

W_ALWAYS_INLINE float WGameObject::GetLocalUniformScaling() const
{
  return m_pTransformationData->m_localScaling.w();
}

W_ALWAYS_INLINE WTransform WGameObject::GetLocalTransform() const
{
  return WSimdConversion::ToTransform(GetLocalTransformSimd());
}


W_ALWAYS_INLINE void WGameObject::SetGlobalPosition(const WVec3& vPosition)
{
  SetGlobalPosition(WSimdConversion::ToVec3(vPosition));
}

W_ALWAYS_INLINE WVec3 WGameObject::GetGlobalPosition() const
{
  return WSimdConversion::ToVec3(m_pTransformationData->m_globalTransform.m_Position);
}


W_ALWAYS_INLINE void WGameObject::SetGlobalRotation(const WQuat& qRotation)
{
  SetGlobalRotation(WSimdConversion::ToQuat(qRotation));
}

W_ALWAYS_INLINE WQuat WGameObject::GetGlobalRotation() const
{
  return WSimdConversion::ToQuat(m_pTransformationData->m_globalTransform.m_Rotation);
}


W_ALWAYS_INLINE void WGameObject::SetGlobalScaling(const WVec3& vScaling)
{
  SetGlobalScaling(WSimdConversion::ToVec3(vScaling));
}

W_ALWAYS_INLINE WVec3 WGameObject::GetGlobalScaling() const
{
  return WSimdConversion::ToVec3(m_pTransformationData->m_globalTransform.m_Scale);
}


W_ALWAYS_INLINE void WGameObject::SetGlobalTransform(const WTransform& transform)
{
  SetGlobalTransform(WSimdConversion::ToTransform(transform));
}

W_ALWAYS_INLINE WTransform WGameObject::GetGlobalTransform() const
{
  return WSimdConversion::ToTransform(m_pTransformationData->m_globalTransform);
}

W_ALWAYS_INLINE WTransform WGameObject::GetLastGlobalTransform() const
{
  return WSimdConversion::ToTransform(GetLastGlobalTransformSimd());
}


W_ALWAYS_INLINE void WGameObject::SetLocalPosition(const WSimdVec4f& vPosition, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localPosition = vPosition;

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

W_ALWAYS_INLINE const WSimdVec4f& WGameObject::GetLocalPositionSimd() const
{
  return m_pTransformationData->m_localPosition;
}


W_ALWAYS_INLINE void WGameObject::SetLocalRotation(const WSimdQuat& qRotation, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localRotation = qRotation;

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

W_ALWAYS_INLINE const WSimdQuat& WGameObject::GetLocalRotationSimd() const
{
  return m_pTransformationData->m_localRotation;
}


W_ALWAYS_INLINE void WGameObject::SetLocalScaling(const WSimdVec4f& vScaling, UpdateBehaviorIfStatic updateBehavior)
{
  WSimdFloat uniformScale = m_pTransformationData->m_localScaling.w();
  m_pTransformationData->m_localScaling = vScaling;
  m_pTransformationData->m_localScaling.SetW(uniformScale);

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

W_ALWAYS_INLINE const WSimdVec4f& WGameObject::GetLocalScalingSimd() const
{
  return m_pTransformationData->m_localScaling;
}


W_ALWAYS_INLINE void WGameObject::SetLocalUniformScaling(const WSimdFloat& fScaling, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localScaling.SetW(fScaling);

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

W_ALWAYS_INLINE WSimdFloat WGameObject::GetLocalUniformScalingSimd() const
{
  return m_pTransformationData->m_localScaling.w();
}

W_ALWAYS_INLINE WSimdTransform WGameObject::GetLocalTransformSimd() const
{
  const WSimdVec4f vScale = m_pTransformationData->m_localScaling * m_pTransformationData->m_localScaling.w();
  return WSimdTransform(m_pTransformationData->m_localPosition, m_pTransformationData->m_localRotation, vScale);
}


W_ALWAYS_INLINE void WGameObject::SetGlobalPosition(const WSimdVec4f& vPosition)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform.m_Position = vPosition;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

W_ALWAYS_INLINE const WSimdVec4f& WGameObject::GetGlobalPositionSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Position;
}


W_ALWAYS_INLINE void WGameObject::SetGlobalRotation(const WSimdQuat& qRotation)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform.m_Rotation = qRotation;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

W_ALWAYS_INLINE const WSimdQuat& WGameObject::GetGlobalRotationSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Rotation;
}


W_ALWAYS_INLINE void WGameObject::SetGlobalScaling(const WSimdVec4f& vScaling)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform.m_Scale = vScaling;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

W_ALWAYS_INLINE const WSimdVec4f& WGameObject::GetGlobalScalingSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Scale;
}


W_ALWAYS_INLINE void WGameObject::SetGlobalTransform(const WSimdTransform& transform)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform = transform;

  // WTransformTemplate<Type>::SetLocalTransform will produce NaNs in w components
  // of pos and scale if scale.w is not set to 1 here. This only affects builds that
  // use W_SIMD_IMPLEMENTATION_FPU, e.g. arm atm.
  m_pTransformationData->m_globalTransform.m_Scale.SetW(1.0f);
  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

W_ALWAYS_INLINE const WSimdTransform& WGameObject::GetGlobalTransformSimd() const
{
  return m_pTransformationData->m_globalTransform;
}

W_ALWAYS_INLINE const WSimdTransform& WGameObject::GetLastGlobalTransformSimd() const
{
#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
  return m_pTransformationData->m_lastGlobalTransform;
#else
  return m_pTransformationData->m_globalTransform;
#endif
}

W_ALWAYS_INLINE void WGameObject::EnableStaticTransformChangesNotifications()
{
  m_Flags.Add(WObjectFlags::StaticTransformChangesNotifications);
}

W_ALWAYS_INLINE void WGameObject::DisableStaticTransformChangesNotifications()
{
  m_Flags.Remove(WObjectFlags::StaticTransformChangesNotifications);
}

W_ALWAYS_INLINE WBoundingBoxSphere WGameObject::GetLocalBounds() const
{
  return WSimdConversion::ToBBoxSphere(m_pTransformationData->m_localBounds);
}

W_ALWAYS_INLINE WBoundingBoxSphere WGameObject::GetGlobalBounds() const
{
  return WSimdConversion::ToBBoxSphere(m_pTransformationData->m_globalBounds);
}

W_ALWAYS_INLINE const WSimdBBoxSphere& WGameObject::GetLocalBoundsSimd() const
{
  return m_pTransformationData->m_localBounds;
}

W_ALWAYS_INLINE const WSimdBBoxSphere& WGameObject::GetGlobalBoundsSimd() const
{
  return m_pTransformationData->m_globalBounds;
}

W_ALWAYS_INLINE WSpatialDataHandle WGameObject::GetSpatialData() const
{
  return m_pTransformationData->m_hSpatialData;
}

W_ALWAYS_INLINE void WGameObject::EnableComponentChangesNotifications()
{
  m_Flags.Add(WObjectFlags::ComponentChangesNotifications);
}

W_ALWAYS_INLINE void WGameObject::DisableComponentChangesNotifications()
{
  m_Flags.Remove(WObjectFlags::ComponentChangesNotifications);
}

template <typename T>
W_ALWAYS_INLINE bool WGameObject::TryGetComponentOfBaseType(T*& out_pComponent)
{
  return TryGetComponentOfBaseType(WGetStaticRTTI<T>(), (WComponent*&)out_pComponent);
}

template <typename T>
W_ALWAYS_INLINE bool WGameObject::TryGetComponentOfBaseType(const T*& out_pComponent) const
{
  return TryGetComponentOfBaseType(WGetStaticRTTI<T>(), (const WComponent*&)out_pComponent);
}

template <typename T>
void WGameObject::TryGetComponentsOfBaseType(WDynamicArray<T*>& out_components)
{
  out_components.Clear();

  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    WComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf<T>())
    {
      out_components.PushBack(static_cast<T*>(pComponent));
    }
  }
}

template <typename T>
void WGameObject::TryGetComponentsOfBaseType(WDynamicArray<const T*>& out_components) const
{
  out_components.Clear();

  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    WComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf<T>())
    {
      out_components.PushBack(static_cast<const T*>(pComponent));
    }
  }
}

W_ALWAYS_INLINE WArrayPtr<WComponent* const> WGameObject::GetComponents()
{
  return m_Components;
}

W_ALWAYS_INLINE WArrayPtr<const WComponent* const> WGameObject::GetComponents() const
{
  return WMakeArrayPtr(const_cast<const WComponent* const*>(m_Components.GetData()), m_Components.GetCount());
}

W_ALWAYS_INLINE WUInt16 WGameObject::GetComponentVersion() const
{
  return m_Components.GetUserData<ComponentUserData>().m_uiVersion;
}

W_ALWAYS_INLINE bool WGameObject::SendMessage(WMessage& ref_msg)
{
  return SendMessageInternal(ref_msg, false);
}

W_ALWAYS_INLINE bool WGameObject::SendMessage(WMessage& ref_msg) const
{
  return SendMessageInternal(ref_msg, false);
}

W_ALWAYS_INLINE bool WGameObject::SendMessageRecursive(WMessage& ref_msg)
{
  return SendMessageRecursiveInternal(ref_msg, false);
}

W_ALWAYS_INLINE bool WGameObject::SendMessageRecursive(WMessage& ref_msg) const
{
  return SendMessageRecursiveInternal(ref_msg, false);
}

W_ALWAYS_INLINE const WTagSet& WGameObject::GetTags() const
{
  return m_Tags;
}

W_ALWAYS_INLINE bool WGameObject::HasTag(const WTempHashedString& sTagName) const
{
  return m_Tags.IsSetByName(sTagName);
}

W_ALWAYS_INLINE WUInt32 WGameObject::GetStableRandomSeed() const
{
  return m_pTransformationData->m_uiStableRandomSeed;
}

W_ALWAYS_INLINE void WGameObject::SetStableRandomSeed(WUInt32 uiSeed)
{
  m_pTransformationData->m_uiStableRandomSeed = uiSeed;
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE void WGameObject::TransformationData::UpdateGlobalTransformWithoutParent(WUInt32 uiUpdateCounter)
{
  UpdateLastGlobalTransform(uiUpdateCounter);

  m_globalTransform.m_Position = m_localPosition;
  m_globalTransform.m_Rotation = m_localRotation;
  m_globalTransform.m_Scale = m_localScaling * m_localScaling.w();
}

W_ALWAYS_INLINE void WGameObject::TransformationData::UpdateGlobalTransformWithParent(WUInt32 uiUpdateCounter)
{
  UpdateLastGlobalTransform(uiUpdateCounter);

  const WSimdVec4f vScale = m_localScaling * m_localScaling.w();
  const WSimdTransform localTransform(m_localPosition, m_localRotation, vScale);
  m_globalTransform = WSimdTransform::MakeGlobalTransform(m_pParentData->m_globalTransform, localTransform);
}

W_FORCE_INLINE void WGameObject::TransformationData::UpdateGlobalBounds()
{
  m_globalBounds = m_localBounds;
  m_globalBounds.Transform(m_globalTransform);
}

W_ALWAYS_INLINE void WGameObject::TransformationData::UpdateLastGlobalTransform(WUInt32 uiUpdateCounter)
{
#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
  if (m_uiLastGlobalTransformUpdateCounter != uiUpdateCounter)
  {
    m_lastGlobalTransform = m_globalTransform;
    m_uiLastGlobalTransformUpdateCounter = uiUpdateCounter;
  }
#endif
}
