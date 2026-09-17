#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
#define ADD_OFFSET(opCode) static_cast<WExpressionByteCode::OpCode::Enum>((opCode) + uiOffset)

  static WExpressionByteCode::OpCode::Enum NodeTypeToOpCode(WExpressionAST::NodeType::Enum nodeType, WExpressionAST::DataType::Enum dataType, bool bRightIsConstant)
  {
    const WExpression::RegisterType::Enum registerType = WExpressionAST::DataType::GetRegisterType(dataType);
    const bool bFloat = registerType == WExpression::RegisterType::Float;
    const bool bInt = registerType == WExpression::RegisterType::Int;
    const WUInt32 uiOffset = bRightIsConstant ? WExpressionByteCode::OpCode::FirstBinaryWithConstant - WExpressionByteCode::OpCode::FirstBinary : 0;

    switch (nodeType)
    {
      case WExpressionAST::NodeType::Absolute:
        return bFloat ? WExpressionByteCode::OpCode::AbsF_R : WExpressionByteCode::OpCode::AbsI_R;
      case WExpressionAST::NodeType::Sqrt:
        return WExpressionByteCode::OpCode::SqrtF_R;

      case WExpressionAST::NodeType::Exp:
        return WExpressionByteCode::OpCode::ExpF_R;
      case WExpressionAST::NodeType::Ln:
        return WExpressionByteCode::OpCode::LnF_R;
      case WExpressionAST::NodeType::Log2:
        return bFloat ? WExpressionByteCode::OpCode::Log2F_R : WExpressionByteCode::OpCode::Log2I_R;
      case WExpressionAST::NodeType::Log10:
        return WExpressionByteCode::OpCode::Log10F_R;
      case WExpressionAST::NodeType::Pow2:
        return WExpressionByteCode::OpCode::Pow2F_R;

      case WExpressionAST::NodeType::Sin:
        return WExpressionByteCode::OpCode::SinF_R;
      case WExpressionAST::NodeType::Cos:
        return WExpressionByteCode::OpCode::CosF_R;
      case WExpressionAST::NodeType::Tan:
        return WExpressionByteCode::OpCode::TanF_R;

      case WExpressionAST::NodeType::ASin:
        return WExpressionByteCode::OpCode::ASinF_R;
      case WExpressionAST::NodeType::ACos:
        return WExpressionByteCode::OpCode::ACosF_R;
      case WExpressionAST::NodeType::ATan:
        return WExpressionByteCode::OpCode::ATanF_R;

      case WExpressionAST::NodeType::Round:
        return WExpressionByteCode::OpCode::RoundF_R;
      case WExpressionAST::NodeType::Floor:
        return WExpressionByteCode::OpCode::FloorF_R;
      case WExpressionAST::NodeType::Ceil:
        return WExpressionByteCode::OpCode::CeilF_R;
      case WExpressionAST::NodeType::Trunc:
        return WExpressionByteCode::OpCode::TruncF_R;

      case WExpressionAST::NodeType::BitwiseNot:
        return WExpressionByteCode::OpCode::NotI_R;
      case WExpressionAST::NodeType::LogicalNot:
        return WExpressionByteCode::OpCode::NotB_R;

      case WExpressionAST::NodeType::TypeConversion:
        return bFloat ? WExpressionByteCode::OpCode::IToF_R : WExpressionByteCode::OpCode::FToI_R;

      case WExpressionAST::NodeType::Add:
        return ADD_OFFSET(bFloat ? WExpressionByteCode::OpCode::AddF_RR : WExpressionByteCode::OpCode::AddI_RR);
      case WExpressionAST::NodeType::Subtract:
        return ADD_OFFSET(bFloat ? WExpressionByteCode::OpCode::SubF_RR : WExpressionByteCode::OpCode::SubI_RR);
      case WExpressionAST::NodeType::Multiply:
        return ADD_OFFSET(bFloat ? WExpressionByteCode::OpCode::MulF_RR : WExpressionByteCode::OpCode::MulI_RR);
      case WExpressionAST::NodeType::Divide:
        return ADD_OFFSET(bFloat ? WExpressionByteCode::OpCode::DivF_RR : WExpressionByteCode::OpCode::DivI_RR);
      case WExpressionAST::NodeType::Min:
        return ADD_OFFSET(bFloat ? WExpressionByteCode::OpCode::MinF_RR : WExpressionByteCode::OpCode::MinI_RR);
      case WExpressionAST::NodeType::Max:
        return ADD_OFFSET(bFloat ? WExpressionByteCode::OpCode::MaxF_RR : WExpressionByteCode::OpCode::MaxI_RR);

      case WExpressionAST::NodeType::BitshiftLeft:
        return ADD_OFFSET(WExpressionByteCode::OpCode::ShlI_RR);
      case WExpressionAST::NodeType::BitshiftRight:
        return ADD_OFFSET(WExpressionByteCode::OpCode::ShrI_RR);
      case WExpressionAST::NodeType::BitwiseAnd:
        return ADD_OFFSET(WExpressionByteCode::OpCode::AndI_RR);
      case WExpressionAST::NodeType::BitwiseXor:
        return ADD_OFFSET(WExpressionByteCode::OpCode::XorI_RR);
      case WExpressionAST::NodeType::BitwiseOr:
        return ADD_OFFSET(WExpressionByteCode::OpCode::OrI_RR);

      case WExpressionAST::NodeType::Equal:
        if (bFloat)
          return ADD_OFFSET(WExpressionByteCode::OpCode::EqF_RR);
        else if (bInt)
          return ADD_OFFSET(WExpressionByteCode::OpCode::EqI_RR);
        else
          return ADD_OFFSET(WExpressionByteCode::OpCode::EqB_RR);
      case WExpressionAST::NodeType::NotEqual:
        if (bFloat)
          return ADD_OFFSET(WExpressionByteCode::OpCode::NEqF_RR);
        else if (bInt)
          return ADD_OFFSET(WExpressionByteCode::OpCode::NEqI_RR);
        else
          return ADD_OFFSET(WExpressionByteCode::OpCode::NEqB_RR);
      case WExpressionAST::NodeType::Less:
        return ADD_OFFSET(bFloat ? WExpressionByteCode::OpCode::LtF_RR : WExpressionByteCode::OpCode::LtI_RR);
      case WExpressionAST::NodeType::LessEqual:
        return ADD_OFFSET(bFloat ? WExpressionByteCode::OpCode::LEqF_RR : WExpressionByteCode::OpCode::LEqI_RR);
      case WExpressionAST::NodeType::Greater:
        return ADD_OFFSET(bFloat ? WExpressionByteCode::OpCode::GtF_RR : WExpressionByteCode::OpCode::GtI_RR);
      case WExpressionAST::NodeType::GreaterEqual:
        return ADD_OFFSET(bFloat ? WExpressionByteCode::OpCode::GEqF_RR : WExpressionByteCode::OpCode::GEqI_RR);

      case WExpressionAST::NodeType::LogicalAnd:
        return ADD_OFFSET(WExpressionByteCode::OpCode::AndB_RR);
      case WExpressionAST::NodeType::LogicalOr:
        return ADD_OFFSET(WExpressionByteCode::OpCode::OrB_RR);

      case WExpressionAST::NodeType::Select:
        if (bFloat)
          return WExpressionByteCode::OpCode::SelF_RRR;
        else if (bInt)
          return WExpressionByteCode::OpCode::SelI_RRR;
        else
          return WExpressionByteCode::OpCode::SelB_RRR;

      case WExpressionAST::NodeType::Constant:
        return WExpressionByteCode::OpCode::MovX_C;
      case WExpressionAST::NodeType::Input:
        return bFloat ? WExpressionByteCode::OpCode::LoadF : WExpressionByteCode::OpCode::LoadI;
      case WExpressionAST::NodeType::Output:
        return bFloat ? WExpressionByteCode::OpCode::StoreF : WExpressionByteCode::OpCode::StoreI;
      case WExpressionAST::NodeType::FunctionCall:
        return WExpressionByteCode::OpCode::Call;
      case WExpressionAST::NodeType::ConstructorCall:
        W_REPORT_FAILURE("Constructor calls should not exist anymore after AST transformations");
        return WExpressionByteCode::OpCode::Nop;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
        return WExpressionByteCode::OpCode::Nop;
    }
  }

