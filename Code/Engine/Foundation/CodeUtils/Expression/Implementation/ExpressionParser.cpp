#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace
{
  struct AssignOperator
  {
    WStringView m_sName;
    WExpressionAST::NodeType::Enum m_NodeType;
  };

  static constexpr AssignOperator s_assignOperators[] = {
    {"+="_wsv, WExpressionAST::NodeType::Add},
    {"-="_wsv, WExpressionAST::NodeType::Subtract},
    {"*="_wsv, WExpressionAST::NodeType::Multiply},
    {"/="_wsv, WExpressionAST::NodeType::Divide},
    {"%="_wsv, WExpressionAST::NodeType::Modulo},
    {"<<="_wsv, WExpressionAST::NodeType::BitshiftLeft},
    {">>="_wsv, WExpressionAST::NodeType::BitshiftRight},
    {"&="_wsv, WExpressionAST::NodeType::BitwiseAnd},
    {"^="_wsv, WExpressionAST::NodeType::BitwiseXor},
    {"|="_wsv, WExpressionAST::NodeType::BitwiseOr},
  };

  struct BinaryOperator
  {
    WStringView m_sName;
    WExpressionAST::NodeType::Enum m_NodeType;
    int m_iPrecedence;
  };

  // Operator precedence according to https://en.cppreference.com/w/cpp/language/operator_precedence,
  // lower value means higher precedence
  // sorted by string length to simplify the test against a token stream
  static constexpr BinaryOperator s_binaryOperators[] = {
    {"&&"_wsv, WExpressionAST::NodeType::LogicalAnd, 14},
    {"||"_wsv, WExpressionAST::NodeType::LogicalOr, 15},
    {"<<"_wsv, WExpressionAST::NodeType::BitshiftLeft, 7},
    {">>"_wsv, WExpressionAST::NodeType::BitshiftRight, 7},
    {"=="_wsv, WExpressionAST::NodeType::Equal, 10},
    {"!="_wsv, WExpressionAST::NodeType::NotEqual, 10},
    {"<="_wsv, WExpressionAST::NodeType::LessEqual, 9},
    {">="_wsv, WExpressionAST::NodeType::GreaterEqual, 9},
    {"<"_wsv, WExpressionAST::NodeType::Less, 9},
    {">"_wsv, WExpressionAST::NodeType::Greater, 9},
    {"&"_wsv, WExpressionAST::NodeType::BitwiseAnd, 11},
    {"^"_wsv, WExpressionAST::NodeType::BitwiseXor, 12},
    {"|"_wsv, WExpressionAST::NodeType::BitwiseOr, 13},
    {"?"_wsv, WExpressionAST::NodeType::Select, 16},
    {"+"_wsv, WExpressionAST::NodeType::Add, 6},
    {"-"_wsv, WExpressionAST::NodeType::Subtract, 6},
    {"*"_wsv, WExpressionAST::NodeType::Multiply, 5},
    {"/"_wsv, WExpressionAST::NodeType::Divide, 5},
    {"%"_wsv, WExpressionAST::NodeType::Modulo, 5},
  };

  static WHashTable<WHashedString, WEnum<WExpressionAST::DataType>> s_KnownTypes;
  static WHashTable<WHashedString, WEnum<WExpressionAST::NodeType>> s_BuiltinFunctions;

} // namespace

using namespace WTokenParseUtils;

WExpressionParser::WExpressionParser()
{
  RegisterKnownTypes();
  RegisterBuiltinFunctions();
}

WExpressionParser::~WExpressionParser() = default;

// static
const WHashTable<WHashedString, WEnum<WExpressionAST::DataType>>& WExpressionParser::GetKnownTypes()
{
  RegisterKnownTypes();

  return s_KnownTypes;
}

// static
const WHashTable<WHashedString, WEnum<WExpressionAST::NodeType>>& WExpressionParser::GetBuiltinFunctions()
{
  RegisterBuiltinFunctions();

  return s_BuiltinFunctions;
}

