#include <Foundation/Basics.h>
#include <Foundation/Strings/HashedString.h>

#if W_ENABLED(W_USE_PROFILING) && TRACY_ENABLE && !W_DOCS

#  include <tracy/tracy/Tracy.hpp>

W_ALWAYS_INLINE WUInt32 __tracyEzStringLength(const char* szString)
{
  return WStringUtils::GetStringElementCount(szString);
}

W_ALWAYS_INLINE WUInt32 __tracyEzStringLength(WStringView sString)
{
  return sString.GetElementCount();
}

W_ALWAYS_INLINE WUInt32 __tracyEzStringLength(const WString& sString)
{
  return sString.GetElementCount();
}

W_ALWAYS_INLINE WUInt32 __tracyEzStringLength(const WStringBuilder& sString)
{
  return sString.GetElementCount();
}

W_ALWAYS_INLINE WUInt32 __tracyEzStringLength(const WHashedString& sString)
{
  return sString.GetView().GetElementCount();
}

W_ALWAYS_INLINE const char* __tracyEzStringToConstChar(const WString& sString)
{
  return sString.GetData();
}

W_ALWAYS_INLINE const char* __tracyEzStringToConstChar(const WStringBuilder& sString)
{
  return sString.GetData();
}

W_ALWAYS_INLINE const char* __tracyEzStringToConstChar(const WHashedString& sString)
{
  return sString.GetData();
}

W_ALWAYS_INLINE const char* __tracyEzStringToConstChar(const WStringView& sString)
{
  // can just return the string views start pointer, because this is used together with __tracyEzStringLength
  return sString.GetStartPointer();
}

W_ALWAYS_INLINE const char* __tracyEzStringToConstChar(const char* szString)
{
  return szString;
}

constexpr WUInt32 __tracyCol(WUInt8 r, WUInt8 g, WUInt8 b)
{
  return 0xFF000000 | static_cast<WUInt32>(r) << 16 | static_cast<WUInt32>(g) << 8 | static_cast<WUInt32>(b);
}

