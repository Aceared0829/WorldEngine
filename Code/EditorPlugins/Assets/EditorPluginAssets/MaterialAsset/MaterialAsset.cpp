#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/ShaderTypeRegistry.h>
#include <EditorPluginAssets/VisualShader/VsCodeGenerator.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

namespace
{
  WResult AddDefines(WDynamicArray<WString>& inout_defines, const WDocumentObject* pObject, const WAbstractProperty* pProp)
  {
    WStringBuilder sDefine;

    const char* szName = pProp->GetPropertyName();
    if (pProp->GetSpecificType()->GetVariantType() == WVariantType::Bool)
    {
      sDefine.Set(szName, " ", pObject->GetTypeAccessor().GetValue(szName).Get<bool>() ? "TRUE" : "FALSE");
      inout_defines.PushBack(sDefine);
      return W_SUCCESS;
    }
    else if (pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
    {
      WInt64 iValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<WInt64>();

      WTempHybridArray<WReflectionUtils::EnumKeyValuePair, 16> enumValues;
      WReflectionUtils::GetEnumKeysAndValues(pProp->GetSpecificType(), enumValues, WReflectionUtils::EnumConversionMode::ValueNameOnly);
      for (auto& enumValue : enumValues)
      {
        sDefine.SetFormat("{} {}", enumValue.m_sKey, enumValue.m_iValue);
        inout_defines.PushBack(sDefine);

        if (enumValue.m_iValue == iValue)
        {
          sDefine.Set(szName, " ", enumValue.m_sKey);
          inout_defines.PushBack(sDefine);
        }
      }

      return W_SUCCESS;
    }

    W_REPORT_FAILURE("Invalid shader permutation property type '{0}'", pProp->GetSpecificType()->GetTypeName());
    return W_FAILURE;
  }

  /// Adds preprocessor defines for a permutation variable that the shader pinned to a fixed value
  /// in its [PERMUTATIONS] section, e.g. "BLEND_MODE = BLEND_MODE_OPAQUE".
  ///
  /// Such a variable has no material property (it is not listed in [MATERIALPARAMETER]), so
  /// AddDefines cannot produce its defines. Without this, a [MATERIALCONFIG] section that evaluates
  /// the variable fails with "Undefined variable is evaluated". Enum variables also need every one
  /// of their values defined, because the config compares against those names.
  void AddFixedPermutationVarDefines(WDynamicArray<WString>& inout_defines, const WPermutationVar& permVar)
  {
    WStringBuilder sDefine;

    WStringBuilder sPath;
    sPath.SetFormat("Shaders/PermutationVars/{0}.WPermVar", permVar.m_sName);

    WString sAbsPath = sPath;
    WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsPath);

    WFileReader file;
    if (file.Open(sAbsPath).Succeeded())
    {
      WStringBuilder sContent;
      sContent.ReadAll(file);

      WVariant defaultValue;
      WShaderParser::EnumDefinition enumDefinition;
      WShaderParser::ParsePermutationVarConfig(sContent, defaultValue, enumDefinition);

      for (const auto& ev : enumDefinition.m_Values)
      {
        sDefine.SetFormat("{} {}", ev.m_sValueName, ev.m_iValueValue);
        inout_defines.PushBack(sDefine);
      }
    }

    sDefine.Set(permVar.m_sName.GetView(), " ", permVar.m_sValue.GetView());
    inout_defines.PushBack(sDefine);
  }

  WResult ParseMaterialConfig(WStringView sRelativeFileName, const WDocumentObject* pShaderPropertyObject, WVariantDictionary& out_materialConfig)
  {
    WFileReader file;
    if (file.Open(sRelativeFileName).Failed())
      return W_FAILURE;

    WString sContent;
    sContent.ReadAll(file);
    WShaderHelper::WTextSectionizer sections;
    WShaderHelper::GetShaderSections(sContent, sections);
    WUInt32 uiFirstLine = 0;
    WStringView sSectionContent = sections.GetSectionContent(WShaderHelper::WShaderSections::MATERIALCONFIG, uiFirstLine);

    WTempHybridArray<WString, 16> defines;
    {
      WTempHybridArray<const WAbstractProperty*, 32> permutationProperties;
      {
        WTempHybridArray<const WAbstractProperty*, 32> properties;
        pShaderPropertyObject->GetType()->GetAllProperties(properties);

        for (auto& pProp : properties)
        {
          const WCategoryAttribute* pCategory = pProp->GetAttributeByType<WCategoryAttribute>();
          if (pCategory == nullptr || WStringUtils::IsEqual(pCategory->GetCategory(), "Permutation") == false)
            continue;

          permutationProperties.PushBack(pProp);
        }
      }

      // Permutation variables that the shader pinned to a fixed value in [PERMUTATIONS] are not
      // exposed as material properties, but [MATERIALCONFIG] may still evaluate them. Define those
      // as well, skipping any that the material exposes, whose own value takes precedence.
      {
        WTempHybridArray<WHashedString, 16> permVars;
        WTempHybridArray<WPermutationVar, 16> fixedPermVars;
        WShaderParser::ParsePermutationSection(sections.GetSectionContent(WShaderHelper::WShaderSections::PERMUTATIONS, uiFirstLine), permVars, fixedPermVars);

        for (const auto& permVar : fixedPermVars)
        {
          bool bIsMaterialProperty = false;
          for (auto& pProp : permutationProperties)
          {
            if (permVar.m_sName.GetView().IsEqual(pProp->GetPropertyName()))
            {
              bIsMaterialProperty = true;
              break;
            }
          }

          if (!bIsMaterialProperty)
          {
            AddFixedPermutationVarDefines(defines, permVar);
          }
        }
      }

      for (auto& pProp : permutationProperties)
      {
        W_SUCCEED_OR_RETURN(AddDefines(defines, pShaderPropertyObject, pProp));
      }
    }

    WStringBuilder sOutput;
    W_SUCCEED_OR_RETURN(WShaderParser::PreprocessSection(sSectionContent, defines, sOutput));

    WTempHybridArray<WStringView, 32> allAssignments;
    sOutput.Split(false, allAssignments, "\n", ";", "\r");

    WStringBuilder temp;
    WTempHybridArray<WStringView, 4> components;
    for (const WStringView& assignment : allAssignments)
    {
      temp = assignment;
      temp.Trim(" \t\r\n;");
      if (temp.IsEmpty())
        continue;

      temp.Split(false, components, " ", "\t", "=", "\r");

      if (components.GetCount() != 2)
      {
        WLog::Error("Malformed shader state assignment: '{0}'", temp);
        continue;
      }

      out_materialConfig[components[0]] = components[1];
    }

    return W_SUCCESS;
  }
} // namespace

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WMaterialAssetPreview, 1)
  W_ENUM_CONSTANT(WMaterialAssetPreview::Ball),
  W_ENUM_CONSTANT(WMaterialAssetPreview::Sphere),
  W_ENUM_CONSTANT(WMaterialAssetPreview::Box),
  W_ENUM_CONSTANT(WMaterialAssetPreview::Plane),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WMaterialShaderMode, 1)
