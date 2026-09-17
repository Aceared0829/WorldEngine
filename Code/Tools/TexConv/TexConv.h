#pragma once

#include <Foundation/Application/Application.h>
#include <Texture/TexConv/TexComparer.h>

class WStreamWriter;

struct WTexConvMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Convert,
    Compare,
    Reduce, ///< Loads a single DDS or TGA file and saves it as JPG (no alpha) or PNG (has alpha), without any processing.

    Default = Convert
  };
};

class WTexConv : public WApplication
{
public:
  using SUPER = WApplication;

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(WStringView sKey, WInt32 iVal)
      : m_sKey(sKey)
      , m_iEnumValue(iVal)
    {
    }

    WStringView m_sKey;
    WInt32 m_iEnumValue = -1;
  };

  WTexConv();

public:
  virtual void Run() override;
  virtual WResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  WResult ParseCommandLine();
  WResult ParseMode();
  WResult ParseCompareMode();
  WResult ParseReduceMode();
  WResult ParseOutputType();
  WResult DetectOutputFormat();
  WResult ParseInputFiles();
  WResult ParseOutputFiles();
  WResult ParseChannelMappings();
  WResult ParseChannelSliceMapping(WInt32 iSlice);
  WResult ParseChannelMappingConfig(WTexConvChannelMapping& out_mapping, WStringView sCfg, WInt32 iChannelIndex, bool bSingleChannel);
  WResult ParseUsage();
  WResult ParseMipmapMode();
  WResult ParseTargetPlatform();
  WResult ParseCompressionMode();
  WResult ParseWrapModes();
  WResult ParseFilterModes();
  WResult ParseResolutionModifiers();
  WResult ParseMiscOptions();
  WResult ParseAssetHeader();
  WResult ParseBumpMapFilter();

  WResult ParseUIntOption(WStringView sOption, WInt32 iMinValue, WInt32 iMaxValue, WUInt32& ref_uiResult) const;
  WResult ParseStringOption(WStringView sOption, const WDynamicArray<KeyEnumValuePair>& allowed, WInt32& ref_iResult) const;
  void PrintOptionValues(WStringView sOption, const WDynamicArray<KeyEnumValuePair>& allowed) const;
  void PrintOptionValuesHelp(WStringView sOption, const WDynamicArray<KeyEnumValuePair>& allowed) const;
  bool ParseFile(WStringView sOption, WString& ref_sResult) const;

  bool IsTexFormat() const;
  WResult WriteTexFile(WStreamWriter& inout_stream, const WImage& image);
  WResult WriteOutputFile(WStringView sFile, const WImage& image);
  WResult RunReduce();
  WResult ReduceSingleFile(WStringView sInputFile, WStringView sOutputDir, WStringView sExplicitOutputFile = {});

private:
  WString m_sOutputFile;
  WString m_sOutputThumbnailFile;
  WString m_sOutputAssetInfoFile;
  WString m_sOutputLowResFile;
  WString m_sReduceInputFile;
  bool m_bDeleteSource = false;

  bool m_bOutputSupports2D = false;
  bool m_bOutputSupports3D = false;
  bool m_bOutputSupportsCube = false;
  bool m_bOutputSupportsAtlas = false;
  bool m_bOutputSupportsMipmaps = false;
  bool m_bOutputSupportsFiltering = false;
  bool m_bOutputSupportsCompression = false;

  WEnum<WTexConvMode> m_Mode;
  WTexConvProcessor m_Processor;

  // Comparer specific

  WTexComparer m_Comparer;
  WString m_sHtmlTitle;
};
