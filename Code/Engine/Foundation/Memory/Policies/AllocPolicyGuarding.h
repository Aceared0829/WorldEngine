#pragma once

#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

class WAllocPolicyGuarding
{
public:
  WAllocPolicyGuarding(WAllocator* pParent);
  W_ALWAYS_INLINE ~WAllocPolicyGuarding() = default;

  void* Allocate(size_t uiSize, size_t uiAlign);
  void Deallocate(void* pPtr);

  W_ALWAYS_INLINE WAllocator* GetParent() const { return nullptr; }

private:
  WMutex m_Mutex;

  WUInt32 m_uiPageSize;

  WStaticRingBuffer<void*, (1 << 16)> m_AllocationsToFreeLater;
};
