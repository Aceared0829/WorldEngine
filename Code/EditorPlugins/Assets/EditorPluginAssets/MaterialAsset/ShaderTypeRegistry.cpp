#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/ShaderTypeRegistry.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

W_IMPLEMENT_SINGLETON(WShaderTypeRegistry);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, ShaderTypeRegistry)

BEGIN_SUBSYSTEM_DEPENDENCIES
  "ReflectedTypeManager"
END_SUBSYSTEM_DEPENDENCIES

ON_CORESYSTEMS_STARTUP
{
  W_DEFAULT_NEW(WShaderTypeRegistry);
}

ON_CORESYSTEMS_SHUTDOWN
{
  WShaderTypeRegistry* pDummy = WShaderTypeRegistry::GetSingleton();
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
  struct PermutationVarConfig
  {
    WVariant m_DefaultValue;
    const WRTTI* m_pType;
  };

  static WHashTable<WString, PermutationVarConfig> s_PermutationVarConfigs;
  static WHashTable<WString, const WRTTI*> s_EnumTypes;

  void ClearCachedTypes()
  {
    s_PermutationVarConfigs.Clear();
    s_EnumTypes.Clear();
  }

  const WRTTI* GetPermutationType(const WShaderParser::ParameterDefinition& def)
  {
    W_ASSERT_DEV(def.m_sType.IsEqual("Permutation"), "");

    PermutationVarConfig* pConfig = nullptr;
    if (s_PermutationVarConfigs.TryGetValue(def.m_sName, pConfig))
    {
      return pConfig->m_pType;
    }

    WStringBuilder sTemp;
    sTemp.SetFormat("Shaders/PermutationVars/{0}.WPermVar", def.m_sName);

    WString sPath = sTemp;
    WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);

    WFileReader file;
    if (file.Open(sPath).Failed())
    {
      return nullptr;
    }

    sTemp.ReadAll(file);

    WVariant defaultValue;
    WShaderParser::EnumDefinition enumDefinition;

    WShaderParser::ParsePermutationVarConfig(sTemp, defaultValue, enumDefinition);
    if (defaultValue.IsValid())
    {
      pConfig = &(s_PermutationVarConfigs[def.m_sName]);
      pConfig->m_DefaultValue = defaultValue;

      if (defaultValue.IsA<bool>())
      {
        pConfig->m_pType = WGetStaticRTTI<bool>();
      }
      else
      {
        WReflectedTypeDescriptor descEnum;
        descEnum.m_sTypeName = def.m_sName;
        descEnum.m_sPluginName = "ShaderTypes";
        descEnum.m_sParentTypeName = WGetStaticRTTI<WEnumBase>()->GetTypeName();
        descEnum.m_Flags = WTypeFlags::IsEnum;
        descEnum.m_uiTypeVersion = 1;

        WArrayPtr<WPropertyAttribute* const> noAttributes;

        WStringBuilder sEnumName;
        sEnumName.SetFormat("{0}::Default", def.m_sName);

        descEnum.m_Properties.PushBack(WReflectedPropertyDescriptor(sEnumName, defaultValue.Get<WUInt32>(), noAttributes));

        for (const auto& ev : enumDefinition.m_Values)
        {
          WStringBuilder sEnumName;
          sEnumName.SetFormat("{0}::{1}", def.m_sName, ev.m_sValueName);

          descEnum.m_Properties.PushBack(WReflectedPropertyDescriptor(sEnumName, ev.m_iValueValue, noAttributes));
        }

        pConfig->m_pType = WPhantomRttiManager::RegisterType(descEnum);
      }

      return pConfig->m_pType;
    }

    return nullptr;
  }

  const WRTTI* GetEnumType(const WShaderParser::EnumDefinition& def)
  {
    const WRTTI* pType = nullptr;
    if (s_EnumTypes.TryGetValue(def.m_sName, pType))
    {
      return pType;
    }

    WReflectedTypeDescriptor descEnum;
    descEnum.m_sTypeName = def.m_sName;
    descEnum.m_sPluginName = "ShaderTypes";
    descEnum.m_sParentTypeName = WGetStaticRTTI<WEnumBase>()->GetTypeName();
    descEnum.m_Flags = WTypeFlags::IsEnum;
    descEnum.m_uiTypeVersion = 1;

    WArrayPtr<WPropertyAttribute* const> noAttributes;

    WStringBuilder sEnumName;
    sEnumName.SetFormat("{0}::Default", def.m_sName);

    descEnum.m_Properties.PushBack(WReflectedPropertyDescriptor(sEnumName, def.m_uiDefaultValue, noAttributes));

    for (const auto& ev : def.m_Values)
    {
      WStringBuilder sEnumName;
      sEnumName.SetFormat("{0}::{1}", def.m_sName, ev.m_sValueName);

      descEnum.m_Properties.PushBack(WReflectedPropertyDescriptor(sEnumName, ev.m_iValueValue, noAttributes));
    }

    pType = WPhantomRttiManager::RegisterType(descEnum);

    s_EnumTypes.Insert(def.m_sName, pType);

    return pType;
  }

  const WRTTI* GetType(const WShaderParser::ParameterDefinition& def)
  {
    if (def.m_pType != nullptr)
    {
      return def.m_pType;
    }

    if (def.m_sType.IsEqual("Permutation"))
    {
      return GetPermutationType(def);
    }

    const WRTTI* pType = nullptr;
    s_EnumTypes.TryGetValue(def.m_sType, pType);

    return pType;
  }

  void AddAttributes(WShaderParser::ParameterDefinition& ref_def, const WRTTI* pType, WDynamicArray<const WPropertyAttribute*>& ref_attributes)
  {
    if (ref_def.m_sType.StartsWith_NoCase("texture"))
    {
      if (ref_def.m_sType.IsEqual("Texture2D"))
      {
        ref_attributes.PushBack(W_DEFAULT_NEW(WCategoryAttribute, "Texture 2D"));
        ref_attributes.PushBack(W_DEFAULT_NEW(WAssetBrowserAttribute, "CompatibleAsset_Texture_2D"));
      }
      else if (ref_def.m_sType.IsEqual("Texture2DArray"))
      {
        ref_attributes.PushBack(W_DEFAULT_NEW(WCategoryAttribute, "Texture 2D Array"));
        ref_attributes.PushBack(W_DEFAULT_NEW(WAssetBrowserAttribute, "CompatibleAsset_Texture_2D"));
      }
      else if (ref_def.m_sType.IsEqual("Texture3D"))
      {
        ref_attributes.PushBack(W_DEFAULT_NEW(WCategoryAttribute, "Texture 3D"));
        ref_attributes.PushBack(W_DEFAULT_NEW(WAssetBrowserAttribute, "CompatibleAsset_Texture_3D"));
      }
      else if (ref_def.m_sType.IsEqual("TextureCube"))
      {
        ref_attributes.PushBack(W_DEFAULT_NEW(WCategoryAttribute, "Texture Cube"));
        ref_attributes.PushBack(W_DEFAULT_NEW(WAssetBrowserAttribute, "CompatibleAsset_Texture_Cube"));
      }
    }
    else if (ref_def.m_sType.StartsWith_NoCase("permutation"))
    {
      ref_attributes.PushBack(W_DEFAULT_NEW(WCategoryAttribute, "Permutation"));
    }
    else
    {
      ref_attributes.PushBack(W_DEFAULT_NEW(WCategoryAttribute, "Constant"));
    }

    for (auto& attributeDef : ref_def.m_Attributes)
    {
      if (attributeDef.m_sType.IsEqual("Default") && attributeDef.m_Values.GetCount() >= 1)
      {
        if (pType == WGetStaticRTTI<WColor>())
        {
          // always expose the alpha channel for color properties
          ref_attributes.PushBack(W_DEFAULT_NEW(WExposeColorAlphaAttribute));

          // patch default type, VSE writes float4 instead of color
          if (attributeDef.m_Values[0].GetType() == WVariantType::Vector4)
          {
            WVec4 v = attributeDef.m_Values[0].Get<WVec4>();
            attributeDef.m_Values[0] = WColor(v.x, v.y, v.z, v.w);
          }
        }

        ref_attributes.PushBack(W_DEFAULT_NEW(WDefaultValueAttribute, attributeDef.m_Values[0]));
      }
      else if (attributeDef.m_sType.IsEqual("Clamp") && attributeDef.m_Values.GetCount() >= 2)
      {
        ref_attributes.PushBack(W_DEFAULT_NEW(WClampValueAttribute, attributeDef.m_Values[0], attributeDef.m_Values[1]));
      }
      else if (attributeDef.m_sType.IsEqual("Group"))
      {
        if (attributeDef.m_Values.GetCount() >= 1 && attributeDef.m_Values[0].CanConvertTo<WString>())
        {
          ref_attributes.PushBack(W_DEFAULT_NEW(WGroupAttribute, attributeDef.m_Values[0].ConvertTo<WString>()));
        }
        else
        {
          ref_attributes.PushBack(W_DEFAULT_NEW(WGroupAttribute));
        }
      }
    }
  }
} // namespace

