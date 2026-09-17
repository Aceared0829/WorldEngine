

template <typename Object>
WRttiMappedObjectFactory<Object>::WRttiMappedObjectFactory() = default;

template <typename Object>
WRttiMappedObjectFactory<Object>::~WRttiMappedObjectFactory() = default;

template <typename Object>
void WRttiMappedObjectFactory<Object>::RegisterCreator(const WRTTI* pType, CreateObjectFunc creator)
{
  W_ASSERT_DEV(!m_Creators.Contains(pType), "Type already registered.");

  m_Creators.Insert(pType, creator);
  Event e;
  e.m_Type = Event::Type::CreatorAdded;
  e.m_pRttiType = pType;
  m_Events.Broadcast(e);
}

template <typename Object>
void WRttiMappedObjectFactory<Object>::UnregisterCreator(const WRTTI* pType)
{
  W_ASSERT_DEV(m_Creators.Contains(pType), "Type was never registered.");
  m_Creators.Remove(pType);

  Event e;
  e.m_Type = Event::Type::CreatorRemoved;
  e.m_pRttiType = pType;
  m_Events.Broadcast(e);
}

template <typename Object>
Object* WRttiMappedObjectFactory<Object>::CreateObject(const WRTTI* pType)
{
  CreateObjectFunc* creator = nullptr;
  while (pType != nullptr)
  {
    if (m_Creators.TryGetValue(pType, creator))
    {
      return (*creator)(pType);
    }
    pType = pType->GetParentType();
  }
  return nullptr;
}
