#include <RendererCore/RendererCorePCH.h>

#include <Core/Interfaces/RemoteToolingInterface.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WShaderProgramCompiler, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  static bool PlatformEnabled(const WString& sPlatforms, const char* szPlatform)
  {
    WStringBuilder sTemp;
    sTemp = szPlatform;

    sTemp.Prepend("!");

    // if it contains '!platform'
    if (sPlatforms.FindWholeWord_NoCase(sTemp, WStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return false;

    sTemp = szPlatform;

    // if it contains 'platform'
    if (sPlatforms.FindWholeWord_NoCase(sTemp, WStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    // do not enable this when ALL is specified
    if (WStringUtils::IsEqual(szPlatform, "DEBUG"))
      return false;

    // if it contains 'ALL'
    if (sPlatforms.FindWholeWord_NoCase("ALL", WStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    return false;
  }

  static void GenerateDefines(const char* szPlatform, const WArrayPtr<WPermutationVar>& permutationVars, WDynamicArray<WString>& out_defines)
  {
    WStringBuilder sTemp;

    if (out_defines.IsEmpty())
    {
      out_defines.PushBack("TRUE 1");
      out_defines.PushBack("FALSE 0");

      sTemp = szPlatform;
      sTemp.ToUpper();

      out_defines.PushBack(sTemp);
    }

    for (const WPermutationVar& var : permutationVars)
    {
      const char* szValue = var.m_sValue;
      const bool isBoolVar = WStringUtils::IsEqual(szValue, "TRUE") || WStringUtils::IsEqual(szValue, "FALSE");

      if (isBoolVar)
      {
        sTemp.Set(var.m_sName, " ", var.m_sValue);
        out_defines.PushBack(sTemp);
      }
      else
      {
        const char* szName = var.m_sName;
        auto enumValues = WShaderManager::GetPermutationEnumValues(var.m_sName);

        for (const auto& ev : enumValues)
        {
          sTemp.SetFormat("{1} {2}", szName, ev.m_sValueName, ev.m_iValueValue);
          out_defines.PushBack(sTemp);
        }

        if (WStringUtils::StartsWith(szValue, szName))
        {
          sTemp.Set(szName, " ", szValue);
        }
        else
        {
          sTemp.Set(szName, " ", szName, "_", szValue);
        }
        out_defines.PushBack(sTemp);
      }
    }
  }

  static const char* s_szStageDefines[WGALShaderStage::ENUM_COUNT] = {"VERTEX_SHADER", "HULL_SHADER", "DOMAIN_SHADER", "GEOMETRY_SHADER", "PIXEL_SHADER", "COMPUTE_SHADER"};
} // namespace

WResult WShaderCompiler::FileOpen(WStringView sAbsoluteFile, WDynamicArray<WUInt8>& FileContent, WTimestamp& out_FileModification)
{
  W_PROFILE_SCOPE("WShaderCompiler::FileOpen");

  if (sAbsoluteFile == "ShaderRenderState")
  {
    const WString& sData = m_ShaderData.m_StateSource;
    const WUInt32 uiCount = sData.GetElementCount();
    WStringView sString = sData;

    FileContent.SetCountUninitialized(uiCount);

    if (uiCount > 0)
    {
      WMemoryUtils::Copy<WUInt8>(FileContent.GetData(), (const WUInt8*)sString.GetStartPointer(), uiCount);
    }

    return W_SUCCESS;
  }

  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_StageSourceFile[stage] == sAbsoluteFile)
    {
      const WString& sData = m_ShaderData.m_ShaderStageSource[stage];
      const WUInt32 uiCount = sData.GetElementCount();
      const char* szString = sData;

      FileContent.SetCountUninitialized(uiCount);

      if (uiCount > 0)
      {
        WMemoryUtils::Copy<WUInt8>(FileContent.GetData(), (const WUInt8*)szString, uiCount);
      }

      return W_SUCCESS;
    }
  }

  m_IncludeFiles.Insert(sAbsoluteFile);

  WFileReader r;
  if (r.Open(sAbsoluteFile).Failed())
  {
    WLog::Error("Could not find include file '{0}'", sAbsoluteFile);
    return W_FAILURE;
  }

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  WFileStats stats;
  if (WFileSystem::GetFileStats(sAbsoluteFile, stats).Succeeded())
  {
    out_FileModification = stats.m_LastModificationTime;
  }
#endif

  WUInt8 Temp[4096];

  while (WUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    FileContent.PushBackRange(WArrayPtr<WUInt8>(Temp, (WUInt32)uiRead));
  }

  return W_SUCCESS;
}

void WShaderCompiler::ShaderCompileMsg(WRemoteMessage& msg)
{
  if (msg.GetMessageID() == 'CRES')
  {
    m_bCompilingShaderRemote = false;
    m_RemoteShaderCompileResult = W_SUCCESS;

    bool success = false;
    msg.GetReader() >> success;
    m_RemoteShaderCompileResult = success ? W_SUCCESS : W_FAILURE;

    WStringBuilder log;
    msg.GetReader() >> log;

    if (!success)
    {
      WLog::Error("Shader compilation failed:\n{}", log);
    }
  }
}

WResult WShaderCompiler::CompileShaderPermutationForPlatforms(WStringView sFile, const WArrayPtr<const WPermutationVar>& permutationVars, WLogInterface* pLog, WStringView sPlatform, WTokenizedFileCache* pFileCache)
{
  W_PROFILE_SCOPE("WShaderCompiler::CompileShaderPermutationForPlatforms");

  if (WRemoteToolingInterface* pTooling = WSingletonRegistry::GetSingletonInstance<WRemoteToolingInterface>())
  {
    auto pNet = pTooling->GetRemoteInterface();

    if (pNet && pNet->IsConnectedToServer())
    {
      m_bCompilingShaderRemote = true;

      pNet->SetMessageHandler('SHDR', WMakeDelegate(&WShaderCompiler::ShaderCompileMsg, this));

      WRemoteMessage msg('SHDR', 'CMPL');
      msg.GetWriter() << sFile;
      msg.GetWriter() << sPlatform;
      msg.GetWriter() << permutationVars.GetCount();
      for (auto& pv : permutationVars)
      {
        msg.GetWriter() << pv.m_sName;
        msg.GetWriter() << pv.m_sValue;
      }

      pNet->Send(WRemoteTransmitMode::Reliable, msg);

      while (m_bCompilingShaderRemote)
      {
        pNet->UpdateRemoteInterface();
        pNet->ExecuteAllMessageHandlers();
      }

      pNet->SetMessageHandler('SHDR', {});

      return m_RemoteShaderCompileResult;
    }
  }

  WStringBuilder sFileContent, sTemp;

  {
    WFileReader File;
    if (File.Open(sFile).Failed())
      return W_FAILURE;

    sFileContent.ReadAll(File);
  }

  WShaderHelper::WTextSectionizer Sections;
  WShaderHelper::GetShaderSections(sFileContent, Sections);

  WUInt32 uiFirstLine = 0;
  sTemp = Sections.GetSectionContent(WShaderHelper::WShaderSections::PLATFORMS, uiFirstLine);
  sTemp.ToUpper();

  m_ShaderData.m_Platforms = sTemp;

  WTempHybridArray<WHashedString, 16> usedPermutations;
  WShaderParser::ParsePermutationSection(Sections.GetSectionContent(WShaderHelper::WShaderSections::PERMUTATIONS, uiFirstLine), usedPermutations, m_ShaderData.m_FixedPermVars);

  for (const WHashedString& usedPermutationVar : usedPermutations)
  {
    WUInt32 uiIndex = WInvalidIndex;
    for (WUInt32 i = 0; i < permutationVars.GetCount(); ++i)
    {
      if (permutationVars[i].m_sName == usedPermutationVar)
      {
        uiIndex = i;
        break;
      }
    }

    if (uiIndex != WInvalidIndex)
    {
      m_ShaderData.m_Permutations.PushBack(permutationVars[uiIndex]);
    }
    else
    {
      WLog::Error("No value given for permutation var '{0}'. Assuming default value of zero.", usedPermutationVar);

      WPermutationVar& finalVar = m_ShaderData.m_Permutations.ExpandAndGetRef();
      finalVar.m_sName = usedPermutationVar;
      finalVar.m_sValue.Assign("0");
    }
  }

  m_ShaderData.m_StateSource = Sections.GetSectionContent(WShaderHelper::WShaderSections::RENDERSTATE, uiFirstLine);

  WUInt32 uiFirstShaderLine = 0;
  WStringView sShaderSource = Sections.GetSectionContent(WShaderHelper::WShaderSections::SHADER, uiFirstShaderLine);

  WUInt32 uiFirstMaterialConstantsLine = 0;
  WStringView sMaterialConstantsSource = Sections.GetSectionContent(WShaderHelper::WShaderSections::MATERIALCONSTANTS, uiFirstMaterialConstantsLine);

  WStringView sMaterialParametersSection = Sections.GetSectionContent(WShaderHelper::WShaderSections::MATERIALPARAMETER, uiFirstLine);

  // Gather material parameters to force these into the material bind group to make migration of other shaders easier.
  m_MaterialParameters.Clear();
  WTempHybridArray<WShaderParser::ParameterDefinition, 16> parameters;
  WTempHybridArray<WShaderParser::EnumDefinition, 4> enumDefinitions;
  WShaderParser::ParseMaterialParameterSection(sMaterialParametersSection, parameters, enumDefinitions);
  for (WUInt32 i = 0; i < parameters.GetCount(); ++i)
  {
    const WShaderParser::ParameterDefinition& param = parameters[i];
    if (param.m_sType == "Texture2D" || param.m_sType == "TextureCube" || param.m_sType == "Texture3D")
    {
      m_MaterialParameters.Insert(param.m_sName);
    }
  }

  WStringBuilder sMaterialConstantsTemplate;
  // If this is a material shader (i.e. it has a [MATERIALCONSTANTS] section), we need to parse the section and also load the MaterialConstants.template file which will be used to generate the material constants struct which is prepended before every shader and defines the HAS_MATERIAL_CONSTANTS define.
  if (!sMaterialConstantsSource.IsEmpty())
  {
    m_pMaterialBufferLayout = W_DEFAULT_NEW(WShaderConstantBufferLayout);
    WStatus res = WShaderParser::ParseMaterialConstantsSection(sMaterialConstantsSource, m_pMaterialBufferLayout);
    if (res.LogFailure())
      return W_FAILURE;

    WStringView sMaterialConstantsTemplateFile = "Shaders/Materials/MaterialConstants.template";
    WFileReader materialConstantsTemplate;
    if (materialConstantsTemplate.Open(sMaterialConstantsTemplateFile).Failed())
    {
      WLog::Error(pLog, "Failed to load the '{}' file. Can't compile material shader", sMaterialConstantsTemplateFile);
      return W_FAILURE;
    }
    sMaterialConstantsTemplate.ReadAll(materialConstantsTemplate);
    m_IncludeFiles.Insert(sMaterialConstantsTemplateFile);
  }

  WStringView extensions[]{"vs", "hs", "ds", "gs", "ps", "cs"};
  static_assert(W_ARRAY_SIZE(extensions) == WGALShaderStage::ENUM_COUNT);
  WStringBuilder tmp = sFile;
  tmp.MakeCleanPath();

  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    m_StageSourceFile[stage] = tmp;
    m_StageSourceFile[stage].ChangeFileExtension(extensions[stage]);

    WStringView sStageSource = Sections.GetSectionContent(WShaderHelper::WShaderSections::VERTEXSHADER + stage, uiFirstLine);

    // later code checks whether the string is empty, to see whether we have any shader source, so this has to be kept empty
    if (!sStageSource.IsEmpty())
    {
      sTemp.Clear();

      // prepend material constants section if there is any
      if (!sMaterialConstantsSource.IsEmpty())
      {
        // We need to fill teh template not only with the section content but also the starting line and filename to get correct line numbers for all compile failures.
        sTemp.AppendFormat(sMaterialConstantsTemplate, uiFirstMaterialConstantsLine, m_StageSourceFile[stage], sMaterialConstantsSource);
        if (!sTemp.EndsWith_NoCase("\n"))
          sTemp.Append("\n");
      }

      // prepend common shader section if there is any
      if (!sShaderSource.IsEmpty())
      {
        sTemp.AppendFormat("#line {0}\n{1}", uiFirstShaderLine, sShaderSource);
        if (!sTemp.EndsWith_NoCase("\n"))
          sTemp.Append("\n");
      }

      sTemp.AppendFormat("#line {0}\n{1}", uiFirstLine, sStageSource);

      m_ShaderData.m_ShaderStageSource[stage] = sTemp;
    }
    else
    {
      m_ShaderData.m_ShaderStageSource[stage].Clear();
    }
  }

  // try out every compiler that we can find
  WTempHybridArray<const WRTTI*, 2> compilers;
  WRTTI::ForEachDerivedType<WShaderProgramCompiler>(
    [&](const WRTTI* pRtti)
    {
      compilers.PushBack(pRtti);
    },
    WRTTI::ForEachOptions::ExcludeNonAllocatable);

  WResult result = W_SUCCESS;
  for (auto pCompilerRtti : compilers)
  {
    WUniquePtr<WShaderProgramCompiler> pCompiler = pCompilerRtti->GetAllocator()->Allocate<WShaderProgramCompiler>();

    if (RunShaderCompiler(sFile, sPlatform, pCompiler.Borrow(), pLog, pFileCache).Failed())
      result = W_FAILURE;
  }
  return result;
}

WResult WShaderCompiler::RunShaderCompiler(WStringView sFile, WStringView sPlatform, WShaderProgramCompiler* pCompiler, WLogInterface* pLog, WTokenizedFileCache* pFileCache)
{
  W_PROFILE_SCOPE("WShaderCompiler::RunShaderCompiler");
  W_LOG_BLOCK(pLog, "Compiling Shader", sFile);

  WStringBuilder sProcessed[WGALShaderStage::ENUM_COUNT];

  WTempHybridArray<WString, 4> Platforms;
  pCompiler->GetSupportedPlatforms(Platforms);
  if (m_pMaterialBufferLayout)
  {
    WShaderParser::LayoutMaterialConstants(*m_pMaterialBufferLayout, pCompiler->GetMaterialBufferLayout(sPlatform));
  }

  for (WUInt32 p = 0; p < Platforms.GetCount(); ++p)
  {
    if (!PlatformEnabled(sPlatform, Platforms[p]))
      continue;

    // if this shader is not tagged for this platform, ignore it
    if (!PlatformEnabled(m_ShaderData.m_Platforms, Platforms[p]))
      continue;

    W_LOG_BLOCK(pLog, "Platform", Platforms[p]);

    WShaderProgramData spd;
    spd.m_sSourceFile = sFile;
    spd.m_sPlatform = Platforms[p];
    spd.m_MaterialParameters = m_MaterialParameters;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    // 'DEBUG' is a platform tag that enables additional compiler flags
    if (PlatformEnabled(m_ShaderData.m_Platforms, "DEBUG"))
    {
      WLog::Warning(pLog, "Shader specifies the 'DEBUG' platform, which enables the debug shader compiler flag.");
      spd.m_Flags.Add(WShaderCompilerFlags::Debug);
    }
#endif

    m_IncludeFiles.Clear();

    WTempHybridArray<WString, 32> defines;
    GenerateDefines(Platforms[p], m_ShaderData.m_Permutations, defines);
    GenerateDefines(Platforms[p], m_ShaderData.m_FixedPermVars, defines);

    WShaderPermutationBinary shaderPermutationBinary;

    // Generate Shader State Source
    {
      W_LOG_BLOCK(pLog, "Preprocessing Shader State Source");

      WPreprocessor pp;
      pp.SetCustomFileCache(pFileCache != nullptr ? pFileCache : &m_FileCache);
      pp.SetLogInterface(WLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(WPreprocessor::FileOpenCB(&WShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(false);
      pp.SetPassThroughLine(false);

      for (auto& define : defines)
      {
        W_SUCCEED_OR_RETURN(pp.AddCustomDefine(define));
      }

      bool bFoundUndefinedVars = false;
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars, pLog](const WPreprocessor::ProcessingEvent& e)
        {
        if (e.m_Type == WPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          WLog::Error(pLog, "Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        } });

      WStringBuilder sOutput;
      if (pp.Process("ShaderRenderState", sOutput, false).Failed() || bFoundUndefinedVars)
      {
        WLog::Error(pLog, "Preprocessing the Shader State block failed");
        return W_FAILURE;
      }
      else
      {
        if (shaderPermutationBinary.m_StateDescriptor.Parse(sOutput).Failed())
        {
          WLog::Error(pLog, "Failed to interpret the shader state block");
          return W_FAILURE;
        }
      }
    }

    // Shader Preprocessing
    for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
    {
      spd.m_uiSourceHash[stage] = 0;

      if (m_ShaderData.m_ShaderStageSource[stage].IsEmpty())
        continue;

      bool bFoundUndefinedVars = false;

      WPreprocessor pp;
      pp.SetCustomFileCache(pFileCache != nullptr ? pFileCache : &m_FileCache);
      pp.SetLogInterface(WLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(WPreprocessor::FileOpenCB(&WShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(true);
      pp.SetPassThroughUnknownCmdsCB(WMakeDelegate(&WShaderCompiler::PassThroughUnknownCommandCB, this));
      pp.SetPassThroughLine(false);
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars, pLog](const WPreprocessor::ProcessingEvent& e)
        {
        if (e.m_Type == WPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          WLog::Error(pLog, "Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        } });

      W_SUCCEED_OR_RETURN(pp.AddCustomDefine(s_szStageDefines[stage]));
      for (auto& define : defines)
      {
        W_SUCCEED_OR_RETURN(pp.AddCustomDefine(define));
      }

      if (pp.Process(m_StageSourceFile[stage], sProcessed[stage], true, true, true).Failed() || bFoundUndefinedVars)
      {
        sProcessed[stage].Clear();
        spd.m_sShaderSource[stage] = m_StageSourceFile[stage];

        WLog::Error(pLog, "Shader preprocessing failed");
        return W_FAILURE;
      }
      else
      {
        spd.m_sShaderSource[stage] = sProcessed[stage];
      }
    }

    // Let the shader compiler make any modifications to the source code before we hash and compile the shader.
    if (pCompiler->ModifyShaderSource(spd, pLog).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return W_FAILURE;
    }

    // Load shader cache
    for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
    {
      WUInt32 uiSourceStringLen = spd.m_sShaderSource[stage].GetElementCount();
      spd.m_uiSourceHash[stage] = uiSourceStringLen == 0 ? 0u : WHashingUtils::xxHash32(spd.m_sShaderSource[stage].GetData(), uiSourceStringLen);

      if (spd.m_uiSourceHash[stage] != 0)
      {
        WShaderStageBinary* pBinary = WShaderStageBinary::LoadStageBinary((WGALShaderStage::Enum)stage, spd.m_uiSourceHash[stage], sPlatform);

        if (pBinary)
        {
          spd.m_ByteCode[stage] = pBinary->m_pGALByteCode;
          spd.m_bWriteToDisk[stage] = false;
        }
        else
        {
          // Can't find shader with given hash on disk, create a new WGALShaderByteCode and let the compiler build it.
          spd.m_ByteCode[stage] = W_DEFAULT_NEW(WGALShaderByteCode);
          spd.m_ByteCode[stage]->m_Stage = (WGALShaderStage::Enum)stage;
          spd.m_ByteCode[stage]->m_bWasCompiledWithDebug = spd.m_Flags.IsSet(WShaderCompilerFlags::Debug);
        }
      }
    }

    // copy the source hashes
    for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
    {
      shaderPermutationBinary.m_uiShaderStageHashes[stage] = spd.m_uiSourceHash[stage];
    }

    // if compilation failed, the stage binary for the source hash will simply not exist and therefore cannot be loaded
    // the .WPermutation file should be updated, however, to store the new source hash to the broken shader
    if (pCompiler->Compile(spd, WLog::GetThreadLocalLogSystem()).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return W_FAILURE;
    }

    WTempHashedString sMaterialConstants("WMaterialConstants");
    WTempHashedString sMaterialData("materialData");
    for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
    {
      if (!spd.m_ByteCode[stage])
        continue;

      for (const WShaderResourceBinding& binding : spd.m_ByteCode[stage]->m_ShaderResourceBindings)
      {
        if (binding.m_sName == sMaterialConstants)
        {
          if (sFile.EndsWith(".autogen.WShader"))
          {
            WLog::Error(pLog, "Compiled {} references a WMaterialConstants buffer in the reflection. As this is a Visual Shader, please re-transform your material asset. File: {}", WGALShaderStage::Names[stage], sFile);
          }
          else
          {
            WLog::Error(pLog, "Compiled {} references a WMaterialConstants buffer in the reflection. Please port your material shader to the new [MATERIALCONSTANTS] section. File: {}", WGALShaderStage::Names[stage], sFile);
          }

          return W_FAILURE;
        }

        if (binding.m_sName != sMaterialData || !m_pMaterialBufferLayout)
          continue;

        if (binding.m_pLayout && *binding.m_pLayout != *m_pMaterialBufferLayout)
        {
          WLog::Error(pLog, "Compiled {}'s layout of WMaterialConstants struct differs from the parsed result via WShaderParser::ParseMaterialConstantsSection / LayoutMaterialConstants. Either ifdefs where used in the [MATERIALCONSTANTS], unsupported macros where used or one of the functions is bugged. File: {}", WGALShaderStage::Names[stage], sFile);
          return W_FAILURE;
        }
      }
    }

    for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
    {
      if (spd.m_uiSourceHash[stage] != 0 && spd.m_bWriteToDisk[stage])
      {
        WShaderStageBinary bin;
        bin.m_uiSourceHash = spd.m_uiSourceHash[stage];
        bin.m_pGALByteCode = spd.m_ByteCode[stage];

        if (bin.WriteStageBinary(pLog, sPlatform).Failed())
        {
          WLog::Error(pLog, "Writing stage {0} binary failed", stage);
          return W_FAILURE;
        }
        WShaderStageBinary::s_ShaderStageBinaries[stage].Insert(bin.m_uiSourceHash, bin);
      }
    }

    WStringBuilder sTemp = WShaderManager::GetCacheDirectory();
    sTemp.AppendPath(Platforms[p]);
    sTemp.AppendPath(sFile);
    sTemp.ChangeFileExtension("");
    if (sTemp.EndsWith("."))
      sTemp.Shrink(0, 1);

    const WUInt32 uiPermutationHash = WShaderHelper::CalculateHash(m_ShaderData.m_Permutations);
    sTemp.AppendFormat("_{0}.WPermutation", WArgU(uiPermutationHash, 8, true, 16, true));

    shaderPermutationBinary.m_DependencyFile.Clear();
    shaderPermutationBinary.m_DependencyFile.AddFileDependency(sFile);

    for (auto it = m_IncludeFiles.GetIterator(); it.IsValid(); ++it)
    {
      shaderPermutationBinary.m_DependencyFile.AddFileDependency(it.Key());
    }

    shaderPermutationBinary.m_PermutationVars = m_ShaderData.m_Permutations;

    WDeferredFileWriter PermutationFileOut;
    PermutationFileOut.SetOutput(sTemp);
    W_SUCCEED_OR_RETURN(shaderPermutationBinary.Write(PermutationFileOut));

    if (PermutationFileOut.Close().Failed())
    {
      WLog::Error(pLog, "Could not open file for writing: '{0}'", sTemp);
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}


void WShaderCompiler::WriteFailedShaderSource(WShaderProgramData& spd, WLogInterface* pLog)
{
  W_PROFILE_SCOPE("WShaderCompiler::WriteFailedShaderSource");

  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (spd.m_uiSourceHash[stage] != 0 && spd.m_bWriteToDisk[stage])
    {
      WStringBuilder sShaderStageFile = WShaderManager::GetCacheDirectory();

      sShaderStageFile.AppendPath(WShaderManager::GetActivePlatform());
      sShaderStageFile.AppendFormat("/_Failed_{0}_{1}.WShaderSource", WGALShaderStage::Names[stage], WArgU(spd.m_uiSourceHash[stage], 8, true, 16, true));

      WFileWriter StageFileOut;
      if (StageFileOut.Open(sShaderStageFile).Succeeded())
      {
        StageFileOut.WriteBytes(spd.m_sShaderSource[stage].GetData(), spd.m_sShaderSource[stage].GetElementCount()).AssertSuccess();
        WLog::Info(pLog, "Failed shader source written to '{0}'", sShaderStageFile);
      }
    }
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderCompiler);
