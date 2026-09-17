#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Memory/LinearAllocator.h>

class WDGMLGraph;

class W_FOUNDATION_DLL WExpressionAST
{
public:
  struct NodeType
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Invalid,
      Default = Invalid,

      // Unary
      FirstUnary,
      Negate,
      Absolute,
      Saturate,
      Sqrt,
      Exp,
      Ln,
      Log2,
      Log10,
      Pow2,
      Sin,
      Cos,
      Tan,
      ASin,
      ACos,
      ATan,
      RadToDeg,
      DegToRad,
      Round,
      Floor,
      Ceil,
      Trunc,
      Frac,
      Length,
      Normalize,
      BitwiseNot,
      LogicalNot,
      All,
      Any,
      TypeConversion,
      LastUnary,

      // Binary
      FirstBinary,
      Add,
      Subtract,
      Multiply,
      Divide,
      Modulo,
      Log,
      Pow,
      Min,
      Max,
      Dot,
      Cross,
      Reflect,
      BitshiftLeft,
      BitshiftRight,
      BitwiseAnd,
      BitwiseXor,
      BitwiseOr,
      Equal,
      NotEqual,
      Less,
      LessEqual,
      Greater,
      GreaterEqual,
      LogicalAnd,
      LogicalOr,
      LastBinary,

      // Ternary
      FirstTernary,
      Clamp,
      Select,
      Lerp,
      SmoothStep,
      SmootherStep,
      LastTernary,

      Constant,
      Swizzle,
      Input,
      Output,

      FunctionCall,
      ConstructorCall,

      Count
    };

    static bool IsUnary(Enum nodeType);
    static bool IsBinary(Enum nodeType);
    static bool IsTernary(Enum nodeType);
    static bool IsConstant(Enum nodeType);
    static bool IsSwizzle(Enum nodeType);
    static bool IsInput(Enum nodeType);
    static bool IsOutput(Enum nodeType);
    static bool IsFunctionCall(Enum nodeType);
    static bool IsConstructorCall(Enum nodeType);

    static bool IsCommutative(Enum nodeType);
    static bool AlwaysReturnsSingleElement(Enum nodeType);

    static const char* GetName(Enum nodeType);
  };

  struct DataType
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Unknown,
      Unknown2,
      Unknown3,
      Unknown4,

      Bool,
      Bool2,
      Bool3,
      Bool4,

      Int,
      Int2,
      Int3,
      Int4,

      Float,
      Float2,
      Float3,
      Float4,

      Count,

      Default = Unknown,
    };

    static WVariantType::Enum GetVariantType(Enum dataType);

    static Enum FromStreamType(WProcessingStream::DataType dataType);

    W_ALWAYS_INLINE static WExpression::RegisterType::Enum GetRegisterType(Enum dataType)
    {
      return static_cast<WExpression::RegisterType::Enum>(dataType >> 2);
    }

    W_ALWAYS_INLINE static Enum FromRegisterType(WExpression::RegisterType::Enum registerType, WUInt32 uiElementCount = 1)
    {
      return static_cast<WExpressionAST::DataType::Enum>((registerType << 2) + uiElementCount - 1);
    }

    W_ALWAYS_INLINE static WUInt32 GetElementCount(Enum dataType) { return (dataType & 0x3) + 1; }

    static const char* GetName(Enum dataType);
  };

  struct VectorComponent
  {
    using StorageType = WUInt8;

    enum Enum
    {
      X,
      Y,
      Z,
      W,

      R = X,
      G = Y,
      B = Z,
      A = W,

      Count,

      Default = X
    };

    static const char* GetName(Enum vectorComponent);

    static Enum FromChar(WUInt32 uiChar);
  };

  struct Node
  {
    WEnum<NodeType> m_Type;
    WEnum<DataType> m_ReturnType;
    WUInt8 m_uiOverloadIndex = 0xFF;
    WUInt8 m_uiNumInputElements = 0;

    WUInt32 m_uiHash = 0;
  };

  struct UnaryOperator : public Node
  {
    Node* m_pOperand = nullptr;
  };

  struct BinaryOperator : public Node
  {
    Node* m_pLeftOperand = nullptr;
    Node* m_pRightOperand = nullptr;
  };

  struct TernaryOperator : public Node
  {
    Node* m_pFirstOperand = nullptr;
    Node* m_pSecondOperand = nullptr;
    Node* m_pThirdOperand = nullptr;
  };

  struct Constant : public Node
  {
    WVariant m_Value;
  };

  struct Swizzle : public Node
  {
    WEnum<VectorComponent> m_Components[4];
    WUInt32 m_NumComponents = 0;
    Node* m_pExpression = nullptr;
  };

  struct Input : public Node
  {
    WExpression::StreamDesc m_Desc;
  };

  struct Output : public Node
  {
    WExpression::StreamDesc m_Desc;
    Node* m_pExpression = nullptr;
  };

  struct FunctionCall : public Node
  {
    WSmallArray<const WExpression::FunctionDesc*, 1> m_Descs;
    WSmallArray<Node*, 8> m_Arguments;
  };

  struct ConstructorCall : public Node
  {
    WSmallArray<Node*, 4> m_Arguments;
  };

