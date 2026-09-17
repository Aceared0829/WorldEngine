#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace
{
  WExpressionAST::NodeType::Enum GetOperator(WProcGenBinaryOperator::Enum blendMode)
  {
    switch (blendMode)
    {
      case WProcGenBinaryOperator::Add:
        return WExpressionAST::NodeType::Add;
      case WProcGenBinaryOperator::Subtract:
        return WExpressionAST::NodeType::Subtract;
      case WProcGenBinaryOperator::Multiply:
        return WExpressionAST::NodeType::Multiply;
      case WProcGenBinaryOperator::Divide:
        return WExpressionAST::NodeType::Divide;
      case WProcGenBinaryOperator::Max:
        return WExpressionAST::NodeType::Max;
      case WProcGenBinaryOperator::Min:
        return WExpressionAST::NodeType::Min;

        W_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return WExpressionAST::NodeType::Invalid;
  }

  WExpressionAST::Node* CreateRandom(WUInt32 uiSeed, WExpressionAST& out_ast, const WProcGenNodeBase::GraphContext& context)
  {
    W_ASSERT_DEV(context.m_OutputType != WProcGenNodeBase::GraphContext::Unknown, "Unkown output type");

    auto pointIndexDataType = context.m_OutputType == WProcGenNodeBase::GraphContext::Placement ? WProcessingStream::DataType::Short : WProcessingStream::DataType::Int;
    WExpressionAST::Node* pPointIndex = out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPointIndex, pointIndexDataType});

    WExpressionAST::Node* pSeed = out_ast.CreateFunctionCall(WProcGenExpressionFunctions::s_GetInstanceSeedFunc.m_Desc, WArrayPtr<WExpressionAST::Node*>());
    pSeed = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Add, pSeed, out_ast.CreateConstant(uiSeed, WExpressionAST::DataType::Int));

    WExpressionAST::Node* arguments[] = {pPointIndex, pSeed};
    return out_ast.CreateFunctionCall(WDefaultExpressionFunctions::s_RandomFunc.m_Desc, arguments);
  }

  WExpressionAST::Node* CreateRemapFrom01(WExpressionAST::Node* pInput, float fMin, float fMax, WExpressionAST& out_ast)
  {
    auto pOffset = out_ast.CreateConstant(fMin);
    auto pScale = out_ast.CreateConstant(fMax - fMin);

    auto pValue = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Multiply, pInput, pScale);
    pValue = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Add, pValue, pOffset);

    return pValue;
  }

  WExpressionAST::Node* CreateRemapTo01WithFadeout(WExpressionAST::Node* pInput, float fMin, float fMax, float fLowerFade, float fUpperFade, WExpressionAST& out_ast)
  {
    // Note that we need to clamp the scale if it is below eps or we would end up with a division by 0.
    // To counter the clamp we move the lower and upper bounds by eps.
    // If no fade out is specified we would get a value of 0 for inputs that are exactly on the bounds otherwise which is not the expected behavior.

    const float eps = WMath::DefaultEpsilon<float>();
    const float fLowerScale = WMath::Max((fMax - fMin), 0.0f) * fLowerFade;
    const float fUpperScale = WMath::Max((fMax - fMin), 0.0f) * fUpperFade;

    if (fLowerScale < eps)
      fMin = fMin - eps;
    if (fUpperScale < eps)
      fMax = fMax + eps;

    auto pLowerOffset = out_ast.CreateConstant(fMin);
    auto pLowerValue = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Subtract, pInput, pLowerOffset);
    auto pLowerScale = out_ast.CreateConstant(WMath::Max(fLowerScale, eps));
    pLowerValue = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Divide, pLowerValue, pLowerScale);

    auto pUpperOffset = out_ast.CreateConstant(fMax);
    auto pUpperValue = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Subtract, pUpperOffset, pInput);
    auto pUpperScale = out_ast.CreateConstant(WMath::Max(fUpperScale, eps));
    pUpperValue = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Divide, pUpperValue, pUpperScale);

    auto pValue = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Min, pLowerValue, pUpperValue);
    return out_ast.CreateUnaryOperator(WExpressionAST::NodeType::Saturate, pValue);
  }

  void AddDefaultInputs(WExpressionAST& out_ast)
  {
    out_ast.m_InputNodes.Clear();

    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPositionX, WProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPositionY, WProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPositionZ, WProcessingStream::DataType::Float}));

    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sNormalX, WProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sNormalY, WProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sNormalZ, WProcessingStream::DataType::Float}));
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGenNodeBase, 1, WRTTINoAllocator)
{
  flags.Add(WTypeFlags::Abstract);
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGenOutput, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new WDynamicStringEnumAttribute("ProcGenOutputNameEnum")),
  }
  W_END_PROPERTIES;

  flags.Add(WTypeFlags::Abstract);
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WProcGenOutput::Save(WStreamWriter& inout_stream)
{
  inout_stream << m_sName;
  inout_stream.WriteArray(m_VolumeTagSetIndices).IgnoreResult();
  inout_stream.WriteArray(m_CurveIndices).IgnoreResult();
}

