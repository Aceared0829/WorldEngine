#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/CurveFunctions.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>

// Default are D3D convention before a renderer is initialized.
WClipSpaceDepthRange::Enum WClipSpaceDepthRange::Default = WClipSpaceDepthRange::ZeroToOne;
WClipSpaceYMode::Enum WClipSpaceYMode::RenderToTextureDefault = WClipSpaceYMode::Regular;

WHandedness::Enum WHandedness::Default = WHandedness::LeftHanded;

bool WMath::IsPowerOf(WInt32 value, WInt32 iBase)
{
  if (value == 1)
    return true;

  while (value > iBase)
  {
    if (value % iBase == 0)
      value /= iBase;
    else
      return false;
  }

  return (value == iBase);
}

WUInt32 WMath::PowerOfTwo_Floor(WUInt32 uiNpot)
{
  return static_cast<WUInt32>(PowerOfTwo_Floor(static_cast<WUInt64>(uiNpot)));
}

WUInt64 WMath::PowerOfTwo_Floor(WUInt64 uiNpot)
{
  if (IsPowerOf2(uiNpot))
    return (uiNpot);

  for (WUInt32 i = 1; i <= (sizeof(uiNpot) * 8); ++i)
  {
    uiNpot >>= 1;

    if (uiNpot == 1)
      return (uiNpot << i);
  }

  return (1);
}

WUInt32 WMath::PowerOfTwo_Ceil(WUInt32 uiNpot)
{
  return static_cast<WUInt32>(PowerOfTwo_Ceil(static_cast<WUInt64>(uiNpot)));
}

WUInt64 WMath::PowerOfTwo_Ceil(WUInt64 uiNpot)
{
  if (IsPowerOf2(uiNpot))
    return (uiNpot);

  for (WUInt32 i = 1; i <= (sizeof(uiNpot) * 8); ++i)
  {
    uiNpot >>= 1;

    if (uiNpot == 1)
    {
      // note: left shift by 32 bits is undefined behavior and typically just returns the left operand unchanged
      // so for npot values larger than 1^31 we do run into this code path, but instead of returning 0, as one may expect, it will usually return 1
      return uiNpot << (i + 1u);
    }
  }

  return (1u);
}


WUInt32 WMath::GreatestCommonDivisor(WUInt32 a, WUInt32 b)
{
  // https://lemire.me/blog/2013/12/26/fastest-way-to-compute-the-greatest-common-divisor/
  if (a == 0)
  {
    return b;
  }
  if (b == 0)
  {
    return a;
  }

  WUInt32 shift = FirstBitLow(a | b);
  a >>= FirstBitLow(a);
  do
  {
    b >>= FirstBitLow(b);
    if (a > b)
    {
      Swap(a, b);
    }
    b = b - a;
  } while (b != 0);
  return a << shift;
}

WResult WMath::TryMultiply32(WUInt32& out_uiResult, WUInt32 a, WUInt32 b, WUInt32 c, WUInt32 d)
{
  WUInt64 result = static_cast<WUInt64>(a) * static_cast<WUInt64>(b);

  if (result > 0xFFFFFFFFllu)
  {
    return W_FAILURE;
  }

  result *= static_cast<WUInt64>(c);

  if (result > 0xFFFFFFFFllu)
  {
    return W_FAILURE;
  }

  result *= static_cast<WUInt64>(d);

  if (result > 0xFFFFFFFFllu)
  {
    return W_FAILURE;
  }

  out_uiResult = static_cast<WUInt32>(result & 0xFFFFFFFFllu);
  return W_SUCCESS;
}

WUInt32 WMath::SafeMultiply32(WUInt32 a, WUInt32 b, WUInt32 c, WUInt32 d)
{
  WUInt32 result = 0;
  if (TryMultiply32(result, a, b, c, d).Succeeded())
  {
    return result;
  }

  W_REPORT_FAILURE("Safe multiplication failed: {0} * {1} * {2} * {3} exceeds UInt32 range.", a, b, c, d);
  std::terminate();
}