void WExpressionParser::RegisterFunction(const WExpression::FunctionDesc& funcDesc)
{
  W_ASSERT_DEV(funcDesc.m_uiNumRequiredInputs <= funcDesc.m_InputTypes.GetCount(), "Not enough input types defined. {} inputs are required but only {} types given.", funcDesc.m_uiNumRequiredInputs, funcDesc.m_InputTypes.GetCount());

  auto& functionDescs = m_FunctionDescs[funcDesc.m_sName];
  if (functionDescs.Contains(funcDesc) == false)
  {
    functionDescs.PushBack(funcDesc);
  }
}

void WExpressionParser::UnregisterFunction(const WExpression::FunctionDesc& funcDesc)
{
  if (auto pFunctionDescs = m_FunctionDescs.GetValue(funcDesc.m_sName))
  {
    pFunctionDescs->RemoveAndCopy(funcDesc);
  }
}

WResult WExpressionParser::Parse(WStringView sCode, WArrayPtr<WExpression::StreamDesc> inputs, WArrayPtr<WExpression::StreamDesc> outputs, const Options& options, WExpressionAST& out_ast)
{
  if (sCode.IsEmpty())
    return W_FAILURE;

  m_Options = options;

  m_pAST = &out_ast;
  SetupInAndOutputs(inputs, outputs);

  WTokenizer tokenizer;
  tokenizer.Tokenize(WArrayPtr<const WUInt8>((const WUInt8*)sCode.GetStartPointer(), sCode.GetElementCount()), WLog::GetThreadLocalLogSystem());

  WUInt32 readTokens = 0;
  while (tokenizer.GetNextLine(readTokens, m_TokenStream).Succeeded())
  {
    m_uiCurrentToken = 0;

    while (m_uiCurrentToken < m_TokenStream.GetCount())
    {
      W_SUCCEED_OR_RETURN(ParseStatement());

      if (m_uiCurrentToken < m_TokenStream.GetCount() && AcceptStatementTerminator() == false)
      {
        auto pCurrentToken = m_TokenStream[m_uiCurrentToken];
        ReportError(pCurrentToken, WFmt("Syntax error, unexpected token '{}'", pCurrentToken->m_DataView));
        return W_FAILURE;
      }
    }
  }

  W_SUCCEED_OR_RETURN(CheckOutputs());

  return W_SUCCESS;
}

// static
void WExpressionParser::RegisterKnownTypes()
{
  if (s_KnownTypes.IsEmpty() == false)
    return;

  s_KnownTypes.Insert(WMakeHashedString("var"), WExpressionAST::DataType::Unknown);

  s_KnownTypes.Insert(WMakeHashedString("vec2"), WExpressionAST::DataType::Float2);
  s_KnownTypes.Insert(WMakeHashedString("vec3"), WExpressionAST::DataType::Float3);
  s_KnownTypes.Insert(WMakeHashedString("vec4"), WExpressionAST::DataType::Float4);

  s_KnownTypes.Insert(WMakeHashedString("vec2i"), WExpressionAST::DataType::Int2);
  s_KnownTypes.Insert(WMakeHashedString("vec3i"), WExpressionAST::DataType::Int3);
  s_KnownTypes.Insert(WMakeHashedString("vec4i"), WExpressionAST::DataType::Int4);

  WStringBuilder sTypeName;
  for (WUInt32 type = WExpressionAST::DataType::Bool; type < WExpressionAST::DataType::Count; ++type)
  {
    sTypeName = WExpressionAST::DataType::GetName(static_cast<WExpressionAST::DataType::Enum>(type));
    sTypeName.ToLower();

    WHashedString sTypeNameHashed;
    sTypeNameHashed.Assign(sTypeName);

    s_KnownTypes.Insert(sTypeNameHashed, static_cast<WExpressionAST::DataType::Enum>(type));
  }
}

