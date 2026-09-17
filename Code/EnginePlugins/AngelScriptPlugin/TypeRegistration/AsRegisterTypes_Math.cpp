#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>

//////////////////////////////////////////////////////////////////////////
// WMath
//////////////////////////////////////////////////////////////////////////

void WAngelScriptEngineSingleton::Register_Math()
{
  // static functions
  m_pEngine->SetDefaultNamespace("WMath");

  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsNaN(float value)", asFUNCTION(WMath::IsNaN<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsNaN(double value)", asFUNCTION(WMath::IsNaN<double>), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsFinite(float value)", asFUNCTION(WMath::IsFinite<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsFinite(double value)", asFUNCTION(WMath::IsFinite<double>), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Sin(WAngle a)", asFUNCTION(WMath::Sin<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Cos(WAngle a)", asFUNCTION(WMath::Cos<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Tan(WAngle a)", asFUNCTION(WMath::Tan<float>), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("WAngle ASin(float f)", asFUNCTION(WMath::ASin<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WAngle ACos(float f)", asFUNCTION(WMath::ACos<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WAngle ATan(float f)", asFUNCTION(WMath::ATan<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WAngle ATan2(float x, float y)", asFUNCTION(WMath::ATan2<float>), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Exp(float f)", asFUNCTIONPR(WMath::Exp, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Ln(float f)", asFUNCTIONPR(WMath::Ln, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Log2(float f)", asFUNCTIONPR(WMath::Log2, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("uint32 Log2i(uint32 uiVal)", asFUNCTIONPR(WMath::Log2i, (WUInt32), WUInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Log10(float f)", asFUNCTIONPR(WMath::Log10, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Log(float fBase, float f)", asFUNCTIONPR(WMath::Log, (float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Pow2(float f)", asFUNCTIONPR(WMath::Pow2, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Pow(float fBase, float fExp)", asFUNCTIONPR(WMath::Pow, (float, float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WInt32 Pow2(WInt32 i)", asFUNCTIONPR(WMath::Pow2, (WInt32), WInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WInt32 Pow(WInt32 iBase, WInt32 iExp)", asFUNCTIONPR(WMath::Pow, (WInt32, WInt32), WInt32), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Sqrt(float f)", asFUNCTIONPR(WMath::Sqrt, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("double Sqrt(double f)", asFUNCTIONPR(WMath::Sqrt, (double), double), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Sign(float f)", asFUNCTION(WMath::Sign<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WInt32 Sign(WInt32 f)", asFUNCTION(WMath::Sign<WInt32>), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Abs(float f)", asFUNCTION(WMath::Abs<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WInt32 Abs(WInt32 f)", asFUNCTION(WMath::Abs<WInt32>), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("WInt32 Min(WInt32 f1, WInt32 f2)", asFUNCTIONPR(WMath::Min, (WInt32, WInt32), WInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Min(float f1, float f2)", asFUNCTIONPR(WMath::Min, (float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("WInt32 Max(WInt32 f1, WInt32 f2)", asFUNCTIONPR(WMath::Max, (WInt32, WInt32), WInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Max(float f1, float f2)", asFUNCTIONPR(WMath::Max, (float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("WInt32 Clamp(WInt32 val, WInt32 min, WInt32 max)", asFUNCTIONPR(WMath::Clamp, (WInt32, WInt32, WInt32), WInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Clamp(float val, float min, float max)", asFUNCTIONPR(WMath::Clamp, (float, float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Floor(float f)", asFUNCTIONPR(WMath::Floor, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Ceil(float f)", asFUNCTIONPR(WMath::Ceil, (float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("WInt32 FloorToInt(float f)", asFUNCTIONPR(WMath::FloorToInt, (float), WInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WInt32 CeilToInt(float f)", asFUNCTIONPR(WMath::CeilToInt, (float), WInt32), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Lerp(float from, float to, float factor)", asFUNCTIONPR(WMath::Lerp, (float, float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec2 Lerp(WVec2 from, WVec2 to, float factor)", asFUNCTIONPR(WMath::Lerp, (WVec2, WVec2, float), WVec2), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 Lerp(WVec3 from, WVec3 to, float factor)", asFUNCTIONPR(WMath::Lerp, (WVec3, WVec3, float), WVec3), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec4 Lerp(WVec4 from, WVec4 to, float factor)", asFUNCTIONPR(WMath::Lerp, (WVec4, WVec4, float), WVec4), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WColor Lerp(WColor from, WColor to, float factor)", asFUNCTIONPR(WMath::Lerp, (WColor, WColor, float), WColor), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Unlerp(float from, float to, float value)", asFUNCTIONPR(WMath::Unlerp, (float, float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsEqual(float lhs, float rhs, float fEpsilon)", asFUNCTIONPR(WMath::IsEqual, (float, float, float), bool), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsZero(float value, float fEpsilon)", asFUNCTIONPR(WMath::IsZero, (float, float), bool), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsInRange(float value, float min, float max)", asFUNCTIONPR(WMath::IsInRange, (float, float, float), bool), asCALL_CDECL));

  // TODO AngelScript: finish WMath registration

  /* not exposed yet:

  WUInt32 WrapUInt(WUInt32 uiValue, WUInt32 uiExcludedMaxValue);
  WInt32 WrapInt(WInt32 iValue, WUInt32 uiExcludedMaxValue);
  WInt32 WrapInt(WInt32 iValue, WInt32 iMinValue, WInt32 iExcludedMaxValue);
  float WrapFloat01(float fValue);
  float WrapFloat(float fValue, float fMinValue, float fMaxValue);

  T Saturate(T value);

  WInt32 FloatToInt(float value);

  float Round(float f);
  WInt32 RoundToInt(float f);
  double Round(double f);
  float RoundToMultiple(float f, float fMultiple);
  double RoundToMultiple(double f, double fMultiple);

  Type Fraction(Type f);

  float Mod(float value, float fDiv);
  double Mod(double f, double fDiv);

  float RoundDown(float f, float fMultiple);
  double RoundDown(double f, double fMultiple);
  float RoundUp(float f, float fMultiple);
  double RoundUp(double f, double fMultiple);

  WInt32 RoundUp(WInt32 value, WUInt16 uiMultiple);
  WInt32 RoundDown(WInt32 value, WUInt16 uiMultiple);
  WUInt32 RoundUp(WUInt32 value, WUInt16 uiMultiple);
  WUInt32 RoundDown(WUInt32 value, WUInt16 uiMultiple);

  bool IsOdd(WInt32 i);
  bool IsEven(WInt32 i);

  T Step(T value, T edge);
  Type SmoothStep(Type value, Type edge1, Type edge2);
  Type SmootherStep(Type value, Type edge1, Type edge2);

  bool IsPowerOf(WInt32 value, WInt32 iBase);
  bool IsPowerOf2(WInt32 value);
  bool IsPowerOf2(WUInt32 value);
  bool IsPowerOf2(WUInt64 value);

  WUInt32 PowerOfTwo_Floor(WUInt32 value);
  WUInt64 PowerOfTwo_Floor(WUInt64 value);
  WUInt32 PowerOfTwo_Ceil(WUInt32 value);
  WUInt64 PowerOfTwo_Ceil(WUInt64 value);

  WUInt32 GreatestCommonDivisor(WUInt32 a, WUInt32 b);

  WUInt32 ColorFloatToUnsignedInt(float value);
  WUInt8 ColorFloatToByte(float value);
  WUInt16 ColorFloatToShort(float value);
  WInt8 ColorFloatToSignedByte(float value);
  WInt16 ColorFloatToSignedShort(float value);

  float ColorByteToFloat(WUInt8 value);
  float ColorShortToFloat(WUInt16 value);
  float ColorSignedByteToFloat(WInt8 value);
  float ColorSignedShortToFloat(WInt16 value);
  */

  m_pEngine->SetDefaultNamespace("");
}

//////////////////////////////////////////////////////////////////////////
// WAngle
//////////////////////////////////////////////////////////////////////////

static int WAngle_opCmp(const WAngle& lhs, const WAngle& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

void WAngelScriptEngineSingleton::Register_Angle()
{
  // static functions
  {
    m_pEngine->SetDefaultNamespace("WAngle");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("float DegToRad(float fDegree)", asFUNCTION(WAngleTemplate<float>::DegToRad), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("float RadToDeg(float fRadians)", asFUNCTION(WAngleTemplate<float>::RadToDeg), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WAngle MakeZero()", asFUNCTION(WAngle::MakeZero), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WAngle MakeFromDegree(float fDegree)", asFUNCTION(WAngle::MakeFromDegree), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WAngle MakeFromRadian(float fRadians)", asFUNCTION(WAngle::MakeFromRadian), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WAngle AngleBetween(WAngle a1, WAngle a2)", asFUNCTION(WAngle::AngleBetween), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "float GetDegree() const", asMETHOD(WAngle, GetDegree), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "float GetRadian() const", asMETHOD(WAngle, GetRadian), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "void SetRadian(float fRadians)", asMETHOD(WAngle, SetRadian), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "void NormalizeRange()", asMETHOD(WAngle, NormalizeRange), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "WAngle GetNormalizedRange() const", asMETHOD(WAngle, GetNormalizedRange), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "bool IsEqualSimple(WAngle rhs, WAngle epsilon) const", asMETHOD(WAngle, IsEqualSimple), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "bool IsEqualNormalized(WAngle rhs, WAngle epsilon) const", asMETHOD(WAngle, IsEqualNormalized), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "WAngle opNeg() const", asMETHODPR(WAngle, operator-, () const, WAngle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "WAngle opAdd(WAngle) const", asMETHODPR(WAngle, operator+, (WAngle) const, WAngle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "WAngle opSub(WAngle) const", asMETHODPR(WAngle, operator-, (WAngle) const, WAngle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "void opAddAssign(WAngle)", asMETHOD(WAngle, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "void opSubAssign(WAngle)", asMETHOD(WAngle, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "bool opEquals(const WAngle& in) const", asMETHODPR(WAngle, operator==, (const WAngle&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "int opCmp(const WAngle& in) const", asFUNCTION(WAngle_opCmp), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "WAngle opMul(float) const", asFUNCTIONPR(operator*, (const WAngle&, float), WAngle), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "WAngle opMul_r(float) const", asFUNCTIONPR(operator*, (const WAngle&, float), WAngle), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "WAngle opDiv(float) const", asFUNCTIONPR(operator/, (const WAngle&, float), WAngle), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WAngle", "float opDiv(const WAngle& in) const", asFUNCTIONPR(operator/, (const WAngle&, const WAngle&), float), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// WVec2
//////////////////////////////////////////////////////////////////////////

static int WVec2_opCmp(const WVec2& lhs, const WVec2& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

static void WVec2_Construct1(void* pMemory, float fXyz)
{
  new (pMemory) WVec2(fXyz);
}

static void WVec2_Construct2(void* pMemory, float x, float y)
{
  new (pMemory) WVec2(x, y);
}

void WAngelScriptEngineSingleton::Register_Vec2()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("WVec2", "float x", asOFFSET(WVec2, x)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WVec2", "float y", asOFFSET(WVec2, y)));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("WVec2");
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec2 MakeNaN()", asFUNCTION(WVec2::MakeNaN<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec2 MakeZero()", asFUNCTION(WVec2::MakeZero), asCALL_CDECL));
    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec3 GetAsVec3(float z) const", asMETHOD(WVec2, GetAsVec3), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec4 GetAsVec4(float z, float w) const", asMETHOD(WVec2, GetAsVec4), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "void Set(float xyz)", asMETHODPR(WVec2, Set, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "void Set(float x, float y)", asMETHODPR(WVec2, Set, (float, float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "void SetZero()", asMETHOD(WVec2, SetZero), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "float GetLength() const", asMETHODPR(WVec2, GetLength, () const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "float GetDistanceTo(const WVec2& in rhs) const", asMETHODPR(WVec2, GetDistanceTo, (const WVec2&) const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "float GetSquaredDistanceTo(const WVec2& in rhs) const", asMETHODPR(WVec2, GetSquaredDistanceTo, (const WVec2&) const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "float GetLengthSquared() const", asMETHOD(WVec2, GetLengthSquared), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "float GetLengthAndNormalize()", asMETHODPR(WVec2, GetLengthAndNormalize, (), float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 GetNormalized() const", asMETHODPR(WVec2, GetNormalized, () const, const WVec2), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "void Normalize()", asMETHODPR(WVec2, Normalize, (), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "bool IsZero() const", asMETHODPR(WVec2, IsZero, () const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "bool IsZero(float fEpsilon) const", asMETHODPR(WVec2, IsZero, (float) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "bool IsNormalized(float fEpsilon = 0.001f) const", asMETHODPR(WVec2, IsNormalized, (float) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "bool IsNaN() const", asMETHOD(WVec2, IsNaN), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "bool IsValid() const", asMETHOD(WVec2, IsValid), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "void opAddAssign(const WVec2& in)", asMETHOD(WVec2, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "void opSubAssign(const WVec2& in)", asMETHOD(WVec2, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "void opMulAssign(float)", asMETHODPR(WVec2, operator*=, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "void opDivAssign(float)", asMETHODPR(WVec2, operator/=, (float), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "bool IsIdentical(const WVec2& in) const", asMETHOD(WVec2, IsIdentical), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "bool IsEqual(const WVec2& in, float fEpsilon) const", asMETHOD(WVec2, IsEqual), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "float Dot(const WVec2& in) const", asMETHOD(WVec2, Dot), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 CompMin(const WVec2& in rhs) const", asMETHOD(WVec2, CompMin), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 CompMax(const WVec2& in rhs) const", asMETHOD(WVec2, CompMax), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 CompClamp(const WVec2& in rhs) const", asMETHOD(WVec2, CompClamp), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 CompMul(const WVec2& in rhs) const", asMETHOD(WVec2, CompMul), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 CompDiv(const WVec2& in rhs) const", asMETHOD(WVec2, CompDiv), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 Abs() const", asMETHOD(WVec2, Abs), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "void MakeOrthogonalTo(const WVec2& in)", asMETHODPR(WVec2, MakeOrthogonalTo, (const WVec2&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 GetOrthogonalVector() const", asMETHOD(WVec2, GetOrthogonalVector), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 GetReflectedVector(const WVec2& in) const", asMETHODPR(WVec2, GetReflectedVector, (const WVec2&) const, const WVec2), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 opNeg() const", asMETHODPR(WVec2, operator-, () const, const WVec2), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 opAdd(const WVec2& in) const", asFUNCTIONPR(operator+, (const WVec2&, const WVec2&), const WVec2), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 opSub(const WVec2& in) const", asFUNCTIONPR(operator-, (const WVec2&, const WVec2&), const WVec2), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 opMul(float) const", asFUNCTIONPR(operator*, (const WVec2&, float), const WVec2), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 opMul_r(float) const", asFUNCTIONPR(operator*, (float, const WVec2&), const WVec2), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "WVec2 opDiv(float) const", asFUNCTIONPR(operator/, (const WVec2&, float), const WVec2), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "bool opEquals(const WVec2& in) const", asFUNCTIONPR(operator==, (const WVec2&, const WVec2&), bool), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec2", "int opCmp(const WVec2& in) const", asFUNCTIONPR(WVec2_opCmp, (const WVec2&, const WVec2&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WVec2", asBEHAVE_CONSTRUCT, "void f(float x, float y)", asFUNCTION(WVec2_Construct2), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WVec2", asBEHAVE_CONSTRUCT, "void f(float xyz)", asFUNCTION(WVec2_Construct1), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// WVec3
//////////////////////////////////////////////////////////////////////////

static int WVec3_opCmp(const WVec3& lhs, const WVec3& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

static void WVec3_Construct1(void* pMemory, float fXyz)
{
  new (pMemory) WVec3(fXyz);
}
static void WVec3_Construct3(void* pMemory, float x, float y, float z)
{
  new (pMemory) WVec3(x, y, z);
}

void WAngelScriptEngineSingleton::Register_Vec3()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("WVec3", "float x", asOFFSET(WVec3, x)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WVec3", "float y", asOFFSET(WVec3, y)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WVec3", "float z", asOFFSET(WVec3, z)));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("WVec3");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeNaN()", asFUNCTION(WVec3::MakeNaN<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeZero()", asFUNCTION(WVec3::MakeZero), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeAxisX()", asFUNCTION(WVec3::MakeAxisX), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeAxisY()", asFUNCTION(WVec3::MakeAxisY), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeAxisZ()", asFUNCTION(WVec3::MakeAxisZ), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 Make(float x, float y, float z)", asFUNCTION(WVec3::Make), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeRandomDirection(WRandom& inout rng)", asFUNCTION(WVec3::MakeRandomDirection<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeRandomPointInSphere(WRandom& inout rng)", asFUNCTION(WVec3::MakeRandomPointInSphere<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeRandomDeviationX(WRandom& inout rng, const WAngle& in maxDeviation)", asFUNCTION(WVec3::MakeRandomDeviationX<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeRandomDeviationY(WRandom& inout rng, const WAngle& in maxDeviation)", asFUNCTION(WVec3::MakeRandomDeviationY<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeRandomDeviationZ(WRandom& inout rng, const WAngle& in maxDeviation)", asFUNCTION(WVec3::MakeRandomDeviationZ<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec3 MakeRandomDeviation(WRandom& inout rng, const WAngle& in maxDeviation, const WVec3& in normal)", asFUNCTION(WVec3::MakeRandomDeviation<float>), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec2 GetAsVec2() const", asMETHOD(WVec3, GetAsVec2), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec4 GetAsVec4(float w) const", asMETHOD(WVec3, GetAsVec4), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec4 GetAsPositionVec4() const", asMETHOD(WVec3, GetAsPositionVec4), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec4 GetAsDirectionVec4() const", asMETHOD(WVec3, GetAsDirectionVec4), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void Set(float xyz)", asMETHODPR(WVec3, Set, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void Set(float x, float y, float z)", asMETHODPR(WVec3, Set, (float, float, float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void SetZero()", asMETHOD(WVec3, SetZero), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "float GetLength() const", asMETHODPR(WVec3, GetLength, () const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "float GetDistanceTo(const WVec3& in rhs) const", asMETHODPR(WVec3, GetDistanceTo, (const WVec3&) const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "float GetSquaredDistanceTo(const WVec3& in rhs) const", asMETHODPR(WVec3, GetSquaredDistanceTo, (const WVec3&) const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "float GetLengthSquared() const", asMETHOD(WVec3, GetLengthSquared), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "float GetLengthAndNormalize()", asMETHODPR(WVec3, GetLengthAndNormalize, (), float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 GetNormalized() const", asMETHODPR(WVec3, GetNormalized, () const, const WVec3), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void Normalize()", asMETHODPR(WVec3, Normalize, (), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "bool IsZero() const", asMETHODPR(WVec3, IsZero, () const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "bool IsZero(float fEpsilon) const", asMETHODPR(WVec3, IsZero, (float) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "bool IsNormalized(float fEpsilon = 0.001f) const", asMETHODPR(WVec3, IsNormalized, (float) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "bool IsNaN() const", asMETHOD(WVec3, IsNaN), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "bool IsValid() const", asMETHOD(WVec3, IsValid), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void opAddAssign(const WVec3& in)", asMETHOD(WVec3, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void opSubAssign(const WVec3& in)", asMETHOD(WVec3, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void opMulAssign(const WVec3& in)", asMETHODPR(WVec3, operator*=, (const WVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void opDivAssign(const WVec3& in)", asMETHODPR(WVec3, operator/=, (const WVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void opMulAssign(float)", asMETHODPR(WVec3, operator*=, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void opDivAssign(float)", asMETHODPR(WVec3, operator/=, (float), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "bool IsIdentical(const WVec3& in) const", asMETHOD(WVec3, IsIdentical), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "bool IsEqual(const WVec3& in, float fEpsilon) const", asMETHOD(WVec3, IsEqual), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "float Dot(const WVec3& in) const", asMETHOD(WVec3, Dot), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 CrossRH(const WVec3& in) const", asMETHOD(WVec3, CrossRH), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 CompMin(const WVec3& in) const", asMETHOD(WVec3, CompMin), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 CompMax(const WVec3& in) const", asMETHOD(WVec3, CompMax), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 CompClamp(const WVec3& in) const", asMETHOD(WVec3, CompClamp), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 CompMul(const WVec3& in) const", asMETHOD(WVec3, CompMul), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 CompDiv(const WVec3& in) const", asMETHOD(WVec3, CompDiv), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 Abs() const", asMETHOD(WVec3, Abs), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "void MakeOrthogonalTo(const WVec3& in)", asMETHODPR(WVec3, MakeOrthogonalTo, (const WVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 GetOrthogonalVector() const", asMETHODPR(WVec3, GetOrthogonalVector, () const, const WVec3), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 GetReflectedVector(const WVec3& in) const", asMETHODPR(WVec3, GetReflectedVector, (const WVec3&) const, const WVec3), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 opNeg() const", asMETHODPR(WVec3, operator-, () const, const WVec3), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 opAdd(const WVec3& in) const", asFUNCTIONPR(operator+, (const WVec3&, const WVec3&), const WVec3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 opSub(const WVec3& in) const", asFUNCTIONPR(operator-, (const WVec3&, const WVec3&), const WVec3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 opMul(float) const", asFUNCTIONPR(operator*, (const WVec3&, float), const WVec3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 opMul_r(float) const", asFUNCTIONPR(operator*, (float, const WVec3&), const WVec3), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "WVec3 opDiv(float) const", asFUNCTIONPR(operator/, (const WVec3&, float), const WVec3), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "bool opEquals(const WVec3& in) const", asFUNCTIONPR(operator==, (const WVec3&, const WVec3&), bool), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec3", "int opCmp(const WVec3& in) const", asFUNCTIONPR(WVec3_opCmp, (const WVec3&, const WVec3&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WVec3", asBEHAVE_CONSTRUCT, "void f(float x, float y, float z)", asFUNCTION(WVec3_Construct3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WVec3", asBEHAVE_CONSTRUCT, "void f(float xyz)", asFUNCTION(WVec3_Construct1), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// WVec4
//////////////////////////////////////////////////////////////////////////

static int WVec4_opCmp(const WVec4& lhs, const WVec4& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

static void WVec4_Construct1(void* pMemory, float fXyzw)
{
  new (pMemory) WVec4(fXyzw);
}
static void WVec4_Construct4(void* pMemory, float x, float y, float z, float w)
{
  new (pMemory) WVec4(x, y, z, w);
}

void WAngelScriptEngineSingleton::Register_Vec4()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("WVec4", "float x", asOFFSET(WVec4, x)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WVec4", "float y", asOFFSET(WVec4, y)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WVec4", "float z", asOFFSET(WVec4, z)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WVec4", "float w", asOFFSET(WVec4, w)));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("WVec4");
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec4 MakeNaN()", asFUNCTION(WVec4::MakeNaN<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WVec4 MakeZero()", asFUNCTION(WVec4::MakeZero), asCALL_CDECL));
    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec2 GetAsVec2() const", asMETHOD(WVec4, GetAsVec2), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec3 GetAsVec3() const", asMETHOD(WVec4, GetAsVec3), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "void Set(float xyzw)", asMETHODPR(WVec4, Set, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "void Set(float x, float y, float z, float w)", asMETHODPR(WVec4, Set, (float, float, float, float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "void SetZero()", asMETHOD(WVec4, SetZero), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "float GetLength() const", asMETHODPR(WVec4, GetLength, () const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "float GetLengthSquared() const", asMETHOD(WVec4, GetLengthSquared), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "float GetLengthAndNormalize()", asMETHODPR(WVec4, GetLengthAndNormalize, (), float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 GetNormalized() const", asMETHODPR(WVec4, GetNormalized, () const, const WVec4), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "void Normalize()", asMETHODPR(WVec4, Normalize, (), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "bool IsZero() const", asMETHODPR(WVec4, IsZero, () const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "bool IsZero(float fEpsilon) const", asMETHODPR(WVec4, IsZero, (float) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "bool IsNormalized(float fEpsilon = 0.001f) const", asMETHODPR(WVec4, IsNormalized, (float) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "bool IsNaN() const", asMETHOD(WVec4, IsNaN), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "bool IsValid() const", asMETHOD(WVec4, IsValid), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "void opAddAssign(const WVec4& in)", asMETHOD(WVec4, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "void opSubAssign(const WVec4& in)", asMETHOD(WVec4, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "void opMulAssign(float)", asMETHODPR(WVec4, operator*=, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "void opDivAssign(float)", asMETHODPR(WVec4, operator/=, (float), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "bool IsIdentical(const WVec4& in) const", asMETHOD(WVec4, IsIdentical), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "bool IsEqual(const WVec4& in, float) const", asMETHOD(WVec4, IsEqual), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "float Dot(const WVec4& in) const", asMETHOD(WVec4, Dot), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 CompMin(const WVec4& in) const", asMETHOD(WVec4, CompMin), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 CompMax(const WVec4& in) const", asMETHOD(WVec4, CompMax), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 CompClamp(const WVec4& in) const", asMETHOD(WVec4, CompClamp), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 CompMul(const WVec4& in) const", asMETHOD(WVec4, CompMul), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 CompDiv(const WVec4& in) const", asMETHOD(WVec4, CompDiv), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 Abs() const", asMETHOD(WVec4, Abs), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 opNeg() const", asMETHODPR(WVec4, operator-, () const, const WVec4), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 opAdd(const WVec4& in) const", asFUNCTIONPR(operator+, (const WVec4&, const WVec4&), const WVec4), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 opSub(const WVec4& in) const", asFUNCTIONPR(operator-, (const WVec4&, const WVec4&), const WVec4), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 opMul(float) const", asFUNCTIONPR(operator*, (const WVec4&, float), const WVec4), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 opMul_r(float) const", asFUNCTIONPR(operator*, (float, const WVec4&), const WVec4), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "WVec4 opDiv(float) const", asFUNCTIONPR(operator/, (const WVec4&, float), const WVec4), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "bool opEquals(const WVec4& in) const", asFUNCTIONPR(operator==, (const WVec4&, const WVec4&), bool), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WVec4", "int opCmp(const WVec4& in) const", asFUNCTIONPR(WVec4_opCmp, (const WVec4&, const WVec4&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WVec4", asBEHAVE_CONSTRUCT, "void f(float x, float y, float z, float w)", asFUNCTION(WVec4_Construct4), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WVec4", asBEHAVE_CONSTRUCT, "void f(float xyzw)", asFUNCTION(WVec4_Construct1), asCALL_CDECL_OBJFIRST));
}

//////////////////////////////////////////////////////////////////////////
// WQuat
//////////////////////////////////////////////////////////////////////////

void WAngelScriptEngineSingleton::Register_Quat()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("WQuat", "float x", asOFFSET(WQuat, x)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WQuat", "float y", asOFFSET(WQuat, y)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WQuat", "float z", asOFFSET(WQuat, z)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WQuat", "float w", asOFFSET(WQuat, w)));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("WQuat");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WQuat MakeIdentity()", asFUNCTION(WQuat::MakeIdentity), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WQuat MakeFromElements(float x, float y, float z, float w)", asFUNCTION(WQuat::MakeFromElements), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WQuat MakeFromAxisAndAngle(const WVec3& in vAxis , WAngle angle)", asFUNCTION(WQuat::MakeFromAxisAndAngle), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WQuat MakeShortestRotation(const WVec3& in vFrom, const WVec3& in vTo)", asFUNCTION(WQuat::MakeShortestRotation), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WQuat MakeFromMat3(const WMat3& in)", asFUNCTION(WQuat::MakeFromMat3), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WQuat MakeSlerp(const WQuat& in qFrom, const WQuat& in qTo, float fFactor)", asFUNCTION(WQuat::MakeSlerp), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WQuat MakeFromEulerAngles(const WAngle& in x, const WAngle& in y, const WAngle& in z)", asFUNCTION(WQuat::MakeFromEulerAngles), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "void SetIdentity()", asMETHOD(WQuat, SetIdentity), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "void ReconstructFromMat3(const WMat3& in)", asMETHOD(WQuat, ReconstructFromMat3), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "void ReconstructFromMat4(const WMat3& in)", asMETHOD(WQuat, ReconstructFromMat4), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "void Normalize()", asMETHOD(WQuat, Normalize), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "void GetRotationAxisAndAngle(WVec3& out, WAngle& out, float fEpsilon = 0.00001) const", asMETHOD(WQuat, GetRotationAxisAndAngle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "WVec3 GetVectorPart() const", asMETHOD(WQuat, GetVectorPart), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "WMat3 GetAsMat3() const", asMETHOD(WQuat, GetAsMat3), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "WMat3 GetAsMat4() const", asMETHOD(WQuat, GetAsMat4), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "bool IsValid(float fEpsilon = 0.00001) const", asMETHOD(WQuat, IsValid), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "bool IsNaN() const", asMETHOD(WQuat, IsNaN), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "bool IsEqualRotation(const WQuat& in, float fEpsilon = 0.00001) const", asMETHOD(WQuat, IsEqualRotation), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "void Invert()", asMETHOD(WQuat, Invert), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "WQuat GetInverse() const", asMETHOD(WQuat, GetInverse), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "WQuat GetNegated() const", asMETHOD(WQuat, GetNegated), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "float Dot(const WQuat& in) const", asMETHOD(WQuat, Dot), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "WVec3 Rotate(const WVec3& in) const", asMETHOD(WQuat, Rotate), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "void GetAsEulerAngles(float& out, float& out, float& out) const", asMETHOD(WQuat, GetAsEulerAngles), asCALL_THISCALL));


  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "WQuat opMul(const WQuat& in) const", asFUNCTIONPR(operator*, (const WQuat&, const WQuat&), const WQuat), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "WVec3 opMul(const WVec3& in) const", asFUNCTIONPR(operator*, (const WQuat&, const WVec3&), const WVec3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WQuat", "bool opEquals(const WQuat& in) const", asFUNCTIONPR(operator==, (const WQuat&, const WQuat&), bool), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// WTransform
//////////////////////////////////////////////////////////////////////////

static void WTransform_Construct3(void* pMemory, const WVec3& v, const WQuat& r, const WVec3& s)
{
  new (pMemory) WTransform(v, r, s);
}

void WAngelScriptEngineSingleton::Register_Transform()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("WTransform", "WVec3 m_vPosition", asOFFSET(WTransform, m_vPosition)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WTransform", "WQuat m_qRotation", asOFFSET(WTransform, m_qRotation)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WTransform", "WVec3 m_vScale", asOFFSET(WTransform, m_vScale)));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WTransform", asBEHAVE_CONSTRUCT, "void f(const WVec3& in vPosition, const WQuat& in qRotation = WQuat::MakeIdentity(), const WVec3& in vScale = WVec3(1))", asFUNCTION(WTransform_Construct3), asCALL_CDECL_OBJFIRST));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("WTransform");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTransform Make(const WVec3& in vPosition, const WQuat& in qRotation = WQuat::MakeIdentity(), const WVec3& in vScale = WVec3(1))", asFUNCTION(WTransform::Make), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTransform MakeIdentity()", asFUNCTION(WTransform::MakeIdentity), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTransform MakeFromMat4(const WMat4& in)", asFUNCTION(WTransform::MakeFromMat4), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTransform MakeLocalTransform(const WTransform& in, const WTransform& in)", asFUNCTION(WTransform::MakeLocalTransform), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTransform MakeGlobalTransform(const WTransform& in, const WTransform& in)", asFUNCTION(WTransform::MakeGlobalTransform), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "void SetIdentity()", asMETHOD(WTransform, SetIdentity), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "float GetMaxScale() const", asMETHOD(WTransform, GetMaxScale), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "bool HasMirrorScaling() const", asMETHOD(WTransform, HasMirrorScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "bool HasOnlyUniformScaling() const", asMETHOD(WTransform, HasOnlyUniformScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "bool IsValid() const", asMETHOD(WTransform, IsValid), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "bool IsIdentical(const WTransform& in) const", asMETHOD(WTransform, IsIdentical), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "bool IsEqual(const WTransform& in, float fEpsilon) const", asMETHOD(WTransform, IsEqual), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "void Invert()", asMETHOD(WTransform, Invert), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "WTransform GetInverse() const", asMETHOD(WTransform, GetInverse), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "WVec3 TransformPosition(const WVec3& in vPosition) const", asMETHOD(WTransform, TransformPosition), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "WVec3 TransformDirection(const WVec3& in vDirection) const", asMETHOD(WTransform, TransformDirection), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "void opAddAssign(const WVec3& in)", asMETHOD(WTransform, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "void opSubAssign(const WVec3& in)", asMETHOD(WTransform, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "WMat4 GetAsMat4() const", asMETHOD(WTransform, GetAsMat4), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "WVec3 opMul(const WVec3& in) const", asFUNCTIONPR(operator*, (const WTransform&, const WVec3&), const WVec3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "WTransform opMul_r(const WQuat& in qRotation) const", asFUNCTIONPR(operator*, (const WQuat&, const WTransform&), const WTransform), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "WTransform opMul(const WQuat& in qRotation) const", asFUNCTIONPR(operator*, (const WTransform&, const WQuat&), const WTransform), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "WTransform opAdd(const WVec3& in) const", asFUNCTIONPR(operator+, (const WTransform&, const WVec3&), const WTransform), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "WTransform opSub(const WVec3& in) const", asFUNCTIONPR(operator-, (const WTransform&, const WVec3&), const WTransform), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "WTransform opMul(const WTransform& in) const", asFUNCTIONPR(operator*, (const WTransform&, const WTransform&), const WTransform), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTransform", "bool opEquals(const WTransform& in) const", asFUNCTIONPR(operator==, (const WTransform&, const WTransform&), bool), asCALL_CDECL_OBJFIRST));
}

//////////////////////////////////////////////////////////////////////////
// WMat3
//////////////////////////////////////////////////////////////////////////

void WAngelScriptEngineSingleton::Register_Mat3()
{
  // static functions
  {
    m_pEngine->SetDefaultNamespace("WMat3");
    m_pEngine->SetDefaultNamespace("");
  }

  // TODO AngelScript: Register WMat3
}

//////////////////////////////////////////////////////////////////////////
// WMat4
//////////////////////////////////////////////////////////////////////////

void WAngelScriptEngineSingleton::Register_Mat4()
{
  // static functions
  {
    m_pEngine->SetDefaultNamespace("WMat4");
    m_pEngine->SetDefaultNamespace("");
  }

  // TODO AngelScript: Register WMat4
}
