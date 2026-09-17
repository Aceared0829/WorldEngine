#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Expression.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WParticleExpressionInput, WNoBase, 1, WRTTIDefaultAllocator<WParticleExpressionInput>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("CurveSource", WCurveSource, m_CurveSource),
    W_MEMBER_PROPERTY("Curve", m_Curve),
    W_RESOURCE_MEMBER_PROPERTY("SharedCurve", m_hSharedCurve)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Expression, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_Expression>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Expression", m_sExpression)->AddAttributes(new WExpressionWidgetAttribute(
      "inPos;inVel;inColor;inSize;inRotSpeed;inLifeTime;"
      "outPos;outVel;outColor;outSize;outRotSpeed;outLifeTime;outDiscard;"
      "timeDiff;sampleCurve",
      WColorGammaUB(WColorScheme::DarkUI(WColorScheme::Teal)))),
    W_ARRAY_MEMBER_PROPERTY("Inputs", m_Inputs),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Expression, 1, WRTTIDefaultAllocator<WParticleBehavior_Expression>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

static WHashedString s_sTimeDiff = WMakeHashedString("timeDiff");

static WHashedString s_sInPos = WMakeHashedString("inPos");
static WHashedString s_sInVel = WMakeHashedString("inVel");
static WHashedString s_sInColor = WMakeHashedString("inColor");
static WHashedString s_sInSize = WMakeHashedString("inSize");
static WHashedString s_sInRotSpeed = WMakeHashedString("inRotSpeed");
static WHashedString s_sInLifeTime = WMakeHashedString("inLifeTime");
static WHashedString s_sOutPos = WMakeHashedString("outPos");
static WHashedString s_sOutVel = WMakeHashedString("outVel");
static WHashedString s_sOutColor = WMakeHashedString("outColor");
static WHashedString s_sOutSize = WMakeHashedString("outSize");
static WHashedString s_sOutRotSpeed = WMakeHashedString("outRotSpeed");
static WHashedString s_sOutLifeTime = WMakeHashedString("outLifeTime");
static WHashedString s_sOutDiscard = WMakeHashedString("outDiscard");

//////////////////////////////////////////////////////////////////////////

WParticleBehaviorFactory_Expression::WParticleBehaviorFactory_Expression() = default;

const WRTTI* WParticleBehaviorFactory_Expression::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Expression>();
}

void WParticleBehaviorFactory_Expression::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Expression* pBehavior = static_cast<WParticleBehavior_Expression*>(pObject);

  pBehavior->m_bUsesDiscard = m_bUsesDiscard;
  pBehavior->m_InputStreams = m_InputStreams;
  pBehavior->m_OutputStreams = m_OutputStreams;
  pBehavior->m_pByteCode = GetByteCode();
  pBehavior->m_pCurveSamples = m_CurveSamples.IsEmpty() ? nullptr : &m_CurveSamples;
  pBehavior->m_pFloatParamNames = m_FloatParamNames.IsEmpty() ? nullptr : &m_FloatParamNames;
  pBehavior->m_pColorParamNames = m_ColorParamNames.IsEmpty() ? nullptr : &m_ColorParamNames;
}

void WParticleBehaviorFactory_Expression::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  if (m_OutputStreams.IsSet(WParticleStreamMask::Velocity))
  {
    inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_ApplyVelocity>());
  }
}

void WParticleBehaviorFactory_Expression::Save(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  inout_stream << m_sExpression;

  const WUInt32 uiCount = m_Inputs.GetCount();
  inout_stream << uiCount;
  for (const auto& input : m_Inputs)
  {
    inout_stream << input.m_CurveSource;
    inout_stream << input.m_hSharedCurve;
    input.m_Curve.ConvertToRuntimeData(input.m_RuntimeCurve);
    input.m_RuntimeCurve.SortControlPoints();
    input.m_RuntimeCurve.Save(inout_stream);
  }
}

void WParticleBehaviorFactory_Expression::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  const auto version = inout_stream.ReadVersion(1);

  inout_stream >> m_sExpression;

  WUInt32 uiCount = 0;
  inout_stream >> uiCount;
  m_Inputs.SetCount(uiCount);
  for (auto& input : m_Inputs)
  {
    inout_stream >> input.m_CurveSource;
    inout_stream >> input.m_hSharedCurve;
    input.m_RuntimeCurve.Load(inout_stream);
    input.m_RuntimeCurve.SortControlPoints();
    input.m_RuntimeCurve.CreateLinearApproximation();
  }

  CompileExpression(ownerEffectDescriptor);
  BuildCurveSamples();
}

