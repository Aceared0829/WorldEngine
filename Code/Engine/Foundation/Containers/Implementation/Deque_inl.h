#pragma once

#include <Foundation/Math/Math.h>

#define REDUCE_SIZE(iReduction)     \
  m_iReduceSizeTimer -= iReduction; \
  if (m_iReduceSizeTimer <= 0)      \
    ReduceSize(0);

#define RESERVE(uiCount)                                         \
  if (uiCount > m_uiCount)                                       \
  {                                                              \
    m_uiMaxCount = WMath::Max(m_uiMaxCount, uiCount);           \
    if ((m_uiFirstElement <= 0) || (GetCurMaxCount() < uiCount)) \
      Reserve(uiCount);                                          \
  }

#define CHUNK_SIZE(Type) (4096 / sizeof(Type) < 32 ? 32 : 4096 / sizeof(Type))
//(sizeof(Type) <= 8 ? 256 : (sizeof(Type) <= 16 ? 128 : (sizeof(Type) <= 32 ? 64 : 32))) // although this is Pow(2), this is slower than just having
// larger chunks

template <typename T, bool Construct>
void WDequeBase<T, Construct>::Constructor(WAllocator* pAllocator)
{
  m_pAllocator = pAllocator;
  m_pChunks = nullptr;
  m_uiChunks = 0;
  m_uiFirstElement = 0;
  m_uiCount = 0;
  m_uiAllocatedChunks = 0;
  m_uiMaxCount = 0;

  ResetReduceSizeCounter();

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  m_uiChunkSize = CHUNK_SIZE(T);
#endif
}

template <typename T, bool Construct>
WDequeBase<T, Construct>::WDequeBase(WAllocator* pAllocator)
{
  Constructor(pAllocator);
}

template <typename T, bool Construct>
WDequeBase<T, Construct>::WDequeBase(const WDequeBase<T, Construct>& rhs, WAllocator* pAllocator)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  Constructor(pAllocator);

  *this = rhs;
}

template <typename T, bool Construct>
WDequeBase<T, Construct>::WDequeBase(WDequeBase<T, Construct>&& rhs, WAllocator* pAllocator)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  Constructor(pAllocator);

  *this = std::move(rhs);
}

template <typename T, bool Construct>
WDequeBase<T, Construct>::~WDequeBase()
{
  DeallocateAll();
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::operator=(const WDequeBase<T, Construct>& rhs)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  Clear();                // does not deallocate anything
  RESERVE(rhs.m_uiCount); // allocates data, if required
  m_uiCount = rhs.m_uiCount;

  // copy construct all the elements
  for (WUInt32 i = 0; i < rhs.m_uiCount; ++i)
    WMemoryUtils::CopyConstruct(&ElementAt(i), rhs[i], 1);
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::operator=(WDequeBase<T, Construct>&& rhs)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  if (m_pAllocator != rhs.m_pAllocator)
    operator=(static_cast<WDequeBase<T, Construct>&>(rhs));
  else
  {
    DeallocateAll();

    m_uiCount = rhs.m_uiCount;
    m_iReduceSizeTimer = rhs.m_iReduceSizeTimer;
    m_pChunks = rhs.m_pChunks;
    m_uiAllocatedChunks = rhs.m_uiAllocatedChunks;
    m_uiChunks = rhs.m_uiChunks;
    m_uiFirstElement = rhs.m_uiFirstElement;
    m_uiMaxCount = rhs.m_uiMaxCount;

    rhs.m_uiCount = 0;
    rhs.m_pChunks = nullptr;
    rhs.m_uiAllocatedChunks = 0;
    rhs.m_uiChunks = 0;
    rhs.m_uiFirstElement = 0;
    rhs.m_uiMaxCount = 0;
  }
}

