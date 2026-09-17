#pragma once

#include <Foundation/Basics.h>

/// RAII wrapper providing thread-safe access to an object with automatic lock management
///
/// Combines object access with lock acquisition/release in a single type. The lock is acquired
/// in the constructor and automatically released in the destructor, ensuring exception-safe
/// critical sections. Only move semantics are supported to prevent accidental lock duplication.
///
/// Template parameters:
/// - T: Lock type (e.g., WMutex, WSharedMutex)
/// - O: Object type being protected
///
/// Typical usage involves creating this as a temporary object to access shared data safely.
template <typename T, typename O>
class WLockedObject
{
public:
  W_ALWAYS_INLINE explicit WLockedObject(T& ref_lock, O* pObject)
    : m_pLock(&ref_lock)
    , m_pObject(pObject)
  {
    m_pLock->Lock();
  }

  WLockedObject() = default;

  W_ALWAYS_INLINE WLockedObject(WLockedObject<T, O>&& rhs) { *this = std::move(rhs); }

  WLockedObject(const WLockedObject<T, O>& rhs) = delete;

  void operator=(const WLockedObject<T, O>&& rhs)
  {
    if (m_pLock)
    {
      m_pLock->Unlock();
    }

    m_pLock = rhs.m_pLock;
    rhs.m_pLock = nullptr;
    m_pObject = rhs.m_pObject;
    rhs.m_pObject = nullptr;
  }

  void operator=(const WLockedObject<T, O>& rhs) = delete;

  W_ALWAYS_INLINE ~WLockedObject()
  {
    if (m_pLock)
    {
      m_pLock->Unlock();
    }
  }

  /// Whether the encapsulated object exists at all or is nullptr
  W_ALWAYS_INLINE bool isValid() const { return m_pObject != nullptr; }

  O* Borrow() { return m_pObject; }

  const O* Borrow() const { return m_pObject; }

  O* operator->() { return m_pObject; }

  const O* operator->() const { return m_pObject; }

  O& operator*() { return *m_pObject; }

  const O& operator*() const { return *m_pObject; }

  bool operator==(const O* rhs) const { return m_pObject == rhs; }

  bool operator!=(const O* rhs) const { return m_pObject != rhs; }

  bool operator!() const { return m_pObject == nullptr; }

  operator bool() const { return m_pObject != nullptr; }

private:
  mutable T* m_pLock = nullptr;
  mutable O* m_pObject = nullptr;
};
