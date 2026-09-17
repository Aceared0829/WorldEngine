
#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringUtils.h>

/// Hash helper to be used as a template argument to WHashTable / WHashSet for case insensitive string keys.
struct W_FOUNDATION_DLL WHashHelperString_NoCase
{
  inline static WUInt32 Hash(WStringView sValue);                       // [tested]

  W_ALWAYS_INLINE static bool Equal(WStringView lhs, WStringView rhs); // [tested]
};

#include <Foundation/Algorithm/Implementation/HashHelperString_inl.h>