#undef ADD_OFFSET
} // namespace

WExpressionCompiler::WExpressionCompiler() = default;
WExpressionCompiler::~WExpressionCompiler() = default;

WResult WExpressionCompiler::Compile(WExpressionAST& ref_ast, WExpressionByteCode& out_byteCode, WStringView sDebugAstOutputPath /*= WStringView()*/)
{
  out_byteCode.Clear();

  W_SUCCEED_OR_RETURN(TransformAndOptimizeAST(ref_ast, sDebugAstOutputPath));
  W_SUCCEED_OR_RETURN(BuildNodeInstructions(ref_ast));
  W_SUCCEED_OR_RETURN(UpdateRegisterLifetime());
  W_SUCCEED_OR_RETURN(AssignRegisters());
  W_SUCCEED_OR_RETURN(GenerateByteCode(ref_ast, out_byteCode));

  return W_SUCCESS;
}

WResult WExpressionCompiler::TransformAndOptimizeAST(WExpressionAST& ast, WStringView sDebugAstOutputPath)
{
  DumpAST(ast, sDebugAstOutputPath, "_00");

  W_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, WMakeDelegate(&WExpressionAST::TypeDeductionAndConversion, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_01_TypeConv");

  W_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, WMakeDelegate(&WExpressionAST::ReplaceVectorInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_02_ReplacedVectorInst");

  W_SUCCEED_OR_RETURN(ast.ScalarizeInputs());
  W_SUCCEED_OR_RETURN(ast.ScalarizeOutputs());
  W_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, WMakeDelegate(&WExpressionAST::ScalarizeVectorInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_03_Scalarized");

  W_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, WMakeDelegate(&WExpressionAST::FoldConstants, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_04_ConstantFolded1");

  W_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, WMakeDelegate(&WExpressionAST::ReplaceUnsupportedInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_05_ReplacedUnsupportedInst");

  W_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, WMakeDelegate(&WExpressionAST::FoldConstants, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_06_ConstantFolded2");

  W_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, WMakeDelegate(&WExpressionAST::CommonSubexpressionElimination, &ast)));
  W_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, WMakeDelegate(&WExpressionAST::Validate, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_07_Optimized");

  return W_SUCCESS;
}

WResult WExpressionCompiler::BuildNodeInstructions(const WExpressionAST& ast)
{
  m_NodeStack.Clear();
  m_NodeInstructions.Clear();
  auto& nodeStackTemp = m_NodeInstructions;

  // Build node instruction order aka post order tree traversal
  for (WExpressionAST::Node* pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      return W_FAILURE;

    W_ASSERT_DEV(nodeStackTemp.IsEmpty(), "Implementation error");

    nodeStackTemp.PushBack(pOutputNode);

    while (!nodeStackTemp.IsEmpty())
    {
      auto pCurrentNode = nodeStackTemp.PeekBack();
      nodeStackTemp.PopBack();

      if (pCurrentNode == nullptr)
      {
        return W_FAILURE;
      }

      m_NodeStack.PushBack(pCurrentNode);

      if (WExpressionAST::NodeType::IsBinary(pCurrentNode->m_Type))
      {
        auto pBinary = static_cast<const WExpressionAST::BinaryOperator*>(pCurrentNode);
        nodeStackTemp.PushBack(pBinary->m_pLeftOperand);

        // Do not push the right operand if it is a constant, we don't want a separate mov instruction for it
        // since all binary operators can take a constant as right operand in place.
        const bool bRightIsConstant = WExpressionAST::NodeType::IsConstant(pBinary->m_pRightOperand->m_Type);
        if (!bRightIsConstant)
        {
          nodeStackTemp.PushBack(pBinary->m_pRightOperand);
        }
      }
      else
      {
        auto children = WExpressionAST::GetChildren(pCurrentNode);
        for (auto pChild : children)
        {
          nodeStackTemp.PushBack(pChild);
        }
      }
    }
  }

  if (m_NodeStack.IsEmpty())
  {
    // Nothing to compile
    return W_FAILURE;
  }

  W_ASSERT_DEV(m_NodeInstructions.IsEmpty(), "Implementation error");

  m_NodeToRegisterIndex.Clear();
  m_LiveIntervals.Clear();
  WUInt32 uiNextRegisterIndex = 0;

  // De-duplicate nodes, build final instruction list and assign virtual register indices. Also determine their lifetime start.
  while (!m_NodeStack.IsEmpty())
  {
    auto pCurrentNode = m_NodeStack.PeekBack();
    m_NodeStack.PopBack();

    if (!m_NodeToRegisterIndex.Contains(pCurrentNode))
    {
      m_NodeInstructions.PushBack(pCurrentNode);

      if (WExpressionAST::NodeType::IsOutput(pCurrentNode->m_Type))
        continue;

      m_NodeToRegisterIndex.Insert(pCurrentNode, uiNextRegisterIndex);
      ++uiNextRegisterIndex;

      WUInt32 uiCurrentInstructionIndex = m_NodeInstructions.GetCount() - 1;
      m_LiveIntervals.PushBack({uiCurrentInstructionIndex, uiCurrentInstructionIndex, pCurrentNode});
      W_ASSERT_DEV(m_LiveIntervals.GetCount() == uiNextRegisterIndex, "Implementation error");
    }
  }

  return W_SUCCESS;
}

WResult WExpressionCompiler::UpdateRegisterLifetime()
{
  WUInt32 uiNumInstructions = m_NodeInstructions.GetCount();
  for (WUInt32 uiInstructionIndex = 0; uiInstructionIndex < uiNumInstructions; ++uiInstructionIndex)
  {
    auto pCurrentNode = m_NodeInstructions[uiInstructionIndex];

    auto children = WExpressionAST::GetChildren(pCurrentNode);
    for (auto pChild : children)
    {
      WUInt32 uiRegisterIndex = WInvalidIndex;
      if (m_NodeToRegisterIndex.TryGetValue(pChild, uiRegisterIndex))
      {
        auto& liveRegister = m_LiveIntervals[uiRegisterIndex];

        liveRegister.m_uiStart = WMath::Min(liveRegister.m_uiStart, uiInstructionIndex);
        liveRegister.m_uiEnd = WMath::Max(liveRegister.m_uiEnd, uiInstructionIndex);
      }
      else
      {
        W_ASSERT_DEV(WExpressionAST::NodeType::IsConstant(pChild->m_Type), "Must have a valid register for nodes that are not constants");
      }
    }
  }

  return W_SUCCESS;
}

WResult WExpressionCompiler::AssignRegisters()
{
  // This is an implementation of the linear scan register allocation algorithm without spilling
  // https://www2.seas.gwu.edu/~hchoi/teaching/cs160d/linearscan.pdf

  // Sort register lifetime by start index
  m_LiveIntervals.Sort([](const LiveInterval& a, const LiveInterval& b)
    { return a.m_uiStart < b.m_uiStart; });

  // Assign registers
  WTempHybridArray<LiveInterval, 64> activeIntervals;
  WTempHybridArray<WUInt32, 64> freeRegisters;

  for (auto& liveInterval : m_LiveIntervals)
  {
    // Expire old intervals
    for (WUInt32 uiActiveIndex = activeIntervals.GetCount(); uiActiveIndex-- > 0;)
    {
      auto& activeInterval = activeIntervals[uiActiveIndex];
      if (activeInterval.m_uiEnd <= liveInterval.m_uiStart)
      {
        WUInt32 uiRegisterIndex = m_NodeToRegisterIndex[activeInterval.m_pNode];
        freeRegisters.PushBack(uiRegisterIndex);

        activeIntervals.RemoveAtAndCopy(uiActiveIndex);
      }
    }

    // Allocate register
    WUInt32 uiNewRegister = 0;
    if (!freeRegisters.IsEmpty())
    {
      uiNewRegister = freeRegisters.PeekBack();
      freeRegisters.PopBack();
    }
    else
    {
      uiNewRegister = activeIntervals.GetCount();
    }
    m_NodeToRegisterIndex[liveInterval.m_pNode] = uiNewRegister;

    activeIntervals.PushBack(liveInterval);
  }

  return W_SUCCESS;
}

WResult WExpressionCompiler::GenerateByteCode(const WExpressionAST& ast, WExpressionByteCode& out_byteCode)
{
  WTempHybridArray<WExpression::StreamDesc, 8> inputs;
  WTempHybridArray<WExpression::StreamDesc, 8> outputs;
  WTempHybridArray<WExpression::FunctionDesc, 4> functions;

  m_ByteCode.Clear();

  WUInt32 uiMaxRegisterIndex = 0;

  m_InputToIndex.Clear();
  for (WUInt32 i = 0; i < ast.m_InputNodes.GetCount(); ++i)
  {
    auto& desc = ast.m_InputNodes[i]->m_Desc;
    m_InputToIndex.Insert(desc.m_sName, i);

    inputs.PushBack(desc);
  }

  m_OutputToIndex.Clear();
  for (WUInt32 i = 0; i < ast.m_OutputNodes.GetCount(); ++i)
  {
    auto& desc = ast.m_OutputNodes[i]->m_Desc;
    m_OutputToIndex.Insert(desc.m_sName, i);

    outputs.PushBack(desc);
  }

  m_FunctionToIndex.Clear();

  for (auto pCurrentNode : m_NodeInstructions)
  {
    const WExpressionAST::NodeType::Enum nodeType = pCurrentNode->m_Type;
    WExpressionAST::DataType::Enum dataType = pCurrentNode->m_ReturnType;
    if (dataType == WExpressionAST::DataType::Unknown)
    {
      return W_FAILURE;
    }

    bool bRightIsConstant = false;
    if (WExpressionAST::NodeType::IsBinary(nodeType))
    {
      auto pBinary = static_cast<const WExpressionAST::BinaryOperator*>(pCurrentNode);
      dataType = pBinary->m_pLeftOperand->m_ReturnType;
      bRightIsConstant = WExpressionAST::NodeType::IsConstant(pBinary->m_pRightOperand->m_Type);
    }

    const auto opCode = NodeTypeToOpCode(nodeType, dataType, bRightIsConstant);
    if (opCode == WExpressionByteCode::OpCode::Nop)
      return W_FAILURE;

    WUInt32 uiTargetRegister = m_NodeToRegisterIndex[pCurrentNode];
    if (WExpressionAST::NodeType::IsOutput(nodeType) == false)
    {
      uiMaxRegisterIndex = WMath::Max(uiMaxRegisterIndex, uiTargetRegister);
    }

    if (WExpressionAST::NodeType::IsUnary(nodeType))
    {
      auto pUnary = static_cast<const WExpressionAST::UnaryOperator*>(pCurrentNode);

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiTargetRegister);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pUnary->m_pOperand]);
    }
    else if (WExpressionAST::NodeType::IsBinary(nodeType))
    {
      auto pBinary = static_cast<const WExpressionAST::BinaryOperator*>(pCurrentNode);

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiTargetRegister);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pBinary->m_pLeftOperand]);

      if (bRightIsConstant)
      {
        W_SUCCEED_OR_RETURN(GenerateConstantByteCode(static_cast<const WExpressionAST::Constant*>(pBinary->m_pRightOperand)));
      }
      else
      {
        m_ByteCode.PushBack(m_NodeToRegisterIndex[pBinary->m_pRightOperand]);
      }
    }
    else if (WExpressionAST::NodeType::IsTernary(nodeType))
    {
      auto pTernary = static_cast<const WExpressionAST::TernaryOperator*>(pCurrentNode);

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiTargetRegister);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pFirstOperand]);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pSecondOperand]);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pThirdOperand]);
    }
    else if (WExpressionAST::NodeType::IsConstant(nodeType))
    {
      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiTargetRegister);
      W_SUCCEED_OR_RETURN(GenerateConstantByteCode(static_cast<const WExpressionAST::Constant*>(pCurrentNode)));
    }
    else if (WExpressionAST::NodeType::IsInput(nodeType))
    {
      auto& desc = static_cast<const WExpressionAST::Input*>(pCurrentNode)->m_Desc;
      WUInt32 uiInputIndex = 0;
      if (!m_InputToIndex.TryGetValue(desc.m_sName, uiInputIndex))
      {
        uiInputIndex = inputs.GetCount();
        m_InputToIndex.Insert(desc.m_sName, uiInputIndex);

        inputs.PushBack(desc);
      }

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiTargetRegister);
      m_ByteCode.PushBack(uiInputIndex);
    }
    else if (WExpressionAST::NodeType::IsOutput(nodeType))
    {
      auto pOutput = static_cast<const WExpressionAST::Output*>(pCurrentNode);
      auto& desc = pOutput->m_Desc;
      WUInt32 uiOutputIndex = 0;
      W_VERIFY(m_OutputToIndex.TryGetValue(desc.m_sName, uiOutputIndex), "Invalid output '{}'", desc.m_sName);

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiOutputIndex);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pOutput->m_pExpression]);
    }
    else if (WExpressionAST::NodeType::IsFunctionCall(nodeType))
    {
      auto pFunctionCall = static_cast<const WExpressionAST::FunctionCall*>(pCurrentNode);
      auto pDesc = pFunctionCall->m_Descs[pCurrentNode->m_uiOverloadIndex];
      WHashedString sMangledName = pDesc->GetMangledName();

      WUInt32 uiFunctionIndex = 0;
      if (!m_FunctionToIndex.TryGetValue(sMangledName, uiFunctionIndex))
      {
        uiFunctionIndex = functions.GetCount();
        m_FunctionToIndex.Insert(sMangledName, uiFunctionIndex);

        functions.PushBack(*pDesc);
        functions.PeekBack().m_sName = std::move(sMangledName);
      }

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiFunctionIndex);
      m_ByteCode.PushBack(uiTargetRegister);

      m_ByteCode.PushBack(pFunctionCall->m_Arguments.GetCount());
      for (auto pArg : pFunctionCall->m_Arguments)
      {
        WUInt32 uiArgRegister = m_NodeToRegisterIndex[pArg];
        m_ByteCode.PushBack(uiArgRegister);
      }
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }
  }

  out_byteCode.Init(m_ByteCode, inputs, outputs, functions, uiMaxRegisterIndex + 1, m_NodeInstructions.GetCount());
  return W_SUCCESS;
}

