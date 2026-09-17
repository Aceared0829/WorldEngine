#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

#include <pthread.h>
#include <semaphore.h>

using WThreadHandle = pthread_t;
using WThreadID = pthread_t;
using WMutexHandle = pthread_mutex_t;
using WOSThreadEntryPoint = void* (*)(void* pThreadParameter);

struct WSemaphoreHandle
{
  sem_t* m_pNamedOrUnnamed = nullptr;
  sem_t* m_pNamed = nullptr;
  sem_t m_Unnamed;
};

#define W_THREAD_CLASS_ENTRY_POINT void* WThreadClassEntryPoint(void* pThreadParameter);

struct WConditionVariableData
{
  pthread_cond_t m_ConditionVariable;
};


/// \endcond
