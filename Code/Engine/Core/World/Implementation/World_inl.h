
W_ALWAYS_INLINE WStringView WWorld::GetName() const
{
  return m_Data.m_sName;
}

W_ALWAYS_INLINE WUInt32 WWorld::GetIndex() const
{
  return m_InternalId.m_InstanceIndex;
}

W_ALWAYS_INLINE WWorldHandle WWorld::GetHandle() const
{
  return WWorldHandle(m_InternalId);
}

W_FORCE_INLINE WGameObjectHandle WWorld::CreateObject(const WGameObjectDesc& desc)
{
  WGameObject* pNewObject;
  return CreateObject(desc, pNewObject);
}

W_ALWAYS_INLINE const WEvent<const WGameObject*>& WWorld::GetObjectDeletionEvent() const
{
  return m_Data.m_ObjectDeletionEvent;
}

W_FORCE_INLINE bool WWorld::IsValidObject(const WGameObjectHandle& hObject) const
{
  CheckForReadAccess();
  W_ASSERT_DEV(hObject.IsInvalidated() || hObject.m_InternalId.m_WorldIndex == GetIndex(),
    "Object does not belong to this world. Expected world id {0} got id {1}", GetIndex(), hObject.m_InternalId.m_WorldIndex);

  return m_Data.m_Objects.Contains(hObject);
}

W_FORCE_INLINE bool WWorld::TryGetObject(const WGameObjectHandle& hObject, WGameObject*& out_pObject)
{
  CheckForReadAccess();
  W_ASSERT_DEV(hObject.IsInvalidated() || hObject.m_InternalId.m_WorldIndex == GetIndex(),
    "Object does not belong to this world. Expected world id {0} got id {1}", GetIndex(), hObject.m_InternalId.m_WorldIndex);

  return m_Data.m_Objects.TryGetValue(hObject, out_pObject);
}

W_FORCE_INLINE bool WWorld::TryGetObject(const WGameObjectHandle& hObject, const WGameObject*& out_pObject) const
{
  CheckForReadAccess();
  W_ASSERT_DEV(hObject.IsInvalidated() || hObject.m_InternalId.m_WorldIndex == GetIndex(),
    "Object does not belong to this world. Expected world id {0} got id {1}", GetIndex(), hObject.m_InternalId.m_WorldIndex);

  WGameObject* pObject = nullptr;
  bool bResult = m_Data.m_Objects.TryGetValue(hObject, pObject);
  out_pObject = pObject;
  return bResult;
}

W_FORCE_INLINE bool WWorld::TryGetObjectWithGlobalKey(const WTempHashedString& sGlobalKey, WGameObject*& out_pObject)
{
  CheckForReadAccess();
  WGameObjectId id;
  if (m_Data.m_GlobalKeyToIdTable.TryGetValue(sGlobalKey.GetHash(), id))
  {
    out_pObject = m_Data.m_Objects[id];
    return true;
  }

  return false;
}

W_FORCE_INLINE bool WWorld::TryGetObjectWithGlobalKey(const WTempHashedString& sGlobalKey, const WGameObject*& out_pObject) const
{
  CheckForReadAccess();
  WGameObjectId id;
  if (m_Data.m_GlobalKeyToIdTable.TryGetValue(sGlobalKey.GetHash(), id))
  {
    out_pObject = m_Data.m_Objects[id];
    return true;
  }

  return false;
}

W_FORCE_INLINE WUInt32 WWorld::GetObjectCount() const
{
  CheckForReadAccess();
  // Subtract one to exclude dummy object with instance index 0
  return static_cast<WUInt32>(m_Data.m_Objects.GetCount() - 1);
}

W_FORCE_INLINE WInternal::WorldData::ObjectIterator WWorld::GetObjects()
{
  CheckForWriteAccess();
  return WInternal::WorldData::ObjectIterator(m_Data.m_ObjectStorage.GetIterator(0));
}

