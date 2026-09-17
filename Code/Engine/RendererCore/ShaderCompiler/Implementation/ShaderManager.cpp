#include <RendererCore/RendererCorePCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <RendererCore/Shader/ShaderHelper.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

bool WShaderManager::s_bEnableRuntimeCompilation = false;
WString WShaderManager::s_sPlatform;
WString WShaderManager::s_sPermVarSubDir;
WString WShaderManager::s_sShaderCacheDirectory;

namespace
{
  struct PermutationVarConfig
  {
    WHashedString m_sName;
    WVariant m_DefaultValue;
    WDynamicArray<WShaderParser::EnumValue, WStaticsAllocatorWrapper> m_EnumValues;
  };

  static WDeque<PermutationVarConfig, WStaticsAllocatorWrapper> s_PermutationVarConfigsStorage;
  static WHashTable<WHashedString, PermutationVarConfig*> s_PermutationVarConfigs;
  static WMutex s_PermutationVarConfigsMutex;

  const PermutationVarConfig* FindConfig(const char* szName, const WTempHashedString& sHashedName)
  {
    W_LOCK(s_PermutationVarConfigsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sHashedName, pConfig))
    {
      WShaderManager::ReloadPermutationVarConfig(szName, sHashedName);
      s_PermutationVarConfigs.TryGetValue(sHashedName, pConfig);
    }

    return pConfig;
  }

  const PermutationVarConfig* FindConfig(const WHashedString& sName)
  {
    W_LOCK(s_PermutationVarConfigsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sName, pConfig))
    {
      WShaderManager::ReloadPermutationVarConfig(sName.GetData(), sName);
      s_PermutationVarConfigs.TryGetValue(sName, pConfig);
    }

    return pConfig;
  }

  static WHashedString s_sTrue = WMakeHashedString("TRUE");
  static WHashedString s_sFalse = WMakeHashedString("FALSE");

  bool IsValueAllowed(const PermutationVarConfig& config, const WTempHashedString& sValue, WHashedString& out_sValue)
  {
    if (config.m_DefaultValue.IsA<bool>())
    {
      if (sValue == s_sTrue)
      {
        out_sValue = s_sTrue;
        return true;
      }

      if (sValue == s_sFalse)
      {
        out_sValue = s_sFalse;
        return true;
      }
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
      {
        if (enumValue.m_sValueName == sValue)
        {
          out_sValue = enumValue.m_sValueName;
          return true;
        }
      }
    }

    return false;
  }

  bool IsValueAllowed(const PermutationVarConfig& config, const WTempHashedString& sValue)
  {
    if (config.m_DefaultValue.IsA<bool>())
    {
      return sValue == s_sTrue || sValue == s_sFalse;
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
      {
        if (enumValue.m_sValueName == sValue)
          return true;
      }
    }

    return false;
  }

  static WHashTable<WUInt64, WUntrackedString> s_PermutationPaths;
} // namespace

//////////////////////////////////////////////////////////////////////////

void WShaderManager::Configure(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory, const char* szPermVarSubDirectory)
{
  WStringBuilder s = szActivePlatform;
  s.ToUpper();
  s_sPlatform = s;
  s_bEnableRuntimeCompilation = bEnableRuntimeCompilation;
  s_sShaderCacheDirectory = szShaderCacheDirectory;
  s_sPermVarSubDir = szPermVarSubDirectory;
}

void WShaderManager::ReloadPermutationVarConfig(const char* szName, const WTempHashedString& sHashedName)
{
  // clear earlier data
  {
    W_LOCK(s_PermutationVarConfigsMutex);

    s_PermutationVarConfigs.Remove(sHashedName);
  }

  WStringBuilder sPath;
  sPath.SetFormat("{0}/{1}.WPermVar", s_sPermVarSubDir, szName);

  WStringBuilder sTemp = s_sPlatform;
  sTemp.Append(" 1");

  WPreprocessor pp;
  pp.SetLogInterface(WLog::GetThreadLocalLogSystem());
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);
  pp.AddCustomDefine(sTemp.GetData()).IgnoreResult();

  if (pp.Process(sPath, sTemp, false).Failed())
  {
    WLog::Error("Could not read shader permutation variable '{0}' from file '{1}'", szName, sPath);
  }

  WVariant defaultValue;
  WShaderParser::EnumDefinition enumDef;

  WShaderParser::ParsePermutationVarConfig(sTemp, defaultValue, enumDef);
  if (defaultValue.IsValid())
  {
    W_LOCK(s_PermutationVarConfigsMutex);

    auto pConfig = &s_PermutationVarConfigsStorage.ExpandAndGetRef();
    pConfig->m_sName.Assign(szName);
    pConfig->m_DefaultValue = defaultValue;
    pConfig->m_EnumValues = enumDef.m_Values;

    s_PermutationVarConfigs.Insert(pConfig->m_sName, pConfig);
  }
}

bool WShaderManager::IsPermutationValueAllowed(const char* szName, const WTempHashedString& sHashedName, const WTempHashedString& sValue, WHashedString& out_sName, WHashedString& out_sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(szName, sHashedName);
  if (pConfig == nullptr)
  {
    WLog::Error("Permutation variable '{0}' does not exist", szName);
    return false;
  }

  out_sName = pConfig->m_sName;

  if (!IsValueAllowed(*pConfig, sValue, out_sValue))
  {
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }

    WLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", szName, sValue.GetHash());
    ReloadPermutationVarConfig(szName, sHashedName);

    if (!IsValueAllowed(*pConfig, sValue, out_sValue))
    {
      WLog::Error("Invalid Shader Permutation: '{0}' cannot be set to value '{1}'", szName, sValue.GetHash());
      return false;
    }
  }

  return true;
}