template <typename T, bool Construct>
bool WDequeBase<T, Construct>::operator==(const WDequeBase<T, Construct>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  for (WUInt32 i = 0; i < GetCount(); ++i)
  {
    if ((*this)[i] != rhs[i])
      return false;
  }

  return true;
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::Clear()
{
  if (Construct)
  {
    for (WUInt32 i = 0; i < m_uiCount; ++i)
      WMemoryUtils::Destruct<T>(&operator[](i), 1);
  }

  m_uiCount = 0;

  // since it is much more likely that data is appended at the back of the deque,
  // we do not use the center of the chunk index array, but instead set the first element
  // somewhere more at the front

  // set the first element to a position that allows to add elements at the front
  if (m_uiChunks > 30)
    m_uiFirstElement = CHUNK_SIZE(T) * 16;
  else if (m_uiChunks > 8)
    m_uiFirstElement = CHUNK_SIZE(T) * 4;
  else if (m_uiChunks > 1)
    m_uiFirstElement = CHUNK_SIZE(T) * 1;
  else if (m_uiChunks > 0)
    m_uiFirstElement = 1; // with the current implementation this case should not be possible.
  else
    m_uiFirstElement = 0; // must also work, if Clear is called on a deallocated (not yet allocated) deque
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::Reserve(WUInt32 uiCount)
{
  // This is the function where all the complicated stuff happens.
  // The basic idea is as follows:
  // * do not do anything unless necessary
  // * if the index array (for the redirection) is already large enough to handle the 'address space', try to reuse it
  //   by moving data around (shift it left or right), if necessary
  // * if the chunk index array is not large enough to handle the required amount of redirections, allocate a new
  //   index array and move the old data over
  // This function does not allocate any of the chunks itself (that's what 'ElementAt' does), it only takes care
  // that the amount of reserved elements can be redirected once the deque is enlarged accordingly.

  // no need to change anything in this case
  if (uiCount <= m_uiCount)
    return;

  // keeps track of the largest amount of used elements since the last memory reduction
  m_uiMaxCount = WMath::Max(m_uiMaxCount, uiCount);

  // if there is enough room to hold all requested elements AND one can prepend at least one element (PushFront)
  // do not reallocate
  if ((m_uiFirstElement > 0) && (GetCurMaxCount() >= uiCount))
    return;

  const WUInt32 uiCurFirstChunk = GetFirstUsedChunk();
  const WUInt32 uiRequiredChunks = GetRequiredChunks(uiCount);

  // if we already have enough chunks, just rearrange them
  if (m_uiChunks > uiRequiredChunks + 1) // have at least one spare chunk for the front, and one for the back
  {
    const WUInt32 uiSpareChunks = m_uiChunks - uiRequiredChunks;
    const WUInt32 uiSpareChunksStart = uiSpareChunks / 2;

    W_ASSERT_DEBUG(uiSpareChunksStart > 0, "Implementation error.");

    // always leave one spare chunk at the front, to ensure that one can prepend elements

    W_ASSERT_DEBUG(uiSpareChunksStart != uiCurFirstChunk, "No rearrangement possible.");

    // if the new first active chunk is to the left
    if (uiSpareChunksStart < uiCurFirstChunk)
      MoveIndexChunksLeft(uiCurFirstChunk - uiSpareChunksStart);
    else
      MoveIndexChunksRight(uiSpareChunksStart - uiCurFirstChunk);

    W_ASSERT_DEBUG(m_uiFirstElement > 0, "Did not achieve the desired effect.");
    W_ASSERT_DEBUG(GetCurMaxCount() >= uiCount, "Did not achieve the desired effect ({0} >= {1}).", GetCurMaxCount(), uiCount);
  }
  else
  {
    const WUInt32 uiReallocSize = 16 + uiRequiredChunks + 16;

    T** pNewChunksArray = W_NEW_RAW_BUFFER(m_pAllocator, T*, uiReallocSize);
    WMemoryUtils::ZeroFill(pNewChunksArray, uiReallocSize);

    const WUInt32 uiFirstUsedChunk = m_uiFirstElement / CHUNK_SIZE(T);

    // move all old chunks over
    WUInt32 pos = 16;

    // first the used chunks at the start of the new array
    for (WUInt32 i = 0; i < m_uiChunks - uiFirstUsedChunk; ++i)
    {
      pNewChunksArray[pos] = m_pChunks[uiFirstUsedChunk + i];
      ++pos;
    }

    m_uiFirstElement -= uiFirstUsedChunk * CHUNK_SIZE(T);

    // then the unused chunks at the end of the new array
    for (WUInt32 i = 0; i < uiFirstUsedChunk; ++i)
    {
      pNewChunksArray[pos] = m_pChunks[i];
      ++pos;
    }

    m_uiFirstElement += 16 * CHUNK_SIZE(T);

    W_ASSERT_DEBUG(m_uiFirstElement == (16 * CHUNK_SIZE(T)) + (m_uiFirstElement % CHUNK_SIZE(T)), "");


    W_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks);
    m_pChunks = pNewChunksArray;
    m_uiChunks = uiReallocSize;
  }
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::Compact()
{
  ResetReduceSizeCounter();

  if (IsEmpty())
  {
    DeallocateAll();
    return;
  }

  // this will deallocate ALL unused chunks
  DeallocateUnusedChunks(GetRequiredChunks(m_uiCount));

  // reduces the size of the index array, but keeps some spare pointers, so that scaling up is still possible without reallocation
  CompactIndexArray(0);
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::Swap(WDequeBase<T, Construct>& other)
{
  WMath::Swap(this->m_pAllocator, other.m_pAllocator);
  WMath::Swap(this->m_pChunks, other.m_pChunks);
  WMath::Swap(this->m_uiChunks, other.m_uiChunks);
  WMath::Swap(this->m_uiFirstElement, other.m_uiFirstElement);
  WMath::Swap(this->m_uiCount, other.m_uiCount);
  WMath::Swap(this->m_uiAllocatedChunks, other.m_uiAllocatedChunks);
  WMath::Swap(this->m_iReduceSizeTimer, other.m_iReduceSizeTimer);
  WMath::Swap(this->m_uiMaxCount, other.m_uiMaxCount);
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::CompactIndexArray(WUInt32 uiMinChunksToKeep)
{
  const WUInt32 uiRequiredChunks = WMath::Max<WUInt32>(1, GetRequiredChunks(m_uiCount));
  uiMinChunksToKeep = WMath::Max(uiRequiredChunks, uiMinChunksToKeep);

  // keep some spare pointers for scaling the deque up again
  const WUInt32 uiChunksToKeep = 16 + uiMinChunksToKeep + 16;

  // only reduce the index array, if we can reduce its size at least to half (the +4 is for the very small cases)
  if (uiChunksToKeep + 4 >= m_uiChunks / 2)
    return;

  T** pNewChunkArray = W_NEW_RAW_BUFFER(m_pAllocator, T*, uiChunksToKeep);
  WMemoryUtils::ZeroFill<T*>(pNewChunkArray, uiChunksToKeep);

  const WUInt32 uiFirstChunk = GetFirstUsedChunk();

  // makes sure that no more than this amount of chunks is still allocated -> those can be copied over
  DeallocateUnusedChunks(uiChunksToKeep);

  // moves the used chunks into the new array
  for (WUInt32 i = 0; i < uiRequiredChunks; ++i)
  {
    pNewChunkArray[16 + i] = m_pChunks[uiFirstChunk + i];
    m_pChunks[uiFirstChunk + i] = nullptr;
  }

  // copy all still allocated chunks over to the new index array
  // since we just deallocated enough chunks, all that are found can be copied over as spare chunks
  {
    WUInt32 iPos = 0;
    for (WUInt32 i = 0; i < uiFirstChunk; ++i)
    {
      if (m_pChunks[i])
      {
        W_ASSERT_DEBUG(iPos < 16 || ((iPos >= 16 + uiRequiredChunks) && (iPos < uiChunksToKeep)), "Implementation error.");

        pNewChunkArray[iPos] = m_pChunks[i];
        m_pChunks[i] = nullptr;
        ++iPos;

        if (iPos == 16)
          iPos += uiRequiredChunks;
      }
    }

    for (WUInt32 i = GetLastUsedChunk() + 1; i < m_uiChunks; ++i)
    {
      if (m_pChunks[i])
      {
        W_ASSERT_DEBUG(iPos < 16 || ((iPos >= 16 + uiRequiredChunks) && (iPos < uiChunksToKeep)), "Implementation error.");

        pNewChunkArray[iPos] = m_pChunks[i];
        m_pChunks[i] = nullptr;
        ++iPos;

        if (iPos == 16)
          iPos += uiRequiredChunks;
      }
    }
  }

  W_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks);
  m_pChunks = pNewChunkArray;
  m_uiChunks = uiChunksToKeep;
  m_uiFirstElement = (16 * CHUNK_SIZE(T)) + (m_uiFirstElement % CHUNK_SIZE(T));
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::SetCount(WUInt32 uiCount)
{
  const WUInt32 uiOldCount = m_uiCount;
  const WUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    // grow the deque

    RESERVE(uiNewCount);
    m_uiCount = uiNewCount;

    if (Construct)
    {
      // default construct the new elements
      for (WUInt32 i = uiOldCount; i < uiNewCount; ++i)
        WMemoryUtils::Construct<ConstructAll>(&ElementAt(i), 1);
    }
    else
    {
      for (WUInt32 i = uiOldCount; i < uiNewCount; ++i)
        ElementAt(i);
    }
  }
  else
  {
    if (Construct)
    {
      // destruct elements at the end of the deque
      for (WUInt32 i = uiNewCount; i < uiOldCount; ++i)
        WMemoryUtils::Destruct(&operator[](i), 1);
    }

    m_uiCount = uiNewCount;

    // if enough elements have been destructed, trigger a size reduction (the first time will not deallocate anything though)
    ReduceSize(uiOldCount - uiNewCount);
  }
}

template <typename T, bool Construct>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger
// early.
void WDequeBase<T, Construct>::SetCountUninitialized(WUInt32 uiCount)
{
  static_assert(WIsPodType<T>::value == WTypeIsPod::value, "SetCountUninitialized is only supported for POD types. See W_DEFINE_AS_POD_TYPE() and W_DECLARE_POD_TYPE().");

  const WUInt32 uiOldCount = m_uiCount;
  const WUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    // grow the deque

    RESERVE(uiNewCount);
    m_uiCount = uiNewCount;

    for (WUInt32 i = uiOldCount; i < uiNewCount; ++i)
      ElementAt(i);
  }
  else
  {
    if (Construct)
    {
      // destruct elements at the end of the deque
      for (WUInt32 i = uiNewCount; i < uiOldCount; ++i)
        WMemoryUtils::Destruct(&operator[](i), 1);
    }

    m_uiCount = uiNewCount;

    // if enough elements have been destructed, trigger a size reduction (the first time will not deallocate anything though)
    ReduceSize(uiOldCount - uiNewCount);
  }
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::EnsureCount(WUInt32 uiCount)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount);
  }
}

