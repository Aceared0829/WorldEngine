#pragma once

#ifndef W_INCLUDING_BASICS_H
#  error "Please don't include Types.h directly, but instead include Foundation/Basics.h"
#endif

// ***** Definition of types *****

#include <cstdint>

using WUInt8 = uint8_t;
using WUInt16 = uint16_t;
using WUInt32 = uint32_t;
using WUInt64 = unsigned long long;

using WInt8 = int8_t;
using WInt16 = int16_t;
using WInt32 = int32_t;
using WInt64 = long long;

// no float-types, since those are well portable

// Do some compile-time checks on the types
static_assert(sizeof(bool) == 1);
static_assert(sizeof(char) == 1);
static_assert(sizeof(float) == 4);
static_assert(sizeof(double) == 8);
static_assert(sizeof(WInt8) == 1);
static_assert(sizeof(WInt16) == 2);
static_assert(sizeof(WInt32) == 4);
static_assert(sizeof(WInt64) == 8); // must be defined in the specific compiler header
static_assert(sizeof(WUInt8) == 1);
static_assert(sizeof(WUInt16) == 2);
static_assert(sizeof(WUInt32) == 4);
static_assert(sizeof(WUInt64) == 8); // must be defined in the specific compiler header
static_assert(sizeof(long long int) == 8);

#if W_ENABLED(W_PLATFORM_64BIT)
#  define W_ALIGNMENT_MINIMUM 8
#elif W_ENABLED(W_PLATFORM_32BIT)
#  define W_ALIGNMENT_MINIMUM 4
#else
#  error "Unknown pointer size."
#endif

static_assert(sizeof(void*) == W_ALIGNMENT_MINIMUM);
static_assert(alignof(void*) == W_ALIGNMENT_MINIMUM);

/// Enum values for success and failure. To be used by functions as return values mostly, instead of bool.
enum WResultEnum
{
  W_FAILURE,
  W_SUCCESS
};

/// Default enum for returning failure or success, instead of using a bool.
struct [[nodiscard]] W_FOUNDATION_DLL WResult
{
public:
  WResult(WResultEnum res)
    : m_E(res)
  {
  }

  void operator=(WResultEnum rhs) { m_E = rhs; }
  bool operator==(WResultEnum cmp) const { return m_E == cmp; }
  bool operator!=(WResultEnum cmp) const { return m_E != cmp; }

  [[nodiscard]] W_ALWAYS_INLINE bool Succeeded() const { return m_E == W_SUCCESS; }
  [[nodiscard]] W_ALWAYS_INLINE bool Failed() const { return m_E == W_FAILURE; }

  /// Used to silence compiler warnings, when success or failure doesn't matter.
  W_ALWAYS_INLINE void IgnoreResult()
  {
    /* dummy to be called when a return value is [[nodiscard]] but the result is not needed */
  }

  /// Asserts that the function succeeded. In case of failure, the program will terminate.
  ///
  /// If \a msg is given, this will be the assert message. If \a details is provided, \a msg should contain a formatting element ({}), e.g. "Error: {}".
  void AssertSuccess(const char* szMsg = nullptr, const char* szDetails = nullptr) const;

private:
  WResultEnum m_E;
};

/// Explicit conversion to WResult, can be overloaded for arbitrary types.
///
/// This is intentionally not done via casting operator overload (or even additional constructors) since this usually comes with a
/// considerable data loss.
W_ALWAYS_INLINE WResult WToResult(WResult result)
{
  return result;
}

/// Helper macro to call functions that return WStatus or WResult in a function that returns WStatus (or WResult) as well.
/// If the called function fails, its return value is returned from the calling scope.
#define W_SUCCEED_OR_RETURN(code) \
  do                               \
  {                                \
    auto s = (code);               \
    if (WToResult(s).Failed())    \
      return s;                    \
  } while (false)

/// Like W_SUCCEED_OR_RETURN, but with error logging.
#define W_SUCCEED_OR_RETURN_LOG(code)                                       \
  do                                                                         \
  {                                                                          \
    auto s = (code);                                                         \
    if (WToResult(s).Failed())                                              \
    {                                                                        \
      WLog::Error("Call '{0}' failed with: {1}", W_PP_STRINGIFY(code), s); \
      return s;                                                              \
    }                                                                        \
  } while (false)

/// Like W_SUCCEED_OR_RETURN, but with custom error logging.
#define W_SUCCEED_OR_RETURN_CUSTOM_LOG(code, log)                             \
  do                                                                           \
  {                                                                            \
    auto s = (code);                                                           \
    if (WToResult(s).Failed())                                                \
    {                                                                          \
      WLog::Error("Call '{0}' failed with: {1}", W_PP_STRINGIFY(code), log); \
      return s;                                                                \
    }                                                                          \
  } while (false)

//////////////////////////////////////////////////////////////////////////

class WRTTI;
class WAllocator;

/// Dummy type to pass to templates and macros that expect a base type for a class that has no base.
class WNoBase
{
public:
  static const WRTTI* GetStaticRTTI() { return nullptr; }
};

/// Dummy type to pass to templates and macros that expect a base type for an enum class.
class WEnumBase
{
};

/// Dummy type to pass to templates and macros that expect a base type for an bitflags class.
class WBitflagsBase
{
};

/// Helper struct to get a storage type from a size in byte.
template <size_t SizeInByte>
struct WSizeToType;
/// \cond
template <>
struct WSizeToType<1>
{
  using Type = WUInt8;
};
template <>
struct WSizeToType<2>
{
  using Type = WUInt16;
};
template <>
struct WSizeToType<3>
{
  using Type = WUInt32;
};
template <>
struct WSizeToType<4>
{
  using Type = WUInt32;
};
template <>
struct WSizeToType<5>
{
  using Type = WUInt64;
};
template <>
struct WSizeToType<6>
{
  using Type = WUInt64;
};
template <>
struct WSizeToType<7>
{
  using Type = WUInt64;
};
template <>
struct WSizeToType<8>
{
  using Type = WUInt64;
};
/// \endcond
