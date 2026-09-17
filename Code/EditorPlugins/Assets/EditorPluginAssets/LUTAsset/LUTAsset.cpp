#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>



#include <EditorPluginAssets/LUTAsset/AdobeCUBEReader.h>
#include <EditorPluginAssets/LUTAsset/LUTAsset.h>

#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/WTexFormat/WTexFormat.h>



// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLUTAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WLUTAssetDocument::WLUTAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WLUTAssetProperties>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

WTransformStatus WLUTAssetDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  const auto props = GetProperties();

  // Read CUBE file, convert to 3D texture and write to file
  WFileStats Stats;
  bool bStat = WOSFile::GetFileStats(props->GetAbsoluteInputFilePath(), Stats).Succeeded();

  WFileReader cubeFile;
  if (!bStat || cubeFile.Open(props->GetAbsoluteInputFilePath()).Failed())
  {
    return WStatus(WFmt("Couldn't open CUBE file '{0}'.", props->GetAbsoluteInputFilePath()));
  }

  WAdobeCUBEReader cubeReader;
  auto parseRes = cubeReader.ParseFile(cubeFile);
  if (parseRes.Failed())
    return parseRes;

  const WUInt32 lutSize = cubeReader.GetLUTSize();

  // Build an WImage from the data
  WImageHeader imgHeader;
  imgHeader.SetImageFormat(WImageFormat::R8G8B8A8_UNORM_SRGB);
  imgHeader.SetWidth(lutSize);
  imgHeader.SetHeight(lutSize);
  imgHeader.SetDepth(lutSize);

  WImage img;
  img.ResetAndAlloc(imgHeader);

  if (!img.IsValid())
  {
    return WStatus("Allocated WImage for LUT data is not valid.");
  }



  for (WUInt32 b = 0; b < lutSize; ++b)
  {
    for (WUInt32 g = 0; g < lutSize; ++g)
    {
      for (WUInt32 r = 0; r < lutSize; ++r)
      {
        const WVec3 val = cubeReader.GetLUTEntry(r, g, b);

        WColor col(val.x, val.y, val.z);
        WColorGammaUB colUb(col);

        WColorGammaUB* pPixel = img.GetPixelPointer<WColorGammaUB>(0, 0, 0, r, g, b);

        *pPixel = colUb;
      }
    }
  }

  WDeferredFileWriter file;
  file.SetOutput(szTargetFile);
  W_SUCCEED_OR_RETURN(AssetHeader.Write(file));

  WTexFormat texFormat;
  texFormat.m_bSRGB = true;
  texFormat.m_AddressModeU = WImageAddressMode::Clamp;
  texFormat.m_AddressModeV = WImageAddressMode::Clamp;
  texFormat.m_AddressModeW = WImageAddressMode::Clamp;
  texFormat.m_TextureFilter = WTextureFilterSetting::FixedBilinear;

  texFormat.WriteTextureHeader(file);

  WDdsFileFormat fmt;
  if (fmt.WriteImage(file, img, "dds").Failed())
    return WStatus(WFmt("Writing image to target file failed: '{0}'", szTargetFile));

  if (file.Close().Failed())
    return WStatus(WFmt("Writing to target file failed: '{0}'", szTargetFile));

  return WStatus(W_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLUTAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WLUTAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

WLUTAssetDocumentGenerator::WLUTAssetDocumentGenerator()
{
  AddSupportedFileType("cube");
}

WLUTAssetDocumentGenerator::~WLUTAssetDocumentGenerator() = default;

void WLUTAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
  info.m_Priority = WAssetDocGeneratorPriority::DefaultPriority;
  info.m_sName = "LUTImport.Cube";
  info.m_sIcon = ":/AssetIcons/LUT.svg";
}

WStatus WLUTAssetDocumentGenerator::Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  const WStringBuilder sOutFile = GetImportTargetPath(sInputFileAbs);

  auto pApp = WQtEditorApp::GetSingleton();

  WStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  WDocument* pDoc = pApp->CreateDocument(sOutFile, WDocumentFlags::None);
  if (pDoc == nullptr)
    return WStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  WLUTAssetDocument* pAssetDoc = WDynamicCast<WLUTAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input", sInputFileRel.GetView());

  WLog::Success("Imported LUT: '{}'", sOutFile);

  return WStatus(W_SUCCESS);
}
