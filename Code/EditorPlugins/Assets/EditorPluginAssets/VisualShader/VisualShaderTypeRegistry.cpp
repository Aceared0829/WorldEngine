#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

W_IMPLEMENT_SINGLETON(WVisualShaderTypeRegistry);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, VisualShader)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WVisualShaderTypeRegistry);

    WVisualShaderTypeRegistry::GetSingleton()->LoadNodeData();
    const WRTTI* pBaseType = WVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

    WQtVisualGraphScene::GetPinFactory().RegisterCreator(WGetStaticRTTI<WVisualShaderPin>(), [](const WRTTI* pRtti)->WQtVisualGraphPin* { return new WQtVisualShaderPin(); });
    WQtVisualGraphScene::GetNodeFactory().RegisterCreator(pBaseType, [](const WRTTI* pRtti)->WQtVisualGraphNode* { return new WQtVisualShaderNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const WRTTI* pBaseType = WVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

    WQtVisualGraphScene::GetPinFactory().UnregisterCreator(WGetStaticRTTI<WVisualShaderPin>());
    WQtVisualGraphScene::GetNodeFactory().UnregisterCreator(pBaseType);

    WVisualShaderTypeRegistry* pDummy = WVisualShaderTypeRegistry::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  static const char* s_szColorNames[] = {
    "Red",
    "Pink",
    "Grape",
    "Violet",
    "Indigo",
    "Blue",
    "Cyan",
    "Teal",
    "Green",
    "Lime",
    "Yellow",
    "Orange",
    "Gray",
  };
  static_assert(W_ARRAY_SIZE(s_szColorNames) == WColorScheme::Count);

  static void GetColorFromDdl(const WOpenDdlReaderElement* pElement, WColorGammaUB& out_color)
  {
    if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::String)
    {
      WColorScheme::Enum color = WColorScheme::Gray;
      const WStringView* pValue = pElement->GetPrimitivesString();
      for (WUInt32 i = 0; i < WColorScheme::Count; ++i)
      {
        if (pValue->IsEqual_NoCase(s_szColorNames[i]))
        {
          color = static_cast<WColorScheme::Enum>(i);
          break;
        }
      }

      out_color = WColorScheme::DarkUI(color);
    }
    else
    {
      WOpenDdlUtils::ConvertToColorGamma(pElement, out_color).IgnoreResult();
    }
  }
} // namespace

WVisualShaderTypeRegistry::WVisualShaderTypeRegistry()
  : m_SingletonRegistrar(this)
{
  m_pBaseType = nullptr;
  m_pSamplerPinType = nullptr;
  WQtEditorApp::m_Events.AddEventHandler(WMakeDelegate(&WVisualShaderTypeRegistry::EditorEventHandler, this));
  WToolsProject::s_Events.AddEventHandler(WMakeDelegate(&WVisualShaderTypeRegistry::ProjectEventHandler, this));
}

WVisualShaderTypeRegistry::~WVisualShaderTypeRegistry()
{
  WToolsProject::s_Events.RemoveEventHandler(WMakeDelegate(&WVisualShaderTypeRegistry::ProjectEventHandler, this));
  WQtEditorApp::m_Events.RemoveEventHandler(WMakeDelegate(&WVisualShaderTypeRegistry::EditorEventHandler, this));
}

