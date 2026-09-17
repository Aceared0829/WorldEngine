#include <RendererCore/RendererCorePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/WTexFormat/WTexFormat.h>

static WTextureResourceLoader s_TextureResourceLoader;

WCVarFloat cvar_StreamingTextureLoadDelay("Streaming.TextureLoadDelay", 0.0f, WCVarFlags::Save, "Artificial texture loading slowdown");

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, TextureResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WResourceManager::SetResourceTypeLoader<WTexture2DResource>(&s_TextureResourceLoader);
    WResourceManager::SetResourceTypeLoader<WTexture3DResource>(&s_TextureResourceLoader);
    WResourceManager::SetResourceTypeLoader<WTextureCubeResource>(&s_TextureResourceLoader);
    WResourceManager::SetResourceTypeLoader<WRenderToTexture2DResource>(&s_TextureResourceLoader);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WResourceManager::SetResourceTypeLoader<WTexture2DResource>(nullptr);
    WResourceManager::SetResourceTypeLoader<WTexture3DResource>(nullptr);
    WResourceManager::SetResourceTypeLoader<WTextureCubeResource>(nullptr);
    WResourceManager::SetResourceTypeLoader<WRenderToTexture2DResource>(nullptr);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WResourceLoadData WTextureResourceLoader::OpenDataStream(const WResource* pResource)
{
  LoadedData* pData = W_DEFAULT_NEW(LoadedData);

  WResourceLoadData res;

  WStringView sResourceID = pResource->GetResourceID();

  // Solid Color Textures
  if (sResourceID.HasExtension("color") || sResourceID.StartsWith("#") || (!sResourceID.HasAnyExtension() && !WConversionUtils::IsStringUuid(sResourceID)))
  {
    WStringBuilder sName = pResource->GetResourceID();
    sName.RemoveFileExtension();

    bool bValidColor = false;
    const WColorGammaUB color = WConversionUtils::GetColorByName(sName, &bValidColor);

    if (!bValidColor)
    {
      WLog::Error("'{0}' is not a valid color name. Using 'RebeccaPurple' as fallback.", sName);
    }

    pData->m_TexFormat.m_bSRGB = true;

    WImageHeader header;
    header.SetWidth(4);
    header.SetHeight(4);
    header.SetDepth(1);
    header.SetImageFormat(WImageFormat::R8G8B8A8_UNORM_SRGB);
    header.SetNumMipLevels(1);
    header.SetNumFaces(1);
    pData->m_Image.ResetAndAlloc(header);
    WUInt8* pPixels = pData->m_Image.GetPixelPointer<WUInt8>();

    for (WUInt32 px = 0; px < 4 * 4 * 4; px += 4)
    {
      pPixels[px + 0] = color.r;
      pPixels[px + 1] = color.g;
      pPixels[px + 2] = color.b;
      pPixels[px + 3] = color.a;
    }
  }
  else
  {
    WFileReader File;
    if (File.Open(pResource->GetResourceID()).Failed())
      return res;

    if (File.GetFilePathAbsolute().HasExtension("WBinColorGradient"))
    {
      res.m_sResourceDescription = File.GetFilePathRelative().GetView();

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
      {
        WFileStats stat;
        if (WFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
        {
          res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
        }
      }
#endif

      // skip the asset file header at the start of the file
      WAssetFileHeader AssetHash;
      if (AssetHash.Read(File).Failed())
        return res;

      WColorGradientResourceDescriptor desc;
      desc.Load(File);

      constexpr WUInt32 uiWidth = 512;
      constexpr WUInt32 uiHeight = 1;

      WImageHeader header;
      header.SetWidth(uiWidth);
      header.SetHeight(uiHeight);
      header.SetDepth(1);
      header.SetImageFormat(WImageFormat::R16G16B16A16_FLOAT);
      header.SetNumMipLevels(1);
      header.SetNumFaces(1);
      pData->m_TexFormat.m_AddressModeU = WImageAddressMode::Clamp;
      pData->m_TexFormat.m_AddressModeV = WImageAddressMode::Clamp;
      pData->m_TexFormat.m_TextureFilter = WTextureFilterSetting::FixedBilinear;
      pData->m_Image.ResetAndAlloc(header);
      WFloat16* pPixels = pData->m_Image.GetPixelPointer<WFloat16>();

      // fill each pixel with the color gradient from left to right, duplicate along the vertical pixels
      for (WUInt32 x = 0; x < uiWidth; ++x)
      {
        const double fGradientPos = (double)x / (double)(uiWidth - 1); // normalize to [0, 1]

        WColor color;
        desc.m_Gradient.Evaluate(fGradientPos, color);

        // duplicate along the vertical pixels
        for (WUInt32 y = 0; y < uiHeight; ++y)
        {
          const WUInt32 pixelIndex = (y * 1024 + x) * 4;
          pPixels[pixelIndex + 0] = color.r;
          pPixels[pixelIndex + 1] = color.g;
          pPixels[pixelIndex + 2] = color.b;
          pPixels[pixelIndex + 3] = color.a;
        }
      }
    }
    else
    {
      const WStringBuilder sAbsolutePath = File.GetFilePathAbsolute();
      res.m_sResourceDescription = File.GetFilePathRelative().GetView();

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
      {
        WFileStats stat;
        if (WFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
        {
          res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
        }
      }
#endif

      /// In case this is not a proper asset (WTextureXX format), this is a hack to get the SRGB information for the texture
      const WStringBuilder sName = WPathUtils::GetFileName(sAbsolutePath);
      pData->m_TexFormat.m_bSRGB = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

      if (sAbsolutePath.HasExtension("WBinTexture2D") || sAbsolutePath.HasExtension("WBinTexture3D") || sAbsolutePath.HasExtension("WBinTextureCube") || sAbsolutePath.HasExtension("WBinRenderTarget") || sAbsolutePath.HasExtension("WBinLUT"))
      {
        if (LoadTexFile(File, *pData).Failed())
          return res;
      }
      else
      {
        // read whatever format, as long as WImage supports it
        File.Close();

        if (pData->m_Image.LoadFrom(pResource->GetResourceID()).Failed())
          return res;

        if (pData->m_Image.GetImageFormat() == WImageFormat::B8G8R8_UNORM)
        {
          /// \todo A conversion to B8G8R8X8_UNORM currently fails

          WLog::Warning("Texture resource uses inefficient BGR format, converting to BGRX: '{0}'", sAbsolutePath);
          if (WImageConversion::Convert(pData->m_Image, pData->m_Image, WImageFormat::B8G8R8A8_UNORM).Failed())
            return res;
        }
      }
    }
  }

  WMemoryStreamWriter w(&pData->m_Storage);

  WriteTextureLoadStream(w, *pData);

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  if (cvar_StreamingTextureLoadDelay > 0)
  {
    WThreadUtils::Sleep(WTime::MakeFromSeconds(cvar_StreamingTextureLoadDelay));
  }

  return res;
}

void WTextureResourceLoader::CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData)
{
  LoadedData* pData = (LoadedData*)loaderData.m_pCustomLoaderData;

  W_DEFAULT_DELETE(pData);
}

bool WTextureResourceLoader::IsResourceOutdated(const WResource* pResource) const
{
  // solid color textures are never outdated
  if (WPathUtils::HasExtension(pResource->GetResourceID(), "color"))
    return false;

  WStringView sResourceID = pResource->GetResourceID();

  // procedurally generated textures (color names, hex codes) are never outdated
  if (sResourceID.StartsWith("#") || (!sResourceID.HasAnyExtension() && !WConversionUtils::IsStringUuid(sResourceID)))
    return false;

  // don't try to reload a file that cannot be found
  WStringBuilder sAbs;
  if (WFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if W_ENABLED(W_SUPPORTS_FILE_STATS)

  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    WFileStats stat;
    if (WFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), WTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

WResult WTextureResourceLoader::LoadTexFile(WStreamReader& inout_stream, LoadedData& ref_data)
{
  // read the hash, ignore it
  WAssetFileHeader AssetHash;
  W_SUCCEED_OR_RETURN(AssetHash.Read(inout_stream));

  ref_data.m_TexFormat.ReadHeader(inout_stream);

  if (ref_data.m_TexFormat.m_iRenderTargetResolutionX == 0)
  {
    WDdsFileFormat fmt;
    return fmt.ReadImage(inout_stream, ref_data.m_Image, "dds");
  }
  else
  {
    return W_SUCCESS;
  }
}

void WTextureResourceLoader::WriteTextureLoadStream(WStreamWriter& w, const LoadedData& data)
{
  const WImage* pImage = &data.m_Image;
  w.WriteBytes(&pImage, sizeof(WImage*)).IgnoreResult();

  w << data.m_bIsFallback;
  data.m_TexFormat.WriteRenderTargetHeader(w);
}

W_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureLoader);