public:
  WExpressionAST();
  ~WExpressionAST();

  UnaryOperator* CreateUnaryOperator(NodeType::Enum type, Node* pOperand, DataType::Enum returnType = DataType::Unknown);
  BinaryOperator* CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand);
  TernaryOperator* CreateTernaryOperator(NodeType::Enum type, Node* pFirstOperand, Node* pSecondOperand, Node* pThirdOperand);
  Constant* CreateConstant(const WVariant& value, DataType::Enum dataType = DataType::Float);
  Swizzle* CreateSwizzle(WStringView sSwizzle, Node* pExpression);
  Swizzle* CreateSwizzle(WEnum<VectorComponent> component, Node* pExpression);
  Swizzle* CreateSwizzle(WArrayPtr<WEnum<VectorComponent>> swizzle, Node* pExpression);
  Input* CreateInput(const WExpression::StreamDesc& desc);
  Output* CreateOutput(const WExpression::StreamDesc& desc, Node* pExpression);
  FunctionCall* CreateFunctionCall(const WExpression::FunctionDesc& desc, WArrayPtr<Node*> arguments);
  FunctionCall* CreateFunctionCall(WArrayPtr<const WExpression::FunctionDesc> descs, WArrayPtr<Node*> arguments);
  ConstructorCall* CreateConstructorCall(DataType::Enum dataType, WArrayPtr<Node*> arguments);
  ConstructorCall* CreateConstructorCall(Node* pOldValue, Node* pNewValue, WStringView sPartialAssignmentMask);

  static WArrayPtr<Node*> GetChildren(Node* pNode);
  static WArrayPtr<const Node*> GetChildren(const Node* pNode);

  void PrintGraph(WDGMLGraph& inout_graph) const;

  WSmallArray<Input*, 8> m_InputNodes;
  WSmallArray<Output*, 8> m_OutputNodes;

  // Transforms
  Node* TypeDeductionAndConversion(Node* pNode);
  Node* ReplaceVectorInstructions(Node* pNode);
  Node* ScalarizeVectorInstructions(Node* pNode);
  Node* ReplaceUnsupportedInstructions(Node* pNode);
  Node* FoldConstants(Node* pNode);
  Node* CommonSubexpressionElimination(Node* pNode);
  Node* Validate(Node* pNode);

  WResult ScalarizeInputs();
  WResult ScalarizeOutputs();

private:
  void ResolveOverloads(Node* pNode);

  static DataType::Enum GetExpectedChildDataType(const Node* pNode, WUInt32 uiChildIndex);

  static void UpdateHash(Node* pNode);
  static bool IsEqual(const Node* pNodeA, const Node* pNodeB);

  WLinearAllocator<> m_Allocator;

  WSet<WExpression::FunctionDesc> m_FunctionDescs;

  WHashTable<WUInt32, WSmallArray<Node*, 1>> m_NodeDeduplicationTable;
};
