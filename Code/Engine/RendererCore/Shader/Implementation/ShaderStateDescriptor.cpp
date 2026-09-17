#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderPermutationBinary.h>

struct WShaderStateVersion
{
  enum Enum : WUInt32
  {
    Version0 = 0,
    Version1,
    Version2,
    Version3,
    Version4, // Added stencil reference value

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

void WShaderStateResourceDescriptor::Save(WStreamWriter& inout_stream) const
{
  inout_stream << (WUInt32)WShaderStateVersion::Current;

  // Blend State
  {
    inout_stream << m_BlendDesc.m_bAlphaToCoverage;
    inout_stream << m_BlendDesc.m_bIndependentBlend;

    const WUInt8 iBlends = m_BlendDesc.m_bIndependentBlend ? W_GAL_MAX_RENDERTARGET_COUNT : 1;
    inout_stream << iBlends; // in case W_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (WUInt32 b = 0; b < iBlends; ++b)
    {
      inout_stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      inout_stream << (WUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp;
      inout_stream << (WUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha;
      inout_stream << (WUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend;
      inout_stream << (WUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha;
      inout_stream << (WUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend;
      inout_stream << (WUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha;
      inout_stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    inout_stream << (WUInt8)m_DepthStencilDesc.m_DepthTestFunc;
    inout_stream << m_DepthStencilDesc.m_bDepthEnable;
    inout_stream << m_DepthStencilDesc.m_bDepthWrite;
    bool m_bSeparateFrontAndBack = false;
    inout_stream << m_bSeparateFrontAndBack;
    inout_stream << m_DepthStencilDesc.m_bStencilEnable;
    inout_stream << m_DepthStencilDesc.m_uiStencilReadMask;
    inout_stream << m_DepthStencilDesc.m_uiStencilWriteMask;
    inout_stream << (WUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp;
    inout_stream << (WUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp;
    inout_stream << (WUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp;
    inout_stream << (WUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc;
    inout_stream << (WUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp;
    inout_stream << (WUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp;
    inout_stream << (WUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp;
    inout_stream << (WUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc;
  }

  // Rasterizer State
  {
    inout_stream << m_RasterizerDesc.m_bFrontCounterClockwise;
    inout_stream << m_RasterizerDesc.m_bScissorTest;
    inout_stream << m_RasterizerDesc.m_bWireFrame;
    inout_stream << (WUInt8)m_RasterizerDesc.m_CullMode;
    inout_stream << m_RasterizerDesc.m_fDepthBiasClamp;
    inout_stream << m_RasterizerDesc.m_fSlopeScaledDepthBias;
    inout_stream << m_RasterizerDesc.m_iDepthBias;
    inout_stream << m_RasterizerDesc.m_bConservativeRasterization;
  }

  // Dynamic States
  {
    inout_stream << m_uiShaderStencilRef;
    inout_stream << m_bUseUserStencilRefValue;
  }
}

void WShaderStateResourceDescriptor::Load(WStreamReader& inout_stream)
{
  WUInt32 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion >= WShaderStateVersion::Version1 && uiVersion <= WShaderStateVersion::Current, "Invalid version {0}", uiVersion);

  // Blend State
  {
    inout_stream >> m_BlendDesc.m_bAlphaToCoverage;
    inout_stream >> m_BlendDesc.m_bIndependentBlend;

    WUInt8 iBlends = 0;
    inout_stream >> iBlends; // in case W_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (WUInt32 b = 0; b < iBlends; ++b)
    {
      WUInt8 uiTemp;
      inout_stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp = (WGALBlendOp::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha = (WGALBlendOp::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend = (WGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha = (WGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend = (WGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha = (WGALBlend::Enum)uiTemp;
      inout_stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    WUInt8 uiTemp = 0;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_DepthTestFunc = (WGALCompareFunc::Enum)uiTemp;
    inout_stream >> m_DepthStencilDesc.m_bDepthEnable;
    inout_stream >> m_DepthStencilDesc.m_bDepthWrite;
    bool m_bSeparateFrontAndBack = false;
    inout_stream >> m_bSeparateFrontAndBack;
    inout_stream >> m_DepthStencilDesc.m_bStencilEnable;
    inout_stream >> m_DepthStencilDesc.m_uiStencilReadMask;
    inout_stream >> m_DepthStencilDesc.m_uiStencilWriteMask;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (WGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (WGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (WGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (WGALCompareFunc::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (WGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (WGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (WGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (WGALCompareFunc::Enum)uiTemp;
  }

  // Rasterizer State
  {
    WUInt8 uiTemp = 0;

    if (uiVersion < WShaderStateVersion::Version2)
    {
      bool dummy;
      inout_stream >> dummy;
    }

    inout_stream >> m_RasterizerDesc.m_bFrontCounterClockwise;

    if (uiVersion < WShaderStateVersion::Version2)
    {
      bool dummy;
      inout_stream >> dummy;
      inout_stream >> dummy;
    }

    inout_stream >> m_RasterizerDesc.m_bScissorTest;
    inout_stream >> m_RasterizerDesc.m_bWireFrame;
    inout_stream >> uiTemp;
    m_RasterizerDesc.m_CullMode = (WGALCullMode::Enum)uiTemp;
    inout_stream >> m_RasterizerDesc.m_fDepthBiasClamp;
    inout_stream >> m_RasterizerDesc.m_fSlopeScaledDepthBias;
    inout_stream >> m_RasterizerDesc.m_iDepthBias;

    if (uiVersion >= WShaderStateVersion::Version3)
    {
      inout_stream >> m_RasterizerDesc.m_bConservativeRasterization;
    }
  }

  // Dynamic States
  {
    if (uiVersion >= WShaderStateVersion::Version4)
    {
      inout_stream >> m_uiShaderStencilRef;
      inout_stream >> m_bUseUserStencilRefValue;
    }
  }
}

WUInt32 WShaderStateResourceDescriptor::CalculateHash() const
{
  return m_BlendDesc.CalculateHash() + m_RasterizerDesc.CalculateHash() + m_DepthStencilDesc.CalculateHash() + m_uiShaderStencilRef + (m_bUseUserStencilRefValue ? 1 : 0);
}

static const char* AppendNumber(const char* szString, WInt32 iNumber, WStringBuilder& ref_sTemp)
{
  if (iNumber >= 0)
  {
    ref_sTemp = szString;
    ref_sTemp.AppendFormat("{}", iNumber);
    return ref_sTemp;
  }

  return szString;
}

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
// Parse() runs concurrently (shader permutations are compiled in parallel), so this must be locked.
// Global variables don't use memory tracking, so these won't be reported as memory leaks.
static WMutex s_AllowedVariablesLock;
static WSet<WString> s_AllAllowedVariables;

static void RegisterAllowedVariable(const char* szVariable)
{
  W_LOCK(s_AllowedVariablesLock);
  s_AllAllowedVariables.Insert(szVariable);
}
#endif

static bool GetBoolStateVariable(const WMap<WString, WString>& variables, const char* szVariable, bool bDefValue)
{
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  RegisterAllowedVariable(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return bDefValue;

  if (it.Value() == "true")
    return true;
  if (it.Value() == "false")
    return false;

  WLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Should be 'true' or 'false'", szVariable, it.Value());
  return bDefValue;
}

static WInt32 GetEnumStateVariable(
  const WMap<WString, WString>& variables, const WMap<WString, WInt32>& values, const char* szVariable, WInt32 iDefValue)
{
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  RegisterAllowedVariable(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return iDefValue;

  auto itVal = values.Find(it.Value());
  if (!itVal.IsValid())
  {
    WStringBuilder valid;
    for (auto vv = values.GetIterator(); vv.IsValid(); ++vv)
    {
      valid.Append(" ", vv.Key());
    }

    WLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Valid values are:{2}", szVariable, it.Value(), valid);
    return iDefValue;
  }

  return itVal.Value();
}

static float GetFloatStateVariable(const WMap<WString, WString>& variables, const char* szVariable, float fDefValue)
{
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  RegisterAllowedVariable(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return fDefValue;

  double result = 0;
  if (WConversionUtils::StringToFloat(it.Value(), result).Failed())
  {
    WLog::Error("Shader state variable '{0}' is not a valid float value: '{1}'.", szVariable, it.Value());
    return fDefValue;
  }

  return (float)result;
}

static WInt32 GetIntStateVariable(const WMap<WString, WString>& variables, const char* szVariable, WInt32 iDefValue)
{
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  RegisterAllowedVariable(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return iDefValue;

  WInt32 result = 0;
  if (WConversionUtils::StringToInt(it.Value(), result).Failed())
  {
    WLog::Error("Shader state variable '{0}' is not a valid int value: '{1}'.", szVariable, it.Value());
    return iDefValue;
  }

  return result;
}

// Global variables don't use memory tracking, so these won't reported as memory leaks.
static WMutex StateValuesLock;
static WMap<WString, WInt32> StateValuesBlend;
static WMap<WString, WInt32> StateValuesBlendOp;
static WMap<WString, WInt32> StateValuesCullMode;
static WMap<WString, WInt32> StateValuesCompareFunc;
static WMap<WString, WInt32> StateValuesStencilOp;

WResult WShaderStateResourceDescriptor::Parse(const char* szSource)
{
  WMap<WString, WString> VariableValues;

  // extract all state assignments
  {
    WStringBuilder sSource = szSource;

    WTempHybridArray<WStringView, 32> allAssignments;
    WTempHybridArray<WStringView, 4> components;
    sSource.Split(false, allAssignments, "\n", ";", "\r");

    WStringBuilder temp;
    for (const WStringView& assignment : allAssignments)
    {
      temp = assignment;
      temp.Trim(" \t\r\n;");
      if (temp.IsEmpty())
        continue;

      temp.Split(false, components, " ", "\t", "=", "\r");

      if (components.GetCount() != 2)
      {
        WLog::Error("Malformed shader state assignment: '{0}'", temp);
        continue;
      }

      VariableValues[components[0]] = components[1];
    }
  }

  {
    W_LOCK(StateValuesLock);
    if (StateValuesBlend.IsEmpty())
    {
      // WGALBlend
      {
        StateValuesBlend["Blend_Zero"] = WGALBlend::Zero;
        StateValuesBlend["Blend_One"] = WGALBlend::One;
        StateValuesBlend["Blend_SrcColor"] = WGALBlend::SrcColor;
        StateValuesBlend["Blend_InvSrcColor"] = WGALBlend::InvSrcColor;
        StateValuesBlend["Blend_SrcAlpha"] = WGALBlend::SrcAlpha;
        StateValuesBlend["Blend_InvSrcAlpha"] = WGALBlend::InvSrcAlpha;
        StateValuesBlend["Blend_DestAlpha"] = WGALBlend::DestAlpha;
        StateValuesBlend["Blend_InvDestAlpha"] = WGALBlend::InvDestAlpha;
        StateValuesBlend["Blend_DestColor"] = WGALBlend::DestColor;
        StateValuesBlend["Blend_InvDestColor"] = WGALBlend::InvDestColor;
        StateValuesBlend["Blend_SrcAlphaSaturated"] = WGALBlend::SrcAlphaSaturated;
        StateValuesBlend["Blend_BlendFactor"] = WGALBlend::BlendFactor;
        StateValuesBlend["Blend_InvBlendFactor"] = WGALBlend::InvBlendFactor;
      }

      // WGALBlendOp
      {
        StateValuesBlendOp["BlendOp_Add"] = WGALBlendOp::Add;
        StateValuesBlendOp["BlendOp_Subtract"] = WGALBlendOp::Subtract;
        StateValuesBlendOp["BlendOp_RevSubtract"] = WGALBlendOp::RevSubtract;
        StateValuesBlendOp["BlendOp_Min"] = WGALBlendOp::Min;
        StateValuesBlendOp["BlendOp_Max"] = WGALBlendOp::Max;
      }

      // WGALCullMode
      {
        StateValuesCullMode["CullMode_None"] = WGALCullMode::None;
        StateValuesCullMode["CullMode_Front"] = WGALCullMode::Front;
        StateValuesCullMode["CullMode_Back"] = WGALCullMode::Back;
      }

      // WGALCompareFunc
      {
        StateValuesCompareFunc["CompareFunc_Never"] = WGALCompareFunc::Never;
        StateValuesCompareFunc["CompareFunc_Less"] = WGALCompareFunc::Less;
        StateValuesCompareFunc["CompareFunc_Equal"] = WGALCompareFunc::Equal;
        StateValuesCompareFunc["CompareFunc_LessEqual"] = WGALCompareFunc::LessEqual;
        StateValuesCompareFunc["CompareFunc_Greater"] = WGALCompareFunc::Greater;
        StateValuesCompareFunc["CompareFunc_NotEqual"] = WGALCompareFunc::NotEqual;
        StateValuesCompareFunc["CompareFunc_GreaterEqual"] = WGALCompareFunc::GreaterEqual;
        StateValuesCompareFunc["CompareFunc_Always"] = WGALCompareFunc::Always;
      }

      // WGALStencilOp
      {
        StateValuesStencilOp["StencilOp_Keep"] = WGALStencilOp::Keep;
        StateValuesStencilOp["StencilOp_Zero"] = WGALStencilOp::Zero;
        StateValuesStencilOp["StencilOp_Replace"] = WGALStencilOp::Replace;
        StateValuesStencilOp["StencilOp_IncrementSaturated"] = WGALStencilOp::IncrementSaturated;
        StateValuesStencilOp["StencilOp_DecrementSaturated"] = WGALStencilOp::DecrementSaturated;
        StateValuesStencilOp["StencilOp_Invert"] = WGALStencilOp::Invert;
        StateValuesStencilOp["StencilOp_Increment"] = WGALStencilOp::Increment;
        StateValuesStencilOp["StencilOp_Decrement"] = WGALStencilOp::Decrement;
      }
    }
  }

  // Retrieve Blend State
  {
    m_BlendDesc.m_bAlphaToCoverage = GetBoolStateVariable(VariableValues, "AlphaToCoverage", m_BlendDesc.m_bAlphaToCoverage);
    m_BlendDesc.m_bIndependentBlend = GetBoolStateVariable(VariableValues, "IndependentBlend", m_BlendDesc.m_bIndependentBlend);

    WStringBuilder s;

    // -1 for when no number is given
    for (WInt32 i = -1; i < 8; ++i)
    {
      const WInt32 idx = WMath::Max(i, 0);

      m_BlendDesc.m_RenderTargetBlendDescriptions[idx].m_bBlendingEnabled = GetBoolStateVariable(VariableValues, AppendNumber("BlendingEnabled", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled);
      m_BlendDesc.m_RenderTargetBlendDescriptions[idx].m_BlendOp = (WGALBlendOp::Enum)GetEnumStateVariable(VariableValues, StateValuesBlendOp, AppendNumber("BlendOp", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOp);
      m_BlendDesc.m_RenderTargetBlendDescriptions[idx].m_BlendOpAlpha = (WGALBlendOp::Enum)GetEnumStateVariable(VariableValues, StateValuesBlendOp, AppendNumber("BlendOpAlpha", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOpAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[idx].m_DestBlend = (WGALBlend::Enum)GetEnumStateVariable(VariableValues, StateValuesBlend, AppendNumber("DestBlend", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[idx].m_DestBlendAlpha = (WGALBlend::Enum)GetEnumStateVariable(VariableValues, StateValuesBlend, AppendNumber("DestBlendAlpha", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[idx].m_SourceBlend = (WGALBlend::Enum)GetEnumStateVariable(VariableValues, StateValuesBlend, AppendNumber("SourceBlend", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[idx].m_SourceBlendAlpha = (WGALBlend::Enum)GetEnumStateVariable(VariableValues, StateValuesBlend, AppendNumber("SourceBlendAlpha", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlendAlpha);

      // if WriteMaskN is set, overwrite
      m_BlendDesc.m_RenderTargetBlendDescriptions[idx].m_uiWriteMask = static_cast<WUInt8>(GetIntStateVariable(VariableValues, AppendNumber("WriteMask", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_uiWriteMask)); // deprecated old name

      // if ColorWriteMaskN is set, overwrite
      m_BlendDesc.m_RenderTargetBlendDescriptions[idx].m_uiWriteMask = static_cast<WUInt8>(GetIntStateVariable(VariableValues, AppendNumber("ColorWriteMask", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[idx].m_uiWriteMask));
    }
  }

  // Retrieve Rasterizer State
  {
    m_RasterizerDesc.m_bFrontCounterClockwise =
      GetBoolStateVariable(VariableValues, "FrontCounterClockwise", m_RasterizerDesc.m_bFrontCounterClockwise);
    m_RasterizerDesc.m_bScissorTest = GetBoolStateVariable(VariableValues, "ScissorTest", m_RasterizerDesc.m_bScissorTest);
    m_RasterizerDesc.m_bConservativeRasterization =
      GetBoolStateVariable(VariableValues, "ConservativeRasterization", m_RasterizerDesc.m_bConservativeRasterization);
    m_RasterizerDesc.m_bWireFrame = GetBoolStateVariable(VariableValues, "WireFrame", m_RasterizerDesc.m_bWireFrame);
    m_RasterizerDesc.m_CullMode =
      (WGALCullMode::Enum)GetEnumStateVariable(VariableValues, StateValuesCullMode, "CullMode", m_RasterizerDesc.m_CullMode);
    m_RasterizerDesc.m_fDepthBiasClamp = GetFloatStateVariable(VariableValues, "DepthBiasClamp", m_RasterizerDesc.m_fDepthBiasClamp);
    m_RasterizerDesc.m_fSlopeScaledDepthBias =
      GetFloatStateVariable(VariableValues, "SlopeScaledDepthBias", m_RasterizerDesc.m_fSlopeScaledDepthBias);
    m_RasterizerDesc.m_iDepthBias = GetIntStateVariable(VariableValues, "DepthBias", m_RasterizerDesc.m_iDepthBias);
  }

  // Retrieve Depth-Stencil State
  {
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (WGALStencilOp::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOp, "StencilDepthFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (WGALStencilOp::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOp, "StencilFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (WGALStencilOp::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOp, "StencilPassOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (WGALCompareFunc::Enum)GetEnumStateVariable(VariableValues, StateValuesCompareFunc, "StencilCompareFunc", m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc);

    // uses front-face values as fallback, if not overwritten
    m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (WGALStencilOp::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOp, "StencilBackFaceDepthFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (WGALStencilOp::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOp, "StencilBackFaceFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (WGALStencilOp::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOp, "StencilBackFacePassOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (WGALCompareFunc::Enum)GetEnumStateVariable(VariableValues, StateValuesCompareFunc, "StencilBackFaceCompareFunc", m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_bDepthEnable = GetBoolStateVariable(VariableValues, "DepthTest", m_DepthStencilDesc.m_bDepthEnable); // deprecated old name
    m_DepthStencilDesc.m_bDepthEnable = GetBoolStateVariable(VariableValues, "DepthEnable", m_DepthStencilDesc.m_bDepthEnable);
    m_DepthStencilDesc.m_bDepthWrite = GetBoolStateVariable(VariableValues, "DepthWrite", m_DepthStencilDesc.m_bDepthWrite);
    m_DepthStencilDesc.m_bStencilEnable = GetBoolStateVariable(VariableValues, "StencilEnable", m_DepthStencilDesc.m_bStencilEnable);
    m_DepthStencilDesc.m_DepthTestFunc = (WGALCompareFunc::Enum)GetEnumStateVariable(VariableValues, StateValuesCompareFunc, "DepthTestFunc", m_DepthStencilDesc.m_DepthTestFunc);
    m_DepthStencilDesc.m_uiStencilReadMask = static_cast<WUInt8>(GetIntStateVariable(VariableValues, "StencilReadMask", m_DepthStencilDesc.m_uiStencilReadMask));
    m_DepthStencilDesc.m_uiStencilWriteMask = static_cast<WUInt8>(GetIntStateVariable(VariableValues, "StencilWriteMask", m_DepthStencilDesc.m_uiStencilWriteMask));
  }

  // Dynamic States
  {
    m_bUseUserStencilRefValue = GetBoolStateVariable(VariableValues, "UseUserStencilRef", m_bUseUserStencilRefValue);
    const WInt32 iStencilRef = GetIntStateVariable(VariableValues, "StencilRef", m_uiShaderStencilRef);
    m_uiShaderStencilRef = static_cast<WUInt8>(iStencilRef);

    if (iStencilRef < 0)
    {
      // can either use UseUserStencilRef, or can set StencilRef to -1
      m_bUseUserStencilRefValue = true;
    }
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  // check for invalid variable names
  {
    W_LOCK(s_AllowedVariablesLock);

    for (auto it = VariableValues.GetIterator(); it.IsValid(); ++it)
    {
      if (!s_AllAllowedVariables.Contains(it.Key()))
      {
        WLog::Error("The shader state variable '{0}' does not exist.", it.Key());
      }
    }
  }
#endif


  return W_SUCCESS;
}