W_ENUM_CONSTANTS(WMaterialShaderMode::BaseMaterial, WMaterialShaderMode::File, WMaterialShaderMode::Custom)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMaterialAssetProperties, 4, WRTTIDefaultAllocator<WMaterialAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("ShaderMode", WMaterialShaderMode, GetShaderMode, SetShaderMode),
    W_ACCESSOR_PROPERTY("BaseMaterial", GetBaseMaterial, SetBaseMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material", "*", WDependencyFlags::Transform | WDependencyFlags::Thumbnail | WDependencyFlags::Package)),
    W_ACCESSOR_PROPERTY("Shader", GetShader, SetShader)->AddAttributes(new WFileBrowserAttribute("Select Shader", "*.WShader", "CustomAction_CreateShaderFromTemplate")),
    W_MEMBER_PROPERTY("AssetFilterTags", m_sAssetFilterTags),
    W_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
    // This property holds the phantom shader properties type so it is only used in the object graph but not actually in the instance of this object.
    W_ACCESSOR_PROPERTY("ShaderProperties", GetShaderProperties, SetShaderProperties)->AddFlags(WPropertyFlags::PointerOwner)->AddAttributes(new WContainerAttribute(false, false, false)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMaterialAssetDocument, 12, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WUuid WMaterialAssetDocument::s_LitBaseMaterial;
WUuid WMaterialAssetDocument::s_LitAlphaTextBaseMaterial;
WUuid WMaterialAssetDocument::s_NeutralNormalMap;

void WMaterialAssetProperties::SetBaseMaterial(const char* szBaseMaterial)
{
  if (m_sBaseMaterial == szBaseMaterial)
    return;

  m_sBaseMaterial = szBaseMaterial;

  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;
  if (m_pDocument->GetCommandHistory()->IsInUndoRedo())
    return;
  m_pDocument->SetBaseMaterial(m_sBaseMaterial);
}

const char* WMaterialAssetProperties::GetBaseMaterial() const
{
  return m_sBaseMaterial;
}

void WMaterialAssetProperties::SetShader(const char* szShader)
{
  if (m_sShader != szShader)
  {
    m_sShader = szShader;
    UpdateShader();
  }
}

const char* WMaterialAssetProperties::GetShader() const
{
  return m_sShader;
}

void WMaterialAssetProperties::SetShaderProperties(WReflectedClass* pProperties)
{
  // This property represents the phantom shader type, so it is never actually used.
}

WReflectedClass* WMaterialAssetProperties::GetShaderProperties() const
{
  // This property represents the phantom shader type, so it is never actually used.
  return nullptr;
}

void WMaterialAssetProperties::SetShaderMode(WEnum<WMaterialShaderMode> mode)
{
  if (m_ShaderMode == mode)
    return;

  m_ShaderMode = mode;

  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;
  WCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  WObjectAccessorBase* pAccessor = m_pDocument->GetObjectAccessor();
  // Do not make new commands if we got here in a response to an undo / redo action.
  if (pHistory->IsInUndoRedo())
    return;

  WStringBuilder tmp;

  switch (m_ShaderMode)
  {
    case WMaterialShaderMode::BaseMaterial:
    {
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "Shader", "").AssertSuccess();
    }
    break;
    case WMaterialShaderMode::File:
    {
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "Shader", "").AssertSuccess();
    }
    break;
    case WMaterialShaderMode::Custom:
    {
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "Shader", WConversionUtils::ToString(m_pDocument->GetGuid(), tmp).GetData()).AssertSuccess();
    }
    break;
  }
}

void WMaterialAssetProperties::SetDocument(WMaterialAssetDocument* pDocument)
{
  m_pDocument = pDocument;
  if (!m_sBaseMaterial.IsEmpty())
  {
    m_pDocument->SetBaseMaterial(m_sBaseMaterial);
  }
  UpdateShader(true);
}


void WMaterialAssetProperties::UpdateShader(bool bForce)
{
  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;

  WCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  // Do not make new commands if we got here in a response to an undo / redo action.
  if (pHistory->IsInUndoRedo())
    return;

  W_ASSERT_DEBUG(pHistory->IsInTransaction(), "Missing undo scope on stack.");

  WDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();

  // TODO: If m_sShader is empty, we need to get the shader of our base material and use that one instead
  // for the code below. The type name is the clean path to the shader at the moment.
  WStringBuilder sShaderPath = ResolveRelativeShaderPath();
  sShaderPath.MakeCleanPath();

  if (sShaderPath.IsEmpty())
  {
    // No shader, delete any existing properties object.
    if (pPropObject)
    {
      DeleteProperties();
    }
  }
  else
  {
    if (pPropObject)
    {
      // We already have a shader properties object, test whether
      // it has a different type than the newly set shader. The type name
      // is the clean path to the shader at the moment.
      const WRTTI* pType = pPropObject->GetTypeAccessor().GetType();
      if (sShaderPath != pType->GetTypeName() || bForce) // TODO: Is force even necessary anymore?
      {
        // Shader has changed, delete old and create new one.
        DeleteProperties();
        CreateProperties(sShaderPath);
      }
      else
      {
        // Same shader but it could have changed so try to update it anyway.
        WShaderTypeRegistry::GetSingleton()->GetShaderType(sShaderPath);
      }
    }

    if (!pPropObject)
    {
      // No shader properties exist yet, so create a new one.
      CreateProperties(sShaderPath);
    }
  }
}

void WMaterialAssetProperties::DeleteProperties()
{
  SaveOldValues();
  WCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  WDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  WRemoveObjectCommand cmd;
  cmd.m_Object = pPropObject->GetGuid();
  auto res = pHistory->AddCommand(cmd);
  W_ASSERT_DEV(res.Succeeded(), "Removal of old properties should never fail.");
}

void WMaterialAssetProperties::CreateProperties(const char* szShaderPath)
{
  WCommandHistory* pHistory = m_pDocument->GetCommandHistory();

  const WRTTI* pType = WShaderTypeRegistry::GetSingleton()->GetShaderType(szShaderPath);
  if (!pType && m_ShaderMode == WMaterialShaderMode::Custom)
  {
    // Force generate if custom shader is missing
    WAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(0, m_pDocument->GetAssetTypeVersion());
    m_pDocument->RecreateVisualShaderFile(AssetHeader).LogFailure();
    pType = WShaderTypeRegistry::GetSingleton()->GetShaderType(szShaderPath);
  }

  if (pType)
  {
    WAddObjectCommand cmd;
    cmd.m_pType = pType;
    cmd.m_sParentProperty = "ShaderProperties";
    cmd.m_Parent = m_pDocument->GetPropertyObject()->GetGuid();
    cmd.m_NewObjectGuid = cmd.m_Parent;
    cmd.m_NewObjectGuid.CombineWithSeed(WUuid::MakeStableUuidFromString("ShaderProperties"));

    auto res = pHistory->AddCommand(cmd);
    W_ASSERT_DEV(res.Succeeded(), "Addition of new properties should never fail.");
    LoadOldValues();
  }
}

