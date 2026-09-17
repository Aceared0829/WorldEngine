#include <Foundation/Memory/Policies/AllocPolicyGuarding.h>

WAllocPolicyGuarding::WAllocPolicyGuarding(WAllocator* pParent)
{
  W_ASSERT_NOT_IMPLEMENTED;
  W_IGNORE_UNUSED(m_uiPageSize);
  W_IGNORE_UNUSED(m_Mutex);
  W_IGNORE_UNUSED(m_AllocationsToFreeLater);
}

void* WAllocPolicyGuarding::Allocate(size_t uiSize, size_t uiAlign)
{
  W_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

void WAllocPolicyGuarding::Deallocate(void* ptr)
{
  W_ASSERT_NOT_IMPLEMENTED;
}
