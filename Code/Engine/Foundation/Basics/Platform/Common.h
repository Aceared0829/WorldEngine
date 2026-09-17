#pragma once

// On MSVC 2008 in 64 Bit <cmath> generates a lot of warnings (actually it is math.h, which is included by cmath)
W_WARNING_PUSH()
W_WARNING_DISABLE_MSVC(4985)

// include std header
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <new>

W_WARNING_POP()

// redefine NULL to nullptr
#ifdef NULL
#  undef NULL
#endif
#define NULL nullptr

// include c++11 specific header
#include <type_traits>
#include <utility>

/// Disallow the copy constructor and the assignment operator for this type.
#define W_DISALLOW_COPY_AND_ASSIGN(type) \
  type(const type&) = delete;             \
  void operator=(const type&) = delete

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
/// Macro helper to check alignment
#  define W_CHECK_ALIGNMENT(ptr, alignment) W_ASSERT_DEV(((size_t)ptr & ((alignment) - 1)) == 0, "Wrong alignment.")
#else
/// Macro helper to check alignment
#  define W_CHECK_ALIGNMENT(ptr, alignment)
#endif

#define W_WINCHECK_1 1          // W_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ defined (stringyfied to nothing)
#define W_WINCHECK_1_WINDOWS_ 1 // W_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ undefined (stringyfied to "_WINDOWS_")
#define W_WINCHECK_W_INCLUDED_WINDOWS_H \
  0                              // W_INCLUDED_WINDOWS_H undefined (stringyfied to "W_INCLUDED_WINDOWS_H", _WINDOWS_ defined (stringyfied to nothing)
#define W_WINCHECK_W_INCLUDED_WINDOWS_H_WINDOWS_ \
  1                              // W_INCLUDED_WINDOWS_H undefined (stringyfied to "W_INCLUDED_WINDOWS_H", _WINDOWS_ undefined (stringyfied to "_WINDOWS_")

/// Checks whether Windows.h has been included directly instead of through 'IncludeWindows.h'
///
/// Does this by stringifying the available defines, concatenating them into one long word, which is a known #define that evaluates to 0 or 1
#define W_CHECK_WINDOWS_INCLUDE(W_WINH_INCLUDED, WINH_INCLUDED)                               \
  static_assert(W_PP_CONCAT(W_WINCHECK_, W_PP_CONCAT(W_WINH_INCLUDED, WINH_INCLUDED)) == 1, \
    "Windows.h has been included but not through W. #include <Foundation/Platform/Win/Utils/IncludeWindows.h> instead of Windows.h");

#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)

/// The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of W_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define W_STATICLINK_FILE(LibraryName, UniqueName) W_CHECK_WINDOWS_INCLUDE(W_INCLUDED_WINDOWS_H, _WINDOWS_)

/// Used by the tool 'StaticLinkUtil' to generate the block after W_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see W_STATICLINK_FILE
#  define W_STATICLINK_REFERENCE(UniqueName)

/// This must occur exactly once in each static library, such that all W_STATICLINK_FILE macros can reference it.
#  define W_STATICLINK_LIBRARY(LibraryName) void WReferenceFunction_##LibraryName(bool bReturn = true)

/// Adds a static link reference to a plugin into an application, to make sure all code gets pulled in by the linker.
///
/// Add a line like this to a CPP file of your application:
/// W_STATICLINK_PLUGIN(ParticlePlugin);
///
/// When statically linking, this ensures that all relevant code of that plugin gets added to your app.
/// Without it, the linker may optimize too much code away, such that, for example, component types are unknown at runtime.
///
/// When dynamic linking is used, this macro has no effect, at all.
#  define W_STATICLINK_PLUGIN(PluginName)

/// A marker that can be placed in CPP files to enforce that the StaticLinkUtil doesn't skip this file.
///
/// Needed when a CPP file contains a global variable that's used for registering something (for example an WEnumerable),
/// and there is no other indication for the StaticLinkUtil to consider the file.
#  define W_STATICLINK_FORCE

#else

struct WStaticLinkHelper
{
  using Func = void (*)(bool);
  WStaticLinkHelper(Func f) { f(true); }
};