const WVisualShaderNodeDescriptor* WVisualShaderTypeRegistry::GetDescriptorForType(const WRTTI* pRtti) const
{
  auto it = m_NodeDescriptors.Find(pRtti);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

void WVisualShaderTypeRegistry::EditorEventHandler(const WEditorAppEvent& e)
{
  if (e.m_Type == WEditorAppEvent::Type::EditorStarted)
  {
    UpdateNodeData();
  }
}

void WVisualShaderTypeRegistry::ProjectEventHandler(const WToolsProjectEvent& e)
{
  // The editor startup event fires before any project is loaded, so at that point there are no project
  // data directories to search. They are configured and mounted while ProjectOpened is being handled.
  if (e.m_Type == WToolsProjectEvent::Type::ProjectOpened)
  {
    LoadProjectNodeData();
  }

  if (e.m_Type == WToolsProjectEvent::Type::ProjectClosed)
  {
    UnloadProjectNodeData();
  }
}

void WVisualShaderTypeRegistry::UpdateNodeData()
{
  // If the assets plugin is statically linked, ON_CORESYSTEMS_STARTUP is fired before the editor is running, at which point the data directories are not set up yet so the code below will fail. Therefore, we also run this code in the EditorEventHandler code above to ensure that we run this code at the appropriate time.
  // If linked dynamically, the plugin will be loaded during project open, at which point everything is already running.
  if (!WQtEditorApp::GetSingleton() || !WQtEditorApp::GetSingleton()->IsRunning())
    return;

  // the nodes that the editor itself ships
  WStringBuilder sSearchDir = WApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("VisualShader/*.ddl");

  WFileSystemIterator it;
  for (it.StartSearch(sSearchDir, WFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
  {
    UpdateNodeData(it.GetStats().m_sName);
  }

  // and the nodes of the open project, if there already is one - see ProjectEventHandler()
  LoadProjectNodeData();
}

// Config file paths are stored in the node descriptors and end up as asset transform dependencies.
// Therefore they must never be absolute - make them ':rootname/...' relative to their data directory.
static void MakeConfigFilePathPortable(WStringBuilder& ref_sPath)
{
  if (!WPathUtils::IsAbsolutePath(ref_sPath))
    return;

  WStringBuilder sRelative;
  const WDataDirectoryInfo* pDataDir = nullptr;

  if (WFileSystem::ResolvePath(ref_sPath, nullptr, &sRelative, &pDataDir).Failed())
  {
    WLog::Warning("Visual Shader config file '{}' is not inside a data directory, its path can't be stored in a portable way.", ref_sPath);
    return;
  }

  ref_sPath = sRelative;

  if (!pDataDir->m_sRootName.IsEmpty())
  {
    // the file system stores root names in upper case, but rooted paths are matched case insensitive,
    // so write them in lower case to match the style of all the other paths
    WStringBuilder sRootName = pDataDir->m_sRootName;
    sRootName.ToLower();

    ref_sPath.Prepend(":", sRootName, "/");
  }
}

void WVisualShaderTypeRegistry::LoadProjectNodeData()
{
  if (!WToolsProject::IsProjectOpen())
    return;

  // A project may ship its own nodes in '<data directory>/Editor/VisualShader/*.ddl', e.g. to wrap
  // game specific render states or shader functions, without having to modify the editor's own data.
  // The files are read through the file system, so this must run after the data directories have been
  // applied - which is why this is not part of the editor startup event.
  WStringBuilder sSearchDir, sNodeFile;
  for (const auto& dd : WQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
  {
    if (WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sSearchDir).Failed())
      continue;

    sSearchDir.AppendPath("Editor/VisualShader/*.ddl");

    WFileSystemIterator it;
    for (it.StartSearch(sSearchDir, WFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
    {
      it.GetStats().GetFullPath(sNodeFile);
      MakeConfigFilePathPortable(sNodeFile);

      LoadConfigFile(sNodeFile, true);
    }
  }
}

void WVisualShaderTypeRegistry::UnloadProjectNodeData()
{
  for (const WRTTI* pType : m_ProjectNodeTypes)
  {
    m_NodeDescriptors.Remove(pType);
    WPhantomRttiManager::UnregisterType(pType);
  }

  m_ProjectNodeTypes.Clear();
}


void WVisualShaderTypeRegistry::UpdateNodeData(WStringView sCfgFileRelative)
{
  WStringBuilder sPath = sCfgFileRelative;
  bool bProjectNode = false;

  if (!WPathUtils::IsAbsolutePath(sCfgFileRelative))
  {
    sPath.SetFormat(":app/VisualShader/{}", sCfgFileRelative);
  }
  else
  {
    // absolute paths come from the directory watchers, which watch the editor's folder as well as the
    // project's - anything that isn't the editor's own folder belongs to the project
    WStringBuilder sAppDir = WApplicationServices::GetSingleton()->GetApplicationDataFolder();
    sAppDir.AppendPath("VisualShader");
    sAppDir.MakeCleanPath();

    sPath.MakeCleanPath();
    bProjectNode = !sPath.StartsWith_NoCase(sAppDir);

    if (bProjectNode)
    {
      MakeConfigFilePathPortable(sPath);
    }
    else
    {
      sPath.MakeRelativeTo(sAppDir).IgnoreResult();
      sPath.Prepend(":app/VisualShader/");
    }
  }

  LoadConfigFile(sPath, bProjectNode);
}

void WVisualShaderTypeRegistry::LoadNodeData()
{
  // Base Node Type
  if (m_pBaseType == nullptr)
  {
    WReflectedTypeDescriptor desc;
    desc.m_sTypeName = "WVisualShaderNodeBase";
    desc.m_sPluginName = "VisualShaderTypes";
    desc.m_sParentTypeName = WGetStaticRTTI<WReflectedClass>()->GetTypeName();
    desc.m_Flags = WTypeFlags::Abstract | WTypeFlags::Class;
    desc.m_uiTypeVersion = 1;

    m_pBaseType = WPhantomRttiManager::RegisterType(desc);
  }

  if (m_pSamplerPinType == nullptr)
  {
    WReflectedTypeDescriptor desc;
    desc.m_sTypeName = "WVisualShaderSamplerPin";
    desc.m_sPluginName = "VisualShaderTypes";
    desc.m_sParentTypeName = WGetStaticRTTI<WReflectedClass>()->GetTypeName();
    desc.m_Flags = WTypeFlags::Class;
    desc.m_uiTypeVersion = 1;

    m_pSamplerPinType = WPhantomRttiManager::RegisterType(desc);
  }

  UpdateNodeData();
}

const WRTTI* WVisualShaderTypeRegistry::GenerateTypeFromDesc(const WVisualShaderNodeDescriptor& nd)
{
  WStringBuilder temp;
  temp.Set("ShaderNode::", nd.m_sName);

  WReflectedTypeDescriptor desc;
  desc.m_sTypeName = temp;
  desc.m_sPluginName = "VisualShaderTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = WTypeFlags::Class;
  desc.m_uiTypeVersion = 1;
  desc.m_Properties = nd.m_Properties;

  for (const auto& pin : nd.m_InputPins)
  {
    if (pin.m_PropertyDesc.m_sName.IsEmpty())
      continue;

    desc.m_Properties.PushBack(pin.m_PropertyDesc);
  }

  for (const auto& pin : nd.m_OutputPins)
  {
    if (pin.m_PropertyDesc.m_sName.IsEmpty())
      continue;

    desc.m_Properties.PushBack(pin.m_PropertyDesc);
  }

  return WPhantomRttiManager::RegisterType(desc);
}

void WVisualShaderTypeRegistry::LoadConfigFile(const char* szFile, bool bProjectNode)
{
  W_LOG_BLOCK("Loading Visual Shader Config", szFile);

  WLog::Debug("Loading VSE node config '{0}'", szFile);

  WFileReader file;
  if (file.Open(szFile).Failed())
  {
    WLog::Error("Failed to open Visual Shader config file '{0}'", szFile);
    return;
  }

  if (WPathUtils::HasExtension(szFile, "ddl"))
  {
    WOpenDdlReader ddl;
    if (ddl.ParseDocument(file, 0, WLog::GetThreadLocalLogSystem()).Failed())
    {
      WLog::Error("Failed to parse Visual Shader config file '{0}'", szFile);
      return;
    }

    const WOpenDdlReaderElement* pRoot = ddl.GetRootElement();
    const WOpenDdlReaderElement* pNode = pRoot->GetFirstChild();

    while (pNode != nullptr)
    {
      if (!pNode->IsCustomType() || pNode->GetCustomType() != "Node")
      {
        WLog::Error("Top-Level object is not a 'Node' type");
        continue;
      }

      WVisualShaderNodeDescriptor nd;
      nd.m_sCfgFile = szFile;
      nd.m_sName = pNode->GetName();

      ExtractNodeConfig(pNode, nd);
      ExtractNodeProperties(pNode, nd);
      ExtractNodePins(pNode, "InputPin", nd.m_InputPins, false);
      ExtractNodePins(pNode, "OutputPin", nd.m_OutputPins, true);

      const WRTTI* pType = GenerateTypeFromDesc(nd);
      m_NodeDescriptors.Insert(pType, nd);

      if (bProjectNode && !m_ProjectNodeTypes.Contains(pType))
      {
        m_ProjectNodeTypes.PushBack(pType);
      }

      pNode = pNode->GetSibling();
    }
  }
}

static WVariant ExtractDefaultValue(const WRTTI* pType, const char* szDefault)
{
  if (pType == WGetStaticRTTI<WString>())
  {
    return WVariant(szDefault);
  }

  if (pType == WGetStaticRTTI<bool>())
  {
    bool res = false;
    WConversionUtils::StringToBool(szDefault, res).IgnoreResult();
    return WVariant(res);
  }

  float values[4] = {0, 0, 0, 0};
  WConversionUtils::ExtractFloatsFromString(szDefault, 4, values);

  if (pType == WGetStaticRTTI<float>())
  {
    return WVariant(values[0]);
  }

  if (pType == WGetStaticRTTI<int>())
  {
    return WVariant((int)values[0]);
  }

  if (pType == WGetStaticRTTI<WVec2>())
  {
    return WVariant(WVec2(values[0], values[1]));
  }

  if (pType == WGetStaticRTTI<WVec3>())
  {
    return WVariant(WVec3(values[0], values[1], values[2]));
  }

  if (pType == WGetStaticRTTI<WVec4>())
  {
    return WVariant(WVec4(values[0], values[1], values[2], values[3]));
  }

  if (pType == WGetStaticRTTI<WColor>())
  {
    return WVariant(WColorGammaUB(values[0], values[1], values[2], values[3]));
  }

  return WVariant();
}

void WVisualShaderTypeRegistry::ExtractNodePins(const WOpenDdlReaderElement* pNode, const char* szPinType, WDynamicArray<WVisualShaderPinDescriptor>& pinArray, bool bOutput)
{
  for (const WOpenDdlReaderElement* pElement = pNode->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (pElement->GetCustomType() == szPinType)
    {
      WVisualShaderPinDescriptor pin;

      if (!pElement->HasName())
      {
        WLog::Error("Missing or invalid name for pin");
        continue;
      }

      pin.m_sName = pElement->GetName();

      auto pType = pElement->FindChildOfType(WOpenDdlPrimitiveType::String, "Type");

      if (!pType)
      {
        WLog::Error("Missing or invalid pin type");
        continue;
      }

      {
        const WString& sType = pType->GetPrimitivesString()[0];

        if (sType == "color")
          pin.m_pDataType = WGetStaticRTTI<WColor>();
        else if (sType == "float4")
          pin.m_pDataType = WGetStaticRTTI<WVec4>();
        else if (sType == "float3")
          pin.m_pDataType = WGetStaticRTTI<WVec3>();
        else if (sType == "float2")
          pin.m_pDataType = WGetStaticRTTI<WVec2>();
        else if (sType == "float")
          pin.m_pDataType = WGetStaticRTTI<float>();
        else if (sType == "string")
          pin.m_pDataType = WGetStaticRTTI<WString>();
        else if (sType == "sampler")
          pin.m_pDataType = m_pSamplerPinType;
        else if (sType == "auto")
          pin.m_pDataType = nullptr; // nullptr indicates "auto" type - computed from inputs at code generation time
        else
        {
          WLog::Error("Invalid pin type '{0}'", sType);
          continue;
        }
      }

      if (auto pInline = pElement->FindChildOfType(WOpenDdlPrimitiveType::String, "Inline"))
      {
        pin.m_sShaderCodeInline = pInline->GetPrimitivesString()[0];
      }
      else if (bOutput)
      {
        WLog::Error("Output pin '{0}' has no inline code specified", pin.m_sName);
        continue;
      }

      // this is optional
      if (auto pColor = pElement->FindChild("Color"))
      {
        GetColorFromDdl(pColor, pin.m_Color);
      }

      // this is optional
      if (auto pTooltip = pElement->FindChildOfType(WOpenDdlPrimitiveType::String, "Tooltip"))
      {
        pin.m_sTooltip = pTooltip->GetPrimitivesString()[0];
      }

      // this is optional
      if (auto pDefaultValue = pElement->FindChildOfType(WOpenDdlPrimitiveType::String, "DefaultValue"))
      {
        pin.m_sDefaultValue = pDefaultValue->GetPrimitivesString()[0];
      }

      if (auto pDefineWhenUsingDefaultValue = pElement->FindChildOfType(WOpenDdlPrimitiveType::String, "DefineWhenUsingDefaultValue"))
      {
        const WUInt32 numElements = pDefineWhenUsingDefaultValue->GetNumPrimitives();
        pin.m_sDefinesWhenUsingDefaultValue.Reserve(numElements);

        for (WUInt32 i = 0; i < numElements; ++i)
        {
          pin.m_sDefinesWhenUsingDefaultValue.PushBack(pDefineWhenUsingDefaultValue->GetPrimitivesString()[i]);
        }
      }

      // this is optional
      if (auto pExpose = pElement->FindChildOfType(WOpenDdlPrimitiveType::Bool, "Expose"))
      {
        pin.m_bExposeAsProperty = pExpose->GetPrimitivesBool()[0];
      }

      if (pin.m_bExposeAsProperty)
      {
        pin.m_PropertyDesc.m_sName = pin.m_sName;
        pin.m_PropertyDesc.m_Category = WPropertyCategory::Member;
        pin.m_PropertyDesc.m_Flags.SetValue((WUInt16)WPropertyFlags::StandardType);

        // For "auto" type pins, use float as the fallback type for the property GUI
        const WRTTI* pPropertyType = pin.m_pDataType != nullptr ? pin.m_pDataType : WGetStaticRTTI<float>();
        pin.m_PropertyDesc.m_sType = pPropertyType->GetTypeName();

        const WVariant def = ExtractDefaultValue(pPropertyType, pin.m_sDefaultValue);

        if (def.IsValid())
        {
          pin.m_PropertyDesc.m_Attributes.PushBack(W_DEFAULT_NEW(WDefaultValueAttribute, def));
        }
      }

      pinArray.PushBack(pin);
    }
  }
}

void WVisualShaderTypeRegistry::ExtractNodeProperties(const WOpenDdlReaderElement* pNode, WVisualShaderNodeDescriptor& nd)
{
  for (const WOpenDdlReaderElement* pElement = pNode->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (pElement->GetCustomType() == "Property")
    {
      WInt8 iValueGroup = -1;

      WReflectedPropertyDescriptor prop;
      prop.m_Category = WPropertyCategory::Member;
      prop.m_Flags.SetValue((WUInt16)WPropertyFlags::StandardType);

      if (!pElement->HasName())
      {
        WLog::Error("Property doesn't have a name");
        continue;
      }

      prop.m_sName = pElement->GetName();

      const WOpenDdlReaderElement* pType = pElement->FindChildOfType(WOpenDdlPrimitiveType::String, "Type");
      if (!pType)
      {
        WLog::Error("Property doesn't have a type");
        continue;
      }

      const WRTTI* pRtti = nullptr;

      {
        const WStringView& sType = pType->GetPrimitivesString()[0];

        if (sType == "color")
        {
          pRtti = WGetStaticRTTI<WColor>();

          // always expose the alpha channel for color properties
          WExposeColorAlphaAttribute* pAttr = WExposeColorAlphaAttribute::GetStaticRTTI()->GetAllocator()->Allocate<WExposeColorAlphaAttribute>();
          prop.m_Attributes.PushBack(pAttr);
        }
        else if (sType == "float4")
        {
          pRtti = WGetStaticRTTI<WVec4>();
        }
        else if (sType == "float3")
        {
          pRtti = WGetStaticRTTI<WVec3>();
        }
        else if (sType == "float2")
        {
          pRtti = WGetStaticRTTI<WVec2>();
        }
        else if (sType == "float")
        {
          pRtti = WGetStaticRTTI<float>();
        }
        else if (sType == "int")
        {
          pRtti = WGetStaticRTTI<int>();
        }
        else if (sType == "bool")
        {
          pRtti = WGetStaticRTTI<bool>();
        }
        else if (sType == "string")
        {
          pRtti = WGetStaticRTTI<WString>();
        }
        else if (sType == "identifier")
        {
          pRtti = WGetStaticRTTI<WString>();

          iValueGroup = 1; // currently no way to specify the group
        }
        else if (sType == "enum")
        {
          pRtti = WGetStaticRTTI<WString>();

          // Read enum values from EnumValues property
          const WOpenDdlReaderElement* pEnumValues = pElement->FindChildOfType(WOpenDdlPrimitiveType::String, "EnumValues");
          if (pEnumValues)
          {
            // Create a unique enum name based on the node and property name
            WStringBuilder sEnumName;
            sEnumName.SetFormat("{}_{}", nd.m_sName, prop.m_sName);

            WDynamicStringEnumAttribute* pAttr = W_DEFAULT_NEW(WDynamicStringEnumAttribute, sEnumName);
            prop.m_Attributes.PushBack(pAttr);

            // Parse and register the enum values with the dynamic enum registry
            WStringBuilder enumValuesStr = pEnumValues->GetPrimitivesString()[0];

            // Create or get the dynamic enum
            auto& dynEnum = WDynamicStringEnum::CreateDynamicEnum(sEnumName);
            dynEnum.Clear();

            // Parse comma-separated values
            WTempHybridArray<WStringView, 32> values;
            enumValuesStr.Split(false, values, ",");

            for (const WStringView& value : values)
            {
              WStringBuilder trimmedValue = value;
              trimmedValue.Trim(" \t\r\n");
              if (!trimmedValue.IsEmpty())
              {
                dynEnum.AddValidValue(trimmedValue, false);
              }
            }
          }
          else
          {
            WLog::Error("Property '{}' of type 'enum' is missing 'EnumValues'", prop.m_sName);
            continue;
          }
        }
        else if (sType == "Texture2D")
        {
          pRtti = WGetStaticRTTI<WString>();

          // apparently the attributes are deallocated using the type allocator, so we must allocate them here through RTTI as well
          WAssetBrowserAttribute* pAttr = WAssetBrowserAttribute::GetStaticRTTI()->GetAllocator()->Allocate<WAssetBrowserAttribute>();
          pAttr->SetTypeFilter("CompatibleAsset_Texture_2D");
          prop.m_Attributes.PushBack(pAttr);
        }
        else
        {
          WLog::Error("Invalid property type '{0}'", sType);
          continue;
        }
      }

      prop.m_sType = pRtti->GetTypeName();

      const WOpenDdlReaderElement* pValue = pElement->FindChild("DefaultValue");
      if (pValue && pRtti != nullptr && pValue->HasPrimitives(WOpenDdlPrimitiveType::String))
      {
        WStringBuilder tmp = pValue->GetPrimitivesString()[0];
        const WVariant def = ExtractDefaultValue(pRtti, tmp);

        if (def.IsValid())
        {
          prop.m_Attributes.PushBack(W_DEFAULT_NEW(WDefaultValueAttribute, def));
        }
      }

      nd.m_Properties.PushBack(prop);
      nd.m_UniquePropertyValueGroups.PushBack(iValueGroup);
    }
  }
}

void WVisualShaderTypeRegistry::ExtractNodeConfig(const WOpenDdlReaderElement* pNode, WVisualShaderNodeDescriptor& nd)
{
  WStringBuilder temp;

  const WOpenDdlReaderElement* pElement = pNode->GetFirstChild();

  while (pElement)
  {
    if (pElement->GetName() == "Color")
    {
      GetColorFromDdl(pElement, nd.m_Color);
    }
    else if (pElement->HasPrimitives(WOpenDdlPrimitiveType::String))
    {
      if (pElement->GetName() == "NodeType")
      {
        if (pElement->GetPrimitivesString()[0] == "Main")
          nd.m_NodeType = WVisualShaderNodeType::Main;
        else if (pElement->GetPrimitivesString()[0] == "Texture")
          nd.m_NodeType = WVisualShaderNodeType::Texture;
        else if (pElement->GetPrimitivesString()[0] == "ShaderState")
          nd.m_NodeType = WVisualShaderNodeType::ShaderState;
        else if (pElement->GetPrimitivesString()[0] == "Parameter")
          nd.m_NodeType = WVisualShaderNodeType::Parameter;
        else
          nd.m_NodeType = WVisualShaderNodeType::Generic;
      }
      else if (pElement->GetName() == "Category")
      {
        nd.m_sCategory.Assign(pElement->GetPrimitivesString()[0]);
      }
      else if (pElement->GetName() == "Docs")
      {
        nd.m_sDocs = pElement->GetPrimitivesString()[0];
      }
      else if (pElement->GetName() == "Title")
      {
        nd.m_sTitle = pElement->GetPrimitivesString()[0];
      }
      else if (pElement->GetName() == "CheckPermutations")
      {
        temp = pElement->GetPrimitivesString()[0];
        temp.ReplaceAll(" ", "");
        temp.ReplaceAll("\r", "");
        temp.ReplaceAll("\t", "");
        temp.Trim("\n");
        nd.m_sCheckPermutations = temp;
      }
      else if (pElement->GetName() == "CodePermutations")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePermutations = temp;
      }
      else if (pElement->GetName() == "CodeRenderStates")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeRenderState = temp;
      }
      else if (pElement->GetName() == "CodeMaterialConfig")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeMaterialConfig = temp;
      }
      else if (pElement->GetName() == "CodeShaderShared")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeShaderShared = temp;
      }
      else if (pElement->GetName() == "CodeVertexDefines")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeVertexDefines = temp;
      }
      else if (pElement->GetName() == "CodeVertexIncludes")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeVertexIncludes = temp;
      }
      else if (pElement->GetName() == "CodeVertexBody")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeVertexBody = temp;
      }
      else if (pElement->GetName() == "CodeMaterialParams")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeMaterialParams = temp;
      }
      else if (pElement->GetName() == "CodeMaterialConstants")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeMaterialConstants = temp;
      }
      else if (pElement->GetName() == "CodeMaterialCB")
      {
        temp = pElement->GetPrimitivesString()[0];
        nd.m_sShaderCodeMaterialCB = temp;
      }
      else if (pElement->GetName() == "CodePixelDefines")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelDefines = temp;
      }
      else if (pElement->GetName() == "CodePixelIncludes")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelIncludes = temp;
      }
      else if (pElement->GetName() == "CodePixelSamplers")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelSamplers = temp;
      }
      else if (pElement->GetName() == "CodePixelConstants")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelConstants = temp;
      }
      else if (pElement->GetName() == "CodePixelBody")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelBody = temp;
      }
    }

    pElement = pElement->GetSibling();
  }
}