void WMaterialAssetProperties::SaveOldValues()
{
  WDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  if (pPropObject)
  {
    const WIReflectedTypeAccessor& accessor = pPropObject->GetTypeAccessor();
    const WRTTI* pType = accessor.GetType();
    WTempHybridArray<const WAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetCategory() == WPropertyCategory::Member)
      {
        m_CachedProperties[pProp->GetPropertyName()] = accessor.GetValue(pProp->GetPropertyName());
      }
    }
  }
}

void WMaterialAssetProperties::LoadOldValues()
{
  WDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  WCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  if (pPropObject)
  {
    const WIReflectedTypeAccessor& accessor = pPropObject->GetTypeAccessor();
    const WRTTI* pType = accessor.GetType();
    WTempHybridArray<const WAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetCategory() == WPropertyCategory::Member)
      {
        WString sPropName = pProp->GetPropertyName();
        auto it = m_CachedProperties.Find(sPropName);
        if (it.IsValid())
        {
          if (it.Value() != accessor.GetValue(sPropName.GetData()))
          {
            WSetObjectPropertyCommand cmd;
            cmd.m_Object = pPropObject->GetGuid();
            cmd.m_sProperty = sPropName;
            cmd.m_NewValue = it.Value();

            // Do not check for success, if a cached value failed to apply, simply ignore it.
            pHistory->AddCommand(cmd).AssertSuccess();
          }
        }
      }
    }
  }
}

WString WMaterialAssetProperties::GetAutoGenShaderPathAbs() const
{
  WAssetDocumentManager* pManager = WDynamicCast<WAssetDocumentManager*>(m_pDocument->GetDocumentManager());
  WString sAbsOutputPath = pManager->GetAbsoluteOutputFileName(m_pDocument->GetAssetDocumentTypeDescriptor(), m_pDocument->GetDocumentPath(), WMaterialAssetDocumentManager::s_szShaderOutputTag);
  return sAbsOutputPath;
}

void WMaterialAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WMaterialAssetProperties>())
  {
    WInt64 shaderMode = e.m_pObject->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<WInt64>();

    auto& props = *e.m_pPropertyStates;

    if (shaderMode == WMaterialShaderMode::File)
      props["Shader"].m_Visibility = WPropertyUiState::Default;
    else
      props["Shader"].m_Visibility = WPropertyUiState::Invisible;

    if (shaderMode == WMaterialShaderMode::BaseMaterial)
      props["BaseMaterial"].m_Visibility = WPropertyUiState::Default;
    else
      props["BaseMaterial"].m_Visibility = WPropertyUiState::Invisible;
  }
}

WString WMaterialAssetProperties::ResolveRelativeShaderPath() const
{
  // Which shader a material uses is defined by its shader mode, m_sShader is only a cached value of that:
  //   * Custom       -> the Visual Shader that is generated for this very document
  //   * BaseMaterial -> whatever shader the base material uses
  //   * File         -> the shader file that m_sShader points to
  //
  // m_sShader can be stale, most notably when a material file was copied on disk: the copy keeps the guid
  // of the original, so it would be generating its own Visual Shader but silently render with the shader of
  // the original (including its render states and its render data category). Therefore derive the guid from
  // the shader mode wherever we can and only fall back to m_sShader.

  WUuid shaderGuid;

  if (m_ShaderMode == WMaterialShaderMode::Custom && m_pDocument != nullptr)
  {
    // a Visual Shader material always uses the shader that is generated for itself
    shaderGuid = m_pDocument->GetGuid();
  }
  else if (m_ShaderMode == WMaterialShaderMode::BaseMaterial && WConversionUtils::IsStringUuid(m_sBaseMaterial))
  {
    // if the base material generates a Visual Shader, that's the shader we inherit
    // otherwise the base material uses a plain shader file, whose path we inherit through m_sShader
    const WUuid baseGuid = WConversionUtils::ConvertStringToUuid(m_sBaseMaterial);

    if (auto pBaseAsset = WAssetCurator::GetSingleton()->GetSubAsset(baseGuid))
    {
      if (pBaseAsset->m_pAssetInfo->m_Info->m_Outputs.Contains(WMaterialAssetDocumentManager::s_szShaderOutputTag))
      {
        shaderGuid = baseGuid;
      }
    }
  }

  if (!shaderGuid.IsValid())
  {
    if (!WConversionUtils::IsStringUuid(m_sShader))
      return m_sShader;

    shaderGuid = WConversionUtils::ConvertStringToUuid(m_sShader);
  }

  auto pAsset = WAssetCurator::GetSingleton()->GetSubAsset(shaderGuid);
  if (pAsset == nullptr)
  {
    WStringBuilder sGuid;
    WLog::Error("Could not resolve guid '{0}' for the material shader.", WConversionUtils::ToString(shaderGuid, sGuid));
    return "";
  }

  if (m_pDocument == nullptr)
  {
    WLog::Error("Unknown material document.");
    return "";
  }

  W_ASSERT_DEV(pAsset->m_pAssetInfo->GetManager() == m_pDocument->GetDocumentManager(), "Referenced shader via guid by this material is not of type material asset (WMaterialShaderMode::Custom).");

  WStringBuilder sProjectDir = WAssetCurator::GetSingleton()->FindDataDirectoryForAsset(pAsset->m_pAssetInfo->m_Path);
  WStringBuilder sResult = pAsset->m_pAssetInfo->GetManager()->GetRelativeOutputFileName(m_pDocument->GetAssetDocumentTypeDescriptor(), sProjectDir, pAsset->m_pAssetInfo->m_Path, WMaterialAssetDocumentManager::s_szShaderOutputTag);

  sResult.Prepend("AssetCache/");
  return sResult;
}

//////////////////////////////////////////////////////////////////////////

WMaterialAssetDocument::WMaterialAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WMaterialAssetProperties>(W_DEFAULT_NEW(WMaterialObjectManager), sDocumentPath, WAssetDocEngineConnection::Simple, true)
{
  WQtEditorApp::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WMaterialAssetDocument::EditorEventHandler, this));
}

WMaterialAssetDocument::~WMaterialAssetDocument()
{
  WQtEditorApp::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WMaterialAssetDocument::EditorEventHandler, this));
}

void WMaterialAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  {
    WCommandHistory* pHistory = GetCommandHistory();
    pHistory->StartTransaction("Update Material Shader");
    GetProperties()->SetDocument(this);
    pHistory->FinishTransaction();
  }

  bool bSetModified = false;

  // The above command may patch the doc with the newest shader properties so we need to clear the undo history here.
  GetCommandHistory()->ClearUndoHistory();
  SetModified(bSetModified);
}

