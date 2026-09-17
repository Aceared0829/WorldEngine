#include <TexConv/TexConvPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/AssetInfoFile.h>
#include <TexConv/TexConv.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/WTexFormat/WTexFormat.h>

WTexConv::WTexConv()
  : WApplication("TexConv")
{
}

WResult WTexConv::BeforeCoreSystemsStartup()
{
  WStartup::AddApplicationTag("tool");
  WStartup::AddApplicationTag("texconv");

  return SUPER::BeforeCoreSystemsStartup();
}

void WTexConv::AfterCoreSystemsStartup()
{
  WFileSystem::AddDataDirectory("", "App", ":", WDataDirUsage::AllowWrites).IgnoreResult();

  WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
  WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);
}

void WTexConv::BeforeCoreSystemsShutdown()
{
  WGlobalLog::RemoveLogWriter(WLogWriter::Console::LogMessageHandler);
  WGlobalLog::RemoveLogWriter(WLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

WResult WTexConv::DetectOutputFormat()
{
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = WTexConvOutputType::None;
    return W_SUCCESS;
  }

  WStringBuilder sExt = WPathUtils::GetFileExtension(m_sOutputFile);
  sExt.ToUpper();

  if (sExt == "DDS")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = true;
    return W_SUCCESS;
  }
  if (sExt == "TGA" || sExt == "PNG")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = false;
    return W_SUCCESS;
  }
  if (sExt == "WBINTEXTURE2D")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return W_SUCCESS;
  }
  if (sExt == "WBINTEXTURE3D")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return W_SUCCESS;
  }
  if (sExt == "WBINTEXTURECUBE")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return W_SUCCESS;
  }
  if (sExt == "WBINTEXTUREATLAS")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = true;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return W_SUCCESS;
  }
  if (sExt == "WBINIMAGEDATA")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = false;
    return W_SUCCESS;
  }

  WLog::Error("Output file uses unsupported file format '{}'", sExt);
  return W_FAILURE;
}

bool WTexConv::IsTexFormat() const
{
  const WStringView ext = WPathUtils::GetFileExtension(m_sOutputFile);

  return ext.StartsWith_NoCase("W");
}

