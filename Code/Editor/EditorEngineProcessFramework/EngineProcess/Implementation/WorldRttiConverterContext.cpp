#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>

void WWorldRttiConverterContext::Clear()
{
  WRttiConverterContext::Clear();

  m_pWorld = nullptr;
  m_GameObjectMap.Clear();
  m_ComponentMap.Clear();

  m_OtherPickingMap.Clear();
  m_ComponentPickingMap.Clear();

  m_UnknownTypes.Clear();
}

void WWorldRttiConverterContext::DeleteExistingObjects()
{
  if (m_pWorld == nullptr)
    return;

  m_UnknownTypes.Clear();

  W_LOCK(m_pWorld->GetWriteMarker());

  const auto& map = m_GameObjectMap.GetHandleToGuidMap();
  while (!map.IsEmpty())
  {
    auto it = map.GetIterator();

    WGameObject* pGameObject = nullptr;
    if (m_pWorld->TryGetObject(it.Key(), pGameObject))
    {
      DeleteObject(it.Value());
    }
    else
    {
      m_GameObjectMap.UnregisterObject(it.Value());
    }
  }

  // call base class clear, not the overridden one
  WRttiConverterContext::Clear();

  m_GameObjectMap.Clear();
  m_ComponentMap.Clear();
  m_ComponentPickingMap.Clear();
  // Need to do this to make sure all deleted objects are actually deleted as singleton components are
  // still considered alive until Update actually deletes them.
  const bool bSim = m_pWorld->GetWorldSimulationEnabled();
  m_pWorld->SetWorldSimulationEnabled(false);
  m_pWorld->Update();
  m_pWorld->SetWorldSimulationEnabled(bSim);
  // m_OtherPickingMap.Clear(); // do not clear this
}

WInternal::NewInstance<void> WWorldRttiConverterContext::CreateObject(const WUuid& guid, const WRTTI* pRtti)
{
  W_ASSERT_DEBUG(pRtti != nullptr, "Object type is unknown");

  if (pRtti == WGetStaticRTTI<WGameObject>())
  {
    WStringBuilder tmp;

    WGameObjectDesc d;
    d.m_sName.Assign(WConversionUtils::ToString(guid, tmp).GetData());
    d.m_uiStableRandomSeed = WHashingUtils::xxHash32(tmp.GetData(), tmp.GetElementCount());

    WGameObjectHandle hObject = m_pWorld->CreateObject(d);
    WGameObject* pObject;
    if (m_pWorld->TryGetObject(hObject, pObject))
    {
      RegisterObject(guid, pRtti, pObject);

      Event e;
      e.m_Type = Event::Type::GameObjectCreated;
      e.m_ObjectGuid = guid;
      m_Events.Broadcast(e);

      return {pObject, nullptr};
    }
    else
    {
      WLog::Error("Failed to create WGameObject!");
      return nullptr;
    }
  }
  else if (pRtti->IsDerivedFrom<WComponent>())
  {
    WComponentManagerBase* pMan = m_pWorld->GetOrCreateManagerForComponentType(pRtti);
    if (pMan == nullptr)
    {
      WLog::Error("Component of type '{0}' cannot be created, no component manager is registered", pRtti->GetTypeName());
      return nullptr;
    }

    // Component is added via reflection shortly so passing a nullptr as owner is fine here.
    WComponentHandle hComponent = pMan->CreateComponent(nullptr);
    WComponent* pComponent;
    if (pMan->TryGetComponent(hComponent, pComponent))
    {
      RegisterObject(guid, pRtti, pComponent);
      return {pComponent, nullptr};
    }
    else
    {
      WLog::Error("Component of type '{0}' cannot be found after creation", pRtti->GetTypeName());
      return nullptr;
    }
  }
  else
  {
    return WRttiConverterContext::CreateObject(guid, pRtti);
  }
}

void WWorldRttiConverterContext::DeleteObject(const WUuid& guid)
{
  WRttiConverterObject object = GetObjectByGUID(guid);

  // this can happen when manipulating scenes during simulation
  // and when creating two components of a type that acts like a singleton (and therefore ignores the second instance creation)
  if (object.m_pObject == nullptr)
    return;

  const WRTTI* pRtti = object.m_pType;
  W_ASSERT_DEBUG(pRtti != nullptr, "Object does not exist!");

  if (pRtti == WGetStaticRTTI<WGameObject>())
  {
    auto hObject = m_GameObjectMap.GetHandle(guid);
    UnregisterObject(guid);
    m_pWorld->DeleteObjectNow(hObject, false);

    Event e;
    e.m_Type = Event::Type::GameObjectDeleted;
    e.m_ObjectGuid = guid;
    m_Events.Broadcast(e);
  }
  else if (pRtti->IsDerivedFrom<WComponent>())
  {
    WComponentHandle hComponent = m_ComponentMap.GetHandle(guid);
    WComponentManagerBase* pMan = m_pWorld->GetOrCreateManagerForComponentType(pRtti);
    if (pMan == nullptr)
    {
      WLog::Error("Component of type '{0}' cannot be created, no component manager is registered", pRtti->GetTypeName());
      return;
    }

    UnregisterObject(guid);
    pMan->DeleteComponent(hComponent);
  }
  else
  {
    WRttiConverterContext::DeleteObject(guid);
  }
}