WDocumentObject* WMaterialAssetDocument::GetShaderPropertyObject()
{
  WDocumentObject* pObject = GetObjectManager()->GetRootObject()->GetChildren()[0];
  WIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  WUuid propObjectGuid = accessor.GetValue("ShaderProperties").ConvertTo<WUuid>();
  WDocumentObject* pPropObject = nullptr;
  if (propObjectGuid.IsValid())
  {
    pPropObject = GetObjectManager()->GetObject(propObjectGuid);
  }
  return pPropObject;
}

const WDocumentObject* WMaterialAssetDocument::GetShaderPropertyObject() const
{
  return const_cast<WMaterialAssetDocument*>(this)->GetShaderPropertyObject();
}

void WMaterialAssetDocument::SetBaseMaterial(const char* szBaseMaterial)
{
  WDocumentObject* pObject = GetPropertyObject();
  auto pAssetInfo = WAssetCurator::GetSingleton()->FindSubAsset(szBaseMaterial);
  if (pAssetInfo == nullptr)
  {
    WTempHybridArray<const WDocumentObject*, 2> sel;
    sel.PushBack(pObject);
    UnlinkPrefabs(sel);
  }
  else
  {
    const WStringBuilder& sNewBase = WPrefabCache::GetSingleton()->GetCachedPrefabDocument(pAssetInfo->m_Data.m_Guid);
    const WAbstractObjectGraph* pBaseGraph = WPrefabCache::GetSingleton()->GetCachedPrefabGraph(pAssetInfo->m_Data.m_Guid);

    WUuid seed = GetSeedFromBaseMaterial(pBaseGraph);
    if (sNewBase.IsEmpty() || !pBaseGraph || !seed.IsValid())
    {
      WLog::Error("The selected base material '{0}' is not a valid material file!", szBaseMaterial);
      return;
    }

    {
      auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObject->GetGuid());

      if (pMeta->m_CreateFromPrefab != pAssetInfo->m_Data.m_Guid)
      {
        pMeta->m_sBasePrefab = sNewBase;
        pMeta->m_CreateFromPrefab = pAssetInfo->m_Data.m_Guid;
        pMeta->m_PrefabSeedGuid = seed;
      }
      m_DocumentObjectMetaData->EndModifyMetaData(WDocumentObjectMetaData::PrefabFlag);
    }
    UpdatePrefabs();
  }
}

WUuid WMaterialAssetDocument::GetSeedFromBaseMaterial(const WAbstractObjectGraph* pBaseGraph)
{
  if (!pBaseGraph)
    return WUuid();

  WUuid instanceGuid = GetPropertyObject()->GetGuid();
  WUuid baseGuid = WMaterialAssetDocument::GetMaterialNodeGuid(*pBaseGraph);
  if (baseGuid.IsValid())
  {
    // Create seed that converts base guid into instance guid
    instanceGuid.RevertCombinationWithSeed(baseGuid);
    return instanceGuid;
  }

  return WUuid();
}

WUuid WMaterialAssetDocument::GetMaterialNodeGuid(const WAbstractObjectGraph& graph)
{
  for (auto it = graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetType() == WGetStaticRTTI<WMaterialAssetProperties>()->GetTypeName())
    {
      return it.Value()->GetGuid();
    }
  }
  return WUuid();
}

void WMaterialAssetDocument::UpdatePrefabObject(WDocumentObject* pObject, const WUuid& PrefabAsset, const WUuid& PrefabSeed, WStringView sBasePrefab)
{
  // Base
  WAbstractObjectGraph baseGraph;
  WPrefabUtils::LoadGraph(baseGraph, sBasePrefab);
  baseGraph.PruneGraph(GetMaterialNodeGuid(baseGraph));

  // NewBase
  const WStringBuilder& sLeft = WPrefabCache::GetSingleton()->GetCachedPrefabDocument(PrefabAsset);
  const WAbstractObjectGraph* pLeftGraph = WPrefabCache::GetSingleton()->GetCachedPrefabGraph(PrefabAsset);
  WAbstractObjectGraph leftGraph;
  if (pLeftGraph)
  {
    pLeftGraph->Clone(leftGraph);
  }
  else
  {
    WStringBuilder sGuid;
    WConversionUtils::ToString(PrefabAsset, sGuid);
    WLog::Error("Can't update prefab, new base graph does not exist: {0}", sGuid);
    return;
  }
  leftGraph.PruneGraph(GetMaterialNodeGuid(leftGraph));

  // Instance
  WAbstractObjectGraph rightGraph;
  {
    WDocumentObjectConverterWriter writer(&rightGraph, pObject->GetDocumentObjectManager());
    writer.AddObjectToGraph(pObject);
    rightGraph.ReMapNodeGuids(PrefabSeed, true);
  }

  // Merge diffs relative to base
  WDeque<WAbstractGraphDiffOperation> mergedDiff;
  WPrefabUtils::Merge(baseGraph, leftGraph, rightGraph, mergedDiff);

  // Skip 'ShaderMode' as it should not be inherited, and 'ShaderProperties' is being set by the 'Shader' property
  WDeque<WAbstractGraphDiffOperation> cleanedDiff;
  for (const WAbstractGraphDiffOperation& op : mergedDiff)
  {
    if (op.m_Operation == WAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      if (op.m_sProperty == "ShaderMode" || op.m_sProperty == "ShaderProperties")
        continue;

      cleanedDiff.PushBack(op);
    }
  }

  // Apply diff to base, making it the new instance
  baseGraph.ApplyDiff(cleanedDiff);

  // Do not allow 'Shader' to be overridden, always use the prefab template version.
  if (WAbstractObjectNode* pNode = leftGraph.GetNode(GetMaterialNodeGuid(leftGraph)))
  {
    if (auto pProp = pNode->FindProperty("Shader"))
    {
      if (WAbstractObjectNode* pNodeBase = baseGraph.GetNode(GetMaterialNodeGuid(baseGraph)))
      {
        pNodeBase->ChangeProperty("Shader", pProp->m_Value);
      }
    }
  }

  // Create a new diff that changes our current instance to the new instance
  WDeque<WAbstractGraphDiffOperation> newInstanceToCurrentInstance;
  baseGraph.CreateDiffWithBaseGraph(rightGraph, newInstanceToCurrentInstance);
  if (false)
  {
    WFileWriter file;
    file.Open("C:\\temp\\Material - diff.txt").IgnoreResult();

    WStringBuilder sDiff;
    sDiff.Append("######## New Instance To Instance #######\n");
    WPrefabUtils::WriteDiff(newInstanceToCurrentInstance, sDiff);
    file.WriteBytes(sDiff.GetData(), sDiff.GetElementCount()).IgnoreResult();
  }
  // Apply diff to current instance
  // Shader needs to be set first
  for (WUInt32 i = 0; i < newInstanceToCurrentInstance.GetCount(); ++i)
  {
    if (newInstanceToCurrentInstance[i].m_sProperty == "Shader")
    {
      WAbstractGraphDiffOperation op = newInstanceToCurrentInstance[i];
      newInstanceToCurrentInstance.RemoveAtAndCopy(i);
      newInstanceToCurrentInstance.InsertAt(0, op);
      break;
    }
  }
  for (const WAbstractGraphDiffOperation& op : newInstanceToCurrentInstance)
  {
    if (op.m_Operation == WAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      // Never change this material's mode, as it should not be inherited from prefab base
      if (op.m_sProperty == "ShaderMode")
        continue;

      // these properties may not exist and we do not want to change them either
      if (op.m_sProperty == "MetaBasePrefab" || op.m_sProperty == "MetaPrefabSeed" || op.m_sProperty == "MetaFromPrefab")
        continue;

      WSetObjectPropertyCommand cmd;
      cmd.m_Object = op.m_Node;
      cmd.m_Object.CombineWithSeed(PrefabSeed);
      cmd.m_NewValue = op.m_Value;
      cmd.m_sProperty = op.m_sProperty;

      auto pObj = GetObjectAccessor()->GetObject(cmd.m_Object);
      if (!pObj)
        continue;

      auto pProp = pObj->GetType()->FindPropertyByName(op.m_sProperty);
      if (!pProp)
        continue;

      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
        continue;

      GetCommandHistory()->AddCommand(cmd).AssertSuccess();
    }
  }

  // Update prefab meta data
  {
    auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMeta->m_CreateFromPrefab = PrefabAsset; // Should not change
    pMeta->m_PrefabSeedGuid = PrefabSeed;    // Should not change
    pMeta->m_sBasePrefab = sLeft;

    m_DocumentObjectMetaData->EndModifyMetaData(WDocumentObjectMetaData::PrefabFlag);
  }
}