void WProcGenOutput::CopyValuesFromContext(const GraphContext& context)
{
  m_VolumeTagSetIndices = context.m_VolumeTagSetIndices;
  m_CurveIndices = context.m_CurveIndices;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_PlacementOutput, 2, WRTTIDefaultAllocator<WProcGen_PlacementOutput>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectsToPlace)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab")),
    W_MEMBER_PROPERTY("Footprint", m_fFootprint)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("MinOffset", m_vMinOffset),
    W_MEMBER_PROPERTY("MaxOffset", m_vMaxOffset),
    W_MEMBER_PROPERTY("YawRotationSnap", m_YawRotationSnap)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromRadian(0.0f), WVariant())),
    W_MEMBER_PROPERTY("AlignToNormal", m_fAlignToNormal)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("MinScale", m_vMinScale)->AddAttributes(new WDefaultValueAttribute(WVec3(1.0f)), new WClampValueAttribute(WVec3(0.0f), WVariant())),
    W_MEMBER_PROPERTY("MaxScale", m_vMaxScale)->AddAttributes(new WDefaultValueAttribute(WVec3(1.0f)), new WClampValueAttribute(WVec3(0.0f), WVariant())),
    W_MEMBER_PROPERTY("ColorGradient", m_sColorGradient)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    W_MEMBER_PROPERTY("CullDistance", m_fCullDistance)->AddAttributes(new WDefaultValueAttribute(30.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ENUM_MEMBER_PROPERTY("PlacementMode", WProcPlacementMode, m_PlacementMode),
    W_MEMBER_PROPERTY("NumAdditionalRays", m_uiNumAdditionalRays)->AddAttributes(new WDefaultValueAttribute(4), new WClampValueAttribute(3, 20)),
    W_MEMBER_PROPERTY("RaySpread", m_fRaySpread)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 10.0f)),
    W_ENUM_MEMBER_PROPERTY("PlacementPattern", WProcPlacementPattern, m_PlacementPattern),
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),

    W_MEMBER_PROPERTY("Density", m_DensityPin),
    W_MEMBER_PROPERTY("Scale", m_ScalePin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Pink))),
    W_MEMBER_PROPERTY("ColorIndex", m_ColorIndexPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Violet))),
    W_MEMBER_PROPERTY("ObjectIndex", m_ObjectIndexPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Cyan)))
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("{Active} Placement Output: {Name}"),
    new WCategoryAttribute("Output"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_PlacementOutput::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  AddDefaultInputs(out_ast);
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPointIndex, WProcessingStream::DataType::Short}));

  out_ast.m_OutputNodes.Clear();

  // density
  {
    auto pDensity = inputs[0];
    if (pDensity == nullptr)
    {
      pDensity = out_ast.CreateConstant(1.0f);
    }

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({WProcGenInternal::ExpressionOutputs::s_sOutDensity, WProcessingStream::DataType::Float}, pDensity));
  }

  // scale
  {
    auto pScale = inputs[1];
    if (pScale == nullptr)
    {
      pScale = CreateRandom(11.0f, out_ast, ref_context);
    }

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({WProcGenInternal::ExpressionOutputs::s_sOutScale, WProcessingStream::DataType::Float}, pScale));
  }

  // color index
  {
    auto pColorIndex = inputs[2];
    if (pColorIndex == nullptr)
    {
      pColorIndex = CreateRandom(13.0f, out_ast, ref_context);
    }

    pColorIndex = out_ast.CreateUnaryOperator(WExpressionAST::NodeType::Saturate, pColorIndex);
    pColorIndex = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Multiply, pColorIndex, out_ast.CreateConstant(255.0f));
    pColorIndex = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Add, pColorIndex, out_ast.CreateConstant(0.5f));

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({WProcGenInternal::ExpressionOutputs::s_sOutColorIndex, WProcessingStream::DataType::Byte}, pColorIndex));
  }

  // object index
  {
    auto pObjectIndex = inputs[3];
    if (pObjectIndex == nullptr)
    {
      pObjectIndex = CreateRandom(17.0f, out_ast, ref_context);
    }

    pObjectIndex = out_ast.CreateUnaryOperator(WExpressionAST::NodeType::Saturate, pObjectIndex);
    pObjectIndex = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Multiply, pObjectIndex, out_ast.CreateConstant(m_ObjectsToPlace.GetCount() - 1));
    pObjectIndex = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Add, pObjectIndex, out_ast.CreateConstant(0.5f));

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({WProcGenInternal::ExpressionOutputs::s_sOutObjectIndex, WProcessingStream::DataType::Byte}, pObjectIndex));
  }

  return nullptr;
}

