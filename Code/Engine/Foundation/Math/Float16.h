#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Declarations.h>

/// A 16 bit IEEE float class. Often called "half"
///
/// This class only contains functions to convert between float and float16. It does not support any mathematical operations.
/// It is only intended for conversion, always do all mathematical operations on regular floats (or let the GPU do them on halfs).
class W_FOUNDATION_DLL WFloat16
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  /// Default constructor does not initialize the value.
  WFloat16() = default;

  /// Create float16 from float.
  WFloat16(float f); // [tested]

  /// Create float16 from float.
  void operator=(float f); // [tested]

  /// Create float16 from raw data.
  void SetRawData(WUInt16 uiData) { m_uiData = uiData; } // [tested]

  /// Returns the raw 16 Bit data.
  WUInt16 GetRawData() const { return m_uiData; } // [tested]

  /// Convert float16 to float.
  operator float() const; // [tested]

  /// Returns true, if both values are identical.
  bool operator==(const WFloat16& c2) { return m_uiData == c2.m_uiData; } // [tested]

  /// Returns true, if both values are not identical.
  bool operator!=(const WFloat16& c2) { return m_uiData != c2.m_uiData; } // [tested]

private:
  /// Raw 16 float data.
  WUInt16 m_uiData;
};

/// A simple helper class to use half-precision floats (WFloat16) as vectors
class W_FOUNDATION_DLL WFloat16Vec2
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  WFloat16Vec2() = default;
  WFloat16Vec2(const WVec2& vVec);

  void operator=(const WVec2& vVec);
  operator WVec2() const;

  WFloat16 x, y;
};

/// A simple helper class to use half-precision floats (WFloat16) as vectors
class W_FOUNDATION_DLL WFloat16Vec3
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  WFloat16Vec3() = default;
  WFloat16Vec3(const WVec3& vVec);

  void operator=(const WVec3& vVec);
  operator WVec3() const;

  WFloat16 x, y, z;
};

/// A simple helper class to use half-precision floats (WFloat16) as vectors
class W_FOUNDATION_DLL WFloat16Vec4
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  WFloat16Vec4() = default;
  WFloat16Vec4(const WVec4& vVec);

  void operator=(const WVec4& vVec);
  operator WVec4() const;

  WFloat16 x, y, z, w;
};
