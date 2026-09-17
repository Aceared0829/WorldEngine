#pragma once

#include <Foundation/Math/Math.h>

// **** ListElement ****

template <typename T>
WListBase<T>::ListElementBase::ListElementBase()
  : m_pPrev(nullptr)
  , m_pNext(nullptr)
{
}

template <typename T>
WListBase<T>::ListElement::ListElement(const T& data)
  : m_Data(data)
{
}

// **** WListBase ****

template <typename T>
WListBase<T>::WListBase(WAllocator* pAllocator)
  : m_End(reinterpret_cast<ListElement*>(&m_Last))
  , m_uiCount(0)
  , m_Elements(pAllocator)
  , m_pFreeElementStack(nullptr)
{
  m_First.m_pNext = reinterpret_cast<ListElement*>(&m_Last);
  m_Last.m_pPrev = reinterpret_cast<ListElement*>(&m_First);
}

template <typename T>
WListBase<T>::WListBase(const WListBase<T>& cc, WAllocator* pAllocator)
  : m_End(reinterpret_cast<ListElement*>(&m_Last))
  , m_uiCount(0)
  , m_Elements(pAllocator)
  , m_pFreeElementStack(nullptr)
{
  m_First.m_pNext = reinterpret_cast<ListElement*>(&m_Last);
  m_Last.m_pPrev = reinterpret_cast<ListElement*>(&m_First);

  operator=(cc);
}

template <typename T>
WListBase<T>::~WListBase()
{
  Clear();
}

template <typename T>
void WListBase<T>::operator=(const WListBase<T>& cc)
{
  Clear();
  Insert(GetIterator(), cc.GetIterator(), cc.GetEndIterator());
}

template <typename T>
typename WListBase<T>::ListElement* WListBase<T>::AcquireNode()
{
  ListElement* pNode;

  if (m_pFreeElementStack == nullptr)
  {
    m_Elements.PushBack();
    pNode = &m_Elements.PeekBack();
  }
  else
  {
    pNode = m_pFreeElementStack;
    m_pFreeElementStack = m_pFreeElementStack->m_pNext;
  }

  WMemoryUtils::Construct<SkipTrivialTypes, ListElement>(pNode, 1);
  return pNode;
}

template <typename T>
void WListBase<T>::ReleaseNode(ListElement* pNode)
{
  WMemoryUtils::Destruct<ListElement>(pNode, 1);

  if (pNode == &m_Elements.PeekBack())
  {
    m_Elements.PopBack();
  }
  else if (pNode == &m_Elements.PeekFront())
  {
    m_Elements.PopFront();
  }
  else
  {
    pNode->m_pNext = m_pFreeElementStack;
    m_pFreeElementStack = pNode;
  }

  --m_uiCount;
}


template <typename T>
W_ALWAYS_INLINE typename WListBase<T>::Iterator WListBase<T>::GetIterator()
{
  return Iterator(m_First.m_pNext);
}

template <typename T>
W_ALWAYS_INLINE typename WListBase<T>::Iterator WListBase<T>::GetEndIterator()
{
  return m_End;
}

template <typename T>
W_ALWAYS_INLINE typename WListBase<T>::ConstIterator WListBase<T>::GetIterator() const
{
  return ConstIterator(m_First.m_pNext);
}

template <typename T>
W_ALWAYS_INLINE typename WListBase<T>::ConstIterator WListBase<T>::GetEndIterator() const
{
  return m_End;
}

template <typename T>
W_ALWAYS_INLINE WUInt32 WListBase<T>::GetCount() const
{
  return m_uiCount;
}

template <typename T>
W_ALWAYS_INLINE bool WListBase<T>::IsEmpty() const
{
  return (m_uiCount == 0);
}

template <typename T>
void WListBase<T>::Clear()
{
  if (!IsEmpty())
    Remove(GetIterator(), GetEndIterator());

  m_pFreeElementStack = nullptr;
  m_Elements.Clear();
}

template <typename T>
W_FORCE_INLINE void WListBase<T>::Compact()
{
  m_Elements.Compact();
}

template <typename T>
W_FORCE_INLINE T& WListBase<T>::PeekFront()
{
  W_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_First.m_pNext->m_Data;
}

template <typename T>
W_FORCE_INLINE T& WListBase<T>::PeekBack()
{
  W_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_Last.m_pPrev->m_Data;
}

template <typename T>
W_FORCE_INLINE const T& WListBase<T>::PeekFront() const
{
  W_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_First.m_pNext->m_Data;
}

template <typename T>
W_FORCE_INLINE const T& WListBase<T>::PeekBack() const
{
  W_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_Last.m_pPrev->m_Data;
}


