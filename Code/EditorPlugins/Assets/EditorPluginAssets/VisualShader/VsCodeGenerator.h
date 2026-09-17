#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>

class WVisualGraphObjectManager;

class WVisualShaderCodeGenerator
{
public:
  WVisualShaderCodeGenerator();

  WStatus GenerateVisualShader(const WVisualGraphObjectManager* pNodeMaanger, WStringBuilder& out_sCheckPerms);

  const char* GetFinalShaderCode() const { return m_sFinalShaderCode; }

  void DetermineConfigFileDependencies(const WVisualGraphObjectManager* pNodeManager, WSet<WString>& out_cfgFiles);

private:
  struct NodeState
  {
    NodeState()
    {
      m_uiNodeId = 0;
      m_bCodeGenerated = false;
      m_bInProgress = false;
    }

    WUInt16 m_uiNodeId;
    bool m_bCodeGenerated;
    bool m_bInProgress;
  };

  struct OutputPinState
  {
    OutputPinState() { m_bCodeGenerated = false; }

    bool m_bCodeGenerated;
    WString m_sCodeAtPin;
  };


  WStatus GatherAllNodes(const WDocumentObject* pRootObj);
  WUInt16 DeterminePinId(const WDocumentObject* pOwner, const WVisualGraphPin& pin) const;
  WStatus GenerateNode(const WDocumentObject* pNode);
  WStatus GenerateInputPinCode(WArrayPtr<const WUniquePtr<const WVisualGraphPin>> pins);
  WStatus CheckPropertyValues(const WDocumentObject* pNode, const WVisualShaderNodeDescriptor* pDesc);
  WStatus InsertPropertyValues(const WDocumentObject* pNode, const WVisualShaderNodeDescriptor* pDesc, WStringBuilder& sString);
  WStatus GenerateOutputPinCode(const WDocumentObject* pOwnerNode, const WVisualGraphPin& pinSource);

  WStatus ReplaceInputPinsByCode(const WDocumentObject* pOwnerNode, const WVisualShaderNodeDescriptor* pNodeDesc, WStringBuilder& sInlineCode, WStringBuilder& sCodeForPlacingDefines);
  void ReplaceMainNodeInputPins(const WDocumentObject* pMainNode, const WVisualShaderNodeDescriptor* pNodeDesc, WStringBuilder& sInlineCode, WStringBuilder& sCodeForPlacingDefines, WStringBuilder& out_sHelperFunctions);
  void SetPinDefines(const WDocumentObject* pOwnerNode, WStringBuilder& sInlineCode);
  static void AppendStringIfUnique(WStringBuilder& inout_String, WStringView sAppend);

  // Generates the transitive hull of nodes connected via input pins to pNode.
  void CollectReachableNodes(const WDocumentObject* pNode, WHashSet<const WDocumentObject*>& out_Nodes) const;

  // Gets the value to use for an unconnected input pin (either from property or default value)
  // Optionally appends defines to pDefinesOut when using a default value
  WString GetInputPinDefaultValue(const WDocumentObject* pNode, const WVisualShaderPinDescriptor& pinDesc, WStringBuilder* pDefinesOut = nullptr);

  // Collects all nodes that are connected to the given connection. out_Sorted is sorted with the first element being the source pin of pStartConnection.
  WResult CollectNodesInTopologicalOrder(const WDocumentObject* pRootNode, WDynamicArray<const WDocumentObject*>& out_Sorted) const;

  // Computes the effective output type dimension for all output pins and stores in m_OutputPinDimensions
  void ComputeOutputPinDimensions();

  // Generates a helper function for a main node input pin, returns the function code and the function name
  void GenerateInputHelperFunction(const WVisualGraphPin* pInputPin, WUInt32 uiInputIndex, WStringBuilder& out_sFunctionCode, WStringBuilder& out_sFunctionCall);

  const WDocumentObject* m_pMainNode;
  const WVisualShaderTypeRegistry* m_pTypeRegistry;
  const WVisualGraphObjectManager* m_pNodeManager;
  const WRTTI* m_pNodeBaseRtti;
  WMap<const WDocumentObject*, NodeState> m_Nodes;
  WMap<const WVisualGraphPin*, OutputPinState> m_OutputPins;
  WMap<const WVisualGraphPin*, WUInt8> m_OutputPinDimensions; // Effective output type dimension for each output pin (1-4 for float-float4)
  WMap<WString, WString> m_UsedIdentifiers; // Maps identifier name to node type name
  WMap<WString, WString> m_MaterialParameter;

  WStringBuilder m_sShaderPixelDefines;
  WStringBuilder m_sShaderPixelIncludes;
  WStringBuilder m_sShaderPixelConstants;
  WStringBuilder m_sShaderPixelSamplers;
  WStringBuilder m_sShaderPixelBody;
  WStringBuilder m_sShaderVertexDefines;
  WStringBuilder m_sShaderVertexIncludes;
  WStringBuilder m_sShaderVertexBody;
  WStringBuilder m_sShaderMaterialParam;
  WStringBuilder m_sShaderMaterialConstants;
  WStringBuilder m_sShaderMaterialCB;
  WStringBuilder m_sShaderRenderState;
  WStringBuilder m_sShaderMaterialConfig;
  WStringBuilder m_sShaderPermutations;
  WStringBuilder m_sFinalShaderCode;
};