WResult WExpressionCompiler::GenerateConstantByteCode(const WExpressionAST::Constant* pConstant)
{
  if (pConstant->m_ReturnType == WExpressionAST::DataType::Float)
  {
    m_ByteCode.PushBack(*reinterpret_cast<const WUInt32*>(&pConstant->m_Value.Get<float>()));
    return W_SUCCESS;
  }
  else if (pConstant->m_ReturnType == WExpressionAST::DataType::Int)
  {
    m_ByteCode.PushBack(pConstant->m_Value.Get<int>());
    return W_SUCCESS;
  }
  else if (pConstant->m_ReturnType == WExpressionAST::DataType::Bool)
  {
    m_ByteCode.PushBack(pConstant->m_Value.Get<bool>() ? 0xFFFFFFFF : 0);
    return W_SUCCESS;
  }

  W_ASSERT_NOT_IMPLEMENTED;
  return W_FAILURE;
}

WResult WExpressionCompiler::TransformASTPreOrder(WExpressionAST& ast, TransformFunc func)
{
  m_NodeStack.Clear();
  m_TransformCache.Clear();

  for (WExpressionAST::Output*& pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      return W_FAILURE;

    W_SUCCEED_OR_RETURN(TransformOutputNode(pOutputNode, func));

    m_NodeStack.PushBack(pOutputNode);

    while (!m_NodeStack.IsEmpty())
    {
      auto pParent = m_NodeStack.PeekBack();
      m_NodeStack.PopBack();

      auto children = WExpressionAST::GetChildren(pParent);
      for (auto& pChild : children)
      {
        W_SUCCEED_OR_RETURN(TransformNode(pChild, func));

        m_NodeStack.PushBack(pChild);
      }
    }
  }

  return W_SUCCESS;
}