void WParticleBehaviorFactory_Expression::CompileExpression(const WParticleEffectDescriptor& ownerEffectDescriptor)
{
  if (m_sCompiledExpression == m_sExpression)
    return;

  m_bBytecodeValid = false;
  m_sCompiledExpression = m_sExpression;
  m_FloatParamNames.Clear();
  m_ColorParamNames.Clear();

  if (m_sExpression.IsEmpty())
    return;

  WTempHybridArray<WExpression::StreamDesc, 12> inputs;
  WTempHybridArray<WExpression::StreamDesc, 8> outputs;

  m_InputStreams.Clear();
  m_OutputStreams.Clear();
  m_bUsesDiscard = false;

  WStringBuilder sExpressionNoComments = m_sExpression;
  sExpressionNoComments.RemoveCStyleComments();

  {
    if (sExpressionNoComments.FindSubString("inPos"))
    {
      m_InputStreams.Add(WParticleStreamMask::Position);
      inputs.PushBack(WExpression::StreamDesc(s_sInPos, WProcessingStream::DataType::Float3));
    }
    if (sExpressionNoComments.FindSubString("inVel"))
    {
      m_InputStreams.Add(WParticleStreamMask::Velocity);
      inputs.PushBack(WExpression::StreamDesc(s_sInVel, WProcessingStream::DataType::Half4));
    }
    if (sExpressionNoComments.FindSubString("inColor"))
    {
      m_InputStreams.Add(WParticleStreamMask::Color);
      inputs.PushBack(WExpression::StreamDesc(s_sInColor, WProcessingStream::DataType::Half4));
    }
    if (sExpressionNoComments.FindSubString("inSize"))
    {
      m_InputStreams.Add(WParticleStreamMask::Size);
      inputs.PushBack(WExpression::StreamDesc(s_sInSize, WProcessingStream::DataType::Half));
    }
    if (sExpressionNoComments.FindSubString("inRotSpeed"))
    {
      m_InputStreams.Add(WParticleStreamMask::Rotation);
      inputs.PushBack(WExpression::StreamDesc(s_sInRotSpeed, WProcessingStream::DataType::Half));
    }
    if (sExpressionNoComments.FindSubString("inLifeTime"))
    {
      m_InputStreams.Add(WParticleStreamMask::LifeTime);
      inputs.PushBack(WExpression::StreamDesc(s_sInLifeTime, WProcessingStream::DataType::Half2));
    }

    for (auto it = ownerEffectDescriptor.m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      if (sExpressionNoComments.FindSubString(it.Key()))
      {
        WHashedString sName;
        sName.Assign(it.Key());
        m_FloatParamNames.PushBack(sName);
        inputs.PushBack(WExpression::StreamDesc(sName, WProcessingStream::DataType::Float));
      }
    }

    for (auto it = ownerEffectDescriptor.m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      if (sExpressionNoComments.FindSubString(it.Key()))
      {
        WHashedString sName;
        sName.Assign(it.Key());
        m_ColorParamNames.PushBack(sName);
        inputs.PushBack(WExpression::StreamDesc(sName, WProcessingStream::DataType::Float4));
      }
    }

    // Global variables are always registered so the parser accepts them unconditionally.
    // The VM only maps what the bytecode actually references, so providing unused globals has no runtime cost.
    inputs.PushBack(WExpression::StreamDesc(s_sTimeDiff, WProcessingStream::DataType::Float));
  }

  {
    if (sExpressionNoComments.FindSubString("outPos"))
    {
      m_OutputStreams.Add(WParticleStreamMask::Position);
      outputs.PushBack(WExpression::StreamDesc(s_sOutPos, WProcessingStream::DataType::Float3));
    }
    if (sExpressionNoComments.FindSubString("outVel"))
    {
      m_OutputStreams.Add(WParticleStreamMask::Velocity);
      outputs.PushBack(WExpression::StreamDesc(s_sOutVel, WProcessingStream::DataType::Half4));
    }
    if (sExpressionNoComments.FindSubString("outColor"))
    {
      m_OutputStreams.Add(WParticleStreamMask::Color);
      outputs.PushBack(WExpression::StreamDesc(s_sOutColor, WProcessingStream::DataType::Half4));
    }
    if (sExpressionNoComments.FindSubString("outSize"))
    {
      m_OutputStreams.Add(WParticleStreamMask::Size);
      outputs.PushBack(WExpression::StreamDesc(s_sOutSize, WProcessingStream::DataType::Half));
    }
    if (sExpressionNoComments.FindSubString("outRotSpeed"))
    {
      m_OutputStreams.Add(WParticleStreamMask::Rotation);
      outputs.PushBack(WExpression::StreamDesc(s_sOutRotSpeed, WProcessingStream::DataType::Half));
    }
    if (sExpressionNoComments.FindSubString("outDiscard"))
    {
      m_bUsesDiscard = true;
      outputs.PushBack(WExpression::StreamDesc(s_sOutDiscard, WProcessingStream::DataType::Byte));
    }
  }

  WExpressionParser parser;
  parser.RegisterFunction(WExtendedExpressionFunctions::s_SampleCurveFunc.m_Desc);

  WExpressionParser::Options options;
  WExpressionAST ast;
  if (parser.Parse(m_sExpression, inputs, outputs, options, ast).Failed())
  {
    WLog::Warning("Particle Expression: Failed to parse expression '{0}'", m_sExpression);
    return;
  }

  WExpressionCompiler compiler;
  if (compiler.Compile(ast, m_ByteCode).Failed())
  {
    WLog::Warning("Particle Expression: Failed to compile expression '{0}'", m_sExpression);
    return;
  }

  m_bBytecodeValid = true;
}

