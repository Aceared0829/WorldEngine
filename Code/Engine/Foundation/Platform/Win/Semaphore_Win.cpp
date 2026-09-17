#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Threading/Semaphore.h>

WSemaphore::WSemaphore()
{
  m_hSemaphore = nullptr;
}

WSemaphore::~WSemaphore()
{
  if (m_hSemaphore != nullptr)
  {
    CloseHandle(m_hSemaphore);
    m_hSemaphore = nullptr;
  }
}

WResult WSemaphore::Create(WUInt32 uiInitialTokenCount, WStringView sSharedName /*= nullptr*/)
{
  W_ASSERT_DEV(m_hSemaphore == nullptr, "Semaphore can't be recreated.");

  LPSECURITY_ATTRIBUTES secAttr = nullptr; // default
  const DWORD flags = 0;                   // reserved but unused
  const DWORD access = STANDARD_RIGHTS_ALL | SEMAPHORE_MODIFY_STATE /* needed for ReleaseSemaphore */;

  if (sSharedName.IsEmpty())
  {
    // create an unnamed semaphore

    m_hSemaphore = CreateSemaphoreExW(secAttr, uiInitialTokenCount, WMath::MaxValue<WInt32>(), nullptr, flags, access);
  }
  else
  {
    // create a named semaphore in the 'Local' namespace
    // these are visible session wide, ie. all processes by the same user account can see these, but not across users

    const WStringBuilder semaphoreName("Local\\", sSharedName);

    m_hSemaphore = CreateSemaphoreExW(secAttr, uiInitialTokenCount, WMath::MaxValue<WInt32>(), WStringWChar(semaphoreName).GetData(), flags, access);
  }

  if (m_hSemaphore == nullptr)
  {
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WSemaphore::Open(WStringView sSharedName)
{
  W_ASSERT_DEV(m_hSemaphore == nullptr, "Semaphore can't be recreated.");

  const DWORD access = SYNCHRONIZE /* needed for WaitForSingleObject */ | SEMAPHORE_MODIFY_STATE /* needed for ReleaseSemaphore */;
  const BOOL inheriteHandle = FALSE;

  W_ASSERT_DEV(!sSharedName.IsEmpty(), "Name of semaphore to open mustn't be empty.");

  const WStringBuilder semaphoreName("Local\\", sSharedName);

  m_hSemaphore = OpenSemaphoreW(access, inheriteHandle, WStringWChar(semaphoreName).GetData());

  if (m_hSemaphore == nullptr)
  {
    return W_FAILURE;
  }

  return W_SUCCESS;
}

void WSemaphore::AcquireToken()
{
  W_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");
  W_VERIFY(WaitForSingleObject(m_hSemaphore, INFINITE) == WAIT_OBJECT_0, "Semaphore token acquisition failed.");
}

void WSemaphore::ReturnToken()
{
  W_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");
  W_VERIFY(ReleaseSemaphore(m_hSemaphore, 1, nullptr) != 0, "Returning a semaphore token failed, most likely due to a AcquireToken() / ReturnToken() mismatch.");
}

WResult WSemaphore::TryAcquireToken()
{
  W_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");

  const WUInt32 res = WaitForSingleObject(m_hSemaphore, 0 /* timeout of zero milliseconds */);

  if (res == WAIT_OBJECT_0)
  {
    return W_SUCCESS;
  }

  W_ASSERT_DEV(res == WAIT_OBJECT_0 || res == WAIT_TIMEOUT, "Semaphore TryAcquireToken (WaitForSingleObject) failed with error code {}.", res);

  return W_FAILURE;
}

#endif