WResult WTexConv::WriteTexFile(WStreamWriter& inout_stream, const WImage& image)
{
  WAssetFileHeader asset;
  asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

  W_SUCCEED_OR_RETURN(asset.Write(inout_stream));

  WTexFormat texFormat;
  texFormat.m_bSRGB = WImageFormat::IsSrgb(image.GetImageFormat());
  texFormat.m_AddressModeU = m_Processor.m_Descriptor.m_AddressModeU;
  texFormat.m_AddressModeV = m_Processor.m_Descriptor.m_AddressModeV;
  texFormat.m_AddressModeW = m_Processor.m_Descriptor.m_AddressModeW;
  texFormat.m_TextureFilter = m_Processor.m_Descriptor.m_FilterMode;

  texFormat.WriteTextureHeader(inout_stream);

  WDdsFileFormat ddsWriter;
  if (ddsWriter.WriteImage(inout_stream, image, "dds").Failed())
  {
    WLog::Error("Failed to write DDS image chunk to WTex file.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WTexConv::WriteOutputFile(WStringView sFile, const WImage& image)
{
  if (sFile.HasExtension("WBinImageData"))
  {
    WDeferredFileWriter file;
    file.SetOutput(sFile);

    WAssetFileHeader asset;
    asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

    if (asset.Write(file).Failed())
    {
      WLog::Error("Failed to write asset header to file.");
      return W_FAILURE;
    }

    WUInt8 uiVersion = 1;
    file << uiVersion;

    WUInt8 uiFormat = 1; // 1 == PNG
    file << uiFormat;

    WStbImageFileFormats pngWriter;
    if (pngWriter.WriteImage(file, image, "png").Failed())
    {
      WLog::Error("Failed to write data as PNG to WImageData file.");
      return W_FAILURE;
    }

    return file.Close();
  }
  else if (IsTexFormat())
  {
    WDeferredFileWriter file;
    file.SetOutput(sFile);

    W_SUCCEED_OR_RETURN(WriteTexFile(file, image));

    return file.Close();
  }
  else
  {
    return image.SaveTo(sFile);
  }
}

WResult WTexConv::ReduceSingleFile(WStringView sInputFile, WStringView sOutputDir, WStringView sExplicitOutputFile)
{
  WImage image;
  if (image.LoadFrom(sInputFile).Failed())
  {
    WLog::Error("Failed to load input image '{}'.", sInputFile);
    return W_FAILURE;
  }

  bool bHasAlpha = false;
  if (WImageFormat::GetNumChannels(image.GetImageFormat()) >= 4)
  {
    if (image.Convert(WImageFormat::R32G32B32A32_FLOAT).Succeeded())
    {
      const float* pColors = image.GetPixelPointer<float>();
      pColors += 3; // offset to alpha channel

      W_ASSERT_DEV(image.GetRowPitch() == image.GetWidth() * sizeof(float) * 4, "Unexpected row pitch");

      bool bAllOpaque = true;
      bool bAllTransparent = true;

      for (WUInt32 i = 0; i < image.GetWidth() * image.GetHeight(); ++i)
      {
        const float a = *pColors;
        bAllOpaque = bAllOpaque && WMath::IsEqual(a, 1.0f, 1.0f / 255.0f);
        bAllTransparent = bAllTransparent && WMath::IsEqual(a, 0.0f, 1.0f / 255.0f);
        pColors += 4;

        if (!bAllOpaque && !bAllTransparent)
        {
          bHasAlpha = true;
          break;
        }
      }
    }
    else
    {
      // Conversion failed; assume alpha is present to avoid data loss.
      bHasAlpha = true;
    }
  }

  const WStringView sExt = bHasAlpha ? "png" : "jpg";

  // Determine output path:
  //   sExplicitOutputFile takes priority (single-file mode with -out as a file path)
  //   sOutputDir places the file in a given folder (folder mode, or single-file with -out as directory)
  //   otherwise output goes next to the input file
  WStringBuilder sOutputFile;
  if (!sExplicitOutputFile.IsEmpty())
  {
    sOutputFile = sExplicitOutputFile;
  }
  else if (!sOutputDir.IsEmpty())
  {
    sOutputFile = sOutputDir;
    sOutputFile.AppendPath(WPathUtils::GetFileName(sInputFile));
    sOutputFile.ChangeFileExtension(sExt);
  }
  else
  {
    sOutputFile = sInputFile;
    sOutputFile.ChangeFileExtension(sExt);
  }

  if (WOSFile::ExistsFile(sOutputFile))
  {
    WLog::Info("Skipping '{}' (output already exists).", sOutputFile);
    return W_SUCCESS;
  }

  if (image.SaveTo(sOutputFile).Failed())
  {
    WLog::Error("Failed to write output image '{}'.", sOutputFile);
    return W_FAILURE;
  }

  WLog::Success("Wrote '{}' ({})", sOutputFile, bHasAlpha ? "has alpha -> PNG" : "no alpha -> JPG");

  if (m_bDeleteSource)
  {
    if (WOSFile::DeleteFile(sInputFile).Failed())
    {
      WLog::Error("Failed to delete source file '{}'.", sInputFile);
      return W_FAILURE;
    }

    WLog::Dev("Deleted source file '{}'.", sInputFile);
  }

  return W_SUCCESS;
}

WResult WTexConv::RunReduce()
{
  WStringBuilder sInputPath = m_sReduceInputFile;

  // Check for trailing '*' which signals recursive folder processing
  bool bRecursive = false;
  if (sInputPath.EndsWith("*"))
  {
    bRecursive = true;
    sInputPath.Shrink(0, 1); // remove trailing '*'
    sInputPath.Trim("/\\");  // remove any trailing separator left behind
  }

  // Single file mode: -out is either a redirect directory or a direct output file path
  if (WOSFile::ExistsFile(sInputPath))
  {
    const WStringView sOutputDir = (!m_sOutputFile.IsEmpty() && WOSFile::ExistsDirectory(m_sOutputFile)) ? WStringView(m_sOutputFile) : WStringView();
    const WStringView sOutputFile = (!m_sOutputFile.IsEmpty() && !WOSFile::ExistsDirectory(m_sOutputFile)) ? WStringView(m_sOutputFile) : WStringView();
    return ReduceSingleFile(sInputPath, sOutputDir, sOutputFile);
  }

  // Folder mode: parse -out, which may end with '*' to mirror the input subfolder structure
  WStringBuilder sOutputBase = m_sOutputFile;
  bool bMirrorStructure = false;
  if (sOutputBase.EndsWith("*"))
  {
    bMirrorStructure = true;
    sOutputBase.Shrink(0, 1);
    sOutputBase.Trim("/\\");
  }

  if (!sOutputBase.IsEmpty() && !WOSFile::ExistsDirectory(sOutputBase))
  {
    WLog::Error("The -out path '{}' is not an existing directory.", sOutputBase);
    return W_FAILURE;
  }

  if (WOSFile::ExistsDirectory(sInputPath))
  {
    // Recursive iteration does not support wildcards; filter by extension manually instead.
    const WFileSystemIteratorFlags::Enum flags = bRecursive ? WFileSystemIteratorFlags::ReportFilesRecursive : WFileSystemIteratorFlags::ReportFiles;

    WUInt32 uiConverted = 0;
    WUInt32 uiFailed = 0;

    bool bAnyFound = false;

    WStringBuilder sFullPath;
    WStringBuilder sFileOutputDir;

    {
      WStringBuilder sSearch = sInputPath;
      if (!bRecursive)
        sSearch.AppendPath("*");

      WFileSystemIterator iter;
      iter.StartSearch(sSearch, flags);

      for (; iter.IsValid(); iter.Next())
      {
        const WStringView sName = iter.GetStats().m_sName;
        if (!sName.HasExtension("dds") && !sName.HasExtension("tga"))
          continue;

        bAnyFound = true;

        sFullPath = iter.GetCurrentPath();
        sFullPath.AppendPath(iter.GetStats().m_sName);

        if (sOutputBase.IsEmpty())
        {
          // No -out: place output next to each input file
          sFileOutputDir.Clear();
        }
        else if (bMirrorStructure)
        {
          // -out ends with '*': mirror the subfolder structure under sOutputBase.
          // iter.GetCurrentPath() is the directory containing the current file.
          // Strip the sInputPath prefix to obtain the relative subdirectory.
          WStringView sCurDir = iter.GetCurrentPath();
          WStringView sRelativeDir;
          if (sCurDir.StartsWith(sInputPath))
          {
            sRelativeDir = WStringView(sCurDir.GetStartPointer() + sInputPath.GetElementCount());
            // trim any leading separator
            while (!sRelativeDir.IsEmpty() && (sRelativeDir.GetStartPointer()[0] == '/' || sRelativeDir.GetStartPointer()[0] == '\\'))
            {
              sRelativeDir = WStringView(sRelativeDir.GetStartPointer() + 1, sRelativeDir.GetEndPointer());
            }
          }
          sFileOutputDir = sOutputBase;
          if (!sRelativeDir.IsEmpty())
          {
            sFileOutputDir.AppendPath(sRelativeDir);
            WOSFile::CreateDirectoryStructure(sFileOutputDir).IgnoreResult();
          }
        }
        else
        {
          // -out is a plain directory: place all outputs flat in sOutputBase
          sFileOutputDir = sOutputBase;
        }

        if (ReduceSingleFile(sFullPath, sFileOutputDir).Succeeded())
        {
          ++uiConverted;
        }
        else
        {
          ++uiFailed;
        }
      }
    }

    if (!bAnyFound)
    {
      WLog::Warning("No DDS or TGA files found in '{}'.", sInputPath);
      return W_SUCCESS;
    }

    WLog::Info("Reduce folder '{}': {} processed, {} failed.", sInputPath, uiConverted, uiFailed);
    return uiFailed == 0 ? W_SUCCESS : W_FAILURE;
  }

  WLog::Error("Input path '{}' is neither a file nor an existing directory.", sInputPath);
  return W_FAILURE;
}

void WTexConv::Run()
{
  SetReturnCode(-1);

  if (ParseCommandLine().Failed())
  {
    QuitApplication();
    return;
  }

  if (m_Mode == WTexConvMode::Reduce)
  {
    if (RunReduce().Succeeded())
    {
      SetReturnCode(0);
    }

    QuitApplication();
    return;
  }

  if (m_Mode == WTexConvMode::Compare)
  {
    if (m_Comparer.Compare().Failed())
    {
      QuitApplication();
      return;
    }

    SetReturnCode(0);

    if (m_Comparer.m_bExceededMSE)
    {
      SetReturnCode(m_Comparer.m_OutputMSE);

      if (!m_sOutputFile.IsEmpty())
      {
        WStringBuilder tmp;

        tmp.Set(m_sOutputFile, "-rgb.png");
        m_Comparer.m_OutputImageDiffRgb.SaveTo(tmp).IgnoreResult();

        tmp.Set(m_sOutputFile, "-alpha.png");
        m_Comparer.m_OutputImageDiffAlpha.SaveTo(tmp).IgnoreResult();

        if (!m_sHtmlTitle.IsEmpty())
        {
          tmp.Set(m_sOutputFile, ".htm");

          WFileWriter file;
          if (file.Open(tmp).Succeeded())
          {
            WStringBuilder html;

            WImageUtils::CreateImageDiffHtml(html, m_sHtmlTitle, m_Comparer.m_ExtractedExpectedRgb, m_Comparer.m_ExtractedExpectedAlpha, m_Comparer.m_ExtractedActualRgb, m_Comparer.m_ExtractedActualAlpha, m_Comparer.m_OutputImageDiffRgb, m_Comparer.m_OutputImageDiffAlpha, m_Comparer.m_OutputMSE, m_Comparer.m_Descriptor.m_MeanSquareErrorThreshold, m_Comparer.m_uiOutputMinDiffRgb, m_Comparer.m_uiOutputMaxDiffRgb, m_Comparer.m_uiOutputMinDiffAlpha, m_Comparer.m_uiOutputMaxDiffAlpha);

            file.WriteBytes(html.GetData(), html.GetElementCount()).AssertSuccess();
          }
        }
      }
    }
  }
  else
  {
    if (m_Processor.Process().Failed())
    {
      QuitApplication();
      return;
    }

    if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Atlas)
    {
      WDeferredFileWriter file;
      file.SetOutput(m_sOutputFile);

      WAssetFileHeader header;
      header.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

      header.Write(file).IgnoreResult();

      m_Processor.m_TextureAtlas.CopyToStream(file).IgnoreResult();

      if (file.Close().Succeeded())
      {
        SetReturnCode(0);
      }
      else
      {
        WLog::Error("Failed to write atlas output image.");
      }

      QuitApplication();
      return;
    }

    if (!m_sOutputFile.IsEmpty() && m_Processor.m_OutputImage.IsValid())
    {
      if (WriteOutputFile(m_sOutputFile, m_Processor.m_OutputImage).Failed())
      {
        WLog::Error("Failed to write main result to '{}'", m_sOutputFile);
        QuitApplication();
        return;
      }

      WLog::Success("Wrote main result to '{}'", m_sOutputFile);
    }

    if (!m_sOutputThumbnailFile.IsEmpty() && m_Processor.m_ThumbnailOutputImage.IsValid())
    {
      if (m_Processor.m_ThumbnailOutputImage.SaveTo(m_sOutputThumbnailFile).Failed())
      {
        WLog::Error("Failed to write thumbnail result to '{}'", m_sOutputThumbnailFile);
        QuitApplication();
        return;
      }

      WLog::Success("Wrote thumbnail to '{}'", m_sOutputThumbnailFile);
    }

    if (!m_sOutputAssetInfoFile.IsEmpty() && m_Processor.m_OutputImage.IsValid())
    {
      const WImageHeader& header = m_Processor.m_OutputImage.GetHeader();

      WAssetInfoFile info;
      info.SetValue(WAssetInfoFile::Keys::ImageWidth, header.GetWidth());
      info.SetValue(WAssetInfoFile::Keys::ImageHeight, header.GetHeight());
      info.SetValue(WAssetInfoFile::Keys::Format, WImageFormat::GetName(header.GetImageFormat()));
      info.SetValue("MipLevels", header.GetNumMipLevels());

      WAssetFileHeader assetHeader;
      assetHeader.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

      if (info.WriteToFile(m_sOutputAssetInfoFile, assetHeader).Failed())
      {
        WLog::Error("Failed to write asset info to '{}'", m_sOutputAssetInfoFile);
        QuitApplication();
        return;
      }

      WLog::Success("Wrote asset info to '{}'", m_sOutputAssetInfoFile);
    }

    if (!m_sOutputLowResFile.IsEmpty())
    {
      // the image may not exist, if we do not have enough mips, so make sure any old low-res file is cleaned up
      WOSFile::DeleteFile(m_sOutputLowResFile).IgnoreResult();

      if (m_Processor.m_LowResOutputImage.IsValid())
      {
        if (WriteOutputFile(m_sOutputLowResFile, m_Processor.m_LowResOutputImage).Failed())
        {
          WLog::Error("Failed to write low-res result to '{}'", m_sOutputLowResFile);
          QuitApplication();
          return;
        }

        WLog::Success("Wrote low-res result to '{}'", m_sOutputLowResFile);
      }
    }

    SetReturnCode(0);
  }

  QuitApplication();
}

W_APPLICATION_ENTRY_POINT(WTexConv);
