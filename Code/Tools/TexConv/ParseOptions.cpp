#include <TexConv/TexConvPCH.h>

#include <TexConv/TexConv.h>

#include <Foundation/Utilities/CommandLineOptions.h>

WCommandLineOptionEnum opt_Mode("_TexConv", "-mode", "Mode determines which arguments need to be set.\n\
  In compare mode the mean-square error (MSE) is returned. 0 if it is below the threshold.\n\
  In reduce mode a single DDS or TGA file is loaded and saved as JPG or PNG without any processing.\
",
  "Convert | Compare | Reduce", 0);

WCommandLineOptionBool opt_DeleteSource("_TexConv", "-deleteSource",
  "Only used in reduce mode. If set, the source file is deleted after it has been successfully converted.",
  false);

WCommandLineOptionPath opt_Out("_TexConv", "-out",
  "Absolute path to main output file.\n\
   ext = tga, dds, WBinTexture2D, WBinTexture3D, WBinTextureCube or WBinTextureAtlas.",
  "");


WCommandLineOptionDoc opt_In("_TexConv", "-inX", "\"File\"",
  "Specifies input image X.\n\
   X = 0 .. 63, e.g. -in0, -in1, etc.\n\
   If X is not given, X equals 0.",
  "");

WCommandLineOptionDoc opt_Channels("_TexConv", "-r;-rg;-rgb;-rgba", "inX.rgba",
  "\
  Specifies how many output channels are used (1 - 4) and from which input image to take the data.\n\
  Examples:\n\
  -rgba in0 -> Output has 4 channels, all taken from input image 0.\n\
  -rgb in0 -> Output has 3 channels, all taken from input image 0.\n\
  -rgb in0 -a in1.r -> Output has 4 channels, RGB taken from input image 0 (RGB) Alpha taken from input 1 (Red).\n\
  -rgb in0.bgr -> Output has 3 channels, taken from image 0 and swapped blue and red.\n\
  -r in0.r -g in1.r -b in2.r -a in3.r -> Output has 4 channels, each one taken from another input image (Red).\n\
  -rgb0 in0 -rgb1 in1 -rgb2 in2 -rgb3 in3 -rgb4 in4 -rgb5 in5 -> Output has 3 channels and six faces (-type Cubemap), built from 6 images.\n\
",
  "");

WCommandLineOptionBool opt_MipsPreserveCoverage("_TexConv", "-mipsPreserveCoverage", "Whether to preserve alpha-coverage in mipmaps for alpha-tested geometry.", false);

WCommandLineOptionBool opt_FlipHorz("_TexConv", "-flip_horz", "Whether to flip the output horizontally.", false);

WCommandLineOptionBool opt_Dilate("_TexConv", "-dilate", "Dilate/smear color from opaque areas into transparent areas.", false);

WCommandLineOptionInt opt_DilateStrength("_TexConv", "-dilateStrength", "How many pixels to smear the image, if -dilate is enabled.", 8, 1, 255);

WCommandLineOptionBool opt_Premulalpha("_TexConv", "-premulalpha", "Whether to multiply the alpha channel into the RGB channels.", false);

WCommandLineOptionInt opt_ThumbnailRes("_TexConv", "-thumbnailRes", "Thumbnail resolution. Should be a power-of-two.", 0, 32, 1024);

WCommandLineOptionInt opt_GridX("_TexConv", "-gridX", "Into how many columns the input images are subdivided.", 1, 1, 255);

WCommandLineOptionInt opt_GridY("_TexConv", "-gridY", "Into how many rows the input images are subdivided.", 1, 1, 255);

WCommandLineOptionInt opt_GridCell("_TexConv", "-gridCell",
  "\
  Which cell of the grid to process, the rest of the input images is discarded.\n\
  Cells are counted left to right, top to bottom. Only used when -gridX or -gridY is larger than 1.\n\
",
  0, 0, 0xFFFF);

WCommandLineOptionPath opt_ThumbnailOut("_TexConv", "-thumbnailOut",
  "\
  Path to 2D thumbnail file.\n\
  ext = tga, jpg, png\n\
",
  "");

WCommandLineOptionPath opt_AssetInfoOut("_TexConv", "-assetInfoOut",
  "\
  Path to a file that receives information about the generated texture, such as its final resolution and format.\n\
  Written only when the output is an W texture format.\n\
",
  "");