template <typename T, bool Construct>
inline WUInt32 WDequeBase<T, Construct>::GetContiguousRange(WUInt32 uiIndex) const
{
  W_ASSERT_DEV(uiIndex < m_uiCount, "The deque has {0} elements. Cannot access element {1}.", m_uiCount, uiIndex);

  const WUInt32 uiChunkSize = CHUNK_SIZE(T);

  const WUInt32 uiRealIndex = m_uiFirstElement + uiIndex;
  const WUInt32 uiChunkOffset = uiRealIndex % uiChunkSize;

  const WUInt32 uiRange = uiChunkSize - uiChunkOffset;

  return WMath::Min(uiRange, GetCount() - uiIndex);
}

template <typename T, bool Construct>
inline T& WDequeBase<T, Construct>::operator[](WUInt32 uiIndex)
{
  W_ASSERT_DEBUG(uiIndex < m_uiCount, "The deque has {0} elements. Cannot access element {1}.", m_uiCount, uiIndex);

  const WUInt32 uiRealIndex = m_uiFirstElement + uiIndex;

  const WUInt32 uiChunkIndex = uiRealIndex / CHUNK_SIZE(T);
  const WUInt32 uiChunkOffset = uiRealIndex % CHUNK_SIZE(T);

  return m_pChunks[uiChunkIndex][uiChunkOffset];
}

