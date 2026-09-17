
W_FORCE_INLINE void* WAllocPolicyAlignedHeap::Allocate(size_t uiSize, size_t uiAlign)
{
  uiAlign = WMath::Max<size_t>(uiAlign, 16u);

  void* ptr = _aligned_malloc(uiSize, uiAlign);
  W_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

W_ALWAYS_INLINE void WAllocPolicyAlignedHeap::Deallocate(void* pPtr)
{
  _aligned_free(pPtr);
}
