#pragma once

#ifndef W_INCLUDING_BASICS_H
#  error "Please don't include Assert.h directly, but instead include Foundation/Basics.h"
#endif

/// \file

/// ***** Assert Usage Guidelines *****
///
/// For your typical code, use W_ASSERT_DEV to check that vital preconditions are met.
/// Be aware that W_ASSERT_DEV is removed in non-development builds (ie. when W_COMPILE_FOR_DEVELOPMENT is disabled),
/// INCLUDING your code in the assert condition.
/// If the code that you are checking must be executed, even in non-development builds, use W_VERIFY instead.
/// W_ASSERT_DEV and W_VERIFY will trigger a breakpoint in debug builds, but will not interrupt the application
/// in release builds.
///
/// For conditions that are rarely violated or checking is very costly, use W_ASSERT_DEBUG. This assert is only active
/// in debug builds. This allows to have extra checking while debugging a program, but not waste performance when a
/// development or release build is used.
///
/// If you need to check something that is so vital that the application can only fail (i.e. crash), if that condition
/// is not met, even in release builds, then use W_ASSERT_RELEASE. This should not be used in frequently executed code,
/// as it is not stripped from non-development builds by default.
///
/// If you need to squeeze the last bit of performance out of your code, W_ASSERT_RELEASE can be disabled, by defining
/// W_DISABLE_RELEASE_ASSERTS.
/// Please be aware that W_ASSERT_RELEASE works like the other asserts, i.e. once it is deactivated, the code in the condition
/// is not executed anymore.
///

class WFormatString;

/// Assert handler callback. Should return true to trigger a break point or false if the assert should be ignored
using WAssertHandler = bool (*)(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);

W_FOUNDATION_DLL bool WDefaultAssertHandler(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);

/// Gets the current assert handler. The default assert handler shows a dialog on windows or prints to the console on other platforms.
W_FOUNDATION_DLL WAssertHandler WGetAssertHandler();

/// Sets the assert handler. It is the responsibility of the user to chain assert handlers if needed.
W_FOUNDATION_DLL void WSetAssertHandler(WAssertHandler handler);

/// Called by the assert macros whenever a check failed. Returns true if the user wants to trigger a break point
W_FOUNDATION_DLL bool WFailedCheck(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const class WFormatString& msg);
W_FOUNDATION_DLL bool WFailedCheck(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szMsg);

/// Dummy version of WFmt that only takes a single argument
inline const char* WFmt(const char* szFormat)
{
  return szFormat;
}

#if W_ENABLED(W_COMPILER_MSVC)
// Hides the call to __debugbreak from MSVCs optimizer to work around a bug in VS 2019
// that can lead to code (memcpy) after an assert to be omitted
W_FOUNDATION_DLL void MSVC_OutOfLine_DebugBreak(...);
#endif

#ifdef BUILDSYSTEM_CLANG_TIDY
[[noreturn]] void ClangTidyDoNotReturn();
#  define W_REPORT_FAILURE(szErrorMsg, ...) ClangTidyDoNotReturn()
#else
/// Macro to report a failure when that code is reached. This will ALWAYS be executed, even in release builds, therefore might crash the
/// application (or trigger a debug break).
#  define W_REPORT_FAILURE(szErrorMsg, ...)                                                                       \
    do                                                                                                             \
    {                                                                                                              \
      if (WFailedCheck(W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, "", WFmt(szErrorMsg, ##__VA_ARGS__))) \
        W_DEBUG_BREAK;                                                                                            \
    } while (false)
#endif

#ifdef BUILDSYSTEM_CLANG_TIDY
#  define W_ASSERT_ALWAYS(bCondition, szErrorMsg, ...) \
    do                                                  \
    {                                                   \
      if (!!(bCondition) == false)                      \
        ClangTidyDoNotReturn();                         \
    } while (false)

#  define W_ANALYSIS_ASSUME(bCondition) W_ASSERT_ALWAYS(bCondition, "")
#else
/// Macro to raise an error, if a condition is not met. Allows to write a message using WFormatString style. This assert will be triggered, even in
/// non-development builds and cannot be deactivated.
#  define W_ASSERT_ALWAYS(bCondition, szErrorMsg, ...)                                                                       \
    do                                                                                                                        \
    {                                                                                                                         \
      W_MSVC_ANALYSIS_WARNING_PUSH                                                                                           \
      W_MSVC_ANALYSIS_WARNING_DISABLE(6326) /* disable static analysis for the comparison */                                 \
      if (!!(bCondition) == false)                                                                                            \
      {                                                                                                                       \
        if (WFailedCheck(W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, #bCondition, WFmt(szErrorMsg, ##__VA_ARGS__))) \
          W_DEBUG_BREAK;                                                                                                     \
      }                                                                                                                       \
      W_MSVC_ANALYSIS_WARNING_POP                                                                                            \
    } while (false)

/// Macro to inform the static analysis that the given condition can be assumed to be true. Useful to give additional information to
/// static analysis if it can't figure it out by itself. Will do nothing outside of static analysis runs.
#  define W_ANALYSIS_ASSUME(bCondition)
#endif

/// This type of assert can be used to mark code as 'not (yet) implemented' and makes it easier to find it later on by just searching for these
/// asserts.
#define W_ASSERT_NOT_IMPLEMENTED W_REPORT_FAILURE("Not implemented");

// Occurrences of W_ASSERT_DEBUG are compiled out in non-debug builds
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using WFormatString style.
/// Compiled out in non-debug builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define W_ASSERT_DEBUG W_ASSERT_ALWAYS
#else
/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using WFormatString style.
/// Compiled out in non-debug builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define W_ASSERT_DEBUG(bCondition, szErrorMsg, ...)
#endif


// Occurrences of W_ASSERT_DEV are compiled out in non-development builds
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)

/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using WFormatString style.
/// Compiled out in non-development builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define W_ASSERT_DEV W_ASSERT_ALWAYS

/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using WFormatString style.
/// Compiled out in non-development builds, however the condition is always evaluated,
/// so you may execute important code in it.
#  define W_VERIFY W_ASSERT_ALWAYS

#else

/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using WFormatString style.
/// Compiled out in non-development builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define W_ASSERT_DEV(bCondition, szErrorMsg, ...)

/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using WFormatString style.
/// Compiled out in non-development builds, however the condition is always evaluated,
/// so you may execute important code in it.
#  define W_VERIFY(bCondition, szErrorMsg, ...)                             \
    if (!!(bCondition) == false)                                             \
    { /* The condition is evaluated, even though nothing is done with it. */ \
    }

#endif

#if W_DISABLE_RELEASE_ASSERTS

/// An assert to check conditions even in release builds.
///
/// These asserts can be disabled (and then their condition will not be evaluated),
/// but this needs to be specifically done by the user by defining W_DISABLE_RELEASE_ASSERTS.
/// That should only be done, if you are intending to ship a product, and want get rid of all unnecessary overhead.
#  define W_ASSERT_RELEASE(bCondition, szErrorMsg, ...)

#else

/// An assert to check conditions even in release builds.
///
/// These asserts can be disabled (and then their condition will not be evaluated),
/// but this needs to be specifically done by the user by defining W_DISABLE_RELEASE_ASSERTS.
/// That should only be done, if you are intending to ship a product, and want get rid of all unnecessary overhead.
#  define W_ASSERT_RELEASE W_ASSERT_ALWAYS

#endif

/// Macro to make unhandled cases in a switch block an error.
#define W_DEFAULT_CASE_NOT_IMPLEMENTED \
  default:                              \
    W_ASSERT_NOT_IMPLEMENTED           \
    break;