template <typename T, bool Construct>
inline const T& WDequeBase<T, Construct>::operator[](WUInt32 uiIndex) const
{
  W_ASSERT_DEBUG(uiIndex < m_uiCount, "The deque has {0} elements. Cannot access element {1}.", m_uiCount, uiIndex);

  const WUInt32 uiRealIndex = m_uiFirstElement + uiIndex;

  const WUInt32 uiChunkIndex = uiRealIndex / CHUNK_SIZE(T);
  const WUInt32 uiChunkOffset = uiRealIndex % CHUNK_SIZE(T);

  return m_pChunks[uiChunkIndex][uiChunkOffset];
}

template <typename T, bool Construct>
inline T& WDequeBase<T, Construct>::ExpandAndGetRef()
{
  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  T* pElement = &ElementAt(m_uiCount - 1);

  if (Construct)
  {
    WMemoryUtils::Construct<ConstructAll>(pElement, 1);
  }

  return *pElement;
}

template <typename T, bool Construct>
inline void WDequeBase<T, Construct>::PushBack()
{
  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  T* pElement = &ElementAt(m_uiCount - 1);

  if (Construct)
  {
    WMemoryUtils::Construct<ConstructAll>(pElement, 1);
  }
}

template <typename T, bool Construct>
inline void WDequeBase<T, Construct>::PushBack(const T& element)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  WMemoryUtils::CopyConstruct(&ElementAt(m_uiCount - 1), element, 1);
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::PushBack(T&& element)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  WMemoryUtils::MoveConstruct<T>(&ElementAt(m_uiCount - 1), std::move(element));
}

