#pragma once

#if defined(_MSC_VER)

#  undef W_COMPILER_MSVC
#  define W_COMPILER_MSVC W_ON

#  if __clang__
#    undef W_COMPILER_MSVC_CLANG
#    define W_COMPILER_MSVC_CLANG W_ON
#  else
#    undef W_COMPILER_MSVC_PURE
#    define W_COMPILER_MSVC_PURE W_ON
#  endif

#  ifdef _DEBUG
#    undef W_COMPILE_FOR_DEBUG
#    define W_COMPILE_FOR_DEBUG W_ON
#  endif


// Functions marked as W_ALWAYS_INLINE will be inlined even in Debug builds, which means you will step over them in a debugger
#  define W_ALWAYS_INLINE __forceinline

#  if W_ENABLED(W_COMPILE_FOR_DEBUG)
#    define W_FORCE_INLINE inline
#  else
#    define W_FORCE_INLINE __forceinline
#  endif

#  if W_ENABLED(W_COMPILE_FOR_DEBUG) || (_MSC_VER >= 1929 /* broken in early VS2019 but works again in VS2022 and later 2019 versions*/)

#    define W_DEBUG_BREAK \
      {                    \
        __debugbreak();    \
      }

#  else

#    define W_DEBUG_BREAK                         \
      {                                            \
        /* Declared with DLL export in Assert.h */ \
        MSVC_OutOfLine_DebugBreak();               \
      }

#  endif

#  if W_ENABLED(W_COMPILER_MSVC_CLANG)
#    define W_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  else
#    define W_SOURCE_FUNCTION __FUNCTION__
#  endif

#  define W_SOURCE_LINE __LINE__
#  define W_SOURCE_FILE __FILE__

// W_VA_NUM_ARGS() is a very nifty macro to retrieve the number of arguments handed to a variable-argument macro
// unfortunately, VS 2010 still has this compiler bug which treats a __VA_ARGS__ argument as being one single parameter:
// https://connect.microsoft.com/VisualStudio/feedback/details/521844/variadic-macro-treating-va-args-as-a-single-parameter-for-other-macros#details
#  if _MSC_VER >= 1400 && W_DISABLED(W_COMPILER_MSVC_CLANG)
#    define W_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, N, ...) N
#    define W_VA_NUM_ARGS_REVERSE_SEQUENCE 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
#    define W_LEFT_PARENTHESIS (
#    define W_RIGHT_PARENTHESIS )
#    define W_VA_NUM_ARGS(...) W_VA_NUM_ARGS_HELPER W_LEFT_PARENTHESIS __VA_ARGS__, W_VA_NUM_ARGS_REVERSE_SEQUENCE W_RIGHT_PARENTHESIS
#  endif

#  if W_ENABLED(W_COMPILER_MSVC_CLANG)
// Push/pop both diagnostic stacks, so that W_WARNING_DISABLE_CLANG is scoped as well.
#    define W_WARNING_PUSH() __pragma(warning(push)) _Pragma("clang diagnostic push")
#    define W_WARNING_POP() __pragma(warning(pop)) _Pragma("clang diagnostic pop")
#  else
#    define W_WARNING_PUSH() __pragma(warning(push))
#    define W_WARNING_POP() __pragma(warning(pop))
#  endif
#  define W_WARNING_DISABLE_MSVC(_x) __pragma(warning(disable : _x))

#  define W_DECL_EXPORT __declspec(dllexport)
#  define W_DECL_IMPORT __declspec(dllimport)
#  define W_DECL_EXPORT_FRIEND __declspec(dllexport)
#  define W_DECL_IMPORT_FRIEND __declspec(dllimport)

// These use the __pragma version to control the warnings so that they can be used within other macros etc.
#  define W_MSVC_ANALYSIS_WARNING_PUSH __pragma(warning(push))
#  define W_MSVC_ANALYSIS_WARNING_POP __pragma(warning(pop))
#  define W_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber) __pragma(warning(disable : warningNumber))
#  define W_MSVC_ANALYSIS_ASSUME(expression) __assume(expression)

#else

#  define W_WARNING_DISABLE_MSVC(_x)

/// Define some macros to work with the MSVC analysis warning
/// Note that the StaticAnalysis.h in Basics/Compiler/MSVC will define the MSVC specific versions.
#  define W_MSVC_ANALYSIS_WARNING_PUSH
#  define W_MSVC_ANALYSIS_WARNING_POP
#  define W_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber)
#  define W_MSVC_ANALYSIS_ASSUME(expression)

#endif
