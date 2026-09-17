#pragma once

/// \file

#include <Foundation/Basics.h>

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
#  define W_NAN_ASSERT(obj) (obj)->AssertNotNaN();
#else
#  define W_NAN_ASSERT(obj)
#endif

#define W_DECLARE_IF_FLOAT_TYPE template <typename = typename std::enable_if<std::is_floating_point_v<Type> == true>>
#define W_IMPLEMENT_IF_FLOAT_TYPE template <typename ENABLE_IF_FLOAT>

/// Simple helper union to store ints and floats to modify their bit patterns.
union WIntFloatUnion
{
  constexpr WIntFloatUnion(float fInit)
    : f(fInit)
  {
  }

  constexpr WIntFloatUnion(WUInt32 uiInit)
    : i(uiInit)
  {
  }

  WUInt32 i;
  float f;
};

/// Simple helper union to store ints and doubles to modify their bit patterns.
union WInt64DoubleUnion
{

  constexpr WInt64DoubleUnion(double fInit)
    : f(fInit)
  {
  }
  constexpr WInt64DoubleUnion(WUInt64 uiInit)
    : i(uiInit)
  {
  }

  WUInt64 i;
  double f;
};

/// Enum to describe which memory layout is used to store a matrix in a float array.
///
/// All WMatX classes use column-major format internally. That means they contain one array
/// of, e.g. 16 elements, and the first elements represent the first column, then the second column, etc.
/// So the data is stored column by column and is thus column-major.
/// Some other libraries, such as OpenGL or DirectX require data represented either in column-major
/// or row-major format. WMatrixLayout allows to retrieve the data from an WMatX class in the proper format,
/// and it also allows to pass matrix data as an array back in the WMatX class, and have it converted properly.
/// That means, if you need to pass the content of an WMatX to a function that requires the data in row-major
/// format, you specify that you want to convert the matrix to WMatrixLayout::RowMajor format and you will get
/// the data properly transposed. If a function requires data in column-major format, you specify
/// WMatrixLayout::ColumnMajor and you get it in column-major format (which is simply a memcpy).
struct WMatrixLayout
{
  enum Enum
  {
    RowMajor,   ///< The matrix is stored in row-major format.
    ColumnMajor ///< The matrix is stored in column-major format.
  };
};

/// Describes for which depth range a projection matrix is constructed.
///
/// Different Rendering APIs use different depth ranges.
/// E.g. OpenGL uses -1 for the near plane and +1 for the far plane.
/// DirectX uses 0 for the near plane and 1 for the far plane.
struct WClipSpaceDepthRange
{
  enum Enum
  {
    MinusOneToOne, ///< Near plane at -1, far plane at +1
    ZeroToOne,     ///< Near plane at 0, far plane at 1
  };

  /// Holds the default value for the projection depth range on each platform.
  /// This can be overridden by renderers to ensure the proper range is used when they become active.
  /// On Windows/D3D this is initialized with 'ZeroToOne' by default on all other platforms/OpenGL it is initialized with 'MinusOneToOne' by default.
  W_FOUNDATION_DLL static Enum Default;
};

/// Specifies whether a projection matrix should flip the result along the Y axis or not.
///
/// Mostly needed to compensate for differing Y texture coordinate conventions. Ie. on some platforms
/// the Y texture coordinate origin is at the lower left and on others on the upper left. To prevent having
/// to modify content to compensate, instead textures are simply flipped along Y on texture load.
/// The same has to be done for all render targets, ie. content has to be rendered upside-down.
///
/// Use WClipSpaceYMode::RenderToTextureDefault when rendering to a texture, to always get the correct
/// projection matrix.
struct WClipSpaceYMode
{
  enum Enum
  {
    Regular, ///< Creates a regular projection matrix
    Flipped, ///< Creates a projection matrix that flips the image on its head. On platforms with different Y texture coordinate
             ///< conventions, this can be used to compensate, by rendering images flipped to render targets.
  };

  /// Holds the platform default value for the clip space Y mode when rendering to a texture.
  /// This can be overridden by renderers to ensure the proper mode is used when they become active.
  /// On Windows/D3D this is initialized with 'Regular' by default on all other platforms/OpenGL it is initialized with 'Flipped' by default.
  W_FOUNDATION_DLL static Enum RenderToTextureDefault;
};

/// For selecting a left-handed or right-handed convention
struct WHandedness
{
  enum Enum
  {
    LeftHanded,
    RightHanded,
  };

  /// Holds the default handedness value to use. W uses 'LeftHanded' by default.
  W_FOUNDATION_DLL static Enum Default /*= WHandedness::LeftHanded*/;
};

// forward declarations
template <typename Type>
class WVec2Template;

using WVec2 = WVec2Template<float>;
using WVec2d = WVec2Template<double>;
using WVec2I32 = WVec2Template<WInt32>;
using WVec2U32 = WVec2Template<WUInt32>;
using WVec2I64 = WVec2Template<WInt64>;
using WVec2U64 = WVec2Template<WUInt64>;

template <typename Type>
class WVec3Template;