class WVisualShaderErrorLog : public WLogInterface
{
public:
  WStringBuilder m_sResult;
  WResult m_Status;

  WVisualShaderErrorLog()
    : m_Status(W_SUCCESS)
  {
  }

  virtual void HandleLogMessage(const WLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case WLogMsgType::ErrorMsg:
        m_Status = W_FAILURE;
        m_sResult.Append("Error: ", le.m_sText, "\n");
        break;

      case WLogMsgType::SeriousWarningMsg:
      case WLogMsgType::WarningMsg:
        m_sResult.Append("Warning: ", le.m_sText, "\n");
        break;

      default:
        return;
    }
  }
};

WTransformStatus WMaterialAssetDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  if (sOutputTag.IsEqual(WMaterialAssetDocumentManager::s_szShaderOutputTag))
  {
    WStatus ret = RecreateVisualShaderFile(AssetHeader);

    if (transformFlags.IsSet(WTransformFlags::ForceTransform))
    {
      WMaterialVisualShaderEvent e;

      if (GetProperties()->m_ShaderMode == WMaterialShaderMode::Custom)
      {
        e.m_Type = WMaterialVisualShaderEvent::TransformFailed;
        e.m_sTransformError = ret.GetMessageString();

        if (ret.Succeeded())
        {
          e.m_Type = WMaterialVisualShaderEvent::TransformSucceeded;
          WStringBuilder sAutoGenShader = GetProperties()->GetAutoGenShaderPathAbs();

          QStringList arguments;
          WStringBuilder temp;

          arguments << "-project";
          arguments << QString::fromUtf8(WToolsProject::GetSingleton()->GetProjectDirectory().GetData());

          arguments << "-shader";
          arguments << QString::fromUtf8(sAutoGenShader.GetData());

          arguments << "-platform";
          arguments << "DX11_SM50"; /// \todo Rendering platform is currently hardcoded

          // determine the permutation variables that should get fixed values
          {
            // m_sCheckPermutations are just all fixed perm vars from every node in the VS
            WStringBuilder temp = m_sCheckPermutations;
            WDeque<WStringView> perms;
            temp.Split(false, perms, "\n");

            // remove duplicates
            WSet<WString> uniquePerms;
            for (const WStringView& perm : perms)
            {
              uniquePerms.Insert(perm);
            }

            // pass permutation variable definitions to the compiler: "SOME_VAR=SOME_VAL"
            arguments << "-perm";
            for (auto it = uniquePerms.GetIterator(); it.IsValid(); ++it)
            {
              arguments << it.Key().GetData();
            }
          }

          WVisualShaderErrorLog log;

          ret = WQtEditorApp::GetSingleton()->ExecuteTool("WShaderCompiler", arguments, 60, &log);
          if (ret.Failed())
          {
            e.m_Type = WMaterialVisualShaderEvent::TransformFailed;
            e.m_sTransformError = ret.GetMessageString();
          }
          else
          {
            e.m_Type = log.m_Status.Succeeded() ? WMaterialVisualShaderEvent::TransformSucceeded : WMaterialVisualShaderEvent::TransformFailed;
            e.m_sTransformError = log.m_sResult;
            WLog::Info("Compiled Visual Shader.");
          }
        }
      }
      else
      {
        e.m_Type = WMaterialVisualShaderEvent::VisualShaderNotUsed;
      }

      if (e.m_Type == WMaterialVisualShaderEvent::TransformFailed)
      {
        TagVisualShaderFileInvalid(pAssetProfile, e.m_sTransformError);
      }

      m_VisualShaderEvents.Broadcast(e);
    }

    return ret;
  }
  else
  {
    return SUPER::InternalTransformAsset(szTargetFile, sOutputTag, pAssetProfile, AssetHeader, transformFlags);
  }
}

WTransformStatus WMaterialAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  W_ASSERT_DEV(sOutputTag.IsEmpty(), "Additional output '{0}' not implemented!", sOutputTag);

  return WriteMaterialAsset(stream, pAssetProfile, true);
}

WTransformStatus WMaterialAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  return WAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
}