template <typename T, bool Construct>
inline void WDequeBase<T, Construct>::PopBack(WUInt32 uiElements)
{
  W_ASSERT_DEV(uiElements <= GetCount(), "Cannot remove {0} elements, the deque only contains {1} elements.", uiElements, GetCount());

  for (WUInt32 i = 0; i < uiElements; ++i)
  {
    if (Construct)
      WMemoryUtils::Destruct(&operator[](m_uiCount - 1), 1);

    --m_uiCount;
  }

  // might trigger a memory reduction
  REDUCE_SIZE(uiElements);
}

template <typename T, bool Construct>
inline void WDequeBase<T, Construct>::PushFront(const T& element)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;
  --m_uiFirstElement;

  WMemoryUtils::CopyConstruct(&ElementAt(0), element, 1);
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::PushFront(T&& element)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;
  --m_uiFirstElement;

  WMemoryUtils::MoveConstruct<T>(&ElementAt(0), std::move(element));
}

template <typename T, bool Construct>
inline void WDequeBase<T, Construct>::PushFront()
{
  RESERVE(m_uiCount + 1);
  ++m_uiCount;
  --m_uiFirstElement;

  T* pElement = &ElementAt(0);

  if (Construct)
  {
    WMemoryUtils::Construct<SkipTrivialTypes>(pElement, 1);
  }
}

template <typename T, bool Construct>
inline void WDequeBase<T, Construct>::PopFront(WUInt32 uiElements)
{
  W_ASSERT_DEV(uiElements <= GetCount(), "Cannot remove {0} elements, the deque only contains {1} elements.", uiElements, GetCount());

  for (WUInt32 i = 0; i < uiElements; ++i)
  {
    if (Construct)
    {
      WMemoryUtils::Destruct(&operator[](0), 1);
    }

    --m_uiCount;
    ++m_uiFirstElement;
  }

  // might trigger a memory reduction
  REDUCE_SIZE(uiElements);
}

template <typename T, bool Construct>
W_ALWAYS_INLINE bool WDequeBase<T, Construct>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, bool Construct>
W_ALWAYS_INLINE WUInt32 WDequeBase<T, Construct>::GetCount() const
{
  return m_uiCount;
}

template <typename T, bool Construct>
W_ALWAYS_INLINE const T& WDequeBase<T, Construct>::PeekFront() const
{
  return operator[](0);
}

template <typename T, bool Construct>
W_ALWAYS_INLINE T& WDequeBase<T, Construct>::PeekFront()
{
  return operator[](0);
}

template <typename T, bool Construct>
W_ALWAYS_INLINE const T& WDequeBase<T, Construct>::PeekBack() const
{
  return operator[](m_uiCount - 1);
}

template <typename T, bool Construct>
W_ALWAYS_INLINE T& WDequeBase<T, Construct>::PeekBack()
{
  return operator[](m_uiCount - 1);
}

template <typename T, bool Construct>
W_ALWAYS_INLINE bool WDequeBase<T, Construct>::Contains(const T& value) const
{
  return IndexOf(value) != WInvalidIndex;
}

template <typename T, bool Construct>
WUInt32 WDequeBase<T, Construct>::IndexOf(const T& value, WUInt32 uiStartIndex) const
{
  for (WUInt32 i = uiStartIndex; i < m_uiCount; ++i)
  {
    if (WMemoryUtils::IsEqual(&operator[](i), &value))
      return i;
  }

  return WInvalidIndex;
}

