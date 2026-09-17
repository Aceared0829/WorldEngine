#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <VisualScriptPlugin/Resources/VisualScriptClassResource.h>
#include <VisualScriptPlugin/Runtime/VisualScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptFunctionProperty.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualScriptClassResource, 1, WRTTIDefaultAllocator<WVisualScriptClassResource>)
W_END_DYNAMIC_REFLECTED_TYPE;
W_RESOURCE_IMPLEMENT_COMMON_CODE(WVisualScriptClassResource);

W_BEGIN_SUBSYSTEM_DECLARATION(VisualScript, VisualScriptResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    WResourceManager::RegisterResourceForAssetType("VisualScriptClass", WGetStaticRTTI<WVisualScriptClassResource>());
    WResourceManager::RegisterResourceOverrideType(WGetStaticRTTI<WVisualScriptClassResource>(), [](const WStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".WBinVisualScriptClass");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WResourceManager::UnregisterResourceOverrideType(WGetStaticRTTI<WVisualScriptClassResource>());
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WVisualScriptClassResource::WVisualScriptClassResource() = default;
WVisualScriptClassResource::~WVisualScriptClassResource() = default;

WResourceLoadDesc WVisualScriptClassResource::UnloadData(Unload WhatToUnload)
{
  DeleteScriptType();
  DeleteAllScriptCoroutineTypes();

  WResourceLoadDesc ld;
  ld.m_State = WResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  return ld;
}

WResourceLoadDesc WVisualScriptClassResource::UpdateContent(WStreamReader* pStream)
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

  WString sScriptClassName;
  const WRTTI* pBaseClassType = nullptr;
  WScriptRTTI::FunctionList functions;
  WScriptRTTI::MessageHandlerList messageHandlers;
  {
    WStringDeduplicationReadContext stringDedup(*pStream);

    WChunkStreamReader chunk(*pStream);
    chunk.SetEndChunkFileMode(WChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "Header")
      {
        WString sBaseClassName;
        chunk >> sBaseClassName;
        chunk >> sScriptClassName;
        pBaseClassType = WRTTI::FindTypeByName(sBaseClassName);
        if (pBaseClassType == nullptr)
        {
          WLog::Error("Invalid base class '{}' for Visual Script Class '{}'", sBaseClassName, sScriptClassName);
          return ld;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "ConstantData")
      {
        WSharedPtr<WVisualScriptDataDescription> pConstantDataDesc = W_SCRIPT_NEW(WVisualScriptDataDescription);
        if (pConstantDataDesc->Deserialize(chunk).Failed())
        {
          return ld;
        }

        WSharedPtr<WVisualScriptDataStorage> pConstantDataStorage = W_SCRIPT_NEW(WVisualScriptDataStorage, pConstantDataDesc);
        if (pConstantDataStorage->Deserialize(chunk, WScriptAllocator::GetAllocator()).Succeeded())
        {
          m_pConstantDataStorage = pConstantDataStorage;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "InstanceData")
      {
        WSharedPtr<WVisualScriptDataDescription> pInstanceDataDesc = W_SCRIPT_NEW(WVisualScriptDataDescription);
        if (pInstanceDataDesc->Deserialize(chunk).Succeeded())
        {
          m_pInstanceDataDesc = pInstanceDataDesc;
        }

        WSharedPtr<WVisualScriptInstanceDataMapping> pInstanceDataMapping = W_SCRIPT_NEW(WVisualScriptInstanceDataMapping);
        if (chunk.ReadHashTable(pInstanceDataMapping->m_Content).Succeeded())
        {
          m_pInstanceDataMapping = pInstanceDataMapping;

          // calculate byte offsets from indices
          for (auto& it : m_pInstanceDataMapping->m_Content)
          {
            auto& dataOffset = it.Value().m_DataOffset;
            dataOffset = m_pInstanceDataDesc->GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
          }
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "FunctionGraphs")
      {
        WUInt32 uiNumFunctions;
        chunk >> uiNumFunctions;

        if (m_pInstanceDataDesc == nullptr || m_pConstantDataStorage == nullptr)
        {
          WLog::Error("Old visual script, needs re-export");
          return ld;
        }

        for (WUInt32 i = 0; i < uiNumFunctions; ++i)
        {
          WString sFunctionName;
          WEnum<WVisualScriptNodeDescription::Type> functionType;
          WEnum<WScriptCoroutineCreationMode> coroutineCreationMode;
          chunk >> sFunctionName;
          chunk >> functionType;
          chunk >> coroutineCreationMode;

          WUniquePtr<WVisualScriptGraphDescription> pDesc = W_SCRIPT_NEW(WVisualScriptGraphDescription);
          if (pDesc->Deserialize(chunk, *m_pInstanceDataDesc, m_pConstantDataStorage->GetDesc()).Failed())
          {
            WLog::Error("Invalid visual script desc");
            return ld;
          }

          if (functionType == WVisualScriptNodeDescription::Type::EntryCall)
          {
            WUniquePtr<WVisualScriptFunctionProperty> pFunctionProperty = W_SCRIPT_NEW(WVisualScriptFunctionProperty, sFunctionName, std::move(pDesc));
            functions.PushBack(std::move(pFunctionProperty));
          }
          else if (functionType == WVisualScriptNodeDescription::Type::EntryCall_Coroutine)
          {
            WUniquePtr<WVisualScriptCoroutineAllocator> pCoroutineAllocator = W_SCRIPT_NEW(WVisualScriptCoroutineAllocator, std::move(pDesc));
            auto pCoroutineType = CreateScriptCoroutineType(sScriptClassName, sFunctionName, std::move(pCoroutineAllocator));
            WUniquePtr<WScriptCoroutineFunctionProperty> pFunctionProperty = W_SCRIPT_NEW(WScriptCoroutineFunctionProperty, sFunctionName, pCoroutineType, coroutineCreationMode);
            functions.PushBack(std::move(pFunctionProperty));
          }
          else if (functionType == WVisualScriptNodeDescription::Type::MessageHandler)
          {
            auto desc = pDesc->GetMessageDesc();
            WUniquePtr<WVisualScriptMessageHandler> pMessageHandler = W_SCRIPT_NEW(WVisualScriptMessageHandler, desc, std::move(pDesc));
            messageHandlers.PushBack(std::move(pMessageHandler));
          }
          else if (functionType == WVisualScriptNodeDescription::Type::MessageHandler_Coroutine)
          {
            auto desc = pDesc->GetMessageDesc();
            WUniquePtr<WVisualScriptCoroutineAllocator> pCoroutineAllocator = W_SCRIPT_NEW(WVisualScriptCoroutineAllocator, std::move(pDesc));
            auto pCoroutineType = CreateScriptCoroutineType(sScriptClassName, sFunctionName, std::move(pCoroutineAllocator));
            WUniquePtr<WScriptCoroutineMessageHandler> pMessageHandler = W_SCRIPT_NEW(WScriptCoroutineMessageHandler, sFunctionName, desc, pCoroutineType, coroutineCreationMode);
            messageHandlers.PushBack(std::move(pMessageHandler));
          }
          else
          {
            WLog::Error("Invalid event handler type '{}' for event handler '{}'", WVisualScriptNodeDescription::Type::GetName(functionType), sFunctionName);
            return ld;
          }
        }
      }

      chunk.NextChunk();
    }
  }

  CreateScriptType(sScriptClassName, pBaseClassType, std::move(functions), std::move(messageHandlers));

  ld.m_State = WResourceState::Loaded;
  return ld;
}

void WVisualScriptClassResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (WUInt32)sizeof(WVisualScriptClassResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WUniquePtr<WScriptInstance> WVisualScriptClassResource::Instantiate(WReflectedClass& inout_owner, WWorld* pWorld) const
{
  return W_SCRIPT_NEW(WVisualScriptInstance, inout_owner, pWorld, m_pConstantDataStorage, m_pInstanceDataDesc, m_pInstanceDataMapping);
}


W_STATICLINK_FILE(VisualScriptPlugin, VisualScriptPlugin_Resources_VisualScriptClassResource);
