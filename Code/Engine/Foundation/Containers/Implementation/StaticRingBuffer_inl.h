#pragma once

template <typename T, WUInt32 C>
WStaticRingBuffer<T, C>::WStaticRingBuffer()
{
  m_pElements = GetStaticArray();
  m_uiFirstElement = 0;
  m_uiCount = 0;
}

template <typename T, WUInt32 C>
WStaticRingBuffer<T, C>::WStaticRingBuffer(const WStaticRingBuffer<T, C>& rhs)
{
  m_pElements = GetStaticArray();
  m_uiFirstElement = 0;
  m_uiCount = 0;

  *this = rhs;
}

template <typename T, WUInt32 C>
WStaticRingBuffer<T, C>::~WStaticRingBuffer()
{
  Clear();
}

template <typename T, WUInt32 C>
void WStaticRingBuffer<T, C>::operator=(const WStaticRingBuffer<T, C>& rhs)
{
  Clear();

  for (WUInt32 i = 0; i < rhs.GetCount(); ++i)
    PushBack(rhs[i]);
}

template <typename T, WUInt32 C>
bool WStaticRingBuffer<T, C>::operator==(const WStaticRingBuffer<T, C>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  for (WUInt32 i = 0; i < m_uiCount; ++i)
  {
    if ((*this)[i] != rhs[i])
      return false;
  }

  return true;
}

template <typename T, WUInt32 C>
void WStaticRingBuffer<T, C>::PushBack(const T& element)
{
  W_ASSERT_DEV(CanAppend(), "The ring-buffer is full, no elements can be appended before removing one.");

  const WUInt32 uiLastElement = (m_uiFirstElement + m_uiCount) % C;

  WMemoryUtils::CopyConstruct(&m_pElements[uiLastElement], element, 1);
  ++m_uiCount;
}

template <typename T, WUInt32 C>
void WStaticRingBuffer<T, C>::PushBack(T&& element)
{
  W_ASSERT_DEV(CanAppend(), "The ring-buffer is full, no elements can be appended before removing one.");

  const WUInt32 uiLastElement = (m_uiFirstElement + m_uiCount) % C;

  WMemoryUtils::MoveConstruct(&m_pElements[uiLastElement], std::move(element));
  ++m_uiCount;
}

template <typename T, WUInt32 C>
T& WStaticRingBuffer<T, C>::PeekBack()
{
  W_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the last element.");

  const WUInt32 uiLastElement = (m_uiFirstElement + m_uiCount - 1) % C;
  return m_pElements[uiLastElement];
}

template <typename T, WUInt32 C>
const T& WStaticRingBuffer<T, C>::PeekBack() const
{
  W_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the last element.");

  const WUInt32 uiLastElement = (m_uiFirstElement + m_uiCount - 1) % C;
  return m_pElements[uiLastElement];
}

template <typename T, WUInt32 C>
void WStaticRingBuffer<T, C>::PopFront(WUInt32 uiElements)
{
  W_ASSERT_DEV(m_uiCount >= uiElements, "The ring-buffer contains {0} elements, cannot remove {1} elements from it.", m_uiCount, uiElements);

  while (uiElements > 0)
  {
    WMemoryUtils::Destruct(&m_pElements[m_uiFirstElement], 1);
    ++m_uiFirstElement;
    m_uiFirstElement %= C;
    --m_uiCount;

    --uiElements;
  }
}

template <typename T, WUInt32 C>
W_FORCE_INLINE const T& WStaticRingBuffer<T, C>::PeekFront() const
{
  W_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the first element.");

  return m_pElements[m_uiFirstElement];
}

template <typename T, WUInt32 C>
W_FORCE_INLINE T& WStaticRingBuffer<T, C>::PeekFront()
{
  W_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the first element.");

  return m_pElements[m_uiFirstElement];
}

template <typename T, WUInt32 C>
W_FORCE_INLINE const T& WStaticRingBuffer<T, C>::operator[](WUInt32 uiIndex) const
{
  W_ASSERT_DEBUG(uiIndex < m_uiCount, "The ring-buffer only has {0} elements, cannot access element {1}.", m_uiCount, uiIndex);

  return m_pElements[(m_uiFirstElement + uiIndex) % C];
}

template <typename T, WUInt32 C>
W_FORCE_INLINE T& WStaticRingBuffer<T, C>::operator[](WUInt32 uiIndex)
{
  W_ASSERT_DEBUG(uiIndex < m_uiCount, "The ring-buffer only has {0} elements, cannot access element {1}.", m_uiCount, uiIndex);

  return m_pElements[(m_uiFirstElement + uiIndex) % C];
}

template <typename T, WUInt32 C>
W_ALWAYS_INLINE WUInt32 WStaticRingBuffer<T, C>::GetCount() const
{
  return m_uiCount;
}

template <typename T, WUInt32 C>
W_ALWAYS_INLINE bool WStaticRingBuffer<T, C>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, WUInt32 C>
W_ALWAYS_INLINE bool WStaticRingBuffer<T, C>::CanAppend(WUInt32 uiElements)
{
  return (m_uiCount + uiElements) <= C;
}

template <typename T, WUInt32 C>
void WStaticRingBuffer<T, C>::Clear()
{
  while (!IsEmpty())
    PopFront();
}

template <typename T, WUInt32 C>
W_ALWAYS_INLINE T* WStaticRingBuffer<T, C>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_Data);
}
