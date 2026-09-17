#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

class WOpenDdlReaderElement;

/// Descriptor for a visual shader node pin.
///
/// Defines the properties of an input or output pin on a shader node, including its data type,
/// default value, shader code generation, and visual appearance.
struct WVisualShaderPinDescriptor
{
  WString m_sName;
  const WRTTI* m_pDataType = nullptr;
  WReflectedPropertyDescriptor m_PropertyDesc;
  WColorGammaUB m_Color = WColorScheme::DarkUI(WColorScheme::Gray);
  bool m_bExposeAsProperty = false;
  WString m_sDefaultValue;
  WDynamicArray<WString> m_sDefinesWhenUsingDefaultValue;
  WString m_sShaderCodeInline;
  WString m_sTooltip;
};

struct WVisualShaderNodeType
{
  using StorageType = WUInt8;

  enum Enum
  {
    Generic,
    Main,
    Texture,
    ShaderState, ///< These have no connections, but must be part of the shader
    Parameter,   ///< Will be added to the shader, even if there's no connection, to prevent that data is lost while editing

    Default = Generic
  };
};

/// Descriptor for a visual shader node type.
///
/// Contains all information needed to create and compile a visual shader node, including
/// its pins, properties, shader code fragments, and compilation settings.
/// Node types are typically loaded from configuration files at startup.
struct WVisualShaderNodeDescriptor
{
  WEnum<WVisualShaderNodeType> m_NodeType;
  WString m_sCfgFile; ///< from which config file this node type was loaded
  WString m_sName;
  WString m_sTitle;
  WString m_sDocs;
  WHashedString m_sCategory;
  WString m_sCheckPermutations;
  WColorGammaUB m_Color = WColorScheme::DarkUI(WColorScheme::Gray);
  WString m_sShaderCodePixelDefines;
  WString m_sShaderCodePixelIncludes;
  WString m_sShaderCodePixelSamplers;
  WString m_sShaderCodePixelConstants;
  WString m_sShaderCodePixelBody;
  WString m_sShaderCodePermutations;
  WString m_sShaderCodeMaterialParams;
  WString m_sShaderCodeMaterialConstants;
  WString m_sShaderCodeMaterialCB;
  WString m_sShaderCodeRenderState;
  WString m_sShaderCodeMaterialConfig;
  WString m_sShaderCodeShaderShared;
  WString m_sShaderCodeVertexDefines;
  WString m_sShaderCodeVertexIncludes;
  WString m_sShaderCodeVertexBody;

  WHybridArray<WVisualShaderPinDescriptor, 4> m_InputPins;
  WHybridArray<WVisualShaderPinDescriptor, 4> m_OutputPins;
  WHybridArray<WReflectedPropertyDescriptor, 4> m_Properties;
  WHybridArray<WInt8, 4> m_UniquePropertyValueGroups; // no property in the same group may share the same value, -1 for disabled
};

/// Registry for all available visual shader node types.
///
/// Loads node type definitions from configuration files and provides access to node descriptors.
/// Node types can be dynamically reloaded during development for rapid iteration.
class WVisualShaderTypeRegistry
{
  W_DECLARE_SINGLETON(WVisualShaderTypeRegistry);

public:
  WVisualShaderTypeRegistry();
  ~WVisualShaderTypeRegistry();

  const WVisualShaderNodeDescriptor* GetDescriptorForType(const WRTTI* pRtti) const;

  const WRTTI* GetNodeBaseType() const { return m_pBaseType; }

  const WRTTI* GetPinSamplerType() const { return m_pSamplerPinType; }

  void UpdateNodeData();

  void UpdateNodeData(WStringView sCfgFileRelative);

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorPluginAssets, VisualShader);

  void EditorEventHandler(const WEditorAppEvent& e);
  void ProjectEventHandler(const WToolsProjectEvent& e);
  void LoadNodeData();

  /// Loads the nodes that the open project ships in its own data directories.
  ///
  /// Only callable once the project's data directories are configured and mounted, which is why this
  /// hangs off ProjectOpened and not off the editor startup event.
  void LoadProjectNodeData();

  /// Unregisters everything LoadProjectNodeData() added, so the next project starts clean.
  void UnloadProjectNodeData();

  const WRTTI* GenerateTypeFromDesc(const WVisualShaderNodeDescriptor& desc);
  void LoadConfigFile(const char* szFile, bool bProjectNode);

  void ExtractNodePins(const WOpenDdlReaderElement* pNode, const char* szPinType, WDynamicArray<WVisualShaderPinDescriptor>& pinArray, bool bOutput);
  void ExtractNodeProperties(const WOpenDdlReaderElement* pNode, WVisualShaderNodeDescriptor& nd);
  void ExtractNodeConfig(const WOpenDdlReaderElement* pNode, WVisualShaderNodeDescriptor& nd);


  WMap<const WRTTI*, WVisualShaderNodeDescriptor> m_NodeDescriptors;

  /// The types that came from the open project's data directories, so that they can be removed again
  /// when the project is closed. The editor's own node types stay for the whole session.
  WDynamicArray<const WRTTI*> m_ProjectNodeTypes;

  const WRTTI* m_pBaseType;
  const WRTTI* m_pSamplerPinType;
};