void WParticleBehaviorFactory_Expression::BuildCurveSamples()
{
  m_CurveSamples.SetCount(m_Inputs.GetCount());

  for (WUInt32 i = 0; i < m_Inputs.GetCount(); ++i)
  {
    const auto& input = m_Inputs[i];
    auto& samples = m_CurveSamples[i];

    WCurve1D runtimeCurve;

    if (input.m_CurveSource == WCurveSource::SharedCurve)
    {
      if (input.m_hSharedCurve.IsValid())
      {
        WResourceLock<WCurve1DResource> pResource(input.m_hSharedCurve, WResourceAcquireMode::BlockTillLoaded);
        if (pResource.GetAcquireResult() == WResourceAcquireResult::Final && !pResource->GetDescriptor().m_Curves.IsEmpty())
        {
          runtimeCurve = pResource->GetDescriptor().m_Curves[0];
        }
      }
    }
    else
    {
      // Prefer converting from edit-time data (editor path).
      // Fall back to the loaded runtime curve when edit data is unavailable (game runtime path).
      input.m_Curve.ConvertToRuntimeData(runtimeCurve);
      if (runtimeCurve.GetNumControlPoints() == 0)
      {
        runtimeCurve = input.m_RuntimeCurve;
      }
      else
      {
        runtimeCurve.SortControlPoints();
      }
    }

    if (runtimeCurve.GetNumControlPoints() == 0)
    {
      samples.m_Samples.SetCount(1);
      samples.m_Samples[0] = 0.0f;
      samples.m_fMinX = 0.0f;
      samples.m_fMaxX = 1.0f;
      continue;
    }

    constexpr WUInt32 k_uiSampleCount = 256;
    runtimeCurve.GenerateSampledCurve(k_uiSampleCount, samples).AssertSuccess();
  }
}

//////////////////////////////////////////////////////////////////////////

WParticleBehavior_Expression::WParticleBehavior_Expression()
{
  // execute last
  m_fPriority = 1000.0f;

  m_VM.RegisterFunction(WExtendedExpressionFunctions::s_SampleCurveFunc);
}