WCommandLineOptionPath opt_LowOut("_TexConv", "-lowOut",
  "\
  Path to low-resolution output file.\n\
  ext = Same as main output\n\
",
  "");

WCommandLineOptionInt opt_LowMips("_TexConv", "-lowMips", "Number of mipmaps to use from main result as low-res data.", 0, 0, 8);

WCommandLineOptionInt opt_MinRes("_TexConv", "-minRes", "The minimum resolution allowed for the output.", 16, 4, 8 * 1024);

WCommandLineOptionInt opt_MaxRes("_TexConv", "-maxRes", "The maximum resolution allowed for the output.", 1024 * 8, 4, 16 * 1024);

WCommandLineOptionInt opt_Downscale("_TexConv", "-downscale", "How often to half the input texture resolution.", 0, 0, 10);

WCommandLineOptionFloat opt_MipsAlphaThreshold("_TexConv", "-mipsAlphaThreshold", "Alpha threshold used by renderer for alpha-testing, when alpha-coverage should be preserved. Should match the MaskThreshold of the material that uses the texture.", 0.25f, 0.01f, 0.99f);

WCommandLineOptionFloat opt_HdrExposure("_TexConv", "-hdrExposure", "For scaling HDR image brightness up or down.", 0.0f, -20.0f, +20.0f);

WCommandLineOptionFloat opt_Clamp("_TexConv", "-clamp", "Input values will be clamped to [-value ; +value].", 64000.0f, -64000.0f, 64000.0f);

WCommandLineOptionInt opt_AssetVersion("_TexConv", "-assetVersion", "Asset version number to embed in W specific output formats", 0, 1, 0xFFFF);

WCommandLineOptionString opt_AssetHashLow("_TexConv", "-assetHashLow", "Low part of a 64 bit asset hash value.\n\
Has to be specified as a HEX value.\n\
Required to be non-zero when using W specific output formats.\n\
Example: -assetHashLow 0xABCDABCD",
  "");

WCommandLineOptionString opt_AssetHashHigh("_TexConv", "-assetHashHigh", "High part of a 64 bit asset hash value.\n\
Has to be specified as a HEX value.\n\
Required to be non-zero when using W specific output formats.\n\
Example: -assetHashHigh 0xABCDABCD",
  "");

WCommandLineOptionEnum opt_Type("_TexConv", "-type", "The type of output to generate.", "2D = 1 | Volume = 2 | Cubemap = 3 | Atlas = 4 | Texture2DArray = 5", 1);

WCommandLineOptionEnum opt_Compression("_TexConv", "-compression", "Compression strength for output format.", "Medium = 1 | High = 2 | None = 0", 1);

WCommandLineOptionEnum opt_Usage("_TexConv", "-usage", "What type of data the image contains. Affects which final output format is used and how mipmaps are generated.", "Auto = 0 | Color = 1 | Linear = 2 | HDR = 3 | NormalMap = 4 | NormalMap_Inverted = 5 | BumpMap = 6", 0);

WCommandLineOptionEnum opt_Mipmaps("_TexConv", "-mipmaps", "Whether to generate mipmaps and with which algorithm.", "None = 0 |Linear = 1 | Kaiser = 2", 1);