WResult WMath::TryMultiply64(WUInt64& out_uiResult, WUInt64 a, WUInt64 b, WUInt64 c, WUInt64 d)
{
  if (a == 0 || b == 0 || c == 0 || d == 0)
  {
    out_uiResult = 0;
    return W_SUCCESS;
  }

#if W_ENABLED(W_PLATFORM_ARCH_X86) && W_ENABLED(W_PLATFORM_64BIT) && W_ENABLED(W_COMPILER_MSVC)

  WUInt64 uiHighBits = 0;

  const WUInt64 ab = _umul128(a, b, &uiHighBits);
  if (uiHighBits != 0)
  {
    return W_FAILURE;
  }

  const WUInt64 abc = _umul128(ab, c, &uiHighBits);
  if (uiHighBits != 0)
  {
    return W_FAILURE;
  }

  const WUInt64 abcd = _umul128(abc, d, &uiHighBits);
  if (uiHighBits != 0)
  {
    return W_FAILURE;
  }

#else
  const WUInt64 ab = a * b;
  const WUInt64 abc = ab * c;
  const WUInt64 abcd = abc * d;

  if (a > 1 && b > 1 && (ab / a != b))
  {
    return W_FAILURE;
  }

  if (c > 1 && (abc / c != ab))
  {
    return W_FAILURE;
  }

  if (d > 1 && (abcd / d != abc))
  {
    return W_FAILURE;
  }

#endif

  out_uiResult = abcd;
  return W_SUCCESS;
}

WUInt64 WMath::SafeMultiply64(WUInt64 a, WUInt64 b, WUInt64 c, WUInt64 d)
{
  WUInt64 result = 0;
  if (TryMultiply64(result, a, b, c, d).Succeeded())
  {
    return result;
  }

  W_REPORT_FAILURE("Safe multiplication failed: {0} * {1} * {2} * {3} exceeds WUInt64 range.", a, b, c, d);
  std::terminate();
}

#if W_ENABLED(W_PLATFORM_32BIT)
size_t WMath::SafeConvertToSizeT(WUInt64 uiValue)
{
  size_t result = 0;
  if (TryConvertToSizeT(result, uiValue).Succeeded())
  {
    return result;
  }

  W_REPORT_FAILURE("Given value ({}) can't be converted to size_t because it is too big.", uiValue);
  std::terminate();
}
#endif


float WMath::ReplaceNaN(float fValue, float fFallback)
{
  // ATTENTION: if this is a template, inline or constexpr function, the current MSVC (17.6)
  // seems to generate incorrect code and the IsNaN check doesn't detect NaNs.
  // As an out-of-line function it works.

  if (WMath::IsNaN(fValue))
    return fFallback;

  return fValue;
}

double WMath::ReplaceNaN(double fValue, double fFallback)
{
  // ATTENTION: if this is a template, inline or constexpr function, the current MSVC (17.6)
  // seems to generate incorrect code and the IsNaN check doesn't detect NaNs.
  // As an out-of-line function it works.

  if (WMath::IsNaN(fValue))
    return fFallback;

  return fValue;
}

WVec3 WBasisAxis::GetBasisVector(Enum basisAxis)
{
  switch (basisAxis)
  {
    case WBasisAxis::PositiveX:
      return WVec3(1.0f, 0.0f, 0.0f);

    case WBasisAxis::NegativeX:
      return WVec3(-1.0f, 0.0f, 0.0f);

    case WBasisAxis::PositiveY:
      return WVec3(0.0f, 1.0f, 0.0f);

    case WBasisAxis::NegativeY:
      return WVec3(0.0f, -1.0f, 0.0f);

    case WBasisAxis::PositiveZ:
      return WVec3(0.0f, 0.0f, 1.0f);

    case WBasisAxis::NegativeZ:
      return WVec3(0.0f, 0.0f, -1.0f);

    default:
      W_REPORT_FAILURE("Invalid basis dir {0}", basisAxis);
      return WVec3::MakeZero();
  }
}

WMat3 WBasisAxis::CalculateTransformationMatrix(Enum forwardDir, Enum rightDir, Enum dir, float fUniformScale /*= 1.0f*/, float fScaleX /*= 1.0f*/, float fScaleY /*= 1.0f*/, float fScaleZ /*= 1.0f*/)
{
  WMat3 mResult;
  mResult.SetRow(0, WBasisAxis::GetBasisVector(forwardDir) * fUniformScale * fScaleX);
  mResult.SetRow(1, WBasisAxis::GetBasisVector(rightDir) * fUniformScale * fScaleY);
  mResult.SetRow(2, WBasisAxis::GetBasisVector(dir) * fUniformScale * fScaleZ);

  return mResult;
}


WQuat WBasisAxis::GetBasisRotation_PosX(Enum axis)
{
  return WQuat::MakeShortestRotation(WVec3::MakeAxisX(), GetBasisVector(axis));
}

WQuat WBasisAxis::GetBasisRotation(Enum identity, Enum axis)
{
  return WQuat::MakeShortestRotation(GetBasisVector(identity), GetBasisVector(axis));
}