void WExpressionParser::RegisterBuiltinFunctions()
{
  if (s_BuiltinFunctions.IsEmpty() == false)
    return;

  // Unary
  s_BuiltinFunctions.Insert(WMakeHashedString("abs"), WExpressionAST::NodeType::Absolute);
  s_BuiltinFunctions.Insert(WMakeHashedString("saturate"), WExpressionAST::NodeType::Saturate);
  s_BuiltinFunctions.Insert(WMakeHashedString("sqrt"), WExpressionAST::NodeType::Sqrt);
  s_BuiltinFunctions.Insert(WMakeHashedString("exp"), WExpressionAST::NodeType::Exp);
  s_BuiltinFunctions.Insert(WMakeHashedString("ln"), WExpressionAST::NodeType::Ln);
  s_BuiltinFunctions.Insert(WMakeHashedString("log2"), WExpressionAST::NodeType::Log2);
  s_BuiltinFunctions.Insert(WMakeHashedString("log10"), WExpressionAST::NodeType::Log10);
  s_BuiltinFunctions.Insert(WMakeHashedString("pow2"), WExpressionAST::NodeType::Pow2);
  s_BuiltinFunctions.Insert(WMakeHashedString("sin"), WExpressionAST::NodeType::Sin);
  s_BuiltinFunctions.Insert(WMakeHashedString("cos"), WExpressionAST::NodeType::Cos);
  s_BuiltinFunctions.Insert(WMakeHashedString("tan"), WExpressionAST::NodeType::Tan);
  s_BuiltinFunctions.Insert(WMakeHashedString("asin"), WExpressionAST::NodeType::ASin);
  s_BuiltinFunctions.Insert(WMakeHashedString("acos"), WExpressionAST::NodeType::ACos);
  s_BuiltinFunctions.Insert(WMakeHashedString("atan"), WExpressionAST::NodeType::ATan);
  s_BuiltinFunctions.Insert(WMakeHashedString("radToDeg"), WExpressionAST::NodeType::RadToDeg);
  s_BuiltinFunctions.Insert(WMakeHashedString("rad_to_deg"), WExpressionAST::NodeType::RadToDeg);
  s_BuiltinFunctions.Insert(WMakeHashedString("degToRad"), WExpressionAST::NodeType::DegToRad);
  s_BuiltinFunctions.Insert(WMakeHashedString("deg_to_rad"), WExpressionAST::NodeType::DegToRad);
  s_BuiltinFunctions.Insert(WMakeHashedString("round"), WExpressionAST::NodeType::Round);
  s_BuiltinFunctions.Insert(WMakeHashedString("floor"), WExpressionAST::NodeType::Floor);
  s_BuiltinFunctions.Insert(WMakeHashedString("ceil"), WExpressionAST::NodeType::Ceil);
  s_BuiltinFunctions.Insert(WMakeHashedString("trunc"), WExpressionAST::NodeType::Trunc);
  s_BuiltinFunctions.Insert(WMakeHashedString("frac"), WExpressionAST::NodeType::Frac);
  s_BuiltinFunctions.Insert(WMakeHashedString("length"), WExpressionAST::NodeType::Length);
  s_BuiltinFunctions.Insert(WMakeHashedString("normalize"), WExpressionAST::NodeType::Normalize);
  s_BuiltinFunctions.Insert(WMakeHashedString("all"), WExpressionAST::NodeType::All);
  s_BuiltinFunctions.Insert(WMakeHashedString("any"), WExpressionAST::NodeType::Any);

  // Binary
  s_BuiltinFunctions.Insert(WMakeHashedString("mod"), WExpressionAST::NodeType::Modulo);
  s_BuiltinFunctions.Insert(WMakeHashedString("log"), WExpressionAST::NodeType::Log);
  s_BuiltinFunctions.Insert(WMakeHashedString("pow"), WExpressionAST::NodeType::Pow);
  s_BuiltinFunctions.Insert(WMakeHashedString("min"), WExpressionAST::NodeType::Min);
  s_BuiltinFunctions.Insert(WMakeHashedString("max"), WExpressionAST::NodeType::Max);
  s_BuiltinFunctions.Insert(WMakeHashedString("dot"), WExpressionAST::NodeType::Dot);
  s_BuiltinFunctions.Insert(WMakeHashedString("cross"), WExpressionAST::NodeType::Cross);
  s_BuiltinFunctions.Insert(WMakeHashedString("reflect"), WExpressionAST::NodeType::Reflect);

  // Ternary
  s_BuiltinFunctions.Insert(WMakeHashedString("clamp"), WExpressionAST::NodeType::Clamp);
  s_BuiltinFunctions.Insert(WMakeHashedString("lerp"), WExpressionAST::NodeType::Lerp);
  s_BuiltinFunctions.Insert(WMakeHashedString("smoothstep"), WExpressionAST::NodeType::SmoothStep);
  s_BuiltinFunctions.Insert(WMakeHashedString("smootherstep"), WExpressionAST::NodeType::SmootherStep);
}

