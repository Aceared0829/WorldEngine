
template <typename MetaDataType>
WMessageQueueBase<MetaDataType>::WMessageQueueBase(WAllocator* pAllocator)
  : m_Queue(pAllocator)
{
}

template <typename MetaDataType>
WMessageQueueBase<MetaDataType>::WMessageQueueBase(const WMessageQueueBase& rhs, WAllocator* pAllocator)
  : m_Queue(pAllocator)
{
  m_Queue = rhs.m_Queue;
}

template <typename MetaDataType>
WMessageQueueBase<MetaDataType>::~WMessageQueueBase()
{
  Clear();
}

template <typename MetaDataType>
void WMessageQueueBase<MetaDataType>::operator=(const WMessageQueueBase& rhs)
{
  m_Queue = rhs.m_Queue;
}

template <typename MetaDataType>
W_ALWAYS_INLINE typename WMessageQueueBase<MetaDataType>::Entry& WMessageQueueBase<MetaDataType>::operator[](WUInt32 uiIndex)
{
  return m_Queue[uiIndex];
}

template <typename MetaDataType>
W_ALWAYS_INLINE const typename WMessageQueueBase<MetaDataType>::Entry& WMessageQueueBase<MetaDataType>::operator[](WUInt32 uiIndex) const
{
  return m_Queue[uiIndex];
}

template <typename MetaDataType>
W_ALWAYS_INLINE WUInt32 WMessageQueueBase<MetaDataType>::GetCount() const
{
  return m_Queue.GetCount();
}

template <typename MetaDataType>
W_ALWAYS_INLINE bool WMessageQueueBase<MetaDataType>::IsEmpty() const
{
  return m_Queue.IsEmpty();
}

template <typename MetaDataType>
void WMessageQueueBase<MetaDataType>::Clear()
{
  m_Queue.Clear();
}

template <typename MetaDataType>
W_ALWAYS_INLINE void WMessageQueueBase<MetaDataType>::Reserve(WUInt32 uiCount)
{
  m_Queue.Reserve(uiCount);
}

template <typename MetaDataType>
W_ALWAYS_INLINE void WMessageQueueBase<MetaDataType>::Compact()
{
  m_Queue.Compact();
}

template <typename MetaDataType>
void WMessageQueueBase<MetaDataType>::Enqueue(WMessage* pMessage, const MetaDataType& metaData)
{
  Entry entry;
  entry.m_pMessage = pMessage;
  entry.m_MetaData = metaData;

  {
    W_LOCK(m_Mutex);

    m_Queue.PushBack(entry);
  }
}

template <typename MetaDataType>
bool WMessageQueueBase<MetaDataType>::TryDequeue(WMessage*& out_pMessage, MetaDataType& out_metaData)
{
  W_LOCK(m_Mutex);

  if (!m_Queue.IsEmpty())
  {
    Entry& entry = m_Queue.PeekFront();
    out_pMessage = entry.m_pMessage;
    out_metaData = entry.m_MetaData;

    m_Queue.PopFront();
    return true;
  }

  return false;
}

template <typename MetaDataType>
bool WMessageQueueBase<MetaDataType>::TryPeek(WMessage*& out_pMessage, MetaDataType& out_metaData)
{
  W_LOCK(m_Mutex);

  if (!m_Queue.IsEmpty())
  {
    Entry& entry = m_Queue.PeekFront();
    out_pMessage = entry.m_pMessage;
    out_metaData = entry.m_MetaData;

    return true;
  }

  return false;
}

template <typename MetaDataType>
W_ALWAYS_INLINE typename WMessageQueueBase<MetaDataType>::Entry& WMessageQueueBase<MetaDataType>::Peek()
{
  return m_Queue.PeekFront();
}

template <typename MetaDataType>
W_ALWAYS_INLINE void WMessageQueueBase<MetaDataType>::Dequeue()
{
  m_Queue.PopFront();
}

template <typename MetaDataType>
template <typename Comparer>
W_ALWAYS_INLINE void WMessageQueueBase<MetaDataType>::Sort(const Comparer& comparer)
{
  m_Queue.Sort(comparer);
}

template <typename MetaDataType>
void WMessageQueueBase<MetaDataType>::Lock()
{
  m_Mutex.Lock();
}

template <typename MetaDataType>
void WMessageQueueBase<MetaDataType>::Unlock()
{
  m_Mutex.Unlock();
}


template <typename MD, typename A>
WMessageQueue<MD, A>::WMessageQueue()
  : WMessageQueueBase<MD>(A::GetAllocator())
{
}

template <typename MD, typename A>
WMessageQueue<MD, A>::WMessageQueue(WAllocator* pQueueAllocator)
  : WMessageQueueBase<MD>(pQueueAllocator)
{
}

template <typename MD, typename A>
WMessageQueue<MD, A>::WMessageQueue(const WMessageQueue<MD, A>& rhs)
  : WMessageQueueBase<MD>(rhs, A::GetAllocator())
{
}

template <typename MD, typename A>
WMessageQueue<MD, A>::WMessageQueue(const WMessageQueueBase<MD>& rhs)
  : WMessageQueueBase<MD>(rhs, A::GetAllocator())
{
}

template <typename MD, typename A>
void WMessageQueue<MD, A>::operator=(const WMessageQueue<MD, A>& rhs)
{
  WMessageQueueBase<MD>::operator=(rhs);
}

template <typename MD, typename A>
void WMessageQueue<MD, A>::operator=(const WMessageQueueBase<MD>& rhs)
{
  WMessageQueueBase<MD>::operator=(rhs);
}
