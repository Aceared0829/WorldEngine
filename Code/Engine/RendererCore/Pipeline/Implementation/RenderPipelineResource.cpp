#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderPipelineResource, 1, WRTTIDefaultAllocator<WRenderPipelineResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WRenderPipelineResource);
// clang-format on

WRenderPipelineResource::WRenderPipelineResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WInternal::NewInstance<WRenderPipeline> WRenderPipelineResource::CreateRenderPipeline() const
{
  if (GetLoadingState() != WResourceState::Loaded)
  {
    WLog::Error("Can't create render pipeline '{0}', the resource is not loaded!", GetResourceID());
    return WInternal::NewInstance<WRenderPipeline>(nullptr, nullptr);
  }

  return WRenderPipelineResourceLoader::CreateRenderPipeline(m_Desc);
}

// static
WRenderPipelineResourceHandle WRenderPipelineResource::CreateMissingPipeline()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  WDynamicArray<const WRenderPipelinePass*> passPointers;
  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;

  {
    WUniquePtr<WSourcePass> pPass = W_DEFAULT_NEW(WSourcePass, "ColorSource");
    passPointers.PushBack(pPass.Borrow());
    passes.PushBack(std::move(pPass));
  }

  {
    WUniquePtr<WSimpleRenderPass> pPass = W_DEFAULT_NEW(WSimpleRenderPass);
    pPass->SetMessage("Render pipeline resource is missing. Ensure that the corresponding asset has been transformed.");
    passPointers.PushBack(pPass.Borrow());
    passes.PushBack(std::move(pPass));
  }

  {
    WUniquePtr<WTargetPass> pPass = W_DEFAULT_NEW(WTargetPass);
    passPointers.PushBack(pPass.Borrow());
    passes.PushBack(std::move(pPass));
  }

  connections.PushBack({0, 1, "Output", "Color"});
  connections.PushBack({1, 2, "Color", "Color0"});

  WRenderPipelineResourceDescriptor desc;
  WMemoryStreamContainerWrapperStorage<WDynamicArray<WUInt8>> storage(&desc.m_SerializedPipeline);
  WMemoryStreamWriter writer(&storage);
  WRenderPipelineResourceLoader::ExportPipeline(passPointers, {}, connections, writer).AssertSuccess("Failed to serialize missing render pipeline");

  return WResourceManager::CreateResource<WRenderPipelineResource>("MissingRenderPipeline", std::move(desc), "MissingRenderPipeline");
}

WResourceLoadDesc WRenderPipelineResource::UnloadData(Unload WhatToUnload)
{
  m_Desc.Clear();

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WRenderPipelineResource::UpdateContent(WStreamReader* Stream)
{
  m_Desc.Clear();

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  if (sAbsFilePath.HasExtension("WBinRenderPipeline"))
  {
    WStringBuilder sTemp, sTemp2;

    WAssetFileHeader AssetHash;
    AssetHash.Read(*Stream).IgnoreResult();

    WUInt8 uiVersion = 0;
    (*Stream) >> uiVersion;

    // Version 1 was using old tooling serialization. Code path removed.
    if (uiVersion == 1)
    {
      res.m_State = WResourceState::LoadedResourceMissing;
      WLog::Error("Failed to load old WRenderPipelineResource '{}'. Needs re-transform.", sAbsFilePath);
      return res;
    }
    W_ASSERT_DEV(uiVersion == 2, "Unknown WBinRenderPipeline version {0}", uiVersion);

    WUInt32 uiSize = 0;
    (*Stream) >> uiSize;

    m_Desc.m_SerializedPipeline.SetCountUninitialized(uiSize);
    Stream->ReadBytes(m_Desc.m_SerializedPipeline.GetData(), uiSize);

    W_ASSERT_DEV(uiSize > 0, "RenderPipeline resourse contains no pipeline data!");
  }
  else
  {
    W_REPORT_FAILURE("The file '{0}' is unsupported, only '.WBinRenderPipeline' files can be loaded as WRenderPipelineResource", sAbsFilePath);
  }

  return res;
}

void WRenderPipelineResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WRenderPipelineResource) + (WUInt32)(m_Desc.m_SerializedPipeline.GetCount());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WRenderPipelineResource, WRenderPipelineResourceDescriptor)
{
  m_Desc = descriptor;

  WResourceLoadDesc res;
  res.m_State = WResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResource);