void WExpressionParser::SetupInAndOutputs(WArrayPtr<WExpression::StreamDesc> inputs, WArrayPtr<WExpression::StreamDesc> outputs)
{
  m_KnownVariables.Clear();

  for (auto& inputDesc : inputs)
  {
    auto pInput = m_pAST->CreateInput(inputDesc);
    m_pAST->m_InputNodes.PushBack(pInput);
    m_KnownVariables.Insert(inputDesc.m_sName, pInput);
  }

  for (auto& outputDesc : outputs)
  {
    auto pOutputNode = m_pAST->CreateOutput(outputDesc, nullptr);
    m_pAST->m_OutputNodes.PushBack(pOutputNode);
    m_KnownVariables.Insert(outputDesc.m_sName, pOutputNode);
  }
}

WResult WExpressionParser::ParseStatement()
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  if (AcceptStatementTerminator())
  {
    // empty statement
    return W_SUCCESS;
  }

  if (m_uiCurrentToken >= m_TokenStream.GetCount())
    return W_FAILURE;

  const WToken* pIdentifierToken = m_TokenStream[m_uiCurrentToken];
  if (pIdentifierToken->m_iType != WTokenType::Identifier)
  {
    ReportError(pIdentifierToken, "Syntax error, expected type or variable");
  }

  WEnum<WExpressionAST::DataType> type;
  if (ParseType(pIdentifierToken->m_DataView, type).Succeeded())
  {
    return ParseVariableDefinition(type);
  }

  return ParseAssignment();
}

WResult WExpressionParser::ParseType(WStringView sTypeName, WEnum<WExpressionAST::DataType>& out_type)
{
  WTempHashedString sTypeNameHashed(sTypeName);
  if (s_KnownTypes.TryGetValue(sTypeNameHashed, out_type))
  {
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WExpressionParser::ParseVariableDefinition(WEnum<WExpressionAST::DataType> type)
{
  // skip type
  W_SUCCEED_OR_RETURN(Expect(WTokenType::Identifier));

  const WToken* pIdentifierToken = nullptr;
  W_SUCCEED_OR_RETURN(Expect(WTokenType::Identifier, &pIdentifierToken));

  WHashedString sHashedVarName;
  sHashedVarName.Assign(pIdentifierToken->m_DataView);

  WExpressionAST::Node* pVariableNode;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pVariableNode))
  {
    const char* szExisting = "a variable";
    if (WExpressionAST::NodeType::IsInput(pVariableNode->m_Type))
    {
      szExisting = "an input";
    }
    else if (WExpressionAST::NodeType::IsOutput(pVariableNode->m_Type))
    {
      szExisting = "an output";
    }

    ReportError(pIdentifierToken, WFmt("Local variable '{}' cannot be defined because {} of the same name already exists", pIdentifierToken->m_DataView, szExisting));
    return W_FAILURE;
  }

  W_SUCCEED_OR_RETURN(Expect("="));
  WExpressionAST::Node* pExpression = ParseExpression();
  if (pExpression == nullptr)
    return W_FAILURE;

  m_KnownVariables.Insert(sHashedVarName, EnsureExpectedType(pExpression, type));
  return W_SUCCESS;
}