template <typename T, bool Construct>
WUInt32 WDequeBase<T, Construct>::LastIndexOf(const T& value, WUInt32 uiStartIndex) const
{
  for (WUInt32 i = WMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (WMemoryUtils::IsEqual(&operator[](i), &value))
      return i;
  }
  return WInvalidIndex;
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::RemoveAtAndSwap(WUInt32 uiIndex)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  W_ASSERT_DEV(uiIndex < m_uiCount, "Cannot remove element {0}, the deque only contains {1} elements.", uiIndex, m_uiCount);

  if (uiIndex + 1 < m_uiCount) // do not copy over the same element, if uiIndex is actually the last element
    operator[](uiIndex) = PeekBack();

  PopBack();
}

template <typename T, bool Construct>
W_FORCE_INLINE void WDequeBase<T, Construct>::MoveIndexChunksLeft(WUInt32 uiChunkDiff)
{
  const WUInt32 uiCurFirstChunk = GetFirstUsedChunk();
  const WUInt32 uiRemainingChunks = m_uiChunks - uiCurFirstChunk;
  const WUInt32 uiNewFirstChunk = uiCurFirstChunk - uiChunkDiff;

  // ripple the chunks from the back to the front (in place)
  for (WUInt32 front = 0; front < uiRemainingChunks; ++front)
    WMath::Swap(m_pChunks[uiNewFirstChunk + front], m_pChunks[front + uiCurFirstChunk]);

  // just ensures that the following subtraction is possible
  W_ASSERT_DEBUG(m_uiFirstElement > uiChunkDiff * CHUNK_SIZE(T), "");

  // adjust which element is the first by how much the index array has been moved
  m_uiFirstElement -= uiChunkDiff * CHUNK_SIZE(T);
}

template <typename T, bool Construct>
W_FORCE_INLINE void WDequeBase<T, Construct>::MoveIndexChunksRight(WUInt32 uiChunkDiff)
{
  const WUInt32 uiCurFirstChunk = GetFirstUsedChunk();
  const WUInt32 uiLastChunk = (m_uiCount == 0) ? (m_uiFirstElement / CHUNK_SIZE(T)) : ((m_uiFirstElement + m_uiCount - 1) / CHUNK_SIZE(T));
  const WUInt32 uiCopyChunks = (uiLastChunk - uiCurFirstChunk) + 1;

  // ripple the chunks from the front to the back (in place)
  for (WUInt32 i = 0; i < uiCopyChunks; ++i)
    WMath::Swap(m_pChunks[uiLastChunk - i], m_pChunks[uiLastChunk + uiChunkDiff - i]);

  // adjust which element is the first by how much the index array has been moved
  m_uiFirstElement += uiChunkDiff * CHUNK_SIZE(T);
}

template <typename T, bool Construct>
W_ALWAYS_INLINE WUInt32 WDequeBase<T, Construct>::GetFirstUsedChunk() const
{
  return m_uiFirstElement / CHUNK_SIZE(T);
}

template <typename T, bool Construct>
W_FORCE_INLINE WUInt32 WDequeBase<T, Construct>::GetLastUsedChunk(WUInt32 uiAtSize) const
{
  if (uiAtSize == 0)
    return GetFirstUsedChunk();

  return (m_uiFirstElement + uiAtSize - 1) / CHUNK_SIZE(T);
}

template <typename T, bool Construct>
W_ALWAYS_INLINE WUInt32 WDequeBase<T, Construct>::GetLastUsedChunk() const
{
  return GetLastUsedChunk(m_uiCount);
}

template <typename T, bool Construct>
W_FORCE_INLINE WUInt32 WDequeBase<T, Construct>::GetRequiredChunks(WUInt32 uiAtSize) const
{
  if (uiAtSize == 0)
    return 0;

  return GetLastUsedChunk(uiAtSize) - GetFirstUsedChunk() + 1;
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::DeallocateUnusedChunks(WUInt32 uiMaxChunks)
{
  if (m_uiAllocatedChunks <= uiMaxChunks)
    return;

  // check all unused chunks at the end, deallocate all that are allocated
  for (WUInt32 i = GetLastUsedChunk() + 1; i < m_uiChunks; ++i)
  {
    if (m_pChunks[i])
    {
      --m_uiAllocatedChunks;
      W_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks[i]);

      if (m_uiAllocatedChunks <= uiMaxChunks)
        return;
    }
  }

  // check all unused chunks at the front, deallocate all that are allocated
  const WUInt32 uiFirstChunk = GetFirstUsedChunk();

  for (WUInt32 i = 0; i < uiFirstChunk; ++i)
  {
    if (m_pChunks[i])
    {
      --m_uiAllocatedChunks;
      W_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks[i]);

      if (m_uiAllocatedChunks <= uiMaxChunks)
        return;
    }
  }
}