void WParticleBehavior_Expression::CreateRequiredStreams()
{
  if (m_InputStreams.IsSet(WParticleStreamMask::Position) || m_OutputStreams.IsSet(WParticleStreamMask::Position))
  {
    CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  }

  if (m_InputStreams.IsSet(WParticleStreamMask::Velocity) || m_OutputStreams.IsSet(WParticleStreamMask::Velocity))
  {
    CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
  }

  if (m_InputStreams.IsSet(WParticleStreamMask::Size) || m_OutputStreams.IsSet(WParticleStreamMask::Size))
  {
    CreateStream("Size", WProcessingStream::DataType::Half, &m_pStreamSize, false);
  }

  if (m_InputStreams.IsSet(WParticleStreamMask::Color) || m_OutputStreams.IsSet(WParticleStreamMask::Color))
  {
    CreateStream("Color", WProcessingStream::DataType::Half4, &m_pStreamColor, false);
  }

  if (m_InputStreams.IsSet(WParticleStreamMask::Rotation) || m_OutputStreams.IsSet(WParticleStreamMask::Rotation))
  {
    CreateStream("RotationSpeed", WProcessingStream::DataType::Half, &m_pStreamRotationSpeed, false);
  }

  if (m_InputStreams.IsSet(WParticleStreamMask::LifeTime) || m_OutputStreams.IsSet(WParticleStreamMask::LifeTime) || m_bUsesDiscard)
  {
    CreateStream("LifeTime", WProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  }
}

void WParticleBehavior_Expression::QueryOptionalStreams()
{
}

void WParticleBehavior_Expression::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  UpdateElements(uiStartIndex, uiNumElements);
}

void WParticleBehavior_Expression::Process(WUInt64 uiNumElements)
{
  UpdateElements(0, uiNumElements);
}

