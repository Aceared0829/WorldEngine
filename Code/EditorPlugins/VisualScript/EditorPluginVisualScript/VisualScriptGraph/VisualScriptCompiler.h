#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>

class WVisualScriptCompiler
{
public:
  WVisualScriptCompiler(WVisualScriptNodeManager& ref_nodeManager);
  ~WVisualScriptCompiler();

  void InitModule(WStringView sBaseClassName, WStringView sScriptClassName);

  WResult AddFunction(WStringView sName, const WDocumentObject* pEntryObject, const WDocumentObject* pParentObject = nullptr);

  WResult Compile(WStringView sDebugAstOutputPath = WStringView());

  struct CompiledFunction
  {
    WString m_sName;
    WEnum<WVisualScriptNodeDescription::Type> m_Type;
    WEnum<WScriptCoroutineCreationMode> m_CoroutineCreationMode;
    WDynamicArray<WVisualScriptNodeDescription> m_NodeDescriptions;
    WVisualScriptDataDescription m_LocalDataDesc;
  };

  struct CompiledModule
  {
    CompiledModule();

    WResult Serialize(WStreamWriter& inout_stream) const;

    WString m_sBaseClassName;
    WString m_sScriptClassName;
    WHybridArray<CompiledFunction, 16> m_Functions;

    WVisualScriptDataDescription m_InstanceDataDesc;
    WVisualScriptInstanceDataMapping m_InstanceDataMapping;

    WVisualScriptDataDescription m_ConstantDataDesc;
    WVisualScriptDataStorage m_ConstantDataStorage;
    WHashTable<WVariant, WUInt32> m_ConstantDataToIndex;
  };

  const CompiledModule& GetCompiledModule() const { return m_Module; }

  using DataOffset = WVisualScriptDataDescription::DataOffset;
  struct AstNode;

  struct ExecInput
  {
    W_DECLARE_POD_TYPE();

    AstNode* m_pSourceNode = nullptr;
    WUInt32 m_uiSourcePinIndex = 0;
#if W_ENABLED(W_PLATFORM_64BIT)
    WUInt32 m_uiPadding = 0;
#endif
  };

  struct ExecOutput
  {
    W_DECLARE_POD_TYPE();

    AstNode* m_pTargetNode = nullptr;
  };

  struct DataInput
  {
    W_DECLARE_POD_TYPE();

    AstNode* m_pSourceNode = nullptr;
    WUInt32 m_uiSourcePinIndex = 0;
    DataOffset m_DataOffset;

    W_ALWAYS_INLINE bool IsConnected() const { return m_pSourceNode != nullptr; }
    W_ALWAYS_INLINE bool IsConnectedAndLocal() const { return IsConnected() && m_DataOffset.GetSource() == DataOffset::Source::Local; }
  };

  struct DataOutput
  {
    W_DECLARE_POD_TYPE();

    DataOffset m_DataOffset;

    W_ALWAYS_INLINE bool IsValid() const { return m_DataOffset.IsValid(); }
    W_ALWAYS_INLINE bool IsValidAndLocal() const { return IsValid() && m_DataOffset.GetSource() == DataOffset::Source::Local; }
  };

  struct AstNode
  {
    const WDocumentObject* m_pObject = nullptr;

    WEnum<WVisualScriptNodeDescription::Type> m_Type;
    WEnum<WVisualScriptDataType> m_DeductedDataType;
    bool m_bImplicitExecution = false;

    WHashedString m_sTargetTypeName;
    WVariant m_Value;

    WSmallArray<ExecInput, 2> m_ExecInputs;
    WSmallArray<ExecOutput, 2> m_ExecOutputs;
    WSmallArray<DataInput, 7> m_DataInputs;
    WSmallArray<DataOutput, 4> m_DataOutputs;
  };

#if W_ENABLED(W_PLATFORM_64BIT)
  static_assert(sizeof(AstNode) == 256);
#endif

private:
  W_ALWAYS_INLINE static WStringView GetNiceTypeName(const WDocumentObject* pObject)
  {
    return WVisualScriptNodeManager::GetNiceTypeName(pObject);
  }

  // Ast node creation
  AstNode& CreateAstNode(WVisualScriptNodeDescription::Type::Enum type, WVisualScriptDataType::Enum deductedDataType = WVisualScriptDataType::Invalid, bool bImplicitExecution = false);
  W_ALWAYS_INLINE AstNode& CreateAstNode(WVisualScriptNodeDescription::Type::Enum type, bool bImplicitExecution)
  {
    return CreateAstNode(type, WVisualScriptDataType::Invalid, bImplicitExecution);
  }

