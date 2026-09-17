
template <typename Type>
WProcessingStreamIterator<Type>::WProcessingStreamIterator(const WProcessingStream* pStream, WUInt64 uiNumElements, WUInt64 uiStartIndex)

{
  W_ASSERT_DEV(pStream != nullptr, "Stream pointer may not be null!");
  W_ASSERT_DEV(pStream->GetElementSize() == sizeof(Type), "Data size missmatch");

  m_uiElementStride = pStream->GetElementStride();

  m_pCurrentPtr = WMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<std::ptrdiff_t>(uiStartIndex * m_uiElementStride));
  m_pEndPtr = WMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<std::ptrdiff_t>((uiStartIndex + uiNumElements) * m_uiElementStride));
}

template <typename Type>
W_ALWAYS_INLINE Type& WProcessingStreamIterator<Type>::Current() const
{
  return *static_cast<Type*>(m_pCurrentPtr);
}

template <typename Type>
W_ALWAYS_INLINE bool WProcessingStreamIterator<Type>::HasReachedEnd() const
{
  return m_pCurrentPtr >= m_pEndPtr;
}

template <typename Type>
W_ALWAYS_INLINE void WProcessingStreamIterator<Type>::Advance()
{
  m_pCurrentPtr = WMemoryUtils::AddByteOffset(m_pCurrentPtr, static_cast<std::ptrdiff_t>(m_uiElementStride));
}

template <typename Type>
W_ALWAYS_INLINE void WProcessingStreamIterator<Type>::Advance(WUInt32 uiNumElements)
{
  m_pCurrentPtr = WMemoryUtils::AddByteOffset(m_pCurrentPtr, static_cast<std::ptrdiff_t>(m_uiElementStride * uiNumElements));
}