WResult WExpressionCompiler::TransformASTPostOrder(WExpressionAST& ast, TransformFunc func)
{
  m_NodeStack.Clear();
  m_NodeInstructions.Clear();
  auto& nodeStackTemp = m_NodeInstructions;

  for (WExpressionAST::Node* pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      return W_FAILURE;

    nodeStackTemp.PushBack(pOutputNode);

    while (!nodeStackTemp.IsEmpty())
    {
      auto pParent = nodeStackTemp.PeekBack();
      nodeStackTemp.PopBack();

      m_NodeStack.PushBack(pParent);

      auto children = WExpressionAST::GetChildren(pParent);
      for (auto pChild : children)
      {
        if (pChild != nullptr)
        {
          nodeStackTemp.PushBack(pChild);
        }
      }
    }
  }

  m_TransformCache.Clear();

  while (!m_NodeStack.IsEmpty())
  {
    auto pParent = m_NodeStack.PeekBack();
    m_NodeStack.PopBack();

    auto children = WExpressionAST::GetChildren(pParent);
    for (auto& pChild : children)
    {
      W_SUCCEED_OR_RETURN(TransformNode(pChild, func));
    }
  }

  for (WExpressionAST::Output*& pOutputNode : ast.m_OutputNodes)
  {
    W_SUCCEED_OR_RETURN(TransformOutputNode(pOutputNode, func));
  }

  return W_SUCCESS;
}