using WVec3 = WVec3Template<float>;
using WVec3d = WVec3Template<double>;
using WVec3I32 = WVec3Template<WInt32>;
using WVec3U32 = WVec3Template<WUInt32>;
using WVec3I64 = WVec3Template<WInt64>;
using WVec3U64 = WVec3Template<WUInt64>;

template <typename Type>
class WVec4Template;

using WVec4 = WVec4Template<float>;
using WVec4d = WVec4Template<double>;
using WVec4I64 = WVec4Template<WInt64>;
using WVec4I32 = WVec4Template<WInt32>;
using WVec4I16 = WVec4Template<WInt16>;
using WVec4I8 = WVec4Template<WInt8>;
using WVec4U64 = WVec4Template<WUInt64>;
using WVec4U32 = WVec4Template<WUInt32>;
using WVec4U16 = WVec4Template<WUInt16>;
using WVec4U8 = WVec4Template<WUInt8>;

template <typename Type>
class WMat3Template;

using WMat3 = WMat3Template<float>;
using WMat3d = WMat3Template<double>;

template <typename Type>
class WMat4Template;

using WMat4 = WMat4Template<float>;
using WMat4d = WMat4Template<double>;

template <typename Type>
struct WPlaneTemplate;

using WPlane = WPlaneTemplate<float>;
using WPlaned = WPlaneTemplate<double>;

template <typename Type>
class WQuatTemplate;

using WQuat = WQuatTemplate<float>;
using WQuatd = WQuatTemplate<double>;

template <typename Type>
class WAngleTemplate;

using WAngle = WAngleTemplate<float>;
using WAngled = WAngleTemplate<double>;


template <typename Type>
class WBoundingBoxTemplate;

using WBoundingBox = WBoundingBoxTemplate<float>;
using WBoundingBoxd = WBoundingBoxTemplate<double>;
using WBoundingBoxu32 = WBoundingBoxTemplate<WUInt32>;

template <typename Type>
class WBoundingBoxSphereTemplate;

using WBoundingBoxSphere = WBoundingBoxSphereTemplate<float>;
using WBoundingBoxSphered = WBoundingBoxSphereTemplate<double>;

template <typename Type>
class WBoundingSphereTemplate;

using WBoundingSphere = WBoundingSphereTemplate<float>;
using WBoundingSphered = WBoundingSphereTemplate<double>;

template <WUInt8 DecimalBits>
class WFixedPoint;


template <typename Type>
class WTransformTemplate;

using WTransform = WTransformTemplate<float>;
using WTransformd = WTransformTemplate<double>;

class WColor;
class WColorLinearUB;
class WColorGammaUB;

class WRandom;

template <typename Type>
class WRectTemplate;

using WRectU32 = WRectTemplate<WUInt32>;
using WRectU16 = WRectTemplate<WUInt16>;
using WRectI32 = WRectTemplate<WInt32>;
using WRectI16 = WRectTemplate<WInt16>;
using WRectFloat = WRectTemplate<float>;
using WRectDouble = WRectTemplate<double>;

class WFrustum;


/// An enum that allows to select on of the six main axis (positive / negative)
struct W_FOUNDATION_DLL WBasisAxis
{
  using StorageType = WInt8;

  /// An enum that allows to select on of the six main axis (positive / negative)
  enum Enum : WInt8
  {
    PositiveX,
    PositiveY,
    PositiveZ,
    NegativeX,
    NegativeY,
    NegativeZ,

    Default = PositiveX
  };

  /// Returns the vector for the given axis. E.g. (1, 0, 0) or (0, -1, 0), etc.
  static WVec3 GetBasisVector(WBasisAxis::Enum basisAxis);

  /// Computes a matrix representing the transformation. 'Forward' represents the X axis, 'Right' the Y axis and 'Up' the Z axis.
  static WMat3 CalculateTransformationMatrix(WBasisAxis::Enum forwardDir, WBasisAxis::Enum rightDir, WBasisAxis::Enum dir, float fUniformScale = 1.0f, float fScaleX = 1.0f, float fScaleY = 1.0f, float fScaleZ = 1.0f);

  /// Returns a quaternion that rotates from 'identity' to 'axis'
  static WQuat GetBasisRotation(WBasisAxis::Enum identity, WBasisAxis::Enum axis);

  /// Returns a quaternion that rotates from 'PositiveX' to 'axis'
  static WQuat GetBasisRotation_PosX(WBasisAxis::Enum axis);

  /// Returns the axis that is orthogonal to axis1 and axis2. If 'flip' is set, it returns the negated axis.
  ///
  /// If axis1 and axis2 are not orthogonal to each other, the value of axis1 is returned as the result.
  static WBasisAxis::Enum GetOrthogonalAxis(WBasisAxis::Enum axis1, WBasisAxis::Enum axis2, bool bFlip);
};

/// An enum that represents the operator of a comparison
struct W_FOUNDATION_DLL WComparisonOperator
{
  using StorageType = WUInt8;

  enum Enum
  {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    Default = Equal
  };

  /// Compares a to b with the given operator. This function only needs the == and < operator for T.
  template <typename T>
  static bool Compare(WComparisonOperator::Enum cmp, const T& a, const T& b); // [tested]
};