void WProcGen_PlacementOutput::Save(WStreamWriter& inout_stream)
{
  SUPER::Save(inout_stream);

  inout_stream.WriteArray(m_ObjectsToPlace).IgnoreResult();

  inout_stream << m_fFootprint;

  inout_stream << m_vMinOffset;
  inout_stream << m_vMaxOffset;

  // chunk version 6
  inout_stream << m_YawRotationSnap;
  inout_stream << m_fAlignToNormal;

  inout_stream << m_vMinScale;
  inout_stream << m_vMaxScale;

  inout_stream << m_fCullDistance;

  inout_stream << m_uiCollisionLayer;

  inout_stream << m_sColorGradient;

  // chunk version 3
  inout_stream << m_sSurface;

  // chunk version 5
  inout_stream << m_PlacementMode;

  // chunk version 8
  inout_stream << m_uiNumAdditionalRays;
  inout_stream << m_fRaySpread;

  // chunk version 7
  inout_stream << m_PlacementPattern;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_VertexColorOutput, 2, WRTTIDefaultAllocator<WProcGen_VertexColorOutput>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Red))),
    W_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Green))),
    W_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue))),
    W_MEMBER_PROPERTY("A", m_APin),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("{Active} Vertex Color Output: {Name}"),
    new WCategoryAttribute("Output"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_VertexColorOutput::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  AddDefaultInputs(out_ast);

  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sColorR, WProcessingStream::DataType::Float}));
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sColorG, WProcessingStream::DataType::Float}));
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sColorB, WProcessingStream::DataType::Float}));
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sColorA, WProcessingStream::DataType::Float}));

  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPointIndex, WProcessingStream::DataType::Int}));

  out_ast.m_OutputNodes.Clear();

  WHashedString sOutputNames[4] = {
    WProcGenInternal::ExpressionOutputs::s_sOutColorR,
    WProcGenInternal::ExpressionOutputs::s_sOutColorG,
    WProcGenInternal::ExpressionOutputs::s_sOutColorB,
    WProcGenInternal::ExpressionOutputs::s_sOutColorA,
  };

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(sOutputNames); ++i)
  {
    auto pInput = inputs[i];
    if (pInput == nullptr)
    {
      pInput = out_ast.CreateConstant(0.0f);
    }

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({sOutputNames[i], WProcessingStream::DataType::Float}, pInput));
  }

  return nullptr;
}

