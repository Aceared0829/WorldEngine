#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Tracks/Curve1D.h>

using namespace WExpression;

namespace
{
  static const char* s_szRegisterTypeNames[] = {
    "Unknown",
    "Bool",
    "Int",
    "Float",
  };

  static_assert(W_ARRAY_SIZE(s_szRegisterTypeNames) == RegisterType::Count);

  static const char* s_szRegisterTypeNamesShort[] = {
    "U",
    "B",
    "I",
    "F",
  };

  static_assert(W_ARRAY_SIZE(s_szRegisterTypeNamesShort) == RegisterType::Count);

  static_assert(RegisterType::Count <= W_BIT(RegisterType::MaxNumBits));
} // namespace

// static
const char* RegisterType::GetName(Enum registerType)
{
  W_ASSERT_DEBUG(registerType >= 0 && static_cast<WUInt32>(registerType) < W_ARRAY_SIZE(s_szRegisterTypeNames), "Out of bounds access");
  return s_szRegisterTypeNames[registerType];
}

//////////////////////////////////////////////////////////////////////////

WResult StreamDesc::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  inout_stream << static_cast<WUInt8>(m_DataType);

  return W_SUCCESS;
}

WResult StreamDesc::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_sName;

  WUInt8 dataType = 0;
  inout_stream >> dataType;
  m_DataType = static_cast<WProcessingStream::DataType>(dataType);

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

bool FunctionDesc::operator<(const FunctionDesc& other) const
{
  if (m_sName != other.m_sName)
    return m_sName < other.m_sName;

  if (m_uiNumRequiredInputs != other.m_uiNumRequiredInputs)
    return m_uiNumRequiredInputs < other.m_uiNumRequiredInputs;

  if (m_OutputType != other.m_OutputType)
    return m_OutputType < other.m_OutputType;

  return m_InputTypes.GetArrayPtr() < other.m_InputTypes.GetArrayPtr();
}

WResult FunctionDesc::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  W_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_InputTypes));
  inout_stream << m_uiNumRequiredInputs;
  inout_stream << m_OutputType;

  return W_SUCCESS;
}

WResult FunctionDesc::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_sName;
  W_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_InputTypes));
  inout_stream >> m_uiNumRequiredInputs;
  inout_stream >> m_OutputType;

  return W_SUCCESS;
}