template <typename T>
W_ALWAYS_INLINE T& WListBase<T>::PushBack()
{
  return *Insert(GetEndIterator());
}

template <typename T>
W_ALWAYS_INLINE void WListBase<T>::PushBack(const T& element)
{
  Insert(GetEndIterator(), element);
}

template <typename T>
W_ALWAYS_INLINE T& WListBase<T>::PushFront()
{
  return *Insert(GetIterator());
}

template <typename T>
W_ALWAYS_INLINE void WListBase<T>::PushFront(const T& element)
{
  Insert(GetIterator(), element);
}

template <typename T>
W_FORCE_INLINE void WListBase<T>::PopBack()
{
  W_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  Remove(Iterator(m_Last.m_pPrev));
}

template <typename T>
void WListBase<T>::PopFront()
{
  W_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  Remove(Iterator(m_First.m_pNext));
}

template <typename T>
typename WListBase<T>::Iterator WListBase<T>::Insert(const Iterator& pos)
{
  W_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");

  ++m_uiCount;
  ListElement* elem = AcquireNode();

  elem->m_pNext = pos.m_pElement;
  elem->m_pPrev = pos.m_pElement->m_pPrev;

  pos.m_pElement->m_pPrev->m_pNext = elem;
  pos.m_pElement->m_pPrev = elem;

  return Iterator(elem);
}

template <typename T>
typename WListBase<T>::Iterator WListBase<T>::Insert(const Iterator& pos, const T& data)
{
  W_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");

  ++m_uiCount;
  ListElement* elem = AcquireNode();
  elem->m_Data = data;

  elem->m_pNext = pos.m_pElement;
  elem->m_pPrev = pos.m_pElement->m_pPrev;

  pos.m_pElement->m_pPrev->m_pNext = elem;
  pos.m_pElement->m_pPrev = elem;

  return Iterator(elem);
}

template <typename T>
void WListBase<T>::Insert(const Iterator& pos, ConstIterator first, const ConstIterator& last)
{
  W_ASSERT_DEV(pos.m_pElement != nullptr && first.m_pElement != nullptr && last.m_pElement != nullptr, "One of the iterators is invalid.");

  while (first != last)
  {
    Insert(pos, *first);
    ++first;
  }
}

template <typename T>
typename WListBase<T>::Iterator WListBase<T>::Remove(const Iterator& pos)
{
  W_ASSERT_DEV(!IsEmpty(), "The container is empty.");
  W_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");

  ListElement* pPrev = pos.m_pElement->m_pPrev;
  ListElement* pNext = pos.m_pElement->m_pNext;

  pPrev->m_pNext = pNext;
  pNext->m_pPrev = pPrev;

  ReleaseNode(pos.m_pElement);

  return Iterator(pNext);
}

template <typename T>
typename WListBase<T>::Iterator WListBase<T>::Remove(Iterator first, const Iterator& last)
{
  W_ASSERT_DEV(!IsEmpty(), "The container is empty.");
  W_ASSERT_DEV(first.m_pElement != nullptr && last.m_pElement != nullptr, "An iterator is invalid.");

  while (first != last)
    first = Remove(first);

  return last;
}

/*! If uiNewSize is smaller than the size of the list, elements are popped from the back, until the desired size is reached.
    If uiNewSize is larger than the size of the list, default-constructed elements are appended to the list, until the desired size is reached.
*/
template <typename T>
void WListBase<T>::SetCount(WUInt32 uiNewSize)
{
  while (m_uiCount > uiNewSize)
    PopBack();

  while (m_uiCount < uiNewSize)
    PushBack();
}

template <typename T>
bool WListBase<T>::operator==(const WListBase<T>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  auto itLhs = GetIterator();
  auto itRhs = rhs.GetIterator();

  while (itLhs.IsValid())
  {
    if (*itLhs != *itRhs)
      return false;

    ++itLhs;
    ++itRhs;
  }

  return true;
}

template <typename T, typename A>
WList<T, A>::WList()
  : WListBase<T>(A::GetAllocator())
{
}

template <typename T, typename A>
WList<T, A>::WList(WAllocator* pAllocator)
  : WListBase<T>(pAllocator)
{
}

template <typename T, typename A>
WList<T, A>::WList(const WList<T, A>& other)
  : WListBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
WList<T, A>::WList(const WListBase<T>& other)
  : WListBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
void WList<T, A>::operator=(const WList<T, A>& rhs)
{
  WListBase<T>::operator=(rhs);
}

template <typename T, typename A>
void WList<T, A>::operator=(const WListBase<T>& rhs)
{
  WListBase<T>::operator=(rhs);
}