constexpr WUInt32 __tracyEzZoneColor(WUInt64 uiHash)
{
  // same a WColorScheme::s_Colors
  constexpr WUInt32 colors[128] = {
    __tracyCol(201, 42, 42),
    __tracyCol(224, 49, 49),
    __tracyCol(240, 62, 62),
    __tracyCol(250, 82, 82),
    __tracyCol(255, 107, 107),
    __tracyCol(255, 135, 135),
    __tracyCol(255, 168, 168),
    __tracyCol(255, 201, 201),
    __tracyCol(255, 227, 227),
    __tracyCol(255, 245, 245),
    __tracyCol(166, 30, 77),
    __tracyCol(194, 37, 92),
    __tracyCol(214, 51, 108),
    __tracyCol(230, 73, 128),
    __tracyCol(240, 101, 149),
    __tracyCol(247, 131, 172),
    __tracyCol(250, 162, 193),
    __tracyCol(252, 194, 215),
    __tracyCol(255, 222, 235),
    __tracyCol(255, 240, 246),
    __tracyCol(134, 46, 156),
    __tracyCol(156, 54, 181),
    __tracyCol(174, 62, 201),
    __tracyCol(190, 75, 219),
    __tracyCol(204, 93, 232),
    __tracyCol(218, 119, 242),
    __tracyCol(229, 153, 247),
    __tracyCol(238, 190, 250),
    __tracyCol(243, 217, 250),
    __tracyCol(248, 240, 252),
    __tracyCol(95, 61, 196),
    __tracyCol(103, 65, 217),
    __tracyCol(112, 72, 232),
    __tracyCol(121, 80, 242),
    __tracyCol(132, 94, 247),
    __tracyCol(151, 117, 250),
    __tracyCol(177, 151, 252),
    __tracyCol(208, 191, 255),
    __tracyCol(229, 219, 255),
    __tracyCol(243, 240, 255),
    __tracyCol(54, 79, 199),
    __tracyCol(59, 91, 219),
    __tracyCol(66, 99, 235),
    __tracyCol(76, 110, 245),
    __tracyCol(92, 124, 250),
    __tracyCol(116, 143, 252),
    __tracyCol(145, 167, 255),
    __tracyCol(186, 200, 255),
    __tracyCol(219, 228, 255),
    __tracyCol(237, 242, 255),
    __tracyCol(24, 100, 171),
    __tracyCol(25, 113, 194),
    __tracyCol(28, 126, 214),
    __tracyCol(34, 139, 230),
    __tracyCol(51, 154, 240),
    __tracyCol(77, 171, 247),
    __tracyCol(116, 192, 252),
    __tracyCol(165, 216, 255),
    __tracyCol(208, 235, 255),
    __tracyCol(231, 245, 255),
    __tracyCol(11, 114, 133),
    __tracyCol(12, 133, 153),
    __tracyCol(16, 152, 173),
    __tracyCol(21, 170, 191),
    __tracyCol(34, 184, 207),
    __tracyCol(59, 201, 219),
    __tracyCol(102, 217, 232),
    __tracyCol(153, 233, 242),
    __tracyCol(197, 246, 250),
    __tracyCol(227, 250, 252),
    __tracyCol(8, 127, 91),
    __tracyCol(9, 146, 104),
    __tracyCol(12, 166, 120),
    __tracyCol(18, 184, 134),
    __tracyCol(32, 201, 151),
    __tracyCol(56, 217, 169),
    __tracyCol(99, 230, 190),
    __tracyCol(150, 242, 215),
    __tracyCol(195, 250, 232),
    __tracyCol(230, 252, 245),
    __tracyCol(43, 138, 62),
    __tracyCol(47, 158, 68),
    __tracyCol(55, 178, 77),
    __tracyCol(64, 192, 87),
    __tracyCol(81, 207, 102),
    __tracyCol(105, 219, 124),
    __tracyCol(140, 233, 154),
    __tracyCol(178, 242, 187),
    __tracyCol(211, 249, 216),
    __tracyCol(235, 251, 238),
    __tracyCol(92, 148, 13),
    __tracyCol(102, 168, 15),
    __tracyCol(116, 184, 22),
    __tracyCol(130, 201, 30),
    __tracyCol(148, 216, 45),
    __tracyCol(169, 227, 75),
    __tracyCol(192, 235, 117),
    __tracyCol(216, 245, 162),
    __tracyCol(233, 250, 200),
    __tracyCol(244, 252, 227),
    __tracyCol(230, 119, 0),
    __tracyCol(240, 140, 0),
    __tracyCol(245, 159, 0),
    __tracyCol(250, 176, 5),
    __tracyCol(252, 196, 25),
    __tracyCol(255, 212, 59),
    __tracyCol(255, 224, 102),
    __tracyCol(255, 236, 153),
    __tracyCol(255, 243, 191),
    __tracyCol(255, 249, 219),
    __tracyCol(217, 72, 15),
    __tracyCol(232, 89, 12),
    __tracyCol(247, 103, 7),
    __tracyCol(253, 126, 20),
    __tracyCol(255, 146, 43),
    __tracyCol(255, 169, 77),
    __tracyCol(255, 192, 120),
    __tracyCol(255, 216, 168),
    __tracyCol(255, 232, 204),
    __tracyCol(255, 244, 230),
    __tracyCol(33, 37, 41),
    __tracyCol(52, 58, 64),
    __tracyCol(73, 80, 87),
    __tracyCol(134, 142, 150),
    __tracyCol(173, 181, 189),
    __tracyCol(206, 212, 218),
    __tracyCol(222, 226, 230),
    __tracyCol(233, 236, 239),
  };
  // clang-format on

  return colors[uiHash % W_ARRAY_SIZE(colors)];
}

/// Similar to W_PROFILE_SCOPE, but only forwards to Tracy
#  define W_TRACY_PROFILE_SCOPE(ScopeName)                                            \
    ZoneScoped;                                                                        \
    ZoneName(__tracyEzStringToConstChar(ScopeName), __tracyEzStringLength(ScopeName)); \
    ZoneColor(__tracyEzZoneColor(WHashingUtils::StringHash(ScopeName)))

// Override the standard W profiling macros and inject Tracy profiling scopes

#  undef W_PROFILE_SCOPE
#  define W_PROFILE_SCOPE(ScopeName)                                                                                    \
    WProfilingScope W_PP_CONCAT(_WProfilingScope, W_SOURCE_LINE)(ScopeName, W_SOURCE_FUNCTION, WTime::MakeZero()); \
    W_TRACY_PROFILE_SCOPE(ScopeName)

#  undef W_PROFILE_SCOPE_WITH_TIMEOUT
#  define W_PROFILE_SCOPE_WITH_TIMEOUT(ScopeName, Timeout)                                                   \
    WProfilingScope W_PP_CONCAT(_WProfilingScope, W_SOURCE_LINE)(ScopeName, W_SOURCE_FUNCTION, Timeout); \
    W_TRACY_PROFILE_SCOPE(ScopeName);

#  undef W_PROFILE_LIST_SCOPE
#  define W_PROFILE_LIST_SCOPE(ListName, FirstSectionName)                                                               \
    WProfilingListScope W_PP_CONCAT(_WProfilingScope, W_SOURCE_LINE)(ListName, FirstSectionName, W_SOURCE_FUNCTION); \
    W_TRACY_PROFILE_SCOPE(ListName);

#  undef W_PROFILER_FRAME_MARKER
#  define W_PROFILER_FRAME_MARKER() FrameMark

#else

/// Similar to W_PROFILE_SCOPE, but only forwards to Tracy
#  define W_TRACY_PROFILE_SCOPE(ScopeName)

#endif