WResult WExpressionParser::ParseAssignment()
{
  const WToken* pIdentifierToken = nullptr;
  W_SUCCEED_OR_RETURN(Expect(WTokenType::Identifier, &pIdentifierToken));

  const WStringView sIdentifier = pIdentifierToken->m_DataView;
  WExpressionAST::Node* pVarNode = GetVariable(sIdentifier);
  if (pVarNode == nullptr)
  {
    ReportError(pIdentifierToken, "Syntax error, expected a valid variable");
    return W_FAILURE;
  }

  WStringView sPartialAssignmentMask;
  if (Accept(m_TokenStream, m_uiCurrentToken, "."))
  {
    const WToken* pSwizzleToken = nullptr;
    if (Expect(WTokenType::Identifier, &pSwizzleToken).Failed())
    {
      ReportError(m_TokenStream[m_uiCurrentToken], "Invalid partial assignment");
      return W_FAILURE;
    }

    sPartialAssignmentMask = pSwizzleToken->m_DataView;
  }

  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  WExpressionAST::NodeType::Enum assignOperator = WExpressionAST::NodeType::Invalid;
  for (WUInt32 i = 0; i < W_ARRAY_SIZE(s_assignOperators); ++i)
  {
    auto& op = s_assignOperators[i];
    if (AcceptOperator(op.m_sName))
    {
      assignOperator = op.m_NodeType;
      m_uiCurrentToken += op.m_sName.GetElementCount();
      break;
    }
  }

  if (assignOperator == WExpressionAST::NodeType::Invalid)
  {
    W_SUCCEED_OR_RETURN(Expect("="));
  }

  WExpressionAST::Node* pExpression = ParseExpression();
  if (pExpression == nullptr)
    return W_FAILURE;

  if (assignOperator != WExpressionAST::NodeType::Invalid)
  {
    pExpression = m_pAST->CreateBinaryOperator(assignOperator, Unpack(pVarNode), pExpression);
  }

  if (sPartialAssignmentMask.IsEmpty() == false)
  {
    auto pConstructor = m_pAST->CreateConstructorCall(Unpack(pVarNode, false), pExpression, sPartialAssignmentMask);
    if (pConstructor == nullptr)
    {
      ReportError(pIdentifierToken, WFmt("Invalid partial assignment .{} = {}", sPartialAssignmentMask, WExpressionAST::DataType::GetName(pExpression->m_ReturnType)));
      return W_FAILURE;
    }

    pExpression = pConstructor;
  }

  if (WExpressionAST::NodeType::IsInput(pVarNode->m_Type))
  {
    ReportError(pIdentifierToken, WFmt("Input '{}' is not assignable", sIdentifier));
    return W_FAILURE;
  }
  else if (WExpressionAST::NodeType::IsOutput(pVarNode->m_Type))
  {
    auto pOutput = static_cast<WExpressionAST::Output*>(pVarNode);
    pOutput->m_pExpression = pExpression;
    return W_SUCCESS;
  }

  WHashedString sHashedVarName;
  sHashedVarName.Assign(sIdentifier);
  m_KnownVariables[sHashedVarName] = EnsureExpectedType(pExpression, pVarNode->m_ReturnType);
  return W_SUCCESS;
}

