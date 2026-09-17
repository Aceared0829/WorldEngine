#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>

class W_FOUNDATION_DLL WExpressionParser
{
public:
  WExpressionParser();
  ~WExpressionParser();

  static const WHashTable<WHashedString, WEnum<WExpressionAST::DataType>>& GetKnownTypes();
  static const WHashTable<WHashedString, WEnum<WExpressionAST::NodeType>>& GetBuiltinFunctions();

  void RegisterFunction(const WExpression::FunctionDesc& funcDesc);
  void UnregisterFunction(const WExpression::FunctionDesc& funcDesc);

  struct Options
  {
    bool m_bTreatUnknownVariablesAsInputs = false;
  };

  WResult Parse(WStringView sCode, WArrayPtr<WExpression::StreamDesc> inputs, WArrayPtr<WExpression::StreamDesc> outputs, const Options& options, WExpressionAST& out_ast);

private:
  static constexpr int s_iLowestPrecedence = 20;

  static void RegisterKnownTypes();
  static void RegisterBuiltinFunctions();
  void SetupInAndOutputs(WArrayPtr<WExpression::StreamDesc> inputs, WArrayPtr<WExpression::StreamDesc> outputs);

  WResult ParseStatement();
  WResult ParseType(WStringView sTypeName, WEnum<WExpressionAST::DataType>& out_type);
  WResult ParseVariableDefinition(WEnum<WExpressionAST::DataType> type);
  WResult ParseAssignment();

  WExpressionAST::Node* ParseFactor();
  WExpressionAST::Node* ParseExpression(int iPrecedence = s_iLowestPrecedence);
  WExpressionAST::Node* ParseUnaryExpression();
  WExpressionAST::Node* ParseFunctionCall(WStringView sFunctionName);
  WExpressionAST::Node* ParseSwizzle(WExpressionAST::Node* pExpression);

  bool AcceptStatementTerminator();
  bool AcceptOperator(WStringView sName);
  bool AcceptBinaryOperator(WExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence, WUInt32& out_uiOperatorLength);
  WExpressionAST::Node* GetVariable(WStringView sVarName);
  WExpressionAST::Node* EnsureExpectedType(WExpressionAST::Node* pNode, WExpressionAST::DataType::Enum expectedType);
  WExpressionAST::Node* Unpack(WExpressionAST::Node* pNode, bool bUnassignedError = true);

  WResult Expect(WStringView sToken, const WToken** pExpectedToken = nullptr);
  WResult Expect(WTokenType::Enum Type, const WToken** pExpectedToken = nullptr);

  void ReportError(const WToken* pToken, const WFormatString& message);

  /// Checks whether all outputs have been written
  WResult CheckOutputs();

  Options m_Options;

  WTokenParseUtils::TokenStream m_TokenStream;
  WUInt32 m_uiCurrentToken = 0;
  WExpressionAST* m_pAST = nullptr;

  WHashTable<WHashedString, WExpressionAST::Node*> m_KnownVariables;
  WHashTable<WHashedString, WHybridArray<WExpression::FunctionDesc, 1>> m_FunctionDescs;
};

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionParser_inl.h>