WHashedString FunctionDesc::GetMangledName() const
{
  WStringBuilder sMangledName = m_sName.GetView();
  sMangledName.Append("_");

  for (auto inputType : m_InputTypes)
  {
    sMangledName.Append(s_szRegisterTypeNamesShort[inputType]);
  }

  WHashedString sResult;
  sResult.Assign(sMangledName);
  return sResult;
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  static const WEnum<RegisterType> s_RandomInputTypes[] = {RegisterType::Int, RegisterType::Int};

  static void Random(Inputs inputs, Output output, const GlobalData& globalData)
  {
    W_IGNORE_UNUSED(globalData);

    const Register* pPositions = inputs[0].GetPtr();
    const Register* pPositionsEnd = inputs[0].GetEndPtr();
    Register* pOutput = output.GetPtr();

    if (inputs.GetCount() >= 2)
    {
      const Register* pSeeds = inputs[1].GetPtr();

      while (pPositions < pPositionsEnd)
      {
        pOutput->f = WSimdRandom::FloatZeroToOne(pPositions->i, WSimdVec4u(pSeeds->i));

        ++pPositions;
        ++pSeeds;
        ++pOutput;
      }
    }
    else
    {
      while (pPositions < pPositionsEnd)
      {
        pOutput->f = WSimdRandom::FloatZeroToOne(pPositions->i);

        ++pPositions;
        ++pOutput;
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////

  static WSimdPerlinNoise s_PerlinNoise(12345);
  static const WEnum<RegisterType> s_PerlinNoiseInputTypes[] = {
    RegisterType::Float,
    RegisterType::Float,
    RegisterType::Float,
    RegisterType::Int,
  };

  static void PerlinNoise(Inputs inputs, Output output, const GlobalData& globalData)
  {
    W_IGNORE_UNUSED(globalData);

    const Register* pPosX = inputs[0].GetPtr();
    const Register* pPosY = inputs[1].GetPtr();
    const Register* pPosZ = inputs[2].GetPtr();
    const Register* pPosXEnd = inputs[0].GetEndPtr();

    const WUInt32 uiNumOctaves = (inputs.GetCount() >= 4) ? inputs[3][0].i.x() : 1;

    Register* pOutput = output.GetPtr();

    while (pPosX < pPosXEnd)
    {
      pOutput->f = s_PerlinNoise.NoiseZeroToOne(pPosX->f, pPosY->f, pPosZ->f, uiNumOctaves);

      ++pPosX;
      ++pPosY;
      ++pPosZ;
      ++pOutput;
    }
  }

  //////////////////////////////////////////////////////////////////////////

  static WHashedString s_sCurves = WMakeHashedString("Curves");

  static const WEnum<WExpression::RegisterType> s_SampleCurvesInputTypes[] = {
    WExpression::RegisterType::Int,   // CurveIndex
    WExpression::RegisterType::Float, // X
  };

  static void SampleCurve(WExpression::Inputs inputs, WExpression::Output output, const WExpression::GlobalData& globalData)
  {
    const WVariantArray& curves = globalData.GetValue(s_sCurves)->Get<WVariantArray>();
    if (curves.IsEmpty())
      return;

    WUInt32 uiCurveIndex = inputs[0].GetPtr()->i.x();
    if (uiCurveIndex >= curves.GetCount())
      return;

    auto pSampledCurve = WDynamicCast<const WSampledCurve1D*>(curves[uiCurveIndex].Get<WReflectedClass*>());
    if (pSampledCurve == nullptr || pSampledCurve->m_Samples.IsEmpty())
      return;

    const WUInt32 uiMaxIdx = pSampledCurve->m_Samples.GetCount() - 1;
    const WSimdVec4f vOffsetX = WSimdVec4f(-pSampledCurve->m_fMinX);
    const float fRange = pSampledCurve->m_fMaxX - pSampledCurve->m_fMinX;
    const WSimdVec4f vScale = WSimdVec4f(fRange > 0.0f ? static_cast<float>(uiMaxIdx) / fRange : 0.0f);
    const WSimdVec4i vMaxIdx = WSimdVec4i(uiMaxIdx);
    const float* samples = pSampledCurve->m_Samples.GetData();

    const WExpression::Register* pX = inputs[1].GetPtr();
    const WExpression::Register* pXEnd = inputs[1].GetEndPtr();
    WExpression::Register* pOutput = output.GetPtr();

    while (pX < pXEnd)
    {
      const WSimdVec4f vT = (pX->f + vOffsetX).CompMul(vScale);
      const WSimdVec4i vIdx0 = WSimdVec4i::Truncate(vT).CompMax(WSimdVec4i::MakeZero()).CompMin(vMaxIdx);
      const WSimdVec4i vIdx1 = (vIdx0 + WSimdVec4i(1)).CompMin(vMaxIdx);
      const WSimdVec4f vFrac = vT - vIdx0.ToFloat();

      const float sample0[] = {samples[vIdx0.x()], samples[vIdx0.y()], samples[vIdx0.z()], samples[vIdx0.w()]};
      const float sample1[] = {samples[vIdx1.x()], samples[vIdx1.y()], samples[vIdx1.z()], samples[vIdx1.w()]};
      WSimdVec4f vSample0, vSample1;
      vSample0.Load<4>(sample0);
      vSample1.Load<4>(sample1);

      pOutput->f = WSimdVec4f::Lerp(vSample0, vSample1, vFrac);

      ++pX;
      ++pOutput;
    }
  }

  static WResult SampleCurveValidate(const WExpression::GlobalData& globalData)
  {
    if (!globalData.IsEmpty())
    {
      if (const WVariant* pValue = globalData.GetValue(s_sCurves))
      {
        if (pValue->GetType() == WVariantType::VariantArray)
        {
          return W_SUCCESS;
        }
      }
    }

    return W_FAILURE;
  }
} // namespace

WExpressionFunction WDefaultExpressionFunctions::s_RandomFunc = {
  {WMakeHashedString("random"), WExpression::FunctionDesc::TypeList(s_RandomInputTypes), 1, RegisterType::Float},
  &Random,
};

WExpressionFunction WDefaultExpressionFunctions::s_PerlinNoiseFunc = {
  {WMakeHashedString("perlinNoise"), WExpression::FunctionDesc::TypeList(s_PerlinNoiseInputTypes), 3, RegisterType::Float},
  &PerlinNoise,
};

WExpressionFunction WExtendedExpressionFunctions::s_SampleCurveFunc = {
  {WMakeHashedString("sampleCurve"), WExpression::FunctionDesc::TypeList(s_SampleCurvesInputTypes), 2, WExpression::RegisterType::Float},
  &SampleCurve,
  &SampleCurveValidate,
};

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExpressionWidgetAttribute, 1, WRTTIDefaultAllocator<WExpressionWidgetAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("InputsProperty", m_sInputsProperty),
    W_MEMBER_PROPERTY("OutputsProperty", m_sOutputsProperty),
    W_MEMBER_PROPERTY("CustomKeywords", m_sCustomKeywords),
    W_MEMBER_PROPERTY("CustomKeywordColor", m_CustomKeywordColor),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, WColorGammaUB),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


W_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionDeclarations);
