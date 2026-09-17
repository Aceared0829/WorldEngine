#pragma once

#include <Foundation/Basics.h>

/// Generic two-dimensional size representation with width and height components
///
/// Provides a simple container for representing rectangular dimensions in 2D space.
/// The template parameter allows using different numeric types (integers, floats) depending
/// on precision requirements. Common typedefs include WSizeU32, WSizeFloat, and WSizeDouble.
/// Primarily used for representing viewport dimensions, texture sizes, and UI element bounds.
template <typename Type>
class WSizeTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  // *** Data ***
public:
  Type width;
  Type height;

  // *** Constructors ***
public:
  /// Default constructor does not initialize the data.
  WSizeTemplate();

  /// Constructor to set all values.
  WSizeTemplate(Type width, Type height);

  // *** Common Functions ***
public:
  /// Returns true if the area described by the size is non zero
  bool HasNonZeroArea() const;
};

template <typename Type>
bool operator==(const WSizeTemplate<Type>& v1, const WSizeTemplate<Type>& v2);

template <typename Type>
bool operator!=(const WSizeTemplate<Type>& v1, const WSizeTemplate<Type>& v2);

#include <Foundation/Math/Implementation/Size_inl.h>

using WSizeU32 = WSizeTemplate<WUInt32>;
using WSizeFloat = WSizeTemplate<float>;
using WSizeDouble = WSizeTemplate<double>;

W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WSizeU32& arg);