  AstNode& CreateJumpNode(AstNode* pTargetNode);
  AstNode* CreateAstNodeFromObject(const WDocumentObject* pObject, const WVisualScriptNodeRegistry::NodeDesc* pNodeDesc, const WDocumentObject* pEntryObject, bool bImplicitOnly = false);
  DataInput GetOrCreateDefaultPointerNode(const AstNode& node, const WRTTI* pRtti);

  void MarkAsCoroutine(AstNode* pEntryAstNode);

  // Pins, inputs and outputs
  void AddConstantDataInput(AstNode& node, const WVariant& value);
  WResult AddConstantDataInput(AstNode& node, const WDocumentObject* pObject, const WVisualScriptPin* pPin, WVisualScriptDataType::Enum dataType);
  void AddDataInput(AstNode& node, AstNode* pSourceNode, WUInt32 uiSourcePinIndex, WVisualScriptDataType::Enum dataType);
  void AddDataOutput(AstNode& node, WVisualScriptDataType::Enum dataType);
  DataOutput& GetDataOutputFromInput(const DataInput& dataInput);

  void ConnectExecution(AstNode& sourceNode, AstNode& targetNode, WUInt32 uiSourcePinIndex = WInvalidIndex);
  void DisconnectExecution(AstNode& sourceNode, AstNode& targetNode, WUInt32 uiSourcePinIndex);
  void ExecuteBefore(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode);
  void ExecuteAfter(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode);
  void ReplaceExecution(AstNode& oldNode, AstNode& newNode);

  DataOffset GetInstanceDataOffset(WHashedString sName, WVisualScriptDataType::Enum dataType);

  // Compilation steps
  WResult BuildInstanceDataMapping();
  AstNode* BuildExecutionFlow(const WDocumentObject* pEntryObject);

  WResult BuildDataStack(AstNode* pEntryAstNode, AstNode*& out_pFirstDataNode, AstNode*& out_pLastDataNode);
  WResult BuildDataExecutions(AstNode* pEntryAstNode);

  WResult InsertTypeConversions(AstNode* pEntryAstNode);

  WResult ReplaceLoop(AstNode* pEntryAstNode);
  WResult ReplaceUnsupportedNodes(AstNode* pEntryAstNode);

  WResult AssignInstanceVariables(AstNode* pEntryAstNode);
  WResult AssignLocalVariables(AstNode* pEntryAstNode, WVisualScriptDataDescription& inout_localDataDesc);
  WResult CopyOutputsToInputs(AstNode* pEntryAstNode);

  WResult BuildNodeDescriptions(AstNode* pEntryAstNode, WDynamicArray<WVisualScriptNodeDescription>& out_NodeDescriptions);

  WResult FinalizeConstantData();

  enum class VisitorResult
  {
    Continue,
    Skip,
    Error,
  };

  // Does allow modifications to the AST structure while iterating
  WResult TraverseAstDepthFirst(AstNode* pEntryAstNode, WDelegate<VisitorResult(AstNode*& pAstNode)> func);

  // Does NOT allow modifications to the AST structure while iterating
  WResult TraverseAstTopologicalOrder(const AstNode* pEntryAstNode, WDelegate<VisitorResult(const AstNode* pAstNode)> func);

  void DumpAST(AstNode* pEntryAstNode, WStringView sOutputPath, WStringView sFunctionName, WStringView sSuffix);
  void DumpGraph(WArrayPtr<const WVisualScriptNodeDescription> nodeDescriptions, WStringView sOutputPath, WStringView sFunctionName, WStringView sSuffix);

  WVisualScriptNodeManager& m_NodeManager;

  WDeque<AstNode> m_AstNodes;
  WHybridArray<const WDocumentObject*, 8> m_EntryObjects;

  struct LiveLocalVar
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_uiId = WInvalidIndex;
    DataOffset m_DataOffset;

    WUInt32 m_uiStart = WInvalidIndex;
    WUInt32 m_uiEnd = 0;
  };

  struct CompilationState
  {
    WHashTable<const WDocumentObject*, AstNode*> m_ExecObjectToAstNode;
    WHashTable<const WDocumentObject*, AstNode*> m_DataObjectToAstNode;

    WHashSet<const AstNode*> m_VisitedNodes;

    WDynamicArray<LiveLocalVar> m_LiveLocalVars;
    WUInt32 m_uiNextLocalVarId = 0;

    AstNode* m_pGetScriptOwnerNode = nullptr;

    void Clear()
    {
      m_ExecObjectToAstNode.Clear();
      m_DataObjectToAstNode.Clear();

      m_VisitedNodes.Clear();

      m_LiveLocalVars.Clear();
      m_uiNextLocalVarId = 0;

      m_pGetScriptOwnerNode = nullptr;
    }
  };

  CompilationState m_CompilationState;

  CompiledModule m_Module;
};