void WProcGen_VertexColorOutput::Save(WStreamWriter& inout_stream)
{
  SUPER::Save(inout_stream);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_Random, 2, WRTTIDefaultAllocator<WProcGen_Random>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Seed", m_iSeed)->AddAttributes(new WClampValueAttribute(-1, WVariant()), new WDefaultValueAttribute(-1), new WMinValueTextAttribute("Auto")),
    W_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    W_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new WDefaultValueAttribute(1.0f)),

    W_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_FUNCTION_PROPERTY(OnObjectCreated),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Random: {Seed}"),
    new WCategoryAttribute("Math"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_Random::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  float fSeed = m_iSeed < 0 ? m_uiAutoSeed : m_iSeed;

  auto pRandom = CreateRandom(fSeed, out_ast, ref_context);
  return CreateRemapFrom01(pRandom, m_fOutputMin, m_fOutputMax, out_ast);
}

void WProcGen_Random::OnObjectCreated(const WAbstractObjectNode& node)
{
  m_uiAutoSeed = WHashHelper<WUuid>::Hash(node.GetGuid());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_PerlinNoise, 2, WRTTIDefaultAllocator<WProcGen_PerlinNoise>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Scale", m_Scale)->AddAttributes(new WDefaultValueAttribute(WVec3(10))),
    W_MEMBER_PROPERTY("Offset", m_Offset),
    W_MEMBER_PROPERTY("NumOctaves", m_uiNumOctaves)->AddAttributes(new WClampValueAttribute(1, 6), new WDefaultValueAttribute(3)),
    W_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    W_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new WDefaultValueAttribute(1.0f)),

    W_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Perlin Noise"),
    new WCategoryAttribute("Math"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_PerlinNoise::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  WExpressionAST::Node* pPos = out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPosition, WProcessingStream::DataType::Float3});
  pPos = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Divide, pPos, out_ast.CreateConstant(m_Scale, WExpressionAST::DataType::Float3));
  pPos = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Add, pPos, out_ast.CreateConstant(m_Offset, WExpressionAST::DataType::Float3));

  auto pPosX = out_ast.CreateSwizzle(WExpressionAST::VectorComponent::X, pPos);
  auto pPosY = out_ast.CreateSwizzle(WExpressionAST::VectorComponent::Y, pPos);
  auto pPosZ = out_ast.CreateSwizzle(WExpressionAST::VectorComponent::Z, pPos);

  auto pNumOctaves = out_ast.CreateConstant(m_uiNumOctaves, WExpressionAST::DataType::Int);

  WExpressionAST::Node* arguments[] = {pPosX, pPosY, pPosZ, pNumOctaves};

  auto pNoiseFunc = out_ast.CreateFunctionCall(WDefaultExpressionFunctions::s_PerlinNoiseFunc.m_Desc, arguments);

  return CreateRemapFrom01(pNoiseFunc, m_fOutputMin, m_fOutputMax, out_ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_Blend, 3, WRTTIDefaultAllocator<WProcGen_Blend>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Operator", WProcGenBinaryOperator, m_Operator),
    W_MEMBER_PROPERTY("InputA", m_fInputValueA)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("InputB", m_fInputValueB)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("ClampOutput", m_bClampOutput),

    W_MEMBER_PROPERTY("A", m_InputValueAPin),
    W_MEMBER_PROPERTY("B", m_InputValueBPin),
    W_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("{Operator}({A}, {B})"),
    new WCategoryAttribute("Math"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_Blend::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pInputA = inputs[0];
  if (pInputA == nullptr)
  {
    pInputA = out_ast.CreateConstant(m_fInputValueA);
  }

  auto pInputB = inputs[1];
  if (pInputB == nullptr)
  {
    pInputB = out_ast.CreateConstant(m_fInputValueB);
  }

  WExpressionAST::Node* pBlend = out_ast.CreateBinaryOperator(GetOperator(m_Operator), pInputA, pInputB);

  if (m_bClampOutput)
  {
    pBlend = out_ast.CreateUnaryOperator(WExpressionAST::NodeType::Saturate, pBlend);
  }

  return pBlend;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_Remap, 2, WRTTIDefaultAllocator<WProcGen_Remap>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("InputMin", m_fInputMin),
    W_MEMBER_PROPERTY("InputMax", m_fInputMax)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("ClampIntermediate", m_bClampIntermediate),
    W_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    W_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new WDefaultValueAttribute(1.0f)),

    W_MEMBER_PROPERTY("X", m_InputValuePin),
    W_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Remap: [{InputMin}, {InputMax}] -> [{OutputMin}, {OutputMax}]"),
    new WCategoryAttribute("Math"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_Remap::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_ast.CreateConstant(0.0f);
  }

  WExpressionAST::Node* p01Value = nullptr;
  if (m_fInputMin == 0.0f && m_fInputMax == 1.0f)
  {
    p01Value = pInput;
  }
  else if (WMath::IsEqual(m_fInputMin, m_fInputMax, WMath::DefaultEpsilon<float>()))
  {
    p01Value = out_ast.CreateConstant(1.0f);
  }
  else
  {
    auto pOffset = out_ast.CreateConstant(m_fInputMin);
    auto pValue = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Subtract, pInput, pOffset);
    auto pScale = out_ast.CreateConstant(1.0f / (m_fInputMax - m_fInputMin));
    p01Value = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Multiply, pValue, pScale);
  }

  if (m_bClampIntermediate)
  {
    p01Value = out_ast.CreateUnaryOperator(WExpressionAST::NodeType::Saturate, p01Value);
  }

  if (m_fOutputMin == 0.0f && m_fOutputMax == 1.0f)
  {
    return p01Value;
  }

  auto remapFrom01 = CreateRemapFrom01(p01Value, m_fOutputMin, m_fOutputMax, out_ast);

  return remapFrom01;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_Curve, 2, WRTTIDefaultAllocator<WProcGen_Curve>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Curve", m_CurveData),
    W_MEMBER_PROPERTY("NumSamples", m_uiNumSamples)->AddAttributes(new WClampValueAttribute(8, 256), new WDefaultValueAttribute(32)),
    
    W_MEMBER_PROPERTY("X", m_InputValuePin),
    W_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Math"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_Curve::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_ast.CreateConstant(0.0f);
  }

  WUInt32 uiCurveIndex = 0;
  {
    WCurve1D curve;
    m_CurveData.ConvertToRuntimeData(curve);

    WSampledCurve1D sampledCurve;
    curve.GenerateSampledCurve(m_uiNumSamples, sampledCurve).AssertSuccess();

    uiCurveIndex = ref_context.m_SharedData.AddCurve(std::move(sampledCurve));
    W_ASSERT_DEV(uiCurveIndex <= 255, "Too many curves");
    if (!ref_context.m_CurveIndices.Contains(uiCurveIndex))
    {
      ref_context.m_CurveIndices.PushBack(uiCurveIndex);
    }
  }

  WExpressionAST::Node* arguments[] = {
    out_ast.CreateConstant(uiCurveIndex, WExpressionAST::DataType::Int),
    pInput,
  };

  return out_ast.CreateFunctionCall(WExtendedExpressionFunctions::s_SampleCurveFunc.m_Desc, arguments);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_Contrast, 2, WRTTIDefaultAllocator<WProcGen_Contrast>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Input", m_fInputValue)->AddAttributes(new WDefaultValueAttribute(0.5f)),
    W_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new WClampValueAttribute(-10.0f, 10.0f)),

    W_MEMBER_PROPERTY("X", m_InputValuePin),
    W_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Contrast: {Contrast}"),
    new WCategoryAttribute("Math"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_Contrast::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_ast.CreateConstant(m_fInputValue);
  }

  auto pA = out_ast.CreateConstant(-m_fContrast);
  auto pB = out_ast.CreateConstant(1.0f + m_fContrast);
  auto pLerp = out_ast.CreateTernaryOperator(WExpressionAST::NodeType::Lerp, pA, pB, pInput);
  auto pSaturate = out_ast.CreateUnaryOperator(WExpressionAST::NodeType::Saturate, pLerp);

  return pSaturate;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_Height, 2, WRTTIDefaultAllocator<WProcGen_Height>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MinHeight", m_fMinHeight)->AddAttributes(new WDefaultValueAttribute(0.0f)),
    W_MEMBER_PROPERTY("MaxHeight", m_fMaxHeight)->AddAttributes(new WDefaultValueAttribute(1000.0f)),
    W_MEMBER_PROPERTY("LowerFade", m_fLowerFade)->AddAttributes(new WDefaultValueAttribute(0.2f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("UpperFade", m_fUpperFade)->AddAttributes(new WDefaultValueAttribute(0.2f), new WClampValueAttribute(0.0f, 1.0f)),

    W_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Height: [{MinHeight}, {MaxHeight}]"),
    new WCategoryAttribute("Input"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_Height::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pHeight = out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPositionZ, WProcessingStream::DataType::Float});
  return CreateRemapTo01WithFadeout(pHeight, m_fMinHeight, m_fMaxHeight, m_fLowerFade, m_fUpperFade, out_ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_Slope, 2, WRTTIDefaultAllocator<WProcGen_Slope>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MinSlope", m_MinSlope)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(0.0f))),
    W_MEMBER_PROPERTY("MaxSlope", m_MaxSlope)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(60.0f))),
    W_MEMBER_PROPERTY("LowerFade", m_fLowerFade)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("UpperFade", m_fUpperFade)->AddAttributes(new WDefaultValueAttribute(0.2f), new WClampValueAttribute(0.0f, 1.0f)),


    W_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Slope: [{MinSlope}, {MaxSlope}]"),
    new WCategoryAttribute("Input"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_Slope::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pNormalZ = out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sNormalZ, WProcessingStream::DataType::Float});
  // acos explodes for values slightly larger than 1 so make sure to clamp before
  auto pClampedNormalZ = out_ast.CreateBinaryOperator(WExpressionAST::NodeType::Min, out_ast.CreateConstant(1.0f), pNormalZ);
  auto pAngle = out_ast.CreateUnaryOperator(WExpressionAST::NodeType::ACos, pClampedNormalZ);
  return CreateRemapTo01WithFadeout(pAngle, m_MinSlope.GetRadian(), m_MaxSlope.GetRadian(), m_fLowerFade, m_fUpperFade, out_ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_Position, 2, WRTTIDefaultAllocator<WProcGen_Position>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("X", m_XPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Red))),
    W_MEMBER_PROPERTY("Y", m_YPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Green))),
    W_MEMBER_PROPERTY("Z", m_ZPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue))),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Position"),
    new WCategoryAttribute("Input"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_Position::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  if (sOutputName == "X")
  {
    return out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPositionX, WProcessingStream::DataType::Float});
  }
  else if (sOutputName == "Y")
  {
    return out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPositionY, WProcessingStream::DataType::Float});
  }
  else
  {
    W_ASSERT_DEBUG(sOutputName == "Z", "Implementation error");
    return out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPositionZ, WProcessingStream::DataType::Float});
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_Normal, 2, WRTTIDefaultAllocator<WProcGen_Normal>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("X", m_XPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Red))),
    W_MEMBER_PROPERTY("Y", m_YPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Green))),
    W_MEMBER_PROPERTY("Z", m_ZPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue))),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Normal"),
    new WCategoryAttribute("Input"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_Normal::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  if (sOutputName == "X")
  {
    return out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sNormalX, WProcessingStream::DataType::Float});
  }
  else if (sOutputName == "Y")
  {
    return out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sNormalY, WProcessingStream::DataType::Float});
  }
  else
  {
    W_ASSERT_DEBUG(sOutputName == "Z", "Implementation error");
    return out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sNormalZ, WProcessingStream::DataType::Float});
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_MeshVertexColor, 2, WRTTIDefaultAllocator<WProcGen_MeshVertexColor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Red))),
    W_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Green))),
    W_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue))),
    W_MEMBER_PROPERTY("A", m_APin),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Mesh Vertex Color"),
    new WCategoryAttribute("Input"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_MeshVertexColor::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  if (sOutputName == "R")
  {
    return out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sColorR, WProcessingStream::DataType::Float});
  }
  else if (sOutputName == "G")
  {
    return out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sColorG, WProcessingStream::DataType::Float});
  }
  else if (sOutputName == "B")
  {
    return out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sColorB, WProcessingStream::DataType::Float});
  }
  else
  {
    W_ASSERT_DEBUG(sOutputName == "A", "Implementation error");
    return out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sColorA, WProcessingStream::DataType::Float});
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGen_ApplyVolumes, 2, WRTTIDefaultAllocator<WProcGen_ApplyVolumes>)
{
  W_BEGIN_PROPERTIES
  {
    W_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new WTagSetWidgetAttribute("Default")),

    W_MEMBER_PROPERTY("InputValue", m_fInputValue),

    W_ENUM_MEMBER_PROPERTY("ImageVolumeMode", WProcVolumeImageMode, m_ImageVolumeMode),
    W_MEMBER_PROPERTY("RefColor", m_RefColor)->AddAttributes(new WExposeColorAlphaAttribute()),

    W_MEMBER_PROPERTY("In", m_InputValuePin),
    W_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Volumes: {IncludeTags}"),
    new WCategoryAttribute("Modifiers"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExpressionAST::Node* WProcGen_ApplyVolumes::GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context)
{
  W_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  WUInt32 tagSetIndex = ref_context.m_SharedData.AddTagSet(m_IncludeTags);
  W_ASSERT_DEV(tagSetIndex <= 255, "Too many tag sets");
  if (!ref_context.m_VolumeTagSetIndices.Contains(tagSetIndex))
  {
    ref_context.m_VolumeTagSetIndices.PushBack(tagSetIndex);
  }

  auto pPosX = out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPositionX, WProcessingStream::DataType::Float});
  auto pPosY = out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPositionY, WProcessingStream::DataType::Float});
  auto pPosZ = out_ast.CreateInput({WProcGenInternal::ExpressionInputs::s_sPositionZ, WProcessingStream::DataType::Float});

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_ast.CreateConstant(m_fInputValue);
  }

  WExpressionAST::Node* arguments[] = {
    pPosX,
    pPosY,
    pPosZ,
    pInput,
    out_ast.CreateConstant(tagSetIndex, WExpressionAST::DataType::Int),
    out_ast.CreateConstant(m_ImageVolumeMode.GetValue(), WExpressionAST::DataType::Int),
    out_ast.CreateConstant(WMath::ColorByteToFloat(m_RefColor.r)),
    out_ast.CreateConstant(WMath::ColorByteToFloat(m_RefColor.g)),
    out_ast.CreateConstant(WMath::ColorByteToFloat(m_RefColor.b)),
    out_ast.CreateConstant(WMath::ColorByteToFloat(m_RefColor.a)),
  };

  return out_ast.CreateFunctionCall(WProcGenExpressionFunctions::s_ApplyVolumesFunc.m_Desc, arguments);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WProcGen_Blend_1_2 : public WGraphPatch
{
public:
  WProcGen_Blend_1_2()
    : WGraphPatch("WProcGen_Blend", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pMode = pNode->FindProperty("Mode");
    if (pMode && pMode->m_Value.IsA<WString>())
    {
      WStringBuilder val = pMode->m_Value.Get<WString>();
      val.ReplaceAll("WProcGenBlendMode", "WProcGenBinaryOperator");

      pNode->AddProperty("Operator", val.GetData());
    }
  }
};

WProcGen_Blend_1_2 g_WProcGen_Blend_1_2;