bool WShaderManager::IsPermutationValueAllowed(const WHashedString& sName, const WHashedString& sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
  {
    WLog::Error("Permutation variable '{0}' does not exist", sName);
    return false;
  }

  if (!IsValueAllowed(*pConfig, sValue))
  {
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }

    WLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", sName, sValue);
    ReloadPermutationVarConfig(sName, sName);

    if (!IsValueAllowed(*pConfig, sValue))
    {
      WLog::Error("Invalid Shader Permutation: '{0}' cannot be set to value '{1}'", sName, sValue);
      return false;
    }
  }

  return true;
}

void WShaderManager::GetPermutationValues(const WHashedString& sName, WDynamicArray<WHashedString>& out_values)
{
  out_values.Clear();

  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
    return;

  if (pConfig->m_DefaultValue.IsA<bool>())
  {
    out_values.PushBack(s_sTrue);
    out_values.PushBack(s_sFalse);
  }
  else
  {
    for (const auto& val : pConfig->m_EnumValues)
    {
      out_values.PushBack(val.m_sValueName);
    }
  }
}

WArrayPtr<const WShaderParser::EnumValue> WShaderManager::GetPermutationEnumValues(const WHashedString& sName)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig != nullptr)
  {
    return pConfig->m_EnumValues;
  }

  return {};
}

void WShaderManager::PreloadPermutations(WShaderResourceHandle hShader, const WHashTable<WHashedString, WHashedString>& permVars, WTime shouldBeAvailableIn)
{
  W_ASSERT_NOT_IMPLEMENTED;
#if 0
  WResourceLock<WShaderResource> pShader(hShader, WResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return;

  /*WUInt32 uiPermutationHash = */ FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars);

  generator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

  WTempHybridArray<WPermutationVar, 16> usedPermVars;

  const WUInt32 uiPermutationCount = generator.GetPermutationCount();
  for (WUInt32 uiPermutation = 0; uiPermutation < uiPermutationCount; ++uiPermutation)
  {
    generator.GetPermutation(uiPermutation, usedPermVars);

    PreloadSingleShaderPermutation(hShader, usedPermVars, tShouldBeAvailableIn);
  }
#endif
}

WShaderPermutationResourceHandle WShaderManager::PreloadSinglePermutation(WShaderResourceHandle hShader, const WHashTable<WHashedString, WHashedString>& permVars, bool bAllowFallback)
{
  WResourceLock<WShaderResource> pShader(hShader, bAllowFallback ? WResourceAcquireMode::AllowLoadingFallback : WResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return WShaderPermutationResourceHandle();

  WTempHybridArray<WPermutationVar, 64> filteredPermutationVariables;
  WUInt32 uiPermutationHash = FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars, filteredPermutationVariables);

  return PreloadSinglePermutationInternal(pShader->GetResourceID(), pShader->GetResourceIDHash(), uiPermutationHash, filteredPermutationVariables);
}


WUInt32 WShaderManager::FilterPermutationVars(WArrayPtr<const WHashedString> usedVars, const WHashTable<WHashedString, WHashedString>& permVars, WDynamicArray<WPermutationVar>& out_FilteredPermutationVariables)
{
  for (auto& sName : usedVars)
  {
    auto& var = out_FilteredPermutationVariables.ExpandAndGetRef();
    var.m_sName = sName;

    if (!permVars.TryGetValue(sName, var.m_sValue))
    {
      const PermutationVarConfig* pConfig = FindConfig(sName);
      if (pConfig == nullptr)
        continue;

      const WVariant& defaultValue = pConfig->m_DefaultValue;
      if (defaultValue.IsA<bool>())
      {
        var.m_sValue = defaultValue.Get<bool>() ? s_sTrue : s_sFalse;
      }
      else
      {
        WUInt32 uiDefaultValue = defaultValue.Get<WUInt32>();
        var.m_sValue = pConfig->m_EnumValues[uiDefaultValue].m_sValueName;
      }
    }
  }

  return WShaderHelper::CalculateHash(out_FilteredPermutationVariables);
}



WShaderPermutationResourceHandle WShaderManager::PreloadSinglePermutationInternal(WStringView sResourceId, WUInt64 uiResourceIdHash, WUInt32 uiPermutationHash, WArrayPtr<WPermutationVar> filteredPermutationVariables)
{
  const WUInt64 uiPermutationKey = (WUInt64)WHashingUtils::StringHashTo32(uiResourceIdHash) << 32 | uiPermutationHash;

  WUntrackedString& permutationPath = s_PermutationPaths[uiPermutationKey];
  if (permutationPath.IsEmpty())
  {
    WStringBuilder sShaderFile = GetCacheDirectory();
    sShaderFile.AppendPath(GetActivePlatform().GetData());
    sShaderFile.AppendPath(sResourceId);
    sShaderFile.ChangeFileExtension("");
    if (sShaderFile.EndsWith("."))
      sShaderFile.Shrink(0, 1);
    sShaderFile.AppendFormat("_{0}.WPermutation", WArgU(uiPermutationHash, 8, true, 16, true));

    permutationPath = sShaderFile;
  }

  WShaderPermutationResourceHandle hShaderPermutation = WResourceManager::LoadResource<WShaderPermutationResource>(permutationPath);

  {
    WResourceLock<WShaderPermutationResource> pShaderPermutation(hShaderPermutation, WResourceAcquireMode::PointerOnly);
    if (!pShaderPermutation->IsShaderValid())
    {
      pShaderPermutation->m_PermutationVars = filteredPermutationVariables;
    }
  }

  WResourceManager::PreloadResource(hShaderPermutation);

  return hShaderPermutation;
}