template <typename T, bool Construct>
W_ALWAYS_INLINE void WDequeBase<T, Construct>::ResetReduceSizeCounter()
{
  m_iReduceSizeTimer = CHUNK_SIZE(T) * 8; // every time 8 chunks might be unused -> check whether to reduce the deque's size
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::ReduceSize(WInt32 iReduction)
{
  m_iReduceSizeTimer -= iReduction;

  // only trigger the size reduction every once in a while (after enough size reduction that actually a few chunks might be unused)
  if (m_iReduceSizeTimer > 0)
    return;

  ResetReduceSizeCounter();

  // we keep this amount of chunks
  // m_uiMaxCount will be adjusted over time
  // if the deque is shrunk and operates in this state long enough, m_uiMaxCount will be reduced more and more
  const WUInt32 uiMaxChunks = (m_uiMaxCount / CHUNK_SIZE(T)) + 3; // +1 because of rounding, +2 spare chunks

  W_ASSERT_DEBUG(uiMaxChunks >= GetRequiredChunks(m_uiCount), "Implementation Error.");

  DeallocateUnusedChunks(uiMaxChunks);

  // lerp between the current MaxCount and the actually active number of elements
  // m_uiMaxCount is never smaller than m_uiCount, but m_uiCount might be smaller
  // thus m_uiMaxCount might be reduced over time
  m_uiMaxCount = WMath::Max(m_uiCount, (m_uiMaxCount / 2) + (m_uiCount / 2));

  // Should we really adjust the size of the index array here?
  CompactIndexArray(uiMaxChunks);
}

template <typename T, bool Construct>
W_ALWAYS_INLINE WUInt32 WDequeBase<T, Construct>::GetCurMaxCount() const
{
  return m_uiChunks * CHUNK_SIZE(T) - m_uiFirstElement;
}

template <typename T, bool Construct>
W_FORCE_INLINE T* WDequeBase<T, Construct>::GetUnusedChunk()
{
  // first search for an unused, but already allocated, chunk and reuse it, if possible
  const WUInt32 uiCurFirstChunk = GetFirstUsedChunk();

  // search the unused blocks at the start
  for (WUInt32 i = 0; i < uiCurFirstChunk; ++i)
  {
    if (m_pChunks[i])
    {
      T* pChunk = m_pChunks[i];
      m_pChunks[i] = nullptr;
      return pChunk;
    }
  }

  const WUInt32 uiCurLastChunk = GetLastUsedChunk();

  // search the unused blocks at the end
  for (WUInt32 i = m_uiChunks - 1; i > uiCurLastChunk; --i)
  {
    if (m_pChunks[i])
    {
      T* pChunk = m_pChunks[i];
      m_pChunks[i] = nullptr;
      return pChunk;
    }
  }

  // nothing unused found, allocate a new block
  ResetReduceSizeCounter();
  ++m_uiAllocatedChunks;
  return W_NEW_RAW_BUFFER(m_pAllocator, T, CHUNK_SIZE(T));
}

template <typename T, bool Construct>
T& WDequeBase<T, Construct>::ElementAt(WUInt32 uiIndex)
{
  W_ASSERT_DEBUG(uiIndex < m_uiCount, "");

  const WUInt32 uiRealIndex = m_uiFirstElement + uiIndex;

  const WUInt32 uiChunkIndex = uiRealIndex / CHUNK_SIZE(T);
  const WUInt32 uiChunkOffset = uiRealIndex % CHUNK_SIZE(T);

  W_ASSERT_DEBUG(uiChunkIndex < m_uiChunks, "");

  if (m_pChunks[uiChunkIndex] == nullptr)
    m_pChunks[uiChunkIndex] = GetUnusedChunk();

  return m_pChunks[uiChunkIndex][uiChunkOffset];
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::DeallocateAll()
{
  Clear();

  WUInt32 i = 0;
  while (m_uiAllocatedChunks > 0)
  {
    if (m_pChunks[i])
    {
      --m_uiAllocatedChunks;
      W_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks[i]);
    }

    ++i;
  }

  W_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks);

  Constructor(m_pAllocator);
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::RemoveAtAndCopy(WUInt32 uiIndex)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  W_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex);

  for (WUInt32 i = uiIndex + 1; i < m_uiCount; ++i)
  {
    WMemoryUtils::CopyOverlapped(&operator[](i - 1), &operator[](i), 1);
  }

  PopBack();
}