void WMaterialAssetDocument::InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const
{
  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void WMaterialAssetDocument::AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void WMaterialAssetDocument::RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void WMaterialAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const auto pProperties = GetProperties();

  if (!pProperties->m_sAssetFilterTags.IsEmpty())
  {
    WStringBuilder tags(";", pProperties->m_sAssetFilterTags, ";");
    while (tags.ReplaceAll(";;", ";") > 0)
    {
    }

    pInfo->m_sAssetsDocumentTags = tags;
  }
  else
  {
    pInfo->m_sAssetsDocumentTags.Clear();
  }

  if (pProperties->GetShaderMode() == WMaterialShaderMode::BaseMaterial)
  {
    // if we have a base material, copy the document tags from there
    // TODO: this is problematic, as changes to the tags in the base document would need to be saved in the derived document
    // it would be better, if the asset curator could store a reference to the base document and pull the tags directly from there, on demand

    if (auto pAsset = WAssetCurator::GetSingleton()->FindSubAsset(pProperties->GetBaseMaterial()))
    {
      const WStringView baseTags = pAsset->m_pAssetInfo->m_Info->GetAssetsDocumentTags();
      if (!baseTags.IsEmpty())
      {
        WStringBuilder tmp(pInfo->m_sAssetsDocumentTags, baseTags);
        while (tmp.ReplaceAll(";;", ";") > 0)
        {
        }
        pInfo->m_sAssetsDocumentTags = tmp;
      }
    }
  }
  else
  {
    // remove base material dependency, if it isn't used
    pInfo->m_TransformDependencies.Remove(pProperties->GetBaseMaterial());
    pInfo->m_ThumbnailDependencies.Remove(pProperties->GetBaseMaterial());
  }

  if (pProperties->m_ShaderMode != WMaterialShaderMode::File)
  {
    const bool bInUseByBaseMaterial = pProperties->m_ShaderMode == WMaterialShaderMode::BaseMaterial && WStringUtils::IsEqual(pProperties->GetShader(), pProperties->GetBaseMaterial());

    // remove shader file dependency, if it isn't used and differs from the base material
    if (!bInUseByBaseMaterial)
    {
      pInfo->m_TransformDependencies.Remove(pProperties->GetShader());
      pInfo->m_ThumbnailDependencies.Remove(pProperties->GetShader());
    }
  }

  if (pProperties->m_ShaderMode == WMaterialShaderMode::Custom)
  {
    // We write our own guid into the shader field so BaseMaterial materials can find the shader file.
    // This would cause us to have a dependency to ourselves so we need to remove it.
    WStringBuilder tmp;
    pInfo->m_TransformDependencies.Remove(WConversionUtils::ToString(GetGuid(), tmp));
    pInfo->m_ThumbnailDependencies.Remove(WConversionUtils::ToString(GetGuid(), tmp));

    WVisualShaderCodeGenerator codeGen;

    WSet<WString> cfgFiles;
    codeGen.DetermineConfigFileDependencies(static_cast<const WVisualGraphObjectManager*>(GetObjectManager()), cfgFiles);

    for (const auto& sCfgFile : cfgFiles)
    {
      pInfo->m_TransformDependencies.Insert(sCfgFile);
      pInfo->m_ThumbnailDependencies.Insert(sCfgFile);
    }

    pInfo->m_Outputs.Insert(WMaterialAssetDocumentManager::s_szShaderOutputTag);

    /// \todo The Visual Shader node configuration files would need to be a dependency of the auto-generated shader.
  }
}

WStatus WMaterialAssetDocument::WriteMaterialAsset(WStreamWriter& inout_stream0, const WPlatformProfile* pAssetProfile, bool bEmbedLowResData) const
{
  const WMaterialAssetProperties* pProp = GetProperties();

  WStringBuilder sValue;

  // now generate the .WBinMaterial file
  {
    const WUInt8 uiVersion = 8;

    inout_stream0 << uiVersion;

    WUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    uiCompressionMode = 1;
    WCompressedStreamWriterZstd stream(&inout_stream0, 0, WCompressedStreamWriterZstd::Compression::Average);
#else
    WStreamWriter& stream = stream0;
#endif

    inout_stream0 << uiCompressionMode;

    stream << pProp->m_sBaseMaterial;
    stream << pProp->m_sSurface;

    WString sRelativeShaderPath = pProp->ResolveRelativeShaderPath();
    stream << sRelativeShaderPath;

    WTempHybridArray<const WAbstractProperty*, 16> Textures2D;
    WTempHybridArray<const WAbstractProperty*, 16> TexturesCube;
    WTempHybridArray<const WAbstractProperty*, 16> Permutations;
    WTempHybridArray<const WAbstractProperty*, 16> Constants;

    const WDocumentObject* pObject = GetShaderPropertyObject();
    if (pObject != nullptr)
    {
      bool hasBaseMaterial = WPrefabUtils::GetPrefabRoot(pObject, *m_DocumentObjectMetaData).IsValid();
      auto pType = pObject->GetTypeAccessor().GetType();
      WTempHybridArray<const WAbstractProperty*, 32> properties;
      pType->GetAllProperties(properties);

      WTempHybridArray<WPropertySelection, 1> selection;
      selection.PushBack({pObject, WVariant()});
      WDefaultObjectState defaultState(pType, GetObjectAccessor(), selection.GetArrayPtr());

      for (auto pProp : properties)
      {
        // Starting with version 8, we skip this check and are flattening all base classes into this material. This is effectively removing runtime inheritance of materials due to the high runtime cost of maintaining the inheritance.
        // if (hasBaseMaterial && defaultState.IsDefaultValue(pProp))
        //  continue;

        const WCategoryAttribute* pCategory = pProp->GetAttributeByType<WCategoryAttribute>();

        W_ASSERT_DEBUG(pCategory, "Category cannot be null for a shader property");
        if (pCategory == nullptr)
          continue;

        if (WStringUtils::IsEqual(pCategory->GetCategory(), "Texture 2D") || WStringUtils::IsEqual(pCategory->GetCategory(), "Texture 2D Array"))
        {
          Textures2D.PushBack(pProp);
        }
        else if (WStringUtils::IsEqual(pCategory->GetCategory(), "Texture Cube"))
        {
          TexturesCube.PushBack(pProp);
        }
        else if (WStringUtils::IsEqual(pCategory->GetCategory(), "Permutation"))
        {
          Permutations.PushBack(pProp);
        }
        else if (WStringUtils::IsEqual(pCategory->GetCategory(), "Constant"))
        {
          Constants.PushBack(pProp);
        }
        else
        {
          W_REPORT_FAILURE("Invalid shader property type '{0}'", pCategory->GetCategory());
        }
      }
    }

    // write out the permutation variables
    {
      const WUInt16 uiPermVars = Permutations.GetCount();
      stream << uiPermVars;

      for (auto pProp : Permutations)
      {
        W_ASSERT_DEBUG(pObject != nullptr, "Need object to write out permutation");
        const char* szName = pProp->GetPropertyName();
        if (pProp->GetSpecificType()->GetVariantType() == WVariantType::Bool)
        {
          sValue = pObject->GetTypeAccessor().GetValue(szName).Get<bool>() ? "TRUE" : "FALSE";
        }
        else if (pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
        {
          WReflectionUtils::EnumerationToString(pProp->GetSpecificType(), pObject->GetTypeAccessor().GetValue(szName).ConvertTo<WInt64>(), sValue, WReflectionUtils::EnumConversionMode::ValueNameOnly);
        }
        else
        {
          W_REPORT_FAILURE("Invalid shader permutation property type '{0}'", pProp->GetSpecificType()->GetTypeName());
        }

        stream << szName;
        stream << sValue;
      }
    }

    // write out the 2D textures
    {
      const WUInt16 uiTextures = Textures2D.GetCount();
      stream << uiTextures;

      for (auto pProp : Textures2D)
      {
        W_ASSERT_DEBUG(pObject != nullptr, "Need object to write out texture");
        const char* szName = pProp->GetPropertyName();
        sValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<WString>();

        stream << szName;
        stream << sValue;
      }
    }

    // write out the Cube textures
    {
      const WUInt16 uiTextures = TexturesCube.GetCount();
      stream << uiTextures;

      for (auto pProp : TexturesCube)
      {
        W_ASSERT_DEBUG(pObject != nullptr, "Need object to write out texture cube");
        const char* szName = pProp->GetPropertyName();
        sValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<WString>();

        stream << szName;
        stream << sValue;
      }
    }

    // write out the constants
    {
      const WUInt16 uiConstants = Constants.GetCount();
      stream << uiConstants;

      for (auto pProp : Constants)
      {
        W_ASSERT_DEBUG(pObject != nullptr, "Need object to write out constant");
        const char* szName = pProp->GetPropertyName();
        WVariant value = pObject->GetTypeAccessor().GetValue(szName);

        stream << szName;
        stream << value;
      }
    }

    // render data category
    {
      WVariantDictionary materialConfig;
      if (pObject != nullptr)
      {
        W_SUCCEED_OR_RETURN(ParseMaterialConfig(sRelativeShaderPath, pObject, materialConfig));
      }

      WVariant renderDataCategory;
      materialConfig.TryGetValue("RenderDataCategory", renderDataCategory);

      stream << renderDataCategory.ConvertTo<WString>();
    }

    // find and embed low res texture data
    {
      if (bEmbedLowResData)
      {
        WStringBuilder sFilename, sResourceName;
        WTempArray<WUInt32> content;

        // embed 2D texture data (not array textures - their lowres data has a different DDS structure)
        for (auto prop : Textures2D)
        {
          W_ASSERT_DEBUG(pObject != nullptr, "Need object to write out texture2d");
          const WCategoryAttribute* pCat = prop->GetAttributeByType<WCategoryAttribute>();
          if (pCat != nullptr && WStringUtils::IsEqual(pCat->GetCategory(), "Texture 2D Array"))
            continue;

          const char* szName = prop->GetPropertyName();
          sValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<WString>();

          if (sValue.IsEmpty())
            continue;

          sResourceName = sValue;

          auto asset = WAssetCurator::GetSingleton()->FindSubAsset(sValue);
          if (!asset.isValid())
            continue;

          sValue = asset->m_pAssetInfo->GetManager()->GetAbsoluteOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, asset->m_pAssetInfo->m_Path, "", pAssetProfile);

          sFilename = sValue.GetFileName();
          sFilename.Append("-lowres");

          sValue.ChangeFileName(sFilename);

          WFileReader file;
          if (file.Open(sValue).Failed())
            continue;

          content.SetCountUninitialized(file.GetFileSize());

          file.ReadBytes(content.GetData(), content.GetCount());

          stream << sResourceName;
          stream << content.GetCount();
          W_SUCCEED_OR_RETURN(stream.WriteBytes(content.GetData(), content.GetCount()));
        }
      }

      // marker: end of embedded data
      stream << "";
    }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    W_SUCCEED_OR_RETURN(stream.FinishCompressedStream());

    WLog::Dev("Compressed material data from {0} KB to {1} KB ({2}%%)", WArgF((float)stream.GetUncompressedSize() / 1024.0f, 1), WArgF((float)stream.GetCompressedSize() / 1024.0f, 1), WArgF(100.0f * stream.GetCompressedSize() / stream.GetUncompressedSize(), 1));
#endif
  }

  return WStatus(W_SUCCESS);
}