WShaderTypeRegistry::WShaderTypeRegistry()
  : m_SingletonRegistrar(this)
{
  WShaderTypeRegistry::GetSingleton();

  RegisterBaseType();

  WPhantomRttiManager::s_Events.AddEventHandler(WMakeDelegate(&WShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
  WPlugin::Events().AddEventHandler(WMakeDelegate(&WShaderTypeRegistry::PluginEventHandler, this));
}


WShaderTypeRegistry::~WShaderTypeRegistry()
{
  WPlugin::Events().RemoveEventHandler(WMakeDelegate(&WShaderTypeRegistry::PluginEventHandler, this));
  WPhantomRttiManager::s_Events.RemoveEventHandler(WMakeDelegate(&WShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

void WShaderTypeRegistry::RegisterBaseType()
{
  WReflectedTypeDescriptor desc;
  desc.m_sTypeName = "WShaderTypeBase";
  desc.m_sPluginName = "ShaderTypes";
  desc.m_sParentTypeName = WGetStaticRTTI<WReflectedClass>()->GetTypeName();
  desc.m_Flags = WTypeFlags::Abstract | WTypeFlags::Class;
  desc.m_uiTypeVersion = 2;

  m_pBaseType = WPhantomRttiManager::RegisterType(desc);
}

void WShaderTypeRegistry::PluginEventHandler(const WPluginEvent& e)
{
  // WPhantomRttiManager deletes all phantom types on this event, regardless of which plugin unloads,
  // so every cached type has to go, not just those belonging to e.m_sPluginBinary.
  if (e.m_EventType == WPluginEvent::Type::BeforeUnloading)
  {
    ClearCachedTypes();
    m_ShaderTypes.Clear();
    m_pBaseType = nullptr;
  }

  // This object outlives the types, so the base type has to be restored before it is used again.
  if (e.m_EventType == WPluginEvent::Type::AfterPluginChanges && m_pBaseType == nullptr)
  {
    RegisterBaseType();
  }
}

const WRTTI* WShaderTypeRegistry::GetShaderType(WStringView sShaderPath0)
{
  if (sShaderPath0.IsEmpty())
    return nullptr;

  WStringBuilder sShaderPath = sShaderPath0;
  sShaderPath.MakeCleanPath();

  if (sShaderPath.IsAbsolutePath())
  {
    if (!WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sShaderPath))
    {
      WLog::Error("Could not make shader path '{0}' relative!", sShaderPath);
    }
  }

  auto it = m_ShaderTypes.Find(sShaderPath);
  if (it.IsValid())
  {
    WFileStats Stats;
    if (WOSFile::GetFileStats(it.Value().m_sAbsShaderPath, Stats).Succeeded() &&
        !Stats.m_LastModificationTime.Compare(it.Value().m_fileModifiedTime, WTimestamp::CompareMode::FileTimeEqual))
    {
      UpdateShaderType(it.Value());
    }
  }
  else
  {
    WStringBuilder sAbsPath = sShaderPath0;
    {
      if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
      {
        WLog::Warning("Can't make path absolute: '{0}'", sShaderPath0);
        return nullptr;
      }
      sAbsPath.MakeCleanPath();
    }

    it = m_ShaderTypes.Insert(sShaderPath, ShaderData());
    it.Value().m_sShaderPath = sShaderPath;
    it.Value().m_sAbsShaderPath = sAbsPath;
    UpdateShaderType(it.Value());
  }

  return it.Value().m_pType;
}

void WShaderTypeRegistry::UpdateShaderType(ShaderData& data)
{
  W_LOG_BLOCK("Updating Shader Parameters", data.m_sShaderPath.GetData());

  WTempHybridArray<WShaderParser::ParameterDefinition, 16> parameters;
  WTempHybridArray<WShaderParser::EnumDefinition, 4> enumDefinitions;

  {
    WFileStats Stats;
    bool bStat = WOSFile::GetFileStats(data.m_sAbsShaderPath, Stats).Succeeded();

    WFileReader file;
    if (!bStat || file.Open(data.m_sAbsShaderPath).Failed())
    {
      WLog::Error("Can't update shader '{0}' type information, the file can't be opened.", data.m_sShaderPath);
      return;
    }

    WString sContent;
    sContent.ReadAll(file);
    WShaderHelper::WTextSectionizer sections;
    WShaderHelper::GetShaderSections(sContent, sections);
    WUInt32 uiFirstLine = 0;
    WStringView sSectionContent = sections.GetSectionContent(WShaderHelper::WShaderSections::MATERIALPARAMETER, uiFirstLine);

    WShaderParser::ParseMaterialParameterSection(sSectionContent, parameters, enumDefinitions);
    data.m_fileModifiedTime = Stats.m_LastModificationTime;
  }

  WReflectedTypeDescriptor desc;
  desc.m_sTypeName = data.m_sShaderPath;
  desc.m_sPluginName = "ShaderTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = WTypeFlags::Class;
  desc.m_uiTypeVersion = 2;

  for (auto& enumDef : enumDefinitions)
  {
    GetEnumType(enumDef);
  }

  for (auto& parameter : parameters)
  {
    const WRTTI* pType = GetType(parameter);
    if (pType == nullptr)
    {
      continue;
    }

    WBitflags<WPropertyFlags> flags;
    if (pType->IsDerivedFrom<WEnumBase>())
      flags |= WPropertyFlags::IsEnum;
    if (pType->IsDerivedFrom<WBitflagsBase>())
      flags |= WPropertyFlags::Bitflags;
    if (WReflectionUtils::IsBasicType(pType))
      flags |= WPropertyFlags::StandardType;

    WReflectedPropertyDescriptor propDesc(WPropertyCategory::Member, parameter.m_sName, pType->GetTypeName(), flags);

    AddAttributes(parameter, pType, propDesc.m_Attributes);

    desc.m_Properties.PushBack(propDesc);
  }

  // Register and return the phantom type. If the type already exists this will update the type
  // and patch any existing instances of it so they should show up in the prop grid right away.
  WPhantomRttiManager::s_Events.RemoveEventHandler(WMakeDelegate(&WShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
  {
    // We do not want to listen to type changes that we triggered ourselves.
    data.m_pType = WPhantomRttiManager::RegisterType(desc);
  }
  WPhantomRttiManager::s_Events.AddEventHandler(WMakeDelegate(&WShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

void WShaderTypeRegistry::PhantomTypeRegistryEventHandler(const WPhantomRttiManagerEvent& e)
{
  if (e.m_Type == WPhantomRttiManagerEvent::Type::TypeAdded)
  {
    if (e.m_pChangedType->GetParentType() == m_pBaseType)
    {
      GetShaderType(e.m_pChangedType->GetTypeName());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

/// Changes the base class of all shader types to WShaderTypeBase (version 1) and
/// sets their own version to 2.
class WShaderTypePatch_1_2 : public WGraphPatch
{
public:
  WShaderTypePatch_1_2()
    : WGraphPatch(nullptr, 2, WGraphPatch::PatchType::GraphPatch)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode*) const override
  {
    WString sDescTypeName = WGetStaticRTTI<WReflectedTypeDescriptor>()->GetTypeName();

    auto& nodes = pGraph->GetAllNodes();
    bool bNeedAddBaseClass = false;
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      WAbstractObjectNode* pNode = it.Value();
      if (pNode->GetType() == sDescTypeName)
      {
        auto* pTypeProperty = pNode->FindProperty("TypeName");
        if (WStringUtils::EndsWith(pTypeProperty->m_Value.Get<WString>(), ".WShader"))
        {
          auto* pTypeVersionProperty = pNode->FindProperty("TypeVersion");
          auto* pParentTypeProperty = pNode->FindProperty("ParentTypeName");
          if (pTypeVersionProperty->m_Value == 1)
          {
            pParentTypeProperty->m_Value = "WShaderTypeBase";
            pTypeVersionProperty->m_Value = (WUInt32)2;
            bNeedAddBaseClass = true;
          }
        }
      }
    }

    if (bNeedAddBaseClass)
    {
      WRttiConverterContext context;
      WRttiConverterWriter rttiConverter(pGraph, &context, true, true);

      WReflectedTypeDescriptor desc;
      desc.m_sTypeName = "WShaderTypeBase";
      desc.m_sPluginName = "ShaderTypes";
      desc.m_sParentTypeName = WGetStaticRTTI<WReflectedClass>()->GetTypeName();
      desc.m_Flags = WTypeFlags::Abstract | WTypeFlags::Class;
      desc.m_uiTypeVersion = 1;

      context.RegisterObject(WUuid::MakeStableUuidFromString(desc.m_sTypeName.GetData()), WGetStaticRTTI<WReflectedTypeDescriptor>(), &desc);
      rttiConverter.AddObjectToGraph(WGetStaticRTTI<WReflectedTypeDescriptor>(), &desc);
    }
  }
};

WShaderTypePatch_1_2 g_WShaderTypePatch_1_2;

// TODO: Increase WShaderTypeBase version to 2 and implement enum renames, see WReflectedPropertyDescriptorPatch_1_2
class WShaderBaseTypePatch_1_2 : public WGraphPatch
{
public:
  WShaderBaseTypePatch_1_2()
    : WGraphPatch("WShaderTypeBase", 2)
  {
  }

  static void FixEnumString(WStringBuilder& ref_sValue, const char* szName)
  {
    if (ref_sValue.StartsWith(szName))
      ref_sValue.Shrink(WStringUtils::GetCharacterCount(szName), 0);

    if (ref_sValue.StartsWith("::"))
      ref_sValue.Shrink(2, 0);

    if (ref_sValue.StartsWith(szName))
      ref_sValue.Shrink(WStringUtils::GetCharacterCount(szName), 0);

    if (ref_sValue.StartsWith("_"))
      ref_sValue.Shrink(1, 0);

    ref_sValue.PrependFormat("{0}::{0}_", szName);
  }

  void FixEnum(WAbstractObjectNode* pNode, const char* szEnum) const
  {
    if (WAbstractObjectNode::Property* pProp = pNode->FindProperty(szEnum))
    {
      WStringBuilder sValue = pProp->m_Value.Get<WString>();
      FixEnumString(sValue, szEnum);
      pProp->m_Value = sValue.GetData();
    }
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    FixEnum(pNode, "SHADING_MODE");
    FixEnum(pNode, "BLEND_MODE");
    FixEnum(pNode, "RENDER_PASS");
  }
};

WShaderBaseTypePatch_1_2 g_WShaderBaseTypePatch_1_2;