void WParticleBehavior_Expression::UpdateElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  if (m_pByteCode == nullptr || uiNumElements == 0)
    return;

  W_PROFILE_SCOPE("PFX: Expression");

  static thread_local WDynamicArray<WUInt8> s_DiscardBuffer;

  // When called from InitializeElements, uiStartIndex > 0. The VM always processes from element 0,
  // so we must advance all data pointers by uiStartIndex elements to address the correct particle slots.
  auto OffsetStream = [uiStartIndex](const WHashedString& sName, const WProcessingStream& src)
  {
    const WUInt64 uiByteOffset = uiStartIndex * src.GetElementStride();
    WUInt8* pData = static_cast<WUInt8*>(src.GetWritableData()) + uiByteOffset;
    const WUInt32 uiRemainingBytes = static_cast<WUInt32>(src.GetDataSize() - uiByteOffset);
    return WProcessingStream(sName, WArrayPtr<WUInt8>(pData, uiRemainingBytes), src.GetDataType(), src.GetElementStride());
  };

  // Fetch current effect parameter values. These buffers must outlive the VM execute call.
  WHybridArray<float, 4> floatParamValues;
  if (m_pFloatParamNames != nullptr)
  {
    floatParamValues.SetCount(m_pFloatParamNames->GetCount());
    for (WUInt32 i = 0; i < m_pFloatParamNames->GetCount(); ++i)
    {
      floatParamValues[i] = GetOwnerEffect()->GetFloatParameter(WTempHashedString((*m_pFloatParamNames)[i]), 0.0f);
    }
  }

  WHybridArray<WColor, 2> colorParamValues;
  if (m_pColorParamNames != nullptr)
  {
    colorParamValues.SetCount(m_pColorParamNames->GetCount());
    for (WUInt32 i = 0; i < m_pColorParamNames->GetCount(); ++i)
    {
      colorParamValues[i] = GetOwnerEffect()->GetColorParameter(WTempHashedString((*m_pColorParamNames)[i]), WColor::White);
    }
  }

  WTempHybridArray<WProcessingStream, 8> vmInputs;
  const float fTimeDiff = static_cast<float>(m_TimeDiff.GetSeconds());

  {
    if (m_InputStreams.IsSet(WParticleStreamMask::Position))
    {
      vmInputs.PushBack(OffsetStream(s_sInPos, *m_pStreamPosition));
    }

    if (m_InputStreams.IsSet(WParticleStreamMask::Velocity))
    {
      vmInputs.PushBack(OffsetStream(s_sInVel, *m_pStreamVelocity));
    }
    if (m_InputStreams.IsSet(WParticleStreamMask::Size))
    {
      vmInputs.PushBack(OffsetStream(s_sInSize, *m_pStreamSize));
    }
    if (m_InputStreams.IsSet(WParticleStreamMask::Color))
    {
      vmInputs.PushBack(OffsetStream(s_sInColor, *m_pStreamColor));
    }
    if (m_InputStreams.IsSet(WParticleStreamMask::LifeTime))
    {
      vmInputs.PushBack(OffsetStream(s_sInLifeTime, *m_pStreamLifeTime));
    }
    if (m_InputStreams.IsSet(WParticleStreamMask::Rotation))
    {
      vmInputs.PushBack(OffsetStream(s_sInRotSpeed, *m_pStreamRotationSpeed));
    }

    vmInputs.PushBack(WProcessingStream(s_sTimeDiff,
      WArrayPtr<WUInt8>(const_cast<WUInt8*>(reinterpret_cast<const WUInt8*>(&fTimeDiff)), sizeof(float)),
      WProcessingStream::DataType::Float, /*uiStride*/ 0));

    for (WUInt32 i = 0; i < floatParamValues.GetCount(); ++i)
    {
      vmInputs.PushBack(WProcessingStream((*m_pFloatParamNames)[i],
        WArrayPtr<WUInt8>(reinterpret_cast<WUInt8*>(&floatParamValues[i]), sizeof(float)),
        WProcessingStream::DataType::Float, /*uiStride*/ 0));
    }

    for (WUInt32 i = 0; i < colorParamValues.GetCount(); ++i)
    {
      vmInputs.PushBack(WProcessingStream((*m_pColorParamNames)[i],
        WArrayPtr<WUInt8>(reinterpret_cast<WUInt8*>(&colorParamValues[i]), sizeof(WColor)),
        WProcessingStream::DataType::Float4, /*uiStride*/ 0));
    }
  }

  WTempHybridArray<WProcessingStream, 8> vmOutputs;

  {
    if (m_OutputStreams.IsSet(WParticleStreamMask::Position))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutPos, *m_pStreamPosition));
    }

    if (m_OutputStreams.IsSet(WParticleStreamMask::Velocity))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutVel, *m_pStreamVelocity));
    }
    if (m_OutputStreams.IsSet(WParticleStreamMask::Size))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutSize, *m_pStreamSize));
    }
    if (m_OutputStreams.IsSet(WParticleStreamMask::Color))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutColor, *m_pStreamColor));
    }
    if (m_OutputStreams.IsSet(WParticleStreamMask::LifeTime))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutLifeTime, *m_pStreamLifeTime));
    }
    if (m_OutputStreams.IsSet(WParticleStreamMask::Rotation))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutRotSpeed, *m_pStreamRotationSpeed));
    }
    if (m_bUsesDiscard)
    {
      s_DiscardBuffer.SetCountUninitialized(static_cast<WUInt32>(uiNumElements));
      vmOutputs.PushBack(WProcessingStream(s_sOutDiscard, s_DiscardBuffer.GetArrayPtr(), WProcessingStream::DataType::Byte));
    }
  }

  const WUInt32 uiCount = static_cast<WUInt32>(uiNumElements);

  // Build global data for curve lookups (pointers into factory-owned sample arrays).
  WExpression::GlobalData globalData;
  if (m_pCurveSamples != nullptr && !m_pCurveSamples->IsEmpty())
  {
    WVariantArray curves;
    curves.SetCount(m_pCurveSamples->GetCount());
    for (WUInt32 i = 0; i < m_pCurveSamples->GetCount(); ++i)
    {
      curves[i] = WVariant(const_cast<WSampledCurve1D*>(&(*m_pCurveSamples)[i]));
    }
    globalData.Insert(WMakeHashedString("Curves"), curves);
  }

  if (m_VM.Execute(*m_pByteCode, vmInputs, vmOutputs, uiCount, globalData).Failed())
  {
    WLog::Warning("Particle Expression: Failed to execute expression");
    m_pByteCode = nullptr;
    return;
  }

  if (m_bUsesDiscard && m_pStreamLifeTime != nullptr)
  {
    const WUInt8* pDiscardData = s_DiscardBuffer.GetData();
    WFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetWritableData<WFloat16Vec2>() + uiStartIndex;

    for (WUInt32 i = 0; i < uiCount; ++i)
    {
      if (pDiscardData[i] > 0)
      {
        pLifeTime[i].x = 0.0f;
      }
    }
  }
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Expression);
