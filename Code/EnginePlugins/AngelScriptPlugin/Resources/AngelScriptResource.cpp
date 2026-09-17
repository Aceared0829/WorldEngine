#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Resources/AngelScriptResource.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsFunctionDispatch.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/Component.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAngelScriptResource, 1, WRTTIDefaultAllocator<WAngelScriptResource>)
W_END_DYNAMIC_REFLECTED_TYPE;
W_RESOURCE_IMPLEMENT_COMMON_CODE(WAngelScriptResource);

W_BEGIN_SUBSYSTEM_DECLARATION(AngelScript, AngelScriptResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    WResourceManager::RegisterResourceForAssetType("AngelScriptClass", WGetStaticRTTI<WAngelScriptResource>());
    WResourceManager::RegisterResourceOverrideType(WGetStaticRTTI<WAngelScriptResource>(), [](const WStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".WBinAngelScript");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WResourceManager::UnregisterResourceOverrideType(WGetStaticRTTI<WAngelScriptResource>());
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WAngelScriptResource::WAngelScriptResource() = default;
WAngelScriptResource::~WAngelScriptResource() = default;

WResourceLoadDesc WAngelScriptResource::UnloadData(Unload WhatToUnload)
{
  DeleteScriptType();
  DeleteAllScriptCoroutineTypes();

  if (m_pModule)
  {
    // can't do this here, because other worlds may still use the current state
    // need to track somehow where modules are still in use
    // m_pModule->Discard();
    m_pModule = nullptr;
  }

  WResourceLoadDesc ld;
  ld.m_State = WResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  return ld;
}

WResourceLoadDesc WAngelScriptResource::UpdateContent(WStreamReader* pStream)
{
  WResourceLoadDesc ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;
  ld.m_State = WResourceState::LoadedResourceMissing;

  if (pStream == nullptr)
  {
    return ld;
  }

  // the standard file reader writes the absolute file path into the stream
  WString sAbsFilePath;
  (*pStream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  WAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  WUInt8 uiVersion = 1;
  (*pStream) >> uiVersion;

  WUInt8 uiCompressionMode = 0;

  if (uiVersion >= 4)
  {
    (*pStream) >> uiCompressionMode;
  }

  WStreamReader* pDeCompressor = pStream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  WCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(pStream);
      pDeCompressor = &decompressorZstd;
      break;
#else
      WLog::Error("AngelScript is compressed with zstandard, but support for this compressor is not compiled in.");
      ld.m_State = WResourceState::LoadedResourceMissing;
      return res;
#endif

    default:
      WLog::Error("AngelScript is compressed with an unknown algorithm.");
      ld.m_State = WResourceState::LoadedResourceMissing;
      return ld;
  }

  WStreamReader& stream = *pDeCompressor;
  stream >> m_sClassName;

  WStringBuilder sModuleID;
  sModuleID.SetFormat("{}-{}", GetResourceID(), GetCurrentResourceChangeCounter());

  auto pAs = WAngelScriptEngineSingleton::GetSingleton();

  if (uiVersion == 1)
  {
    stream >> m_sScriptContent;
    m_pModule = pAs->CompileModule(sModuleID, m_sClassName, "main", m_sScriptContent, nullptr, nullptr);
  }
  else
  {
    WTempHybridArray<WUInt8, 1024 * 8> bytecode;
    stream.ReadArray(bytecode).AssertSuccess();

    if (uiVersion >= 3)
    {
      stream >> m_sScriptContent;
      m_pModule = pAs->CompileModule(sModuleID, m_sClassName, "main", m_sScriptContent, nullptr, nullptr);
    }
    else
    {
      m_pModule = WAngelScriptUtils::LoadFromByteCode(pAs->GetEngine(), sModuleID, bytecode);
    }
  }

  if (m_pModule == nullptr)
  {
    return ld;
  }

  const asITypeInfo* pClassType = m_pModule->GetTypeInfoByDecl(m_sClassName);

  WScriptRTTI::FunctionList functions;
  WScriptRTTI::MessageHandlerList messageHandlers;

  const WRTTI* pBaseType = WGetStaticRTTI<WComponent>();

  WStringBuilder sFunctionName;

  WTempHybridArray<WString, 16> funcNames;

  for (auto pCompFunc : pBaseType->GetFunctions())
  {
    const WScriptBaseClassFunctionAttribute* pAttr = pCompFunc->GetAttributeByType<WScriptBaseClassFunctionAttribute>();
    if (pAttr == nullptr)
      continue;

    sFunctionName = pCompFunc->GetPropertyName();
    sFunctionName.TrimWordStart("Reflection_");

    funcNames.PushBack(sFunctionName);

    if (auto pFunc = pClassType->GetMethodByName(sFunctionName))
    {
      WUniquePtr<WAngelScriptFunctionProperty> pFunctionProperty = W_SCRIPT_NEW(WAngelScriptFunctionProperty, sFunctionName, pFunc);
      functions.PushBack(std::move(pFunctionProperty));
    }
  }

  FindMessageHandlers(pClassType, messageHandlers);

  if (functions.IsEmpty() && messageHandlers.IsEmpty())
  {
    WLog::Error("AngelScript code doesn't contain any callable function or message handlers.");
    WLog::Info("Candidates are:");

    for (const auto& s : funcNames)
    {
      WLog::Info("  {}", s);
    }

    return ld;
  }

  CreateScriptType(GetResourceID(), pBaseType, std::move(functions), std::move(messageHandlers));

  ld.m_State = WResourceState::Loaded;
  return ld;
}

void WAngelScriptResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (WUInt32)sizeof(WAngelScriptResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WUniquePtr<WScriptInstance> WAngelScriptResource::Instantiate(WReflectedClass& inout_owner, WWorld* pWorld) const
{
  return W_SCRIPT_NEW(WAngelScriptInstance, inout_owner, pWorld, m_pModule, m_sClassName);
}

void WAngelScriptResource::FindMessageHandlers(const asITypeInfo* pClassType, WScriptRTTI::MessageHandlerList& inout_Handlers)
{
  auto pAs = WAngelScriptEngineSingleton::GetSingleton();

  WStringBuilder sArgType;

  WUniquePtr<WAngelScriptCustomAsMessageHandler> pAsMsgHandler;

  for (WUInt32 i = 0; i < pClassType->GetMethodCount(); ++i)
  {
    asIScriptFunction* pFunc = pClassType->GetMethodByIndex(i, false);

    if (!WStringUtils::StartsWith(pFunc->GetName(), "OnMsg"))
      continue;

    // only allow void functions
    if (pFunc->GetReturnTypeId() != asTYPEID_VOID)
    {
      WLog::Error("Malformed message handler '{}': Return type must be 'void'.", pFunc->GetName());
      continue;
    }

    // that take exactly one argument
    if (pFunc->GetParamCount() != 1)
    {
      WLog::Error("Malformed message handler '{}': Must take exactly one argument.", pFunc->GetName());
      continue;
    }

    int iArgTypeId;
    pFunc->GetParam(0, &iArgTypeId);

    if (iArgTypeId & asTYPEID_APPOBJECT)
    {
      const WRTTI* pArgType = WAngelScriptUtils::MapToRTTI(iArgTypeId, pAs->GetEngine());

      if (pArgType == nullptr)
      {
        WLog::Error("Malformed message handler '{}': Argument has unknown type.", pFunc->GetName());
        continue;
      }

      // has to be a type derived from WMessage
      if (!pArgType->IsDerivedFrom<WMessage>())
      {
        WLog::Error("Malformed message handler '{}': Argument type has to derive from WMessage or WAngelScriptMessage.", pFunc->GetName());
        continue;
      }

      WScriptMessageDesc desc;
      desc.m_pType = pArgType;

      WUniquePtr<WAngelScriptMessageHandler> pFunctionProperty = W_SCRIPT_NEW(WAngelScriptMessageHandler, desc, pFunc);
      inout_Handlers.PushBack(std::move(pFunctionProperty));
    }

    if (iArgTypeId & asTYPEID_SCRIPTOBJECT)
    {
      if (auto pArgType = pAs->GetEngine()->GetTypeInfoById(iArgTypeId))
      {
        if (pArgType->GetInterfaceCount() != 1 || !WStringUtils::IsEqual(pArgType->GetInterface(0)->GetName(), "WAngelScriptMessage"))
        {
          WLog::Error("Malformed message handler '{}': Argument type has to derive from WMessage or WAngelScriptMessage.", pFunc->GetName());
          continue;
        }

        if (!pAsMsgHandler)
        {
          WScriptMessageDesc desc;
          desc.m_pType = WGetStaticRTTI<WMsgDeliverAngelScriptMsg>();

          pAsMsgHandler = W_SCRIPT_NEW(WAngelScriptCustomAsMessageHandler, desc);
        }

        pAsMsgHandler->AddReceiver(pFunc, pArgType->GetName());
      }
    }
  }

  if (pAsMsgHandler)
  {
    inout_Handlers.PushBack(std::move(pAsMsgHandler));
  }
}


W_STATICLINK_FILE(AngelScriptPlugin, AngelScriptPlugin_Resources_AngelScriptResource);