WResult WExpressionCompiler::TransformNode(WExpressionAST::Node*& pNode, TransformFunc& func)
{
  if (pNode == nullptr)
    return W_SUCCESS;

  WExpressionAST::Node* pNewNode = nullptr;
  if (m_TransformCache.TryGetValue(pNode, pNewNode) == false)
  {
    pNewNode = func(pNode);
    if (pNewNode == nullptr)
    {
      return W_FAILURE;
    }

    m_TransformCache.Insert(pNode, pNewNode);
  }

  pNode = pNewNode;

  return W_SUCCESS;
}

WResult WExpressionCompiler::TransformOutputNode(WExpressionAST::Output*& pOutputNode, TransformFunc& func)
{
  if (pOutputNode == nullptr)
    return W_SUCCESS;

  auto pNewOutput = func(pOutputNode);
  if (pNewOutput != pOutputNode)
  {
    if (pNewOutput != nullptr && WExpressionAST::NodeType::IsOutput(pNewOutput->m_Type))
    {
      pOutputNode = static_cast<WExpressionAST::Output*>(pNewOutput);
    }
    else
    {
      WLog::Error("Transformed output node for '{}' is invalid", pOutputNode->m_Desc.m_sName);
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

void WExpressionCompiler::DumpAST(const WExpressionAST& ast, WStringView sOutputPath, WStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  WDGMLGraph dgmlGraph;
  ast.PrintGraph(dgmlGraph);

  WStringView sExt = sOutputPath.GetFileExtension();
  WStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), sSuffix, ".", sExt);

  WDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    WLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    WLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}