WCommandLineOptionEnum opt_AddressU("_TexConv", "-addressU", "Which texture address mode to use along U. Only supported by W-specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);
WCommandLineOptionEnum opt_AddressV("_TexConv", "-addressV", "Which texture address mode to use along V. Only supported by W-specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);
WCommandLineOptionEnum opt_AddressW("_TexConv", "-addressW", "Which texture address mode to use along W. Only supported by W-specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);

WCommandLineOptionEnum opt_Filter("_TexConv", "-filter", "Which texture filter mode to use at runtime. Only supported by W-specific output formats.", "Default = 9 | Lowest = 7 | Low = 8 | High = 10 | Highest = 11 | Nearest = 0 | Bilinear = 1 | Trilinear = 2 | Aniso2x = 3 | Aniso4x = 4 | Aniso8x = 5 | Aniso16x = 6", 9);

WCommandLineOptionEnum opt_BumpMapFilter("_TexConv", "-bumpMapFilter", "Filter used to approximate the x/y bump map gradients.", "Finite = 0 | Sobel = 1 | Scharr = 2", 0);

WCommandLineOptionEnum opt_Platform("_TexConv", "-platform", "What platform to generate the textures for.", "PC | Android", 0);

WCommandLineOptionString opt_CompareHtmlTitle("_TexConv", "-cmpHtml", "Title for the compare result HTML. If empty no HTML file is written.", "");
WCommandLineOptionPath opt_CompareActual("_TexConv", "-cmpImg", "Path to an image to compare with another.", "");
WCommandLineOptionPath opt_CompareExpected("_TexConv", "-cmpRef", "Path to a reference image to compare against.", "");
WCommandLineOptionInt opt_CompareThreshold("_TexConv", "-cmpMSE", "The error threshold for the comparison to be considered as failed.\n\
  No output files are written, if the image difference is below this value.",
  100, 0);
WCommandLineOptionBool opt_CompareRelaxed("_TexConv", "-cmpRelaxed", "Use a more lenient comparison method.\nUseful for images with single-pixel wide rasterized lines.", false);


WResult WTexConv::ParseCommandLine()
{
  if (WCommandLineOption::LogAvailableOptions(WCommandLineOption::LogAvailableModes::IfHelpRequested, "_TexConv"))
    return W_FAILURE;

  W_SUCCEED_OR_RETURN(ParseMode());

  if (m_Mode == WTexConvMode::Compare)
  {
    W_SUCCEED_OR_RETURN(ParseCompareMode());
  }
  else if (m_Mode == WTexConvMode::Reduce)
  {
    W_SUCCEED_OR_RETURN(ParseReduceMode());
  }
  else
  {
    W_SUCCEED_OR_RETURN(ParseOutputFiles());
    W_SUCCEED_OR_RETURN(DetectOutputFormat());

    W_SUCCEED_OR_RETURN(ParseOutputType());
    W_SUCCEED_OR_RETURN(ParseAssetHeader());
    W_SUCCEED_OR_RETURN(ParseTargetPlatform());
    W_SUCCEED_OR_RETURN(ParseCompressionMode());
    W_SUCCEED_OR_RETURN(ParseUsage());
    W_SUCCEED_OR_RETURN(ParseMipmapMode());
    W_SUCCEED_OR_RETURN(ParseWrapModes());
    W_SUCCEED_OR_RETURN(ParseFilterModes());
    W_SUCCEED_OR_RETURN(ParseResolutionModifiers());
    W_SUCCEED_OR_RETURN(ParseMiscOptions());
    W_SUCCEED_OR_RETURN(ParseInputFiles());
    W_SUCCEED_OR_RETURN(ParseChannelMappings());
    W_SUCCEED_OR_RETURN(ParseBumpMapFilter());
  }

  return W_SUCCESS;
}

WResult WTexConv::ParseMode()
{
  switch (opt_Mode.GetOptionValue(WCommandLineOption::LogMode::FirstTime))
  {
    case 0:
      m_Mode = WTexConvMode::Convert;
      return W_SUCCESS;

    case 1:
      m_Mode = WTexConvMode::Compare;
      return W_SUCCESS;

    case 2:
      m_Mode = WTexConvMode::Reduce;
      return W_SUCCESS;
  }

  WLog::Error("Invalid mode selected.");
  return W_FAILURE;
}

WResult WTexConv::ParseCompareMode()
{
  m_sOutputFile = opt_Out.GetOptionValue(WCommandLineOption::LogMode::Always);

  if (m_sOutputFile.IsEmpty())
  {
    WLog::Warning("Output path is not specified. Use option '-out \"path\"' to set the prefix path for the output files.");
  }

  m_sHtmlTitle = opt_CompareHtmlTitle.GetOptionValue(WCommandLineOption::LogMode::FirstTime);

  WStringBuilder tmp, res;
  const auto pCmd = WCommandLineUtils::GetGlobalInstance();

  m_Comparer.m_Descriptor.m_sActualFile = opt_CompareActual.GetOptionValue(WCommandLineOption::LogMode::FirstTime);
  m_Comparer.m_Descriptor.m_sExpectedFile = opt_CompareExpected.GetOptionValue(WCommandLineOption::LogMode::FirstTime);
  m_Comparer.m_Descriptor.m_MeanSquareErrorThreshold = opt_CompareThreshold.GetOptionValue(WCommandLineOption::LogMode::FirstTime);
  m_Comparer.m_Descriptor.m_bRelaxedComparison = opt_CompareRelaxed.GetOptionValue(WCommandLineOption::LogMode::FirstTime);

  if (m_Comparer.m_Descriptor.m_sActualFile.IsEmpty())
  {
    WLog::Error("Image to compare is not specified.");
    return W_FAILURE;
  }

  if (m_Comparer.m_Descriptor.m_sExpectedFile.IsEmpty())
  {
    WLog::Error("Reference image to compare against is not specified.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WTexConv::ParseReduceMode()
{
  const auto pCmd = WCommandLineUtils::GetGlobalInstance();

  m_sReduceInputFile = pCmd->GetAbsolutePathOption("-in");

  if (m_sReduceInputFile.IsEmpty())
  {
    WLog::Error("No input file was specified. Use '-in \"path/to/file.dds\"' or '-in \"path/to/file.tga\"' to specify the input file.");
    return W_FAILURE;
  }

  // -out is optional; if not given the output path is derived from the input in RunReduce()
  m_sOutputFile = opt_Out.GetOptionValue(WCommandLineOption::LogMode::Always);

  m_bDeleteSource = opt_DeleteSource.GetOptionValue(WCommandLineOption::LogMode::Always);

  return W_SUCCESS;
}

WResult WTexConv::ParseOutputType()
{
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = WTexConvOutputType::None;
    return W_SUCCESS;
  }

  WInt32 value = opt_Type.GetOptionValue(WCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_OutputType = static_cast<WTexConvOutputType::Enum>(value);

  if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Texture2D)
  {
    if (!m_bOutputSupports2D)
    {
      WLog::Error("2D textures are not supported by the chosen output file format.");
      return W_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Cubemap)
  {
    if (!m_bOutputSupportsCube)
    {
      WLog::Error("Cubemap textures are not supported by the chosen output file format.");
      return W_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Atlas)
  {
    if (!m_bOutputSupportsAtlas)
    {
      WLog::Error("Atlas textures are not supported by the chosen output file format.");
      return W_FAILURE;
    }

    if (!ParseFile("-atlasDesc", m_Processor.m_Descriptor.m_sTextureAtlasDescFile))
      return W_FAILURE;
  }
  else if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Volume)
  {
    if (!m_bOutputSupports3D)
    {
      WLog::Error("Volume textures are not supported by the chosen output file format.");
      return W_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Texture2DArray)
  {
    if (!m_bOutputSupports2D)
    {
      WLog::Error("2D array textures are not supported by the chosen output file format.");
      return W_FAILURE;
    }
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WTexConv::ParseInputFiles()
{
  if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Atlas)
    return W_SUCCESS;

  WStringBuilder tmp, res;
  const auto pCmd = WCommandLineUtils::GetGlobalInstance();

  auto& files = m_Processor.m_Descriptor.m_InputFiles;

  for (WUInt32 i = 0; i < 64; ++i)
  {
    tmp.SetFormat("-in{0}", i);

    res = pCmd->GetAbsolutePathOption(tmp);

    // stop once an option was not found
    if (res.IsEmpty())
      break;

    files.EnsureCount(i + 1);
    files[i] = res;
  }

  // if no numbered inputs were given, try '-in', ignore it otherwise
  if (files.IsEmpty())
  {
    // short version for -in1
    res = pCmd->GetAbsolutePathOption("-in");

    if (!res.IsEmpty())
    {
      files.PushBack(res);
    }
  }

  if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Cubemap)
  {
    // 0 = +X = Right
    // 1 = -X = Left
    // 2 = +Y = Top
    // 3 = -Y = Bottom
    // 4 = +Z = Front
    // 5 = -Z = Back

    if (files.IsEmpty() && (pCmd->GetOptionIndex("-right") != -1 || pCmd->GetOptionIndex("-px") != -1))
    {
      files.SetCount(6);

      files[0] = pCmd->GetAbsolutePathOption("-right", 0, files[0]);
      files[1] = pCmd->GetAbsolutePathOption("-left", 0, files[1]);
      files[2] = pCmd->GetAbsolutePathOption("-top", 0, files[2]);
      files[3] = pCmd->GetAbsolutePathOption("-bottom", 0, files[3]);
      files[4] = pCmd->GetAbsolutePathOption("-front", 0, files[4]);
      files[5] = pCmd->GetAbsolutePathOption("-back", 0, files[5]);

      files[0] = pCmd->GetAbsolutePathOption("-px", 0, files[0]);
      files[1] = pCmd->GetAbsolutePathOption("-nx", 0, files[1]);
      files[2] = pCmd->GetAbsolutePathOption("-py", 0, files[2]);
      files[3] = pCmd->GetAbsolutePathOption("-ny", 0, files[3]);
      files[4] = pCmd->GetAbsolutePathOption("-pz", 0, files[4]);
      files[5] = pCmd->GetAbsolutePathOption("-nz", 0, files[5]);
    }
  }

  for (WUInt32 i = 0; i < files.GetCount(); ++i)
  {
    if (files[i].IsEmpty())
    {
      WLog::Error("Input file {} is not specified", i);
      return W_FAILURE;
    }

    WLog::Info("Input file {}: '{}'", i, files[i]);
  }

  if (m_Processor.m_Descriptor.m_InputFiles.IsEmpty())
  {
    WLog::Error("No input files were specified. Use \'-in \"path/to/file\"' to specify an input file. Use '-in0', '-in1' etc. to specify "
                 "multiple input files.");
    return W_FAILURE;
  }

  m_Processor.m_Descriptor.m_uiSourceGridX = static_cast<WUInt8>(opt_GridX.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified));
  m_Processor.m_Descriptor.m_uiSourceGridY = static_cast<WUInt8>(opt_GridY.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified));
  m_Processor.m_Descriptor.m_uiSourceGridCell = static_cast<WUInt16>(opt_GridCell.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified));

  return W_SUCCESS;
}

WResult WTexConv::ParseOutputFiles()
{
  m_sOutputFile = opt_Out.GetOptionValue(WCommandLineOption::LogMode::Always);

  m_sOutputThumbnailFile = opt_ThumbnailOut.GetOptionValue(WCommandLineOption::LogMode::Always);

  if (!m_sOutputThumbnailFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_uiThumbnailOutputResolution = opt_ThumbnailRes.GetOptionValue(WCommandLineOption::LogMode::Always);
  }

  m_sOutputAssetInfoFile = opt_AssetInfoOut.GetOptionValue(WCommandLineOption::LogMode::Always);

  m_sOutputLowResFile = opt_LowOut.GetOptionValue(WCommandLineOption::LogMode::Always);

  if (!m_sOutputLowResFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_uiLowResMipmaps = opt_LowMips.GetOptionValue(WCommandLineOption::LogMode::Always);
  }

  return W_SUCCESS;
}

WResult WTexConv::ParseUsage()
{
  if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Atlas)
    return W_SUCCESS;

  const WInt32 value = opt_Usage.GetOptionValue(WCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_Usage = static_cast<WTexConvUsage::Enum>(value);
  return W_SUCCESS;
}

WResult WTexConv::ParseMipmapMode()
{
  if (!m_bOutputSupportsMipmaps)
  {
    WLog::Info("Selected output format does not support -mipmap options.");

    m_Processor.m_Descriptor.m_MipmapMode = WTexConvMipmapMode::None;
    return W_SUCCESS;
  }

  const WInt32 value = opt_Mipmaps.GetOptionValue(WCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_MipmapMode = static_cast<WTexConvMipmapMode::Enum>(value);

  m_Processor.m_Descriptor.m_bPreserveMipmapCoverage = opt_MipsPreserveCoverage.GetOptionValue(WCommandLineOption::LogMode::Always);

  if (m_Processor.m_Descriptor.m_bPreserveMipmapCoverage)
  {
    m_Processor.m_Descriptor.m_fMipmapAlphaThreshold = opt_MipsAlphaThreshold.GetOptionValue(WCommandLineOption::LogMode::Always);
  }

  return W_SUCCESS;
}

WResult WTexConv::ParseTargetPlatform()
{
  WInt32 value = opt_Platform.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);

  m_Processor.m_Descriptor.m_TargetPlatform = static_cast<WTexConvTargetPlatform::Enum>(value);
  return W_SUCCESS;
}

WResult WTexConv::ParseCompressionMode()
{
  if (!m_bOutputSupportsCompression)
  {
    WLog::Info("Selected output format does not support -compression options.");

    m_Processor.m_Descriptor.m_CompressionMode = WTexConvCompressionMode::None;
    return W_SUCCESS;
  }

  const WInt32 value = opt_Compression.GetOptionValue(WCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_CompressionMode = static_cast<WTexConvCompressionMode::Enum>(value);
  return W_SUCCESS;
}

WResult WTexConv::ParseWrapModes()
{
  // cubemaps do not require any wrap mode settings
  if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Cubemap || m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Atlas || m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::None)
    return W_SUCCESS;

  {
    WInt32 value = opt_AddressU.GetOptionValue(WCommandLineOption::LogMode::Always);
    m_Processor.m_Descriptor.m_AddressModeU = static_cast<WImageAddressMode::Enum>(value);
  }
  {
    WInt32 value = opt_AddressV.GetOptionValue(WCommandLineOption::LogMode::Always);
    m_Processor.m_Descriptor.m_AddressModeV = static_cast<WImageAddressMode::Enum>(value);
  }

  if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Volume)
  {
    WInt32 value = opt_AddressW.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);
    m_Processor.m_Descriptor.m_AddressModeW = static_cast<WImageAddressMode::Enum>(value);
  }

  return W_SUCCESS;
}

WResult WTexConv::ParseFilterModes()
{
  if (!m_bOutputSupportsFiltering)
  {
    WLog::Info("Selected output format does not support -filter options.");
    return W_SUCCESS;
  }

  WInt32 value = opt_Filter.GetOptionValue(WCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_FilterMode = static_cast<WTextureFilterSetting::Enum>(value);
  return W_SUCCESS;
}

WResult WTexConv::ParseResolutionModifiers()
{
  if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::None)
    return W_SUCCESS;

  m_Processor.m_Descriptor.m_uiMinResolution = opt_MinRes.GetOptionValue(WCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_uiMaxResolution = opt_MaxRes.GetOptionValue(WCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_uiDownscaleSteps = opt_Downscale.GetOptionValue(WCommandLineOption::LogMode::Always);

  return W_SUCCESS;
}

WResult WTexConv::ParseMiscOptions()
{
  if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Texture2D || m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::None)
  {
    m_Processor.m_Descriptor.m_bFlipHorizontal = opt_FlipHorz.GetOptionValue(WCommandLineOption::LogMode::Always);

    m_Processor.m_Descriptor.m_bPremultiplyAlpha = opt_Premulalpha.GetOptionValue(WCommandLineOption::LogMode::Always);

    if (opt_Dilate.GetOptionValue(WCommandLineOption::LogMode::Always))
    {
      m_Processor.m_Descriptor.m_uiDilateColor = static_cast<WUInt8>(opt_DilateStrength.GetOptionValue(WCommandLineOption::LogMode::Always));
    }
  }

  if (m_Processor.m_Descriptor.m_Usage == WTexConvUsage::Hdr)
  {
    m_Processor.m_Descriptor.m_fHdrExposureBias = opt_HdrExposure.GetOptionValue(WCommandLineOption::LogMode::Always);
  }

  m_Processor.m_Descriptor.m_fMaxValue = opt_Clamp.GetOptionValue(WCommandLineOption::LogMode::Always);

  return W_SUCCESS;
}

WResult WTexConv::ParseAssetHeader()
{
  const WStringView ext = WPathUtils::GetFileExtension(m_sOutputFile);

  if (!ext.StartsWith_NoCase("W"))
    return W_SUCCESS;

  m_Processor.m_Descriptor.m_uiAssetVersion = (WUInt16)opt_AssetVersion.GetOptionValue(WCommandLineOption::LogMode::Always);

  WUInt32 uiHashLow = 0;
  WUInt32 uiHashHigh = 0;
  if (WConversionUtils::ConvertHexStringToUInt32(opt_AssetHashLow.GetOptionValue(WCommandLineOption::LogMode::Always), uiHashLow).Failed() ||
      WConversionUtils::ConvertHexStringToUInt32(opt_AssetHashHigh.GetOptionValue(WCommandLineOption::LogMode::Always), uiHashHigh).Failed())
  {
    WLog::Error("'-assetHashLow 0xHEX32' and '-assetHashHigh 0xHEX32' have not been specified correctly.");
    return W_FAILURE;
  }

  m_Processor.m_Descriptor.m_uiAssetHash = (static_cast<WUInt64>(uiHashHigh) << 32) | static_cast<WUInt64>(uiHashLow);

  if (m_Processor.m_Descriptor.m_uiAssetHash == 0)
  {
    WLog::Error("'-assetHashLow 0xHEX32' and '-assetHashHigh 0xHEX32' have not been specified correctly.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WTexConv::ParseBumpMapFilter()
{
  const WInt32 value = opt_BumpMapFilter.GetOptionValue(WCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_BumpMapFilter = static_cast<WTexConvBumpMapFilter::Enum>(value);
  return W_SUCCESS;
}