WExpressionAST::Node* WExpressionParser::ParseFactor()
{
  WUInt32 uiIdentifierToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, WTokenType::Identifier, &uiIdentifierToken))
  {
    auto pIdentifierToken = m_TokenStream[uiIdentifierToken];
    const WStringView sIdentifier = pIdentifierToken->m_DataView;

    if (Accept(m_TokenStream, m_uiCurrentToken, "("))
    {
      return ParseSwizzle(ParseFunctionCall(sIdentifier));
    }
    else if (sIdentifier == "true")
    {
      return m_pAST->CreateConstant(true, WExpressionAST::DataType::Bool);
    }
    else if (sIdentifier == "false")
    {
      return m_pAST->CreateConstant(false, WExpressionAST::DataType::Bool);
    }
    else if (sIdentifier == "PI")
    {
      return m_pAST->CreateConstant(WMath::Pi<float>(), WExpressionAST::DataType::Float);
    }
    else
    {
      auto pVariable = GetVariable(sIdentifier);
      if (pVariable == nullptr)
      {
        ReportError(pIdentifierToken, WFmt("Undeclared identifier '{}'", sIdentifier));
        return nullptr;
      }
      return ParseSwizzle(Unpack(pVariable));
    }
  }

  WUInt32 uiValueToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, WTokenType::Integer, &uiValueToken))
  {
    const WString sVal = m_TokenStream[uiValueToken]->m_DataView;

    WInt64 iConstant = 0;
    if (sVal.StartsWith_NoCase("0x"))
    {
      WUInt64 uiHexConstant = 0;
      WConversionUtils::ConvertHexStringToUInt64(sVal, uiHexConstant).IgnoreResult();
      iConstant = uiHexConstant;
    }
    else
    {
      WConversionUtils::StringToInt64(sVal, iConstant).IgnoreResult();
    }

    return m_pAST->CreateConstant((int)iConstant, WExpressionAST::DataType::Int);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, WTokenType::Float, &uiValueToken))
  {
    const WString sVal = m_TokenStream[uiValueToken]->m_DataView;

    double fConstant = 0;
    WConversionUtils::StringToFloat(sVal, fConstant).IgnoreResult();

    return m_pAST->CreateConstant((float)fConstant, WExpressionAST::DataType::Float);
  }

  if (Accept(m_TokenStream, m_uiCurrentToken, "("))
  {
    auto pExpression = ParseExpression();
    if (Expect(")").Failed())
      return nullptr;

    return ParseSwizzle(pExpression);
  }

  return nullptr;
}

// Parsing the expression - recursive parser using "precedence climbing".
// http://www.engr.mun.ca/~theo/Misc/exp_parsing.htm
WExpressionAST::Node* WExpressionParser::ParseExpression(int iPrecedence /* = s_iLowestPrecedence*/)
{
  auto pExpression = ParseUnaryExpression();
  if (pExpression == nullptr)
    return nullptr;

  WExpressionAST::NodeType::Enum binaryOp;
  int iBinaryOpPrecedence = 0;
  WUInt32 uiOperatorLength = 0;
  while (AcceptBinaryOperator(binaryOp, iBinaryOpPrecedence, uiOperatorLength) && iBinaryOpPrecedence < iPrecedence)
  {
    // Consume token.
    m_uiCurrentToken += uiOperatorLength;

    auto pSecondOperand = ParseExpression(iBinaryOpPrecedence);
    if (pSecondOperand == nullptr)
      return nullptr;

    if (binaryOp == WExpressionAST::NodeType::Select)
    {
      if (Expect(":").Failed())
        return nullptr;

      auto pThirdOperand = ParseExpression(iBinaryOpPrecedence);
      if (pThirdOperand == nullptr)
        return nullptr;

      pExpression = m_pAST->CreateTernaryOperator(WExpressionAST::NodeType::Select, pExpression, pSecondOperand, pThirdOperand);
    }
    else
    {
      pExpression = m_pAST->CreateBinaryOperator(binaryOp, pExpression, pSecondOperand);
    }
  }

  return pExpression;
}

