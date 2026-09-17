#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Types/Delegate.h>

class WExpressionByteCode;

class W_FOUNDATION_DLL WExpressionCompiler
{
public:
  WExpressionCompiler();
  ~WExpressionCompiler();

  WResult Compile(WExpressionAST& ref_ast, WExpressionByteCode& out_byteCode, WStringView sDebugAstOutputPath = WStringView());

private:
  WResult TransformAndOptimizeAST(WExpressionAST& ast, WStringView sDebugAstOutputPath);
  WResult BuildNodeInstructions(const WExpressionAST& ast);
  WResult UpdateRegisterLifetime();
  WResult AssignRegisters();
  WResult GenerateByteCode(const WExpressionAST& ast, WExpressionByteCode& out_byteCode);
  WResult GenerateConstantByteCode(const WExpressionAST::Constant* pConstant);

  using TransformFunc = WDelegate<WExpressionAST::Node*(WExpressionAST::Node*)>;
  WResult TransformASTPreOrder(WExpressionAST& ast, TransformFunc func);
  WResult TransformASTPostOrder(WExpressionAST& ast, TransformFunc func);
  WResult TransformNode(WExpressionAST::Node*& pNode, TransformFunc& func);
  WResult TransformOutputNode(WExpressionAST::Output*& pOutputNode, TransformFunc& func);

  void DumpAST(const WExpressionAST& ast, WStringView sOutputPath, WStringView sSuffix);

  WHybridArray<WExpressionAST::Node*, 64> m_NodeStack;
  WHybridArray<WExpressionAST::Node*, 64> m_NodeInstructions;
  WHashTable<const WExpressionAST::Node*, WUInt32> m_NodeToRegisterIndex;
  WHashTable<WExpressionAST::Node*, WExpressionAST::Node*> m_TransformCache;

  WHashTable<WHashedString, WUInt32> m_InputToIndex;
  WHashTable<WHashedString, WUInt32> m_OutputToIndex;
  WHashTable<WHashedString, WUInt32> m_FunctionToIndex;

  WDynamicArray<WUInt32> m_ByteCode;

  struct LiveInterval
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_uiStart;
    WUInt32 m_uiEnd;
    const WExpressionAST::Node* m_pNode;
  };

  WDynamicArray<LiveInterval> m_LiveIntervals;
};