void WMaterialAssetDocument::TagVisualShaderFileInvalid(const WPlatformProfile* pAssetProfile, const char* szError)
{
  if (GetProperties()->m_ShaderMode != WMaterialShaderMode::Custom)
    return;

  WAssetDocumentManager* pManager = WDynamicCast<WAssetDocumentManager*>(GetDocumentManager());
  WString sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), WMaterialAssetDocumentManager::s_szShaderOutputTag);

  WStringBuilder all;

  // read shader source
  {
    WFileReader file;
    if (file.Open(sAutoGenShader).Failed())
      return;

    all.ReadAll(file);
  }

  all.PrependFormat("/*\n{0}\n*/\n", szError);

  // write adjusted shader source
  {
    WFileWriter fileOut;
    if (fileOut.Open(sAutoGenShader).Failed())
      return;

    fileOut.WriteBytes(all.GetData(), all.GetElementCount()).IgnoreResult();
  }
}

WStatus WMaterialAssetDocument::RecreateVisualShaderFile(const WAssetFileHeader& assetHeader)
{
  if (GetProperties()->m_ShaderMode != WMaterialShaderMode::Custom)
  {
    return WStatus(W_SUCCESS);
  }

  WAssetDocumentManager* pManager = WDynamicCast<WAssetDocumentManager*>(GetDocumentManager());
  WString sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), WMaterialAssetDocumentManager::s_szShaderOutputTag);

  WVisualShaderCodeGenerator codeGen;

  W_SUCCEED_OR_RETURN(codeGen.GenerateVisualShader(static_cast<const WVisualGraphObjectManager*>(GetObjectManager()), m_sCheckPermutations));

  WFileWriter file;
  if (file.Open(sAutoGenShader).Succeeded())
  {
    WStringBuilder shader = codeGen.GetFinalShaderCode();
    shader.PrependFormat("//{0}|{1}\n", assetHeader.GetFileHash(), assetHeader.GetFileVersion());

    W_SUCCEED_OR_RETURN(file.WriteBytes(shader.GetData(), shader.GetElementCount()));
    file.Close();

    InvalidateCachedShader();

    return WStatus(W_SUCCESS);
  }
  else
    return WStatus(WFmt("Failed to write auto-generated shader to '{0}'", sAutoGenShader));
}

void WMaterialAssetDocument::InvalidateCachedShader()
{
  WAssetDocumentManager* pManager = WDynamicCast<WAssetDocumentManager*>(GetDocumentManager());
  WString sShader;

  if (GetProperties()->m_ShaderMode == WMaterialShaderMode::Custom)
  {
    sShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), WMaterialAssetDocumentManager::s_szShaderOutputTag);
  }
  else
  {
    sShader = GetProperties()->GetShader();
  }

  // This should update the shader parameter section in all affected materials
  WShaderTypeRegistry::GetSingleton()->GetShaderType(sShader);
}

void WMaterialAssetDocument::EditorEventHandler(const WEditorAppEvent& e)
{
  if (e.m_Type == WEditorAppEvent::Type::ReloadResources)
  {
    InvalidateCachedShader();
  }
}