WExpressionAST::Node* WExpressionParser::ParseUnaryExpression()
{
  while (Accept(m_TokenStream, m_uiCurrentToken, "+"))
  {
  }

  if (Accept(m_TokenStream, m_uiCurrentToken, "-"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(WExpressionAST::NodeType::Negate, pOperand);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "~"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(WExpressionAST::NodeType::BitwiseNot, pOperand);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "!"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(WExpressionAST::NodeType::LogicalNot, pOperand);
  }

  return ParseFactor();
}

WExpressionAST::Node* WExpressionParser::ParseFunctionCall(WStringView sFunctionName)
{
  // "(" of the function call
  const WToken* pFunctionToken = m_TokenStream[m_uiCurrentToken - 1];

  WSmallArray<WExpressionAST::Node*, 8> arguments;
  if (Accept(m_TokenStream, m_uiCurrentToken, ")") == false)
  {
    do
    {
      auto pExpression = ParseExpression();
      if (pExpression == nullptr)
      {
        ReportError(m_TokenStream[m_uiCurrentToken], WFmt("Invalid argument {} for '{}'", arguments.GetCount(), sFunctionName));
        return nullptr;
      }

      arguments.PushBack(pExpression);
    } while (Accept(m_TokenStream, m_uiCurrentToken, ","));

    if (Expect(")").Failed())
      return nullptr;
  }

  auto CheckArgumentCount = [&](WUInt32 uiExpectedArgumentCount) -> WResult
  {
    if (arguments.GetCount() != uiExpectedArgumentCount)
    {
      ReportError(pFunctionToken, WFmt("Invalid argument count for '{}'. Expected {} but got {}", sFunctionName, uiExpectedArgumentCount, arguments.GetCount()));
      return W_FAILURE;
    }
    return W_SUCCESS;
  };

  WHashedString sHashedFuncName;
  sHashedFuncName.Assign(sFunctionName);

  WEnum<WExpressionAST::DataType> dataType;
  if (s_KnownTypes.TryGetValue(sHashedFuncName, dataType))
  {
    WUInt32 uiElementCount = WExpressionAST::DataType::GetElementCount(dataType);
    if (arguments.GetCount() > uiElementCount)
    {
      ReportError(pFunctionToken, WFmt("Invalid argument count for '{}'. Expected 0 - {} but got {}", sFunctionName, uiElementCount, arguments.GetCount()));
      return nullptr;
    }

    return m_pAST->CreateConstructorCall(dataType, arguments);
  }

  WEnum<WExpressionAST::NodeType> builtinType;
  if (s_BuiltinFunctions.TryGetValue(sHashedFuncName, builtinType))
  {
    if (WExpressionAST::NodeType::IsUnary(builtinType))
    {
      if (CheckArgumentCount(1).Failed())
        return nullptr;

      return m_pAST->CreateUnaryOperator(builtinType, arguments[0]);
    }
    else if (WExpressionAST::NodeType::IsBinary(builtinType))
    {
      if (CheckArgumentCount(2).Failed())
        return nullptr;

      return m_pAST->CreateBinaryOperator(builtinType, arguments[0], arguments[1]);
    }
    else if (WExpressionAST::NodeType::IsTernary(builtinType))
    {
      if (CheckArgumentCount(3).Failed())
        return nullptr;

      return m_pAST->CreateTernaryOperator(builtinType, arguments[0], arguments[1], arguments[2]);
    }

    W_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  // external function
  const WHybridArray<WExpression::FunctionDesc, 1>* pFunctionDescs = nullptr;
  if (m_FunctionDescs.TryGetValue(sHashedFuncName, pFunctionDescs))
  {
    WUInt32 uiMinArgumentCount = WInvalidIndex;
    for (auto& funcDesc : *pFunctionDescs)
    {
      uiMinArgumentCount = WMath::Min<WUInt32>(uiMinArgumentCount, funcDesc.m_uiNumRequiredInputs);
    }

    if (arguments.GetCount() < uiMinArgumentCount)
    {
      ReportError(pFunctionToken, WFmt("Invalid argument count for '{}'. Expected at least {} but got {}", sFunctionName, uiMinArgumentCount, arguments.GetCount()));
      return nullptr;
    }

    return m_pAST->CreateFunctionCall(*pFunctionDescs, arguments);
  }

  ReportError(pFunctionToken, WFmt("Undeclared function '{}'", sFunctionName));
  return nullptr;
}

WExpressionAST::Node* WExpressionParser::ParseSwizzle(WExpressionAST::Node* pExpression)
{
  if (pExpression != nullptr && Accept(m_TokenStream, m_uiCurrentToken, "."))
  {
    const WToken* pSwizzleToken = nullptr;
    if (Expect(WTokenType::Identifier, &pSwizzleToken).Failed())
      return nullptr;

    pExpression = m_pAST->CreateSwizzle(pSwizzleToken->m_DataView, pExpression);
    if (pExpression == nullptr)
    {
      ReportError(pSwizzleToken, WFmt("Invalid swizzle '{}'", pSwizzleToken->m_DataView));
    }
  }

  return pExpression;
}

// Does NOT advance the current token beyond the operator!
bool WExpressionParser::AcceptOperator(WStringView sName)
{
  const WUInt32 uiOperatorLength = sName.GetElementCount();

  if (m_uiCurrentToken + uiOperatorLength - 1 >= m_TokenStream.GetCount())
    return false;

  for (WUInt32 charIndex = 0; charIndex < uiOperatorLength; ++charIndex)
  {
    const WUInt32 c = sName.GetStartPointer()[charIndex];
    if (m_TokenStream[m_uiCurrentToken + charIndex]->m_DataView.GetCharacter() != c)
    {
      return false;
    }
  }

  return true;
}

// Does NOT advance the current token beyond the binary operator!
bool WExpressionParser::AcceptBinaryOperator(WExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence, WUInt32& out_uiOperatorLength)
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(s_binaryOperators); ++i)
  {
    auto& op = s_binaryOperators[i];
    if (AcceptOperator(op.m_sName))
    {
      out_binaryOp = op.m_NodeType;
      out_iOperatorPrecedence = op.m_iPrecedence;
      out_uiOperatorLength = op.m_sName.GetElementCount();
      return true;
    }
  }

  return false;
}

