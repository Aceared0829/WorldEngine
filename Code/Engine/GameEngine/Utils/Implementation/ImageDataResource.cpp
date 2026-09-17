#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/WTexFormat/WTexFormat.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WImageDataResource, 1, WRTTIDefaultAllocator<WImageDataResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WImageDataResource);
// clang-format on

WImageDataResource::WImageDataResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WImageDataResource::~WImageDataResource() = default;

WResourceLoadDesc WImageDataResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WImageDataResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WImageDataResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  WImageDataResourceDescriptor desc;

  if (sAbsFilePath.HasExtension("WBinImageData"))
  {
    WAssetFileHeader AssetHash;
    if (AssetHash.Read(*Stream).Failed())
    {
      res.m_State = WResourceState::LoadedResourceMissing;
      return res;
    }

    WUInt8 uiVersion = 0;
    WUInt8 uiDataFormat = 0;

    *Stream >> uiVersion;
    *Stream >> uiDataFormat;

    if (uiVersion != 1 || uiDataFormat != 1)
    {
      WLog::Error("Unsupported WImageData file format or version");

      res.m_State = WResourceState::LoadedResourceMissing;
      return res;
    }

    WStbImageFileFormats fmt;
    if (fmt.ReadImage(*Stream, desc.m_Image, "png").Failed())
    {
      res.m_State = WResourceState::LoadedResourceMissing;
      return res;
    }
  }
  else
  {
    WStringBuilder ext;
    ext = sAbsFilePath.GetFileExtension();

    if (WImageFileFormat::GetReaderFormat(ext)->ReadImage(*Stream, desc.m_Image, ext).Failed())
    {
      res.m_State = WResourceState::LoadedResourceMissing;
      return res;
    }
  }


  CreateResource(std::move(desc));

  res.m_State = WResourceState::Loaded;
  return res;
}

void WImageDataResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WImageDataResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += m_pDescriptor->m_Image.GetByteBlobPtr().GetCount();
  }
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WImageDataResource, WImageDataResourceDescriptor)
{
  m_pDescriptor = W_DEFAULT_NEW(WImageDataResourceDescriptor);

  *m_pDescriptor = std::move(descriptor);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  if (m_pDescriptor->m_Image.Convert(WImageFormat::R32G32B32A32_FLOAT).Failed())
  {
    res.m_State = WResourceState::LoadedResourceMissing;
  }

  return res;
}

// WResult WImageDataResourceDescriptor::Serialize(WStreamWriter& stream) const
//{
//  W_SUCCEED_OR_RETURN(WImageFileFormat::GetWriterFormat("png")->WriteImage(stream, m_Image, "png"));
//
//  return W_SUCCESS;
//}
//
// WResult WImageDataResourceDescriptor::Deserialize(WStreamReader& stream)
//{
//  W_SUCCEED_OR_RETURN(WImageFileFormat::GetReaderFormat("png")->ReadImage(stream, m_Image, "png"));
//
//  return W_SUCCESS;
//}


W_STATICLINK_FILE(GameEngine, GameEngine_Utils_Implementation_ImageDataResource);