WBasisAxis::Enum WBasisAxis::GetOrthogonalAxis(Enum axis1, Enum axis2, bool bFlip)
{
  const WVec3 a1 = WBasisAxis::GetBasisVector(axis1);
  const WVec3 a2 = WBasisAxis::GetBasisVector(axis2);

  WVec3 c = a1.CrossRH(a2);

  if (bFlip)
    c = -c;

  if (c.IsEqual(WVec3::MakeAxisX(), 0.01f))
    return WBasisAxis::PositiveX;
  if (c.IsEqual(-WVec3::MakeAxisX(), 0.01f))
    return WBasisAxis::NegativeX;

  if (c.IsEqual(WVec3::MakeAxisY(), 0.01f))
    return WBasisAxis::PositiveY;
  if (c.IsEqual(-WVec3::MakeAxisY(), 0.01f))
    return WBasisAxis::NegativeY;

  if (c.IsEqual(WVec3::MakeAxisZ(), 0.01f))
    return WBasisAxis::PositiveZ;
  if (c.IsEqual(-WVec3::MakeAxisZ(), 0.01f))
    return WBasisAxis::NegativeZ;

  return axis1;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WComparisonOperator, 1)
  W_ENUM_CONSTANTS(WComparisonOperator::Equal, WComparisonOperator::NotEqual)
  W_ENUM_CONSTANTS(WComparisonOperator::Less, WComparisonOperator::LessEqual)
  W_ENUM_CONSTANTS(WComparisonOperator::Greater, WComparisonOperator::GreaterEqual)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WCurveFunction, 1)
 W_ENUM_CONSTANT(WCurveFunction::Linear),
 W_ENUM_CONSTANT(WCurveFunction::ConstantZero),
 W_ENUM_CONSTANT(WCurveFunction::ConstantOne),
 W_ENUM_CONSTANT(WCurveFunction::EaseInSine),
 W_ENUM_CONSTANT(WCurveFunction::EaseOutSine),
 W_ENUM_CONSTANT(WCurveFunction::EaseInOutSine),
 W_ENUM_CONSTANT(WCurveFunction::EaseInQuad),
 W_ENUM_CONSTANT(WCurveFunction::EaseOutQuad),
 W_ENUM_CONSTANT(WCurveFunction::EaseInOutQuad),
 W_ENUM_CONSTANT(WCurveFunction::EaseInCubic),
 W_ENUM_CONSTANT(WCurveFunction::EaseOutCubic),
 W_ENUM_CONSTANT(WCurveFunction::EaseInOutCubic),
 W_ENUM_CONSTANT(WCurveFunction::EaseInQuartic),
 W_ENUM_CONSTANT(WCurveFunction::EaseOutQuartic),
 W_ENUM_CONSTANT(WCurveFunction::EaseInOutQuartic),
 W_ENUM_CONSTANT(WCurveFunction::EaseInQuintic),
 W_ENUM_CONSTANT(WCurveFunction::EaseOutQuintic),
 W_ENUM_CONSTANT(WCurveFunction::EaseInOutQuintic),
 W_ENUM_CONSTANT(WCurveFunction::EaseInExpo),
 W_ENUM_CONSTANT(WCurveFunction::EaseOutExpo),
 W_ENUM_CONSTANT(WCurveFunction::EaseInOutExpo),
 W_ENUM_CONSTANT(WCurveFunction::EaseInCirc),
 W_ENUM_CONSTANT(WCurveFunction::EaseOutCirc),
 W_ENUM_CONSTANT(WCurveFunction::EaseInOutCirc),
 W_ENUM_CONSTANT(WCurveFunction::EaseInBack),
 W_ENUM_CONSTANT(WCurveFunction::EaseOutBack),
 W_ENUM_CONSTANT(WCurveFunction::EaseInOutBack),
 W_ENUM_CONSTANT(WCurveFunction::EaseInElastic),
 W_ENUM_CONSTANT(WCurveFunction::EaseOutElastic),
 W_ENUM_CONSTANT(WCurveFunction::EaseInOutElastic),
 W_ENUM_CONSTANT(WCurveFunction::EaseInBounce),
 W_ENUM_CONSTANT(WCurveFunction::EaseOutBounce),
 W_ENUM_CONSTANT(WCurveFunction::EaseInOutBounce),
 W_ENUM_CONSTANT(WCurveFunction::Conical),
 W_ENUM_CONSTANT(WCurveFunction::FadeInHoldFadeOut),
 W_ENUM_CONSTANT(WCurveFunction::FadeInFadeOut),
 W_ENUM_CONSTANT(WCurveFunction::Bell),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

W_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Math);
