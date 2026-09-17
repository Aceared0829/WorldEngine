
W_FORCE_INLINE void* WAllocPolicyAlignedHeap::Allocate(size_t uiSize, size_t uiAlign)
{
  // alignment has to be at least sizeof(void*) otherwise posix_memalign will fail
  uiAlign = WMath::Max<size_t>(uiAlign, 16u);

  void* ptr = nullptr;

  int res = posix_memalign(&ptr, uiAlign, uiSize);
  W_IGNORE_UNUSED(res);
  W_ASSERT_DEV(res == 0, "posix_memalign failed with error: {0}", res);

  W_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

W_ALWAYS_INLINE void WAllocPolicyAlignedHeap::Deallocate(void* ptr)
{
  free(ptr);
}
