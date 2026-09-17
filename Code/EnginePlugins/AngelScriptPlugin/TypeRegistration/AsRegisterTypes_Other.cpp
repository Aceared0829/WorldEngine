#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/World/SpatialData.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Time/Clock.h>

//////////////////////////////////////////////////////////////////////////
// WRTTI
//////////////////////////////////////////////////////////////////////////

const WRTTI* WRTTI_GetType(WStringView sName)
{
  return WRTTI::FindTypeByName(sName);
}

void WAngelScriptEngineSingleton::Register_RTTI()
{
  // static functions
  {
    m_pEngine->SetDefaultNamespace("WRTTI");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("const WRTTI@ GetType(WStringView)", asFUNCTION(WRTTI_GetType), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }
}

//////////////////////////////////////////////////////////////////////////
// WTime
//////////////////////////////////////////////////////////////////////////

static int WTime_opCmp(const WTime& lhs, const WTime& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

void WAngelScriptEngineSingleton::Register_Time()
{
  // static functions
  {
    m_pEngine->SetDefaultNamespace("WTime");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime Now()", asFUNCTION(WTime::Now), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime MakeFromNanoseconds(double fNanoSeconds)", asFUNCTION(WTime::MakeFromNanoseconds), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime Nanoseconds(double fNanoSeconds)", asFUNCTION(WTime::Nanoseconds), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime MakeFromMicroseconds(double fMicroSeconds)", asFUNCTION(WTime::MakeFromMicroseconds), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime Microseconds(double fMicroSeconds)", asFUNCTION(WTime::Microseconds), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime MakeFromMilliseconds(double fMilliSeconds)", asFUNCTION(WTime::MakeFromMilliseconds), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime Milliseconds(double fMilliSeconds)", asFUNCTION(WTime::Milliseconds), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime MakeFromSeconds(double fSeconds)", asFUNCTION(WTime::MakeFromSeconds), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime Seconds(double fSeconds)", asFUNCTION(WTime::Seconds), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime MakeFromMinutes(double fMinutes)", asFUNCTION(WTime::MakeFromMinutes), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime Minutes(double fMinutes)", asFUNCTION(WTime::Minutes), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime MakeFromHours(double fHours)", asFUNCTION(WTime::MakeFromHours), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime Hours(double fHours)", asFUNCTION(WTime::Hours), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WTime MakeZero()", asFUNCTION(WTime::MakeZero), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "bool IsZero() const", asMETHOD(WTime, IsZero), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "bool IsNegative() const", asMETHOD(WTime, IsNegative), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "bool IsPositive() const", asMETHOD(WTime, IsPositive), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "bool IsZeroOrNegative() const", asMETHOD(WTime, IsZeroOrNegative), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "bool IsZeroOrPositive() const", asMETHOD(WTime, IsZeroOrPositive), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "float AsFloatInSeconds() const", asMETHOD(WTime, AsFloatInSeconds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "double GetNanoseconds() const", asMETHOD(WTime, GetNanoseconds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "double GetMicroseconds() const", asMETHOD(WTime, GetMicroseconds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "double GetMilliseconds() const", asMETHOD(WTime, GetMilliseconds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "double GetSeconds() const", asMETHOD(WTime, GetSeconds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "double GetMinutes() const", asMETHOD(WTime, GetMinutes), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "double GetHours() const", asMETHOD(WTime, GetHours), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "void opSubAssign(const WTime& in)", asMETHOD(WTime, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "void opAddAssign(const WTime& in)", asMETHOD(WTime, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "void opMulAssign(double)", asMETHOD(WTime, operator*=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "void opDivAssign(double)", asMETHOD(WTime, operator/=), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "WTime opSub(const WTime& in) const", asMETHODPR(WTime, operator-, (const WTime&) const, WTime), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "WTime opAdd(const WTime& in) const", asMETHODPR(WTime, operator+, (const WTime&) const, WTime), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "WTime opNeg() const", asMETHODPR(WTime, operator-, () const, WTime), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "int opCmp(const WTime& in) const", asFUNCTIONPR(WTime_opCmp, (const WTime&, const WTime&), int), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "bool opEquals(const WTime& in) const", asMETHODPR(WTime, operator==, (const WTime&) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "WTime opMul(double) const", asFUNCTIONPR(operator*, (const WTime&, double), WTime), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "WTime opMul_r(double) const", asFUNCTIONPR(operator*, (double, const WTime&), WTime), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "WTime opMul(const WTime& in) const", asFUNCTIONPR(operator*, (const WTime&, const WTime&), WTime), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "WTime opDiv(double) const", asFUNCTIONPR(operator/, (const WTime&, double), WTime), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "WTime opDiv_r(double) const", asFUNCTIONPR(operator/, (double, const WTime&), WTime), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTime", "WTime opDiv(const WTime& in) const", asFUNCTIONPR(operator/, (const WTime&, const WTime&), WTime), asCALL_CDECL_OBJFIRST));
}

//////////////////////////////////////////////////////////////////////////
// WClock
//////////////////////////////////////////////////////////////////////////

void WAngelScriptEngineSingleton::Register_Clock()
{
  AS_CHECK(m_pEngine->RegisterObjectMethod("WClock", "void SetPaused(bool)", asMETHOD(WClock, SetPaused), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WClock", "bool GetPaused() const", asMETHOD(WClock, GetPaused), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WClock", "WTime GetTimeDiff() const", asMETHOD(WClock, GetTimeDiff), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WClock", "void SetSpeed(double)", asMETHOD(WClock, SetSpeed), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WClock", "double GetSpeed() const", asMETHOD(WClock, GetSpeed), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WClock", "WTime GetAccumulatedTime() const", asMETHOD(WClock, GetAccumulatedTime), asCALL_THISCALL));
}


//////////////////////////////////////////////////////////////////////////
// WRandom
//////////////////////////////////////////////////////////////////////////

void WAngelScriptEngineSingleton::Register_Random()
{
  // Methods
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "uint32 UInt()", asMETHOD(WRandom, UInt), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "uint32 UIntInRange(uint32 uiRange)", asMETHOD(WRandom, UIntInRange), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "uint32 UInt32Index(WUInt32 uiArraySize, WUInt32 uiFallbackValue = 0xFFFFFFFF)", asMETHOD(WRandom, UInt32Index), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "uint16 UInt16Index(WUInt16 uiArraySize, WUInt16 uiFallbackValue = 0xFFFF)", asMETHOD(WRandom, UInt16Index), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "int32 IntMinMax(WInt32 iMinValue, WInt32 iMaxValue)", asMETHOD(WRandom, IntMinMax), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "bool Bool()", asMETHOD(WRandom, Bool), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "double DoubleZeroToOneExclusive()", asMETHOD(WRandom, DoubleZeroToOneExclusive), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "double DoubleZeroToOneInclusive()", asMETHOD(WRandom, DoubleZeroToOneInclusive), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "double DoubleMinMax(double fMinValue, double fMaxValue)", asMETHOD(WRandom, DoubleMinMax), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "double DoubleVariance(double fValue, double fVariance)", asMETHOD(WRandom, DoubleVariance), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "double DoubleVarianceAroundZero(double fAbsMaxValue)", asMETHOD(WRandom, DoubleVarianceAroundZero), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "float FloatZeroToOneExclusive()", asMETHOD(WRandom, FloatZeroToOneExclusive), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "float FloatZeroToOneInclusive()", asMETHOD(WRandom, FloatZeroToOneInclusive), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "float FloatMinMax(float fMinValue, float fMaxValue)", asMETHOD(WRandom, FloatMinMax), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "float FloatVariance(float fValue, float fVariance)", asMETHOD(WRandom, FloatVariance), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WRandom", "float FloatVarianceAroundZero(float fAbsMaxValue)", asMETHOD(WRandom, FloatVarianceAroundZero), asCALL_THISCALL));
  }
}

//////////////////////////////////////////////////////////////////////////
// WColor
//////////////////////////////////////////////////////////////////////////

static void WColor_ConstructRGBA(void* pMemory, float r, float g, float b, float a)
{
  new (pMemory) WColor(r, g, b, a);
}

static void WColor_ConstructGamma(void* pMemory, const WColorGammaUB& col)
{
  new (pMemory) WColor(col);
}

void WAngelScriptEngineSingleton::Register_Color()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("WColor", "float r", asOFFSET(WColor, r)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WColor", "float g", asOFFSET(WColor, g)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WColor", "float b", asOFFSET(WColor, b)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WColor", "float a", asOFFSET(WColor, a)));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("WColor");

    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor AliceBlue", (void*)&WColor::AliceBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor AntiqueWhite", (void*)&WColor::AntiqueWhite));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Aqua", (void*)&WColor::Aqua));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Aquamarine", (void*)&WColor::Aquamarine));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Azure", (void*)&WColor::Azure));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Beige", (void*)&WColor::Beige));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Bisque", (void*)&WColor::Bisque));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Black", (void*)&WColor::Black));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor BlanchedAlmond", (void*)&WColor::BlanchedAlmond));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Blue", (void*)&WColor::Blue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor BlueViolet", (void*)&WColor::BlueViolet));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Brown", (void*)&WColor::Brown));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor BurlyWood", (void*)&WColor::BurlyWood));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor CadetBlue", (void*)&WColor::CadetBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Chartreuse", (void*)&WColor::Chartreuse));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Chocolate", (void*)&WColor::Chocolate));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Coral", (void*)&WColor::Coral));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor CornflowerBlue", (void*)&WColor::CornflowerBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Cornsilk", (void*)&WColor::Cornsilk));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Crimson", (void*)&WColor::Crimson));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Cyan", (void*)&WColor::Cyan));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkBlue", (void*)&WColor::DarkBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkCyan", (void*)&WColor::DarkCyan));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkGoldenRod", (void*)&WColor::DarkGoldenRod));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkGray", (void*)&WColor::DarkGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkGrey", (void*)&WColor::DarkGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkGreen", (void*)&WColor::DarkGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkKhaki", (void*)&WColor::DarkKhaki));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkMagenta", (void*)&WColor::DarkMagenta));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkOliveGreen", (void*)&WColor::DarkOliveGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkOrange", (void*)&WColor::DarkOrange));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkOrchid", (void*)&WColor::DarkOrchid));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkRed", (void*)&WColor::DarkRed));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkSalmon", (void*)&WColor::DarkSalmon));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkSeaGreen", (void*)&WColor::DarkSeaGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkSlateBlue", (void*)&WColor::DarkSlateBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkSlateGray", (void*)&WColor::DarkSlateGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkSlateGrey", (void*)&WColor::DarkSlateGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkTurquoise", (void*)&WColor::DarkTurquoise));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DarkViolet", (void*)&WColor::DarkViolet));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DeepPink", (void*)&WColor::DeepPink));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DeepSkyBlue", (void*)&WColor::DeepSkyBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DimGray", (void*)&WColor::DimGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DimGrey", (void*)&WColor::DimGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor DodgerBlue", (void*)&WColor::DodgerBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor FireBrick", (void*)&WColor::FireBrick));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor FloralWhite", (void*)&WColor::FloralWhite));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor ForestGreen", (void*)&WColor::ForestGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Fuchsia", (void*)&WColor::Fuchsia));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Gainsboro", (void*)&WColor::Gainsboro));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor GhostWhite", (void*)&WColor::GhostWhite));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Gold", (void*)&WColor::Gold));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor GoldenRod", (void*)&WColor::GoldenRod));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Gray", (void*)&WColor::Gray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Grey", (void*)&WColor::Grey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Green", (void*)&WColor::Green));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor GreenYellow", (void*)&WColor::GreenYellow));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor HoneyDew", (void*)&WColor::HoneyDew));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor HotPink", (void*)&WColor::HotPink));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor IndianRed", (void*)&WColor::IndianRed));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Indigo", (void*)&WColor::Indigo));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Ivory", (void*)&WColor::Ivory));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Khaki", (void*)&WColor::Khaki));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Lavender", (void*)&WColor::Lavender));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LavenderBlush", (void*)&WColor::LavenderBlush));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LawnGreen", (void*)&WColor::LawnGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LemonChiffon", (void*)&WColor::LemonChiffon));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightBlue", (void*)&WColor::LightBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightCoral", (void*)&WColor::LightCoral));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightCyan", (void*)&WColor::LightCyan));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightGoldenRodYellow", (void*)&WColor::LightGoldenRodYellow));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightGray", (void*)&WColor::LightGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightGrey", (void*)&WColor::LightGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightGreen", (void*)&WColor::LightGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightPink", (void*)&WColor::LightPink));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightSalmon", (void*)&WColor::LightSalmon));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightSeaGreen", (void*)&WColor::LightSeaGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightSkyBlue", (void*)&WColor::LightSkyBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightSlateGray", (void*)&WColor::LightSlateGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightSlateGrey", (void*)&WColor::LightSlateGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightSteelBlue", (void*)&WColor::LightSteelBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LightYellow", (void*)&WColor::LightYellow));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Lime", (void*)&WColor::Lime));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor LimeGreen", (void*)&WColor::LimeGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Linen", (void*)&WColor::Linen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Magenta", (void*)&WColor::Magenta));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Maroon", (void*)&WColor::Maroon));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MediumAquaMarine", (void*)&WColor::MediumAquaMarine));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MediumBlue", (void*)&WColor::MediumBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MediumOrchid", (void*)&WColor::MediumOrchid));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MediumPurple", (void*)&WColor::MediumPurple));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MediumSeaGreen", (void*)&WColor::MediumSeaGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MediumSlateBlue", (void*)&WColor::MediumSlateBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MediumSpringGreen", (void*)&WColor::MediumSpringGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MediumTurquoise", (void*)&WColor::MediumTurquoise));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MediumVioletRed", (void*)&WColor::MediumVioletRed));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MidnightBlue", (void*)&WColor::MidnightBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MintCream", (void*)&WColor::MintCream));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor MistyRose", (void*)&WColor::MistyRose));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Moccasin", (void*)&WColor::Moccasin));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor NavajoWhite", (void*)&WColor::NavajoWhite));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Navy", (void*)&WColor::Navy));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor OldLace", (void*)&WColor::OldLace));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Olive", (void*)&WColor::Olive));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor OliveDrab", (void*)&WColor::OliveDrab));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Orange", (void*)&WColor::Orange));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor OrangeRed", (void*)&WColor::OrangeRed));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Orchid", (void*)&WColor::Orchid));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor PaleGoldenRod", (void*)&WColor::PaleGoldenRod));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor PaleGreen", (void*)&WColor::PaleGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor PaleTurquoise", (void*)&WColor::PaleTurquoise));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor PaleVioletRed", (void*)&WColor::PaleVioletRed));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor PapayaWhip", (void*)&WColor::PapayaWhip));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor PeachPuff", (void*)&WColor::PeachPuff));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Peru", (void*)&WColor::Peru));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Pink", (void*)&WColor::Pink));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Plum", (void*)&WColor::Plum));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor PowderBlue", (void*)&WColor::PowderBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Purple", (void*)&WColor::Purple));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor RebeccaPurple", (void*)&WColor::RebeccaPurple));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Red", (void*)&WColor::Red));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor RosyBrown", (void*)&WColor::RosyBrown));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor RoyalBlue", (void*)&WColor::RoyalBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor SaddleBrown", (void*)&WColor::SaddleBrown));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Salmon", (void*)&WColor::Salmon));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor SandyBrown", (void*)&WColor::SandyBrown));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor SeaGreen", (void*)&WColor::SeaGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor SeaShell", (void*)&WColor::SeaShell));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Sienna", (void*)&WColor::Sienna));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Silver", (void*)&WColor::Silver));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor SkyBlue", (void*)&WColor::SkyBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor SlateBlue", (void*)&WColor::SlateBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor SlateGray", (void*)&WColor::SlateGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor SlateGrey", (void*)&WColor::SlateGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Snow", (void*)&WColor::Snow));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor SpringGreen", (void*)&WColor::SpringGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor SteelBlue", (void*)&WColor::SteelBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Tan", (void*)&WColor::Tan));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Teal", (void*)&WColor::Teal));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Thistle", (void*)&WColor::Thistle));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Tomato", (void*)&WColor::Tomato));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Turquoise", (void*)&WColor::Turquoise));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Violet", (void*)&WColor::Violet));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Wheat", (void*)&WColor::Wheat));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor White", (void*)&WColor::White));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor WhiteSmoke", (void*)&WColor::WhiteSmoke));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor Yellow", (void*)&WColor::Yellow));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const WColor YellowGreen", (void*)&WColor::YellowGreen));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("WColor MakeNaN()", asFUNCTION(WColor::MakeNaN), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WColor MakeZero()", asFUNCTION(WColor::MakeZero), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WColor MakeRGBA(float r, float g, float b, float a)", asFUNCTION(WColor::MakeRGBA), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WColor MakeFromKelvin(WUInt32 uiKelvin)", asFUNCTION(WColor::MakeFromKelvin), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("WColor MakeHSV(float fHue, float fSat, float fVal)", asFUNCTION(WColor::MakeHSV), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  // Constructors
  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WColor", asBEHAVE_CONSTRUCT, "void f(float r, float g, float b, float a = 1.0f)", asFUNCTION(WColor_ConstructRGBA), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WColor", asBEHAVE_CONSTRUCT, "void f(const WColorGammaUB& in)", asFUNCTION(WColor_ConstructGamma), asCALL_CDECL_OBJFIRST));
  }

  // Operators
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void opAssign(const WColorGammaUB& in)", asMETHODPR(WColor, operator=, (const WColorGammaUB&), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void opAddAssign(const WColor& in)", asMETHODPR(WColor, operator+=, (const WColor&), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void opSubAssign(const WColor& in)", asMETHODPR(WColor, operator-=, (const WColor&), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void opMulAssign(const WColor& in)", asMETHODPR(WColor, operator*=, (const WColor&), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void opMulAssign(float)", asMETHODPR(WColor, operator*=, (float), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void opDivAssign(float)", asMETHODPR(WColor, operator/=, (float), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void opMulAssign(const WMat4& in)", asMETHODPR(WColor, operator*=, (const WMat4&), void), asCALL_THISCALL));


    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor opAdd(const WColor& in) const", asFUNCTIONPR(operator+, (const WColor&, const WColor&), const WColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor opSub(const WColor& in) const", asFUNCTIONPR(operator-, (const WColor&, const WColor&), const WColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor opMul(const WColor& in) const", asFUNCTIONPR(operator*, (const WColor&, const WColor&), const WColor), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor opMul(float) const", asFUNCTIONPR(operator*, (const WColor&, float), const WColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor opMul_r(float) const", asFUNCTIONPR(operator*, (const WColor&, float), const WColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor opDiv(float) const", asFUNCTIONPR(operator/, (const WColor&, float), const WColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor opMul_r(const WMat4& in) const", asFUNCTIONPR(operator*, (const WMat4&, const WColor&), const WColor), asCALL_CDECL_OBJLAST));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "bool opEquals(const WColor& in) const", asFUNCTIONPR(operator==, (const WColor&, const WColor&), bool), asCALL_CDECL_OBJFIRST));
  }

  // Methods
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void SetRGB(float r, float g, float b)", asMETHOD(WColor, SetRGB), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void SetRGBA(float r, float g, float b, float a = 1.0f)", asMETHOD(WColor, SetRGBA), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void GetHSV(float& out fHue, float& out fSaturation, float& out fValue) const", asMETHOD(WColor, GetHSV), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WVec4 GetAsVec4() const", asMETHOD(WColor, GetAsVec4), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "bool IsNormalized() const", asMETHOD(WColor, IsNormalized), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "float CalcAverageRGB() const", asMETHOD(WColor, CalcAverageRGB), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "float GetSaturation() const", asMETHOD(WColor, GetSaturation), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "float GetLuminance() const", asMETHOD(WColor, GetLuminance), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor GetInvertedColor() const", asMETHOD(WColor, GetInvertedColor), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor GetComplementaryColor() const", asMETHOD(WColor, GetComplementaryColor), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void ScaleRGB(float)", asMETHOD(WColor, ScaleRGB), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void ScaleRGBA(float)", asMETHOD(WColor, ScaleRGBA), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "float ComputeHdrMultiplier() const", asMETHOD(WColor, ComputeHdrMultiplier), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "float ComputeHdrExposureValue() const", asMETHOD(WColor, ComputeHdrExposureValue), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void ApplyHdrExposureValue(float fExposure)", asMETHOD(WColor, ApplyHdrExposureValue), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "void NormalizeToLdrRange()", asMETHOD(WColor, NormalizeToLdrRange), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor GetDarker(float fFactor = 2.0f) const", asMETHOD(WColor, GetDarker), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "bool IsNaN() const", asMETHOD(WColor, IsNaN), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "bool IsValid() const", asMETHOD(WColor, IsValid), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "bool IsIdenticalRGB(const WColor& in) const", asMETHOD(WColor, IsIdenticalRGB), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "bool IsIdenticalRGBA(const WColor& in) const", asMETHOD(WColor, IsIdenticalRGBA), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "bool IsEqualRGB(const WColor& in, float fEpsilon) const", asMETHOD(WColor, IsEqualRGB), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "bool IsEqualRGBA(const WColor& in, float fEpsilon) const", asMETHOD(WColor, IsEqualRGBA), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColor", "WColor WithAlpha(float fAlpha) const", asMETHOD(WColor, WithAlpha), asCALL_THISCALL));
  }
}

//////////////////////////////////////////////////////////////////////////
// WColorGammaUB
//////////////////////////////////////////////////////////////////////////

void WColorGamma_ConstructRGBA(void* pMemory, WUInt8 r, WUInt8 g, WUInt8 b, WUInt8 a)
{
  new (pMemory) WColorGammaUB(r, g, b, a);
}

void WColorGamma_ConstructColor(void* pMemory, const WColor& col)
{
  new (pMemory) WColorGammaUB(col);
}

void WAngelScriptEngineSingleton::Register_ColorGammaUB()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("WColorGammaUB", "uint8 r", asOFFSET(WColorGammaUB, r)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WColorGammaUB", "uint8 g", asOFFSET(WColorGammaUB, g)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WColorGammaUB", "uint8 b", asOFFSET(WColorGammaUB, b)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("WColorGammaUB", "uint8 a", asOFFSET(WColorGammaUB, a)));

  // Constructors
  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WColorGammaUB", asBEHAVE_CONSTRUCT, "void f(uint8 r, uint8 g, uint8 b, uint8 a = 255)", asFUNCTION(WColorGamma_ConstructRGBA), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WColorGammaUB", asBEHAVE_CONSTRUCT, "void f(const WColor& in)", asFUNCTION(WColorGamma_ConstructColor), asCALL_CDECL_OBJFIRST));
  }

  // Operators
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColorGammaUB", "void opAssign(const WColor& in)", asMETHODPR(WColorGammaUB, operator=, (const WColor&), void), asCALL_THISCALL));
  }

  // Methods
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("WColorGammaUB", "WColor ToLinearFloat() const", asMETHOD(WColorGammaUB, ToLinearFloat), asCALL_THISCALL));
  }
}

