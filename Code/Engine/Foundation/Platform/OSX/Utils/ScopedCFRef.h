#pragma once

#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <CoreFoundation/CoreFoundation.h>

/// Helper class to release references of core foundation objects correctly.
template <typename T>
class WScopedCFRef
{
public:
  WScopedCFRef(T Ref)
    : m_Ref(Ref)
  {
  }

  ~WScopedCFRef()
  {
    CFRelease(m_Ref);
  }

  operator T() const
  {
    return m_Ref;
  }

private:
  T m_Ref;
};