static void MarkReachableNodes(WMap<const WDocumentObject*, bool>& ref_allNodes, const WDocumentObject* pRoot, WVisualGraphObjectManager* pNodeManager)
{
  if (ref_allNodes[pRoot])
    return;

  ref_allNodes[pRoot] = true;

  auto allInputs = pNodeManager->GetInputPins(pRoot);

  // we start at the final output, so use the inputs on a node and then walk backwards
  for (auto& pTargetPin : allInputs)
  {
    auto connections = pNodeManager->GetConnections(*pTargetPin);

    // all incoming connections at the input pin, there should only be one though
    for (const WVisualGraphConnection* const pConnection : connections)
    {
      // output pin on other node connecting to this node
      const WVisualGraphPin& sourcePin = pConnection->GetSourcePin();

      // recurse from here
      MarkReachableNodes(ref_allNodes, sourcePin.GetParent(), pNodeManager);
    }
  }
}

void WMaterialAssetDocument::RemoveDisconnectedNodes()
{
  WVisualGraphObjectManager* pNodeManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());

  const WDocumentObject* pRoot = pNodeManager->GetRootObject();
  const WRTTI* pNodeBaseRtti = WVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

  const WTempHybridArray<WDocumentObject*, 8>& children = pRoot->GetChildren();
  WMap<const WDocumentObject*, bool> AllNodes;

  for (WUInt32 i = 0; i < children.GetCount(); ++i)
  {
    if (children[i]->GetType()->IsDerivedFrom(pNodeBaseRtti))
    {
      AllNodes[children[i]] = false;
    }
  }

  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    // skip nodes that have already been marked
    if (it.Value())
      continue;

    auto pDesc = WVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(it.Key()->GetType());

    if (pDesc->m_NodeType == WVisualShaderNodeType::Main || pDesc->m_NodeType == WVisualShaderNodeType::ShaderState)
    {
      MarkReachableNodes(AllNodes, it.Key(), pNodeManager);
    }
  }

  // now purge all nodes that haven't been reached
  {
    auto pHistory = GetCommandHistory();
    pHistory->StartTransaction("Purge unreachable nodes");

    for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
    {
      // skip nodes that have been marked
      if (it.Value())
        continue;

      WRemoveNodeCommand rem;
      rem.m_Object = it.Key()->GetGuid();

      pHistory->AddCommand(rem).AssertSuccess();
    }

    pHistory->FinishTransaction();
  }
}

namespace
{
  /// Looks up one of the fixed Base assets that the importers reference by path.
  WUuid FindBaseAsset(WUuid& inout_cached, const char* szAssetPath, const char* szWhat)
  {
    if (inout_cached.IsValid())
      return inout_cached;

    if (auto assetInfo = WAssetCurator::GetSingleton()->FindSubAsset(szAssetPath, true))
    {
      inout_cached = assetInfo->m_Data.m_Guid;
    }
    else
    {
      WLog::Error("Can't find {} {}", szWhat, szAssetPath);
    }

    return inout_cached;
  }
} // namespace

WUuid WMaterialAssetDocument::GetLitBaseMaterial()
{
  return FindBaseAsset(s_LitBaseMaterial, WMaterialResource::GetDefaultMaterialFileName(WMaterialResource::DefaultMaterialType::Lit), "default lit material");
}

WUuid WMaterialAssetDocument::GetLitAlphaTestBaseMaterial()
{
  return FindBaseAsset(s_LitAlphaTextBaseMaterial, WMaterialResource::GetDefaultMaterialFileName(WMaterialResource::DefaultMaterialType::LitAlphaTest), "default lit alpha test material");
}

WUuid WMaterialAssetDocument::GetNeutralNormalMap()
{
  return FindBaseAsset(s_NeutralNormalMap, "Base/Textures/NeutralNormal.WTextureAsset", "neutral normal map texture");
}

void WMaterialAssetDocument::GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/WEditor.NodeGraph");
}

bool WMaterialAssetDocument::CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_sMimeType) const
{
  out_sMimeType = "application/WEditor.NodeGraph";

  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool WMaterialAssetDocument::Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType)
{
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, WQtVisualGraphScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WMaterialAssetPropertiesPatch_1_2 : public WGraphPatch
{
public:
  WMaterialAssetPropertiesPatch_1_2()
    : WGraphPatch("WMaterialAssetProperties", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Shader Mode", "ShaderMode");
    pNode->RenameProperty("Base Material", "BaseMaterial");
  }
};

WMaterialAssetPropertiesPatch_1_2 g_WMaterialAssetPropertiesPatch_1_2;


class WMaterialAssetPropertiesPatch_2_3 : public WGraphPatch
{
public:
  WMaterialAssetPropertiesPatch_2_3()
    : WGraphPatch("WMaterialAssetProperties", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pBaseMatProp = pNode->FindProperty("BaseMaterial");
    auto* pShaderModeProp = pNode->FindProperty("ShaderMode");
    if (pBaseMatProp && pBaseMatProp->m_Value.IsA<WString>())
    {
      if (!pBaseMatProp->m_Value.Get<WString>().IsEmpty())
      {
        // BaseMaterial is set
        pNode->ChangeProperty("ShaderMode", (WInt32)WMaterialShaderMode::BaseMaterial);
      }
      else
      {
        pNode->ChangeProperty("ShaderMode", (WInt32)WMaterialShaderMode::File);
      }
    }
  }
};

WMaterialAssetPropertiesPatch_2_3 g_WMaterialAssetPropertiesPatch_2_3;

//////////////////////////////////////////////////////////////////////////

class WMaterialAssetPropertiesPatch_10_11 : public WGraphPatch
{
public:
  WMaterialAssetPropertiesPatch_10_11()
    : WGraphPatch(nullptr, 11, WGraphPatch::PatchType::GraphPatch)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode*) const override
  {
    auto& nodes = pGraph->GetAllNodes();
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      WAbstractObjectNode* pNode = it.Value();
      WStringView sType = pNode->GetType();

      const char* szNewName = nullptr;

      if (sType == "ShaderNode::BaseTexture")
      {
        szNewName = "BaseTexture";
      }
      else if (sType == "ShaderNode::EmissiveTexture")
      {
        szNewName = "EmissiveTexture";
      }
      else if (sType == "ShaderNode::MetallicTexture")
      {
        szNewName = "MetallicTexture";
      }

      if (szNewName != nullptr)
      {
        pNode->SetType("ShaderNode::Texture2D");

        if (auto* pNameProp = pNode->FindProperty("Name"))
        {
          pNameProp->m_Value = szNewName;
        }
        else
        {
          pNode->AddProperty("Name", WVariant(szNewName));
        }
      }
    }
  }
};

WMaterialAssetPropertiesPatch_10_11 g_WMaterialAssetPropertiesPatch_10_11;