void WWorldRttiConverterContext::RegisterObject(const WUuid& guid, const WRTTI* pRtti, void* pObject)
{
  if (pRtti == WGetStaticRTTI<WGameObject>())
  {
    WGameObject* pGameObject = static_cast<WGameObject*>(pObject);
    m_GameObjectMap.RegisterObject(guid, pGameObject->GetHandle());
  }
  else if (pRtti->IsDerivedFrom<WComponent>())
  {
    WComponent* pComponent = static_cast<WComponent*>(pObject);

    W_ASSERT_DEV(m_pWorld != nullptr && pComponent->GetWorld() == m_pWorld, "Invalid object to register");

    m_ComponentMap.RegisterObject(guid, pComponent->GetHandle());
    pComponent->SetUniqueID(m_uiNextComponentPickingID++);
    m_ComponentPickingMap.RegisterObject(guid, pComponent->GetUniqueID());
  }

  WRttiConverterContext::RegisterObject(guid, pRtti, pObject);
}

void WWorldRttiConverterContext::UnregisterObject(const WUuid& guid)
{
  WRttiConverterObject object = GetObjectByGUID(guid);

  // this can happen when running a game simulation and the object is destroyed by the game code
  // W_ASSERT_DEBUG(object.m_pObject, "Failed to retrieve object by guid!");

  if (object.m_pType != nullptr)
  {
    const WRTTI* pRtti = object.m_pType;
    if (pRtti == WGetStaticRTTI<WGameObject>())
    {
      m_GameObjectMap.UnregisterObject(guid);
    }
    else if (pRtti->IsDerivedFrom<WComponent>())
    {
      m_ComponentMap.UnregisterObject(guid);
      m_ComponentPickingMap.UnregisterObject(guid);
    }
  }

  WRttiConverterContext::UnregisterObject(guid);
}

WRttiConverterObject WWorldRttiConverterContext::GetObjectByGUID(const WUuid& guid) const
{
  WRttiConverterObject object = WRttiConverterContext::GetObjectByGUID(guid);

  if (!guid.IsValid() || object.m_pType == nullptr)
    return object;

  // We can't look up the ptr via the base class map as it keeps changing, we we need to use the handle.
  if (object.m_pType == WGetStaticRTTI<WGameObject>())
  {
    auto hObject = m_GameObjectMap.GetHandle(guid);
    WGameObject* pGameObject = nullptr;
    if (!m_pWorld->TryGetObject(hObject, pGameObject))
    {
      object.m_pObject = nullptr;
      object.m_pType = nullptr;
      // this can happen when one manipulates a running scene, and an object just deleted itself
      // W_REPORT_FAILURE("Can't resolve game object GUID!");
      return object;
    }

    // Update new ptr of game object
    if (object.m_pObject != pGameObject)
    {
      m_ObjectToGuid.Remove(object.m_pObject);
      object.m_pObject = pGameObject;
      m_ObjectToGuid.Insert(object.m_pObject, guid);
    }
  }
  else if (object.m_pType->IsDerivedFrom<WComponent>())
  {
    auto hComponent = m_ComponentMap.GetHandle(guid);
    WComponent* pComponent = nullptr;
    if (!m_pWorld->TryGetComponent(hComponent, pComponent))
    {
      object.m_pObject = nullptr;
      object.m_pType = nullptr;
      // this can happen when one manipulates a running scene, and an object just deleted itself
      // W_REPORT_FAILURE("Can't resolve component GUID!");
      return object;
    }

    // Update new ptr of component
    if (object.m_pObject != pComponent)
    {
      m_ObjectToGuid.Remove(object.m_pObject);
      object.m_pObject = pComponent;
      m_ObjectToGuid.Insert(object.m_pObject, guid);
    }
  }
  return object;
}

WUuid WWorldRttiConverterContext::GetObjectGUID(const WRTTI* pRtti, const void* pObject) const
{
  if (pRtti == WGetStaticRTTI<WGameObject>())
  {
    const WGameObject* pGameObject = static_cast<const WGameObject*>(pObject);
    return m_GameObjectMap.GetGuid(pGameObject->GetHandle());
  }
  else if (pRtti->IsDerivedFrom<WComponent>())
  {
    const WComponent* pComponent = static_cast<const WComponent*>(pObject);
    return m_ComponentMap.GetGuid(pComponent->GetHandle());
  }
  return WRttiConverterContext::GetObjectGUID(pRtti, pObject);
}

void WWorldRttiConverterContext::OnUnknownTypeError(WStringView sTypeName)
{
  WRttiConverterContext::OnUnknownTypeError(sTypeName);

  m_UnknownTypes.Insert(sTypeName);
}
