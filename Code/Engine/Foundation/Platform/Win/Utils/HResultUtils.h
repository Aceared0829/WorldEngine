#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Platform/Win/Utils/MinWindows.h>
#include <Foundation/Strings/String.h>

/// Build string implementation for HRESULT.
W_FOUNDATION_DLL WString WHRESULTtoString(WMinWindows::HRESULT result);

/// Conversion of HRESULT to WResult.
W_ALWAYS_INLINE WResult WToResult(WMinWindows::HRESULT result)
{
  return result >= 0 ? W_SUCCESS : W_FAILURE;
}

#define W_HRESULT_TO_FAILURE(code)   \
  do                                  \
  {                                   \
    WMinWindows::HRESULT s = (code); \
    if (s < 0)                        \
      return W_FAILURE;              \
  } while (false)

#define W_HRESULT_TO_FAILURE_LOG(code)                                                         \
  do                                                                                            \
  {                                                                                             \
    WMinWindows::HRESULT s = (code);                                                           \
    if (s < 0)                                                                                  \
    {                                                                                           \
      WLog::Error("Call '{0}' failed with: {1}", W_PP_STRINGIFY(code), WHRESULTtoString(s)); \
      return W_FAILURE;                                                                        \
    }                                                                                           \
  } while (false)

#define W_HRESULT_TO_LOG(code)                                                                 \
  do                                                                                            \
  {                                                                                             \
    WMinWindows::HRESULT s = (code);                                                           \
    if (s < 0)                                                                                  \
    {                                                                                           \
      WLog::Error("Call '{0}' failed with: {1}", W_PP_STRINGIFY(code), WHRESULTtoString(s)); \
    }                                                                                           \
  } while (false)

#define W_NO_RETURNVALUE

#define W_HRESULT_TO_LOG_RET(code, ret)                                                        \
  do                                                                                            \
  {                                                                                             \
    WMinWindows::HRESULT s = (code);                                                           \
    if (s < 0)                                                                                  \
    {                                                                                           \
      WLog::Error("Call '{0}' failed with: {1}", W_PP_STRINGIFY(code), WHRESULTtoString(s)); \
      return ret;                                                                               \
    }                                                                                           \
  } while (false)

#define W_HRESULT_TO_ASSERT(code)                                                                     \
  do                                                                                                   \
  {                                                                                                    \
    WMinWindows::HRESULT s = (code);                                                                  \
    W_ASSERT_DEV(s >= 0, "Call '{0}' failed with: {1}", W_PP_STRINGIFY(code), WHRESULTtoString(s)); \
  } while (false)