/// Helper struct to register the existence of statically linked plugins.
/// The macro W_STATICLINK_LIBRARY will register a the given library name prepended with `W` to the WPlugin system.
/// Implemented in Plugin.cpp.
struct W_FOUNDATION_DLL WPluginRegister
{
  WPluginRegister(const char* szName);
};

/// The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of W_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define W_STATICLINK_FILE(LibraryName, UniqueName)       \
    extern "C"                                              \
    {                                                       \
      void WReferenceFunction_##UniqueName(bool bReturn)   \
      {                                                     \
        (void)bReturn;                                      \
      }                                                     \
      void WReferenceFunction_##LibraryName(bool bReturn); \
    }                                                       \
    static WStaticLinkHelper StaticLinkHelper_##UniqueName(WReferenceFunction_##LibraryName);

/// Used by the tool 'StaticLinkUtil' to generate the block after W_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see W_STATICLINK_FILE
#  define W_STATICLINK_REFERENCE(UniqueName)                   \
    void WReferenceFunction_##UniqueName(bool bReturn = true); \
    WReferenceFunction_##UniqueName()

/// This must occur exactly once in each static library, such that all W_STATICLINK_FILE macros can reference it.
#  define W_STATICLINK_LIBRARY(LibraryName)                                                         \
    WPluginRegister WPluginRegister_##LibraryName(W_PP_STRINGIFY(W_PP_CONCAT(W, LibraryName))); \
    extern "C" void WReferenceFunction_##LibraryName(bool bReturn = true)

/// Adds a static link reference to a plugin into an application, to make sure all code gets pulled in by the linker.
///
/// Add a line like this to a CPP file of your application:
/// W_STATICLINK_PLUGIN(ParticlePlugin);
///
/// When statically linking, this ensures that all relevant code of that plugin gets added to your app.
/// Without it, the linker may optimize too much code away, such that, for example, component types are unknown at runtime.
///
/// When dynamic linking is used, this macro has no effect, at all.
#  define W_STATICLINK_PLUGIN(PluginName)                                               \
    extern "C" void W_PP_CONCAT(WReferenceFunction_, PluginName)(bool bReturn = true); \
    WStaticLinkHelper W_PP_CONCAT(WStaticLinkHelper_, PluginName)(W_PP_CONCAT(WReferenceFunction_, PluginName));

/// A marker that can be placed in CPP files to enforce that the StaticLinkUtil doesn't skip this file.
///
/// Needed when a CPP file contains a global variable that's used for registering something (for example an WEnumerable),
/// and there is no other indication for the StaticLinkUtil to consider the file.
#  define W_STATICLINK_FORCE

#endif

namespace WInternal
{
  template <typename T>
  constexpr bool AlwaysFalse = false;

  template <typename T>
  struct ArraySizeHelper
  {
    static_assert(AlwaysFalse<T>, "Cannot take compile time array size of given type");
  };

  template <typename T, size_t N>
  struct ArraySizeHelper<T[N]>
  {
    static constexpr size_t value = N;
  };

} // namespace WInternal

/// Macro to determine the size of a static array
#define W_ARRAY_SIZE(a) (WInternal::ArraySizeHelper<decltype(a)>::value)

/// Template helper which allows to suppress "Unused variable" warnings (e.g. result used in platform specific block, ..)
template <class T>
void W_IGNORE_UNUSED(const T&)
{
}

#if (__cplusplus >= 202002L || _MSVC_LANG >= 202002L)
#  undef W_USE_CPP20_OPERATORS
#  define W_USE_CPP20_OPERATORS W_ON
#endif

#if W_ENABLED(W_USE_CPP20_OPERATORS)
// in C++ 20 we don't need to declare an operator!=, it is automatically generated from operator==
#  define W_ADD_DEFAULT_OPERATOR_NOTEQUAL(...) /*empty*/
#else
#  define W_ADD_DEFAULT_OPERATOR_NOTEQUAL(...)                                   \
    W_ALWAYS_INLINE bool operator!=(W_EXPAND_ARGS_COMMA(__VA_ARGS__) rhs) const \
    {                                                                             \
      return !(*this == rhs);                                                     \
    }
#endif