W_FORCE_INLINE WInternal::WorldData::ConstObjectIterator WWorld::GetObjects() const
{
  CheckForReadAccess();
  return WInternal::WorldData::ConstObjectIterator(m_Data.m_ObjectStorage.GetIterator(0));
}

W_FORCE_INLINE void WWorld::Traverse(VisitorFunc visitorFunc, TraversalMethod method /*= DepthFirst*/)
{
  CheckForWriteAccess();

  if (method == DepthFirst)
  {
    m_Data.TraverseDepthFirst(visitorFunc);
  }
  else // method == BreadthFirst
  {
    m_Data.TraverseBreadthFirst(visitorFunc);
  }
}

template <typename ModuleType>
W_ALWAYS_INLINE ModuleType* WWorld::GetOrCreateModule()
{
  static_assert(W_IS_DERIVED_FROM_STATIC(WWorldModule, ModuleType), "Not a valid module type");

  return WStaticCast<ModuleType*>(GetOrCreateModule(WGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
W_ALWAYS_INLINE void WWorld::DeleteModule()
{
  static_assert(W_IS_DERIVED_FROM_STATIC(WWorldModule, ModuleType), "Not a valid module type");

  DeleteModule(WGetStaticRTTI<ModuleType>());
}

template <typename ModuleType>
W_ALWAYS_INLINE ModuleType* WWorld::GetModule()
{
  static_assert(W_IS_DERIVED_FROM_STATIC(WWorldModule, ModuleType), "Not a valid module type");

  return WStaticCast<ModuleType*>(GetModule(WGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
W_ALWAYS_INLINE const ModuleType* WWorld::GetModule() const
{
  static_assert(W_IS_DERIVED_FROM_STATIC(WWorldModule, ModuleType), "Not a valid module type");

  return WStaticCast<const ModuleType*>(GetModule(WGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
W_ALWAYS_INLINE const ModuleType* WWorld::GetModuleReadOnly() const
{
  return GetModule<ModuleType>();
}

template <typename ManagerType>
ManagerType* WWorld::GetOrCreateComponentManager()
{
  static_assert(W_IS_DERIVED_FROM_STATIC(WComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const WWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  m_Data.m_Modules.EnsureCount(uiTypeId + 1);

  ManagerType* pModule = static_cast<ManagerType*>(m_Data.m_Modules[uiTypeId]);
  if (pModule == nullptr)
  {
    pModule = W_NEW(&m_Data.m_Allocator, ManagerType, this);
    static_cast<WWorldModule*>(pModule)->Initialize();

    m_Data.m_Modules[uiTypeId] = pModule;
    m_Data.m_ModulesToStartSimulation.PushBack(pModule);
  }

  return pModule;
}

W_ALWAYS_INLINE WComponentManagerBase* WWorld::GetOrCreateManagerForComponentType(const WRTTI* pComponentRtti)
{
  W_ASSERT_DEV(pComponentRtti->IsDerivedFrom<WComponent>(), "Invalid component type '%s'", pComponentRtti->GetTypeName());

  return WStaticCast<WComponentManagerBase*>(GetOrCreateModule(pComponentRtti));
}

template <typename ManagerType>
void WWorld::DeleteComponentManager()
{
  static_assert(W_IS_DERIVED_FROM_STATIC(WComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const WWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (ManagerType* pModule = static_cast<ManagerType*>(m_Data.m_Modules[uiTypeId]))
    {
      m_Data.m_Modules[uiTypeId] = nullptr;

      static_cast<WWorldModule*>(pModule)->Deinitialize();
      DeregisterUpdateFunctionsInternal(pModule);
      W_DELETE(&m_Data.m_Allocator, pModule);
    }
  }
}

template <typename ManagerType>
W_FORCE_INLINE ManagerType* WWorld::GetComponentManager()
{
  static_assert(W_IS_DERIVED_FROM_STATIC(WComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const WWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return WStaticCast<ManagerType*>(m_Data.m_Modules[uiTypeId]);
  }

  return nullptr;
}

template <typename ManagerType>
W_FORCE_INLINE const ManagerType* WWorld::GetComponentManager() const
{
  static_assert(W_IS_DERIVED_FROM_STATIC(WComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForReadAccess();

  const WWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return WStaticCast<const ManagerType*>(m_Data.m_Modules[uiTypeId]);
  }

  return nullptr;
}

W_ALWAYS_INLINE WComponentManagerBase* WWorld::GetManagerForComponentType(const WRTTI* pComponentRtti)
{
  W_ASSERT_DEV(pComponentRtti->IsDerivedFrom<WComponent>(), "Invalid component type '{0}'", pComponentRtti->GetTypeName());

  return WStaticCast<WComponentManagerBase*>(GetModule(pComponentRtti));
}

W_ALWAYS_INLINE const WComponentManagerBase* WWorld::GetManagerForComponentType(const WRTTI* pComponentRtti) const
{
  W_ASSERT_DEV(pComponentRtti->IsDerivedFrom<WComponent>(), "Invalid component type '{0}'", pComponentRtti->GetTypeName());

  return WStaticCast<const WComponentManagerBase*>(GetModule(pComponentRtti));
}

inline bool WWorld::IsValidComponent(const WComponentHandle& hComponent) const
{
  CheckForReadAccess();
  const WWorldModuleTypeId uiTypeId = hComponent.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (const WWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      return static_cast<const WComponentManagerBase*>(pModule)->IsValidComponent(hComponent);
    }
  }

  return false;
}

template <typename ComponentType>
inline bool WWorld::TryGetComponent(const WComponentHandle& hComponent, ComponentType*& out_pComponent)
{
  CheckForWriteAccess();
  static_assert(W_IS_DERIVED_FROM_STATIC(WComponent, ComponentType), "Not a valid component type");

  const WWorldModuleTypeId uiTypeId = hComponent.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (WWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      WComponent* pComponent = nullptr;
      bool bResult = static_cast<WComponentManagerBase*>(pModule)->TryGetComponent(hComponent, pComponent);
      out_pComponent = WDynamicCast<ComponentType*>(pComponent);
      return bResult && out_pComponent != nullptr;
    }
  }

  return false;
}

template <typename ComponentType>
inline bool WWorld::TryGetComponent(const WComponentHandle& hComponent, const ComponentType*& out_pComponent) const
{
  CheckForReadAccess();
  static_assert(W_IS_DERIVED_FROM_STATIC(WComponent, ComponentType), "Not a valid component type");

  const WWorldModuleTypeId uiTypeId = hComponent.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (const WWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      const WComponent* pComponent = nullptr;
      bool bResult = static_cast<const WComponentManagerBase*>(pModule)->TryGetComponent(hComponent, pComponent);
      out_pComponent = WDynamicCast<const ComponentType*>(pComponent);
      return bResult && out_pComponent != nullptr;
    }
  }

  return false;
}

W_FORCE_INLINE void WWorld::SendMessage(const WGameObjectHandle& hReceiverObject, WMessage& ref_msg)
{
  CheckForWriteAccess();

  WGameObject* pReceiverObject = nullptr;
  if (TryGetObject(hReceiverObject, pReceiverObject))
  {
    pReceiverObject->SendMessage(ref_msg);
  }
  else
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (ref_msg.GetDebugMessageRouting())
    {
      WLog::Warning("WWorld::SendMessage: The receiver WGameObject for message of type '{0}' does not exist.", ref_msg.GetId());
    }
#endif
  }
}

W_FORCE_INLINE void WWorld::SendMessageRecursive(const WGameObjectHandle& hReceiverObject, WMessage& ref_msg)
{
  CheckForWriteAccess();

  WGameObject* pReceiverObject = nullptr;
  if (TryGetObject(hReceiverObject, pReceiverObject))
  {
    pReceiverObject->SendMessageRecursive(ref_msg);
  }
  else
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (ref_msg.GetDebugMessageRouting())
    {
      WLog::Warning("WWorld::SendMessageRecursive: The receiver WGameObject for message of type '{0}' does not exist.", ref_msg.GetId());
    }
#endif
  }
}

W_ALWAYS_INLINE void WWorld::PostMessage(const WGameObjectHandle& hReceiverObject, const WMessage& msg, WTime delay, WObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.
  PostMessage(hReceiverObject, msg, queueType, delay, false);
}

W_ALWAYS_INLINE void WWorld::PostMessageRecursive(const WGameObjectHandle& hReceiverObject, const WMessage& msg, WTime delay, WObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.
  PostMessage(hReceiverObject, msg, queueType, delay, true);
}

W_FORCE_INLINE void WWorld::SendMessage(const WComponentHandle& hReceiverComponent, WMessage& ref_msg)
{
  CheckForWriteAccess();

  WComponent* pReceiverComponent = nullptr;
  if (TryGetComponent(hReceiverComponent, pReceiverComponent))
  {
    pReceiverComponent->SendMessage(ref_msg);
  }
  else
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (ref_msg.GetDebugMessageRouting())
    {
      WLog::Warning("WWorld::SendMessage: The receiver WComponent for message of type '{0}' does not exist.", ref_msg.GetId());
    }
#endif
  }
}

W_ALWAYS_INLINE void WWorld::SetWorldSimulationEnabled(bool bEnable)
{
  m_Data.m_bSimulateWorld = bEnable;
}

W_ALWAYS_INLINE bool WWorld::GetWorldSimulationEnabled() const
{
  return m_Data.m_bSimulateWorld;
}

W_ALWAYS_INLINE const WSharedPtr<WTask>& WWorld::GetUpdateTask()
{
  return m_pUpdateTask;
}

W_ALWAYS_INLINE WUInt32 WWorld::GetUpdateCounter() const
{
  return m_Data.m_uiUpdateCounter;
}

W_FORCE_INLINE WSpatialSystem* WWorld::GetSpatialSystem()
{
  CheckForWriteAccess();

  return m_Data.m_pSpatialSystem.Borrow();
}

W_FORCE_INLINE const WSpatialSystem* WWorld::GetSpatialSystem() const
{
  CheckForReadAccess();

  return m_Data.m_pSpatialSystem.Borrow();
}

W_ALWAYS_INLINE void WWorld::GetCoordinateSystem(const WVec3& vGlobalPosition, WCoordinateSystem& out_coordinateSystem) const
{
  m_Data.m_pCoordinateSystemProvider->GetCoordinateSystem(vGlobalPosition, out_coordinateSystem);
}

W_ALWAYS_INLINE WCoordinateSystemProvider& WWorld::GetCoordinateSystemProvider()
{
  return *(m_Data.m_pCoordinateSystemProvider.Borrow());
}

W_ALWAYS_INLINE const WCoordinateSystemProvider& WWorld::GetCoordinateSystemProvider() const
{
  return *(m_Data.m_pCoordinateSystemProvider.Borrow());
}

W_ALWAYS_INLINE WClock& WWorld::GetClock()
{
  return m_Data.m_Clock;
}

W_ALWAYS_INLINE const WClock& WWorld::GetClock() const
{
  return m_Data.m_Clock;
}

W_ALWAYS_INLINE WRandom& WWorld::GetRandomNumberGenerator()
{
  return m_Data.m_Random;
}

W_ALWAYS_INLINE const WSharedPtr<WBlackboard>& WWorld::GetBlackboard()
{
  return m_Data.m_pBlackboard;
}

W_ALWAYS_INLINE WSharedPtr<const WBlackboard> WWorld::GetBlackboard() const
{
  return m_Data.m_pBlackboard;
}

W_ALWAYS_INLINE WAllocator* WWorld::GetAllocator()
{
  return &m_Data.m_Allocator;
}

W_ALWAYS_INLINE WInternal::WorldLargeBlockAllocator* WWorld::GetBlockAllocator()
{
  return &m_Data.m_BlockAllocator;
}

W_ALWAYS_INLINE WDoubleBufferedLinearAllocator* WWorld::GetStackAllocator()
{
  return &m_Data.m_LinearAllocator;
}

W_ALWAYS_INLINE WInternal::WorldData::ReadMarker& WWorld::GetReadMarker() const
{
  return m_Data.m_ReadMarker;
}

W_ALWAYS_INLINE WInternal::WorldData::WriteMarker& WWorld::GetWriteMarker()
{
  return m_Data.m_WriteMarker;
}

W_FORCE_INLINE void WWorld::SetUserData(void* pUserData)
{
  CheckForWriteAccess();

  m_Data.m_pUserData = pUserData;
}

W_FORCE_INLINE void* WWorld::GetUserData() const
{
  CheckForReadAccess();

  return m_Data.m_pUserData;
}

constexpr WUInt64 WWorld::GetMaxNumGameObjects()
{
  return WGameObjectId::MAX_INSTANCES - 2;
}

constexpr WUInt64 WWorld::GetMaxNumHierarchyLevels()
{
  return 1 << (sizeof(WGameObject::m_uiHierarchyLevel) * 8);
}

constexpr WUInt64 WWorld::GetMaxNumComponentsPerType()
{
  return WComponentId::MAX_INSTANCES - 1;
}

constexpr WUInt64 WWorld::GetMaxNumWorldModules()
{
  return W_MAX_WORLD_MODULE_TYPES;
}

constexpr WUInt64 WWorld::GetMaxNumComponentTypes()
{
  return W_MAX_COMPONENT_TYPES;
}

constexpr WUInt64 WWorld::GetMaxNumWorlds()
{
  return W_MAX_WORLDS;
}

// static
W_ALWAYS_INLINE WUInt32 WWorld::GetWorldCount()
{
  return s_Worlds.GetCount();
}

// static
W_ALWAYS_INLINE WWorld* WWorld::GetWorld(WUInt8 uiIndex)
{
  return s_Worlds.GetValueUnchecked(uiIndex);
}

// static
W_ALWAYS_INLINE WWorld* WWorld::GetWorld(const WWorldHandle& hWorld)
{
  WWorld* pWorld = nullptr;
  bool _ = s_Worlds.TryGetValue(hWorld.m_InternalId, pWorld);
  W_IGNORE_UNUSED(_);
  return pWorld;
}

// static
W_ALWAYS_INLINE WWorld* WWorld::GetWorld(const WGameObjectHandle& hObject)
{
  return GetWorld(hObject.GetInternalID().m_WorldIndex);
}

// static
W_ALWAYS_INLINE WWorld* WWorld::GetWorld(const WComponentHandle& hComponent)
{
  return GetWorld(hComponent.GetInternalID().m_WorldIndex);
}

W_ALWAYS_INLINE void WWorld::CheckForReadAccess() const
{
  W_ASSERT_DEV(m_Data.m_iReadCounter > 0, "Trying to read from World '{0}', but it is not marked for reading.", GetName());
}

W_ALWAYS_INLINE void WWorld::CheckForWriteAccess() const
{
  W_ASSERT_DEV(
    m_Data.m_WriteThreadID == WThreadUtils::GetCurrentThreadID(), "Trying to write to World '{0}', but it is not marked for writing.", GetName());
}

W_ALWAYS_INLINE WGameObject* WWorld::GetObjectUnchecked(WUInt32 uiIndex) const
{
  return m_Data.m_Objects.GetValueUnchecked(uiIndex);
}

W_ALWAYS_INLINE bool WWorld::ReportErrorWhenStaticObjectMoves() const
{
  return m_Data.m_bReportErrorWhenStaticObjectMoves;
}

W_ALWAYS_INLINE void WWorld::SetReportErrorWhenStaticObjectMoves(bool bReportError)
{
  m_Data.m_bReportErrorWhenStaticObjectMoves = bReportError;
}

W_ALWAYS_INLINE float WWorld::GetInvDeltaSeconds() const
{
  const float fDelta = (float)m_Data.m_Clock.GetTimeDiff().GetSeconds();
  if (fDelta > 0.0f)
  {
    return 1.0f / fDelta;
  }

  // when the clock is paused just use zero
  return 0.0f;
}