WExpressionAST::Node* WExpressionParser::GetVariable(WStringView sVarName)
{
  WHashedString sHashedVarName;
  sHashedVarName.Assign(sVarName);

  WExpressionAST::Node* pVariableNode = nullptr;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pVariableNode) == false && m_Options.m_bTreatUnknownVariablesAsInputs)
  {
    pVariableNode = m_pAST->CreateInput({sHashedVarName, WProcessingStream::DataType::Float});
    m_KnownVariables.Insert(sHashedVarName, pVariableNode);
  }

  return pVariableNode;
}

WExpressionAST::Node* WExpressionParser::EnsureExpectedType(WExpressionAST::Node* pNode, WExpressionAST::DataType::Enum expectedType)
{
  if (expectedType != WExpressionAST::DataType::Unknown)
  {
    const auto nodeRegisterType = WExpressionAST::DataType::GetRegisterType(pNode->m_ReturnType);
    const auto expectedRegisterType = WExpressionAST::DataType::GetRegisterType(expectedType);
    if (nodeRegisterType != expectedRegisterType)
    {
      pNode = m_pAST->CreateUnaryOperator(WExpressionAST::NodeType::TypeConversion, pNode, expectedType);
    }

    const WUInt32 nodeElementCount = WExpressionAST::DataType::GetElementCount(pNode->m_ReturnType);
    const WUInt32 expectedElementCount = WExpressionAST::DataType::GetElementCount(expectedType);
    if (nodeElementCount < expectedElementCount)
    {
      pNode = m_pAST->CreateConstructorCall(expectedType, WMakeArrayPtr(&pNode, 1));
    }
  }

  return pNode;
}

WExpressionAST::Node* WExpressionParser::Unpack(WExpressionAST::Node* pNode, bool bUnassignedError /*= true*/)
{
  if (WExpressionAST::NodeType::IsOutput(pNode->m_Type))
  {
    auto pOutput = static_cast<WExpressionAST::Output*>(pNode);
    if (pOutput->m_pExpression == nullptr && bUnassignedError)
    {
      ReportError(m_TokenStream[m_uiCurrentToken], WFmt("Output '{}' has not been assigned yet", pOutput->m_Desc.m_sName));
    }

    return pOutput->m_pExpression;
  }

  return pNode;
}

WResult WExpressionParser::CheckOutputs()
{
  for (auto pOutputNode : m_pAST->m_OutputNodes)
  {
    if (pOutputNode->m_pExpression == nullptr)
    {
      WLog::Error("Output '{}' was never written", pOutputNode->m_Desc.m_sName);
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}
