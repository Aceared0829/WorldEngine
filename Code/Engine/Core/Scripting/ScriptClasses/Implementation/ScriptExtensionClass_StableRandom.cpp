#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_StableRandom.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptExtensionClass_StableRandom, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(IntMinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
    W_SCRIPT_FUNCTION_PROPERTY(FloatZeroToOne, Inout, "Position", In, "Seed"),
    W_SCRIPT_FUNCTION_PROPERTY(FloatMinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
    W_SCRIPT_FUNCTION_PROPERTY(Vec3MinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WScriptExtensionAttribute("StableRandom"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

// static
int WScriptExtensionClass_StableRandom::IntMinMax(int& inout_iPosition, int iMinValue, int iMaxValue, WUInt32 uiSeed)
{
  const WSimdVec4i result = WSimdVec4i::Truncate(WSimdRandom::FloatMinMax(WSimdVec4i(inout_iPosition), WSimdVec4f((float)iMinValue), WSimdVec4f((float)iMaxValue), WSimdVec4u(uiSeed)));
  ++inout_iPosition;
  return result.x();
}

// static
float WScriptExtensionClass_StableRandom::FloatZeroToOne(int& inout_iPosition, WUInt32 uiSeed)
{
  const WSimdVec4f result = WSimdRandom::FloatZeroToOne(WSimdVec4i(inout_iPosition), WSimdVec4u(uiSeed));
  ++inout_iPosition;
  return result.x();
}

// static
float WScriptExtensionClass_StableRandom::FloatMinMax(int& inout_iPosition, float fMinValue, float fMaxValue, WUInt32 uiSeed)
{
  const WSimdVec4f result = WSimdRandom::FloatMinMax(WSimdVec4i(inout_iPosition), WSimdVec4f(fMinValue), WSimdVec4f(fMaxValue), WSimdVec4u(uiSeed));
  ++inout_iPosition;
  return result.x();
}

// static
WVec3 WScriptExtensionClass_StableRandom::Vec3MinMax(int& inout_iPosition, const WVec3& vMinValue, const WVec3& vMaxValue, WUInt32 uiSeed)
{
  const WSimdVec4i offset(0, 1, 2, 3);
  const WSimdVec4f result = WSimdRandom::FloatMinMax(WSimdVec4i(inout_iPosition) + offset, WSimdConversion::ToVec3(vMinValue), WSimdConversion::ToVec3(vMaxValue), WSimdVec4u(uiSeed));
  inout_iPosition += 4;
  return WSimdConversion::ToVec3(result);
}


W_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_StableRandom);