//////////////////////////////////////////////////////////////////////////
// WSpatial
//////////////////////////////////////////////////////////////////////////

void WSpatial_FindObjectsInSphere(WStringView sCategory, const WVec3& vCenter, float fRadius, asIScriptFunction* pCallback)
{
  WWorld* pWorld = WAngelScriptUtils::GetThreadLocalWorld();

  auto category = WSpatialData::FindCategory(sCategory);
  if (category != WInvalidSpatialDataCategory)
  {
    WSpatialSystem::QueryParams params;
    params.m_uiCategoryBitmask = category.GetBitmask();

    pWorld->GetSpatialSystem()->FindObjectsInSphere(WBoundingSphere::MakeFromCenterAndRadius(vCenter, fRadius), params, [pCallback](WGameObject* go) -> WVisitorExecution::Enum
      {
        asIScriptContext* pCtx = asGetActiveContext();
        pCtx->PushState();

        pCtx->Prepare(pCallback);
        pCtx->SetArgObject(0, go);
        pCtx->Execute();

        const WVisitorExecution::Enum res = (pCtx->GetReturnByte() != 0) ? WVisitorExecution::Continue : WVisitorExecution::Stop;

        pCtx->PopState();

        return res;
        //
      });
  }

  // need to release the refcount
  pCallback->Release();
}

void WAngelScriptEngineSingleton::Register_Spatial()
{
  AS_CHECK(m_pEngine->RegisterFuncdef("bool ReportObjectCB(WGameObject@)"));

  m_pEngine->SetDefaultNamespace("WSpatial");

  AS_CHECK(m_pEngine->RegisterGlobalFunction("void FindObjectsInSphere(WStringView sCategory, const WVec3& in vCenter, float fRadius, ReportObjectCB@ callback)", asFUNCTION(WSpatial_FindObjectsInSphere), asCALL_CDECL));

  m_pEngine->SetDefaultNamespace("");
}
