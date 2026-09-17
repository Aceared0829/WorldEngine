
template <typename T>
W_ALWAYS_INLINE const T& WRenderDataBatch::Iterator<T>::operator*() const
{
  return *WStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
W_ALWAYS_INLINE const T* WRenderDataBatch::Iterator<T>::operator->() const
{
  return WStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
W_ALWAYS_INLINE WRenderDataBatch::Iterator<T>::operator const T*() const
{
  return WStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
W_ALWAYS_INLINE void WRenderDataBatch::Iterator<T>::Next()
{
  ++m_pCurrent;
}

template <typename T>
W_ALWAYS_INLINE bool WRenderDataBatch::Iterator<T>::IsValid() const
{
  return m_pCurrent < m_pEnd;
}

template <typename T>
W_ALWAYS_INLINE void WRenderDataBatch::Iterator<T>::operator++()
{
  Next();
}

template <typename T>
W_ALWAYS_INLINE WRenderDataBatch::Iterator<T>::Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd)
{
  m_pCurrent = pStart;
  m_pEnd = pEnd;
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE WUInt32 WRenderDataBatch::GetDataCount() const
{
  return m_Data.GetCount();
}

template <typename T>
W_ALWAYS_INLINE const T* WRenderDataBatch::GetFirstData() const
{
  return m_Data.IsEmpty() == false ? WStaticCast<const T*>(m_Data.GetPtr()->m_pRenderData) : nullptr;
}

template <typename T>
W_ALWAYS_INLINE WRenderDataBatch::Iterator<T> WRenderDataBatch::GetIterator(WUInt32 uiStartIndex, WUInt32 uiCount) const
{
  WUInt32 uiEndIndex = WMath::Min(uiStartIndex + uiCount, m_Data.GetCount());
  return Iterator<T>(m_Data.GetPtr() + uiStartIndex, m_Data.GetPtr() + uiEndIndex);
}

W_ALWAYS_INLINE WGALBufferHandle WRenderDataBatch::GetDataOffsetsBuffer() const
{
  return m_hDataOffsetsBuffer;
}

W_ALWAYS_INLINE WUInt32 WRenderDataBatch::GetFirstDataOffsetIndex() const
{
  return m_uiFirstDataOffsetIndex;
}

W_ALWAYS_INLINE WUInt32 WRenderDataBatch::GetInstanceCount() const
{
  return m_uiInstanceCount;
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE WUInt32 WRenderDataBatchList::GetBatchCount() const
{
  return m_Batches.GetCount();
}

W_ALWAYS_INLINE const WRenderDataBatch& WRenderDataBatchList::GetBatch(WUInt32 uiIndex) const
{
  return m_Batches[uiIndex];
}