template <typename T, bool Construct>
bool WDequeBase<T, Construct>::RemoveAndCopy(const T& value)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  WUInt32 uiIndex = IndexOf(value);

  if (uiIndex == WInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, bool Construct>
bool WDequeBase<T, Construct>::RemoveAndSwap(const T& value)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  WUInt32 uiIndex = IndexOf(value);

  if (uiIndex == WInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::InsertAt(WUInt32 uiIndex, const T& value)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  // Index 0 inserts before the first element, Index m_uiCount inserts after the last element.
  W_ASSERT_DEV(uiIndex <= m_uiCount, "The deque has {0} elements. Cannot insert an element at index {1}.", m_uiCount, uiIndex);

  PushBack();

  for (WUInt32 i = m_uiCount - 1; i > uiIndex; --i)
  {
    WMemoryUtils::Copy(&operator[](i), &operator[](i - 1), 1);
  }

  WMemoryUtils::Copy(&operator[](uiIndex), &value, 1);
}

template <typename T, bool Construct>
template <typename Comparer>
void WDequeBase<T, Construct>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
    WSorting::QuickSort(*this, comparer);
}

template <typename T, bool Construct>
void WDequeBase<T, Construct>::Sort()
{
  if (m_uiCount > 1)
    WSorting::QuickSort(*this, WCompareHelper<T>());
}

template <typename T, bool Construct>
WUInt64 WDequeBase<T, Construct>::GetHeapMemoryUsage() const
{
  if (m_pChunks == nullptr)
    return 0;

  WUInt64 res = m_uiChunks * sizeof(T*);

  for (WUInt32 i = 0; i < m_uiChunks; ++i)
  {
    if (m_pChunks[i] != nullptr)
    {
      res += (WUInt64)(CHUNK_SIZE(T)) * (WUInt64)sizeof(T);
    }
  }

  return res;
}

#undef REDUCE_SIZE
#undef RESERVE


template <typename T, typename A, bool Construct>
WDeque<T, A, Construct>::WDeque()
  : WDequeBase<T, Construct>(A::GetAllocator())
{
}

template <typename T, typename A, bool Construct>
WDeque<T, A, Construct>::WDeque(WAllocator* pAllocator)
  : WDequeBase<T, Construct>(pAllocator)
{
}

template <typename T, typename A, bool Construct>
WDeque<T, A, Construct>::WDeque(const WDeque<T, A, Construct>& other)
  : WDequeBase<T, Construct>(other, A::GetAllocator())
{
}

template <typename T, typename A, bool Construct>
WDeque<T, A, Construct>::WDeque(WDeque<T, A, Construct>&& other)
  : WDequeBase<T, Construct>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A, bool Construct>
WDeque<T, A, Construct>::WDeque(const WDequeBase<T, Construct>& other)
  : WDequeBase<T, Construct>(other, A::GetAllocator())
{
}

template <typename T, typename A, bool Construct>
WDeque<T, A, Construct>::WDeque(WDequeBase<T, Construct>&& other)
  : WDequeBase<T, Construct>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A, bool Construct>
void WDeque<T, A, Construct>::operator=(const WDeque<T, A, Construct>& rhs)
{
  WDequeBase<T, Construct>::operator=(rhs);
}

template <typename T, typename A, bool Construct>
void WDeque<T, A, Construct>::operator=(WDeque<T, A, Construct>&& rhs)
{
  WDequeBase<T, Construct>::operator=(std::move(rhs));
}

template <typename T, typename A, bool Construct>
void WDeque<T, A, Construct>::operator=(const WDequeBase<T, Construct>& rhs)
{
  WDequeBase<T, Construct>::operator=(rhs);
}

template <typename T, typename A, bool Construct>
void WDeque<T, A, Construct>::operator=(WDequeBase<T, Construct>&& rhs)
{
  WDequeBase<T, Construct>::operator=(std::move(rhs));
}
