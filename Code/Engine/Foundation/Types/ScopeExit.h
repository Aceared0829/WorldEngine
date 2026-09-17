
#pragma once

#include <Foundation/Basics.h>

/// \file
///
/// Scope exit utilities for RAII-style cleanup operations.

/// Executes code automatically when the current scope closes
///
/// Provides a convenient way to ensure cleanup code runs when leaving a scope,
/// regardless of how the scope is exited (normal return, exception, early return).
/// The code is executed in a destructor, guaranteeing cleanup even during stack unwinding.
///
/// Example usage:
/// ```cpp
/// {
///   FILE* file = fopen("test.txt", "r");
///   W_SCOPE_EXIT(if (file) fclose(file););
///   // file will be closed automatically when scope ends
/// }
/// ```
#define W_SCOPE_EXIT(code) auto W_PP_CONCAT(scopeExit_, W_SOURCE_LINE) = WMakeScopeExit([&]() { code; })

/// \internal Helper class implementing RAII scope exit functionality
///
/// Stores a callable object and executes it in the destructor. Used internally
/// by the W_SCOPE_EXIT macro to provide exception-safe cleanup operations.
template <typename T>
struct WScopeExit
{
  W_ALWAYS_INLINE WScopeExit(T&& func)
    : m_func(std::forward<T>(func))
  {
  }

  W_ALWAYS_INLINE ~WScopeExit() { m_func(); }

  T m_func;
};

/// \internal Helper function to implement W_SCOPE_EXIT
template <typename T>
W_ALWAYS_INLINE WScopeExit<T> WMakeScopeExit(T&& func)
{
  return WScopeExit<T>(std::forward<T>(func));
}
