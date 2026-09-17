#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAsset.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

#include <qsettings.h>
#include <qxmlstream.h>

namespace
{
  WResult GetSbsContent(WStringView sAbsolutePath, WStringBuilder& out_sContent)
  {
    WFileReader fileReader;
    W_SUCCEED_OR_RETURN(fileReader.Open(sAbsolutePath));

    out_sContent.ReadAll(fileReader);
    return W_SUCCESS;
  }

  WResult ReadUntilStartElement(QXmlStreamReader& inout_reader, const char* szName)
  {
    while (inout_reader.atEnd() == false)
    {
      auto tokenType = inout_reader.readNext();
      W_IGNORE_UNUSED(tokenType);

      if (inout_reader.isStartElement() && inout_reader.name() == QLatin1StringView(szName))
        return W_SUCCESS;
    }

    return W_FAILURE;
  }

  WResult ReadUntilEndElement(QXmlStreamReader& inout_reader, const char* szName)
  {
    while (inout_reader.atEnd() == false)
    {
      auto tokenType = inout_reader.readNext();
      W_IGNORE_UNUSED(tokenType);

      if (inout_reader.isEndElement() && inout_reader.name() == QLatin1StringView(szName))
        return W_SUCCESS;
    }

    return W_FAILURE;
  }

  template <typename T>
  T GetValueAttribute(QXmlStreamReader& inout_reader)
  {
    WString s(inout_reader.attributes().value("v").toUtf8().data());
    if constexpr (std::is_same_v<T, WString>)
    {
      return s;
    }
    else
    {
      WVariant v = s;
      return v.ConvertTo<T>();
    }
  }

  static const char* s_szSubstanceUsageMapping[] = {
    "",                 // Unknown,

    "baseColor",        // BaseColor,
    "emissive",         // Emissive,
    "height",           // Height,
    "metallic",         // Metallic,
    "mask",             // Mask,
    "normal",           // Normal,
    "ambientOcclusion", // Occlusion,
    "opacity",          // Opacity,
    "roughness",        // Roughness,
  };

  static_assert(W_ARRAY_SIZE(s_szSubstanceUsageMapping) == WSubstanceUsage::Count);

  static WUInt8 s_substanceNumChannelsMapping[] = {
    1, // Unknown,

    3, // BaseColor,
    3, // Emissive,
    1, // Height,
    1, // Metallic,
    1, // Mask,
    3, // Normal,
    1, // Occlusion,
    1, // Opacity,
    1, // Roughness,
  };

  static_assert(W_ARRAY_SIZE(s_szSubstanceUsageMapping) == WSubstanceUsage::Count);


  WSubstanceUsage::Enum GetUsage(QXmlStreamReader& inout_reader)
  {
    WString s = GetValueAttribute<WString>(inout_reader);
    for (WUInt32 i = 0; i < WSubstanceUsage::Count; ++i)
    {
      if (s.IsEqual_NoCase(s_szSubstanceUsageMapping[i]))
      {
        return static_cast<WSubstanceUsage::Enum>(i);
      }
    }

    return WSubstanceUsage::Unknown;
  }

  WResult ParseGraphOutput(QXmlStreamReader& inout_reader, WUInt32 uiGraphUid, WSubstanceGraphOutput& out_graphOutput)
  {
    W_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("graphoutput"), "");

    while (inout_reader.readNextStartElement())
    {
      if (inout_reader.name() == QLatin1StringView("identifier"))
      {
        out_graphOutput.m_sName = GetValueAttribute<WString>(inout_reader);
      }
      else if (inout_reader.name() == QLatin1StringView("uid"))
      {
        WUInt32 outputUid = GetValueAttribute<WUInt32>(inout_reader);
        WUInt64 seed = WUInt64(uiGraphUid) << 32ull | outputUid;
        out_graphOutput.m_Uuid = WUuid::MakeStableUuidFromInt(seed);
      }
      else if (inout_reader.name() == QLatin1StringView("attributes"))
      {
        if (ReadUntilStartElement(inout_reader, "label").Succeeded())
        {
          out_graphOutput.m_sLabel = GetValueAttribute<WString>(inout_reader);
          W_SUCCEED_OR_RETURN(ReadUntilEndElement(inout_reader, "label"));
        }
      }
      else if (inout_reader.name() == QLatin1StringView("usages"))
      {
        if (ReadUntilStartElement(inout_reader, "name").Succeeded())
        {
          out_graphOutput.m_Usage = GetUsage(inout_reader);
          out_graphOutput.m_uiNumChannels = s_substanceNumChannelsMapping[out_graphOutput.m_Usage];
          W_SUCCEED_OR_RETURN(ReadUntilEndElement(inout_reader, "usage"));
        }
      }

      inout_reader.skipCurrentElement();
    }

    return W_SUCCESS;
  }

  struct Option
  {
    WString m_sName;
    WString m_sValue;
  };

  WResult ParseOption(QXmlStreamReader& inout_reader, Option& out_option)
  {
    W_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("option"), "");

    while (inout_reader.readNextStartElement())
    {
      if (inout_reader.name() == QLatin1StringView("name"))
      {
        out_option.m_sName = GetValueAttribute<WString>(inout_reader);
      }
      else if (inout_reader.name() == QLatin1StringView("value"))
      {
        out_option.m_sValue = GetValueAttribute<WString>(inout_reader);
      }

      inout_reader.skipCurrentElement();
    }

    return W_SUCCESS;
  }

  WResult ParseGraph(QXmlStreamReader& inout_reader, WSubstanceGraph& out_graph)
  {
    W_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("graph"), "");

    WUInt32 uiGraphUid = 0;
    while (inout_reader.readNextStartElement())
    {
      if (inout_reader.name() == QLatin1StringView("identifier"))
      {
        out_graph.m_sName = GetValueAttribute<WString>(inout_reader);
        inout_reader.skipCurrentElement();
      }
      else if (inout_reader.name() == QLatin1StringView("uid"))
      {
        uiGraphUid = GetValueAttribute<WUInt32>(inout_reader);
        inout_reader.skipCurrentElement();
      }
      else if (inout_reader.name() == QLatin1StringView("graphOutputs"))
      {
        while (inout_reader.readNextStartElement())
        {
          if (inout_reader.name() == QLatin1StringView("graphoutput"))
          {
            auto& graphOutput = out_graph.m_Outputs.ExpandAndGetRef();
            W_SUCCEED_OR_RETURN(ParseGraphOutput(inout_reader, uiGraphUid, graphOutput));
          }
        }
      }
      else if (inout_reader.name() == QLatin1StringView("options"))
      {
        Option option;
        while (inout_reader.readNextStartElement())
        {
          if (inout_reader.name() == QLatin1StringView("option"))
          {
            W_SUCCEED_OR_RETURN(ParseOption(inout_reader, option));

            if (option.m_sName == "defaultParentSize")
            {
              WStringView sValue = option.m_sValue;
              WUInt32 tmp = 0;
              const char* szLastPos = nullptr;
              W_SUCCEED_OR_RETURN(WConversionUtils::StringToUInt(sValue, tmp, &szLastPos));
              out_graph.m_uiOutputWidth = static_cast<WUInt8>(tmp);

              if (*szLastPos != 'x')
                return W_FAILURE;

              sValue = WStringView(szLastPos + 1);
              W_SUCCEED_OR_RETURN(WConversionUtils::StringToUInt(sValue, tmp));
              out_graph.m_uiOutputHeight = static_cast<WUInt8>(tmp);
            }
            else if (option.m_sName.StartsWith("export/fromGraph/outputs/"))
            {
              const char* szLastSlash = option.m_sName.FindLastSubString("/");
              WStringView sOutputIdentifier = WStringView(szLastSlash + 1);

              bool bEnabled = false;
              W_SUCCEED_OR_RETURN(WConversionUtils::StringToBool(option.m_sValue, bEnabled));

              for (auto& output : out_graph.m_Outputs)
              {
                if (output.m_sName == sOutputIdentifier)
                {
                  output.m_bEnabled = bEnabled;
                }
              }
            }
          }
        }
      }
      else
      {
        inout_reader.skipCurrentElement();
      }
    }

    return W_SUCCESS;
  }

  WStringView AddDependency(WStringView sDependency, WStringView sSbsDir, WSet<WString>& out_dependencies)
  {
    WStringBuilder sFullPath;
    if (sDependency.IsAbsolutePath())
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }
    else
    {
      sFullPath = sSbsDir;
      sFullPath.AppendPath(sDependency);
      sFullPath.MakeCleanPath();
    }

    if (out_dependencies.Contains(sFullPath) == false)
    {
      auto it = out_dependencies.Insert(sFullPath);
      return it.Key();
    }

    return "";
  };

  WResult ReadExternalCopy(QXmlStreamReader& inout_reader, WString& out_sExternalCopy)
  {
    W_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("source"), "");

    W_SUCCEED_OR_RETURN(ReadUntilStartElement(inout_reader, "externalcopy"));
    W_SUCCEED_OR_RETURN(ReadUntilStartElement(inout_reader, "filename"));

    out_sExternalCopy = GetValueAttribute<WString>(inout_reader);

    W_SUCCEED_OR_RETURN(ReadUntilEndElement(inout_reader, "source"));
    return W_SUCCESS;
  }

  WResult ReadResources(QXmlStreamReader& inout_reader, WStringView sSbsDir, WSet<WString>& out_dependencies)
  {
    W_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("content"), "");

    while (inout_reader.readNextStartElement())
    {
      if (inout_reader.name() == QLatin1StringView("resource"))
      {
        WString sFilePath;
        WString sExternalCopy;

        while (inout_reader.readNextStartElement())
        {
          if (inout_reader.name() == QLatin1StringView("filepath"))
          {
            sFilePath = GetValueAttribute<WString>(inout_reader);
            inout_reader.skipCurrentElement();
          }
          else if (inout_reader.name() == QLatin1StringView("source"))
          {
            W_SUCCEED_OR_RETURN(ReadExternalCopy(inout_reader, sExternalCopy));
          }
          else
          {
            inout_reader.skipCurrentElement();
          }
        }

        if (sExternalCopy.IsEmpty() == false)
        {
          sFilePath = sExternalCopy;
        }

        AddDependency(sFilePath, sSbsDir, out_dependencies);
      }
      else if (inout_reader.name() == QLatin1StringView("resourceScene"))
      {
        if (ReadUntilStartElement(inout_reader, "filepath").Failed())
          continue;

        WString sFilePath = GetValueAttribute<WString>(inout_reader);
        AddDependency(sFilePath, sSbsDir, out_dependencies);

        W_SUCCEED_OR_RETURN(ReadUntilEndElement(inout_reader, "resourceScene"));
      }
      else if (inout_reader.name() == QLatin1StringView("group"))
      {
        W_SUCCEED_OR_RETURN(ReadUntilStartElement(inout_reader, "content"));

        W_SUCCEED_OR_RETURN(ReadResources(inout_reader, sSbsDir, out_dependencies));

        W_SUCCEED_OR_RETURN(ReadUntilEndElement(inout_reader, "group"));
      }
      else
      {
        inout_reader.skipCurrentElement();
      }
    }

    return W_SUCCESS;
  }

  WResult ReadDependencies(WStringView sSbsFile, WSet<WString>& out_dependencies)
  {
    WLogBlock logBlock("ReadDependencies", sSbsFile);

    WStringBuilder sAbsolutePath = sSbsFile;
    if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsolutePath))
    {
      return W_FAILURE;
    }

    WStringView sSbsDir = sSbsFile.GetFileDirectory();

    WStringBuilder sFileContent;
    W_SUCCEED_OR_RETURN(GetSbsContent(sAbsolutePath, sFileContent));

    QXmlStreamReader reader(sFileContent.GetData());
    W_SUCCEED_OR_RETURN(ReadUntilStartElement(reader, "dependencies"));

    while (reader.readNextStartElement())
    {
      if (reader.name() != QLatin1StringView("dependency"))
      {
        reader.skipCurrentElement();
        continue;
      }

      if (ReadUntilStartElement(reader, "filename").Failed())
        continue;

      WString sDependency = GetValueAttribute<WString>(reader);
      if (sDependency.EndsWith(".sbs") && sDependency.StartsWith("sbs://") == false)
      {
        WStringView sAddedPath = AddDependency(sDependency, sSbsDir, out_dependencies);
        if (sAddedPath.IsEmpty() == false)
        {
          W_SUCCEED_OR_RETURN(ReadDependencies(sAddedPath, out_dependencies));
        }
      }

      W_SUCCEED_OR_RETURN(ReadUntilEndElement(reader, "dependency"));
    }

    W_SUCCEED_OR_RETURN(ReadUntilStartElement(reader, "content"));
    W_SUCCEED_OR_RETURN(ReadResources(reader, sSbsDir, out_dependencies));

    return W_SUCCESS;
  }

  WResult GetInstallationPath(WStringBuilder& out_sPath)
  {
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
    static WUntrackedString s_CachedPath;
    if (s_CachedPath.IsEmpty() == false)
    {
      out_sPath = s_CachedPath;
      return W_SUCCESS;
    }

    auto CheckPath = [&](WStringView sPath)
    {
      if (sPath.IsEmpty())
        return false;

      WStringBuilder path = sPath;
      path.AppendPath("sbscooker.exe");

      if (path.IsAbsolutePath() && WOSFile::ExistsFile(path))
      {
        s_CachedPath = sPath;
        out_sPath = sPath;
        return true;
      }

      return false;
    };

    WStringBuilder sPath = "C:/Program Files/Allegorithmic/Substance Designer";
    if (CheckPath(sPath))
    {
      return W_SUCCESS;
    }

    QSettings settings("\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{e9e3d6d9-3023-41c7-b223-11d8fdd691b9}_is1", QSettings::NativeFormat);
    sPath = WStringView(settings.value("InstallLocation").toString().toUtf8());

    if (CheckPath(sPath))
    {
      return W_SUCCESS;
    }

    WLog::Error("Installation of Substance Designer could not be located.");
    return W_FAILURE;
#endif

    return W_FAILURE;
  }

  WStatus RunSbsCooker(const char* szSbsFile, const char* szOutputPath)
  {
    WStringBuilder sToolPath;
    W_SUCCEED_OR_RETURN(GetInstallationPath(sToolPath));
    sToolPath.AppendPath("sbscooker");

    QStringList arguments;

    arguments << "--inputs";
    arguments << szSbsFile;

    arguments << "--output-path";
    arguments << szOutputPath;

    arguments << "--no-optimization";

    W_SUCCEED_OR_RETURN(WQtEditorApp::GetSingleton()->ExecuteTool(sToolPath, arguments, 600, WLog::GetThreadLocalLogSystem(), WLogMsgType::InfoMsg));

    return WStatus(W_SUCCESS);
  }

  WStatus RunSbsRender(const char* szSbsarFile, const char* szGraph, const char* szGraphOutput, const char* szOutputName, const char* szOutputPath, WUInt8 uiOutputWidth, WUInt8 uiOutputHeight)
  {
    WStringBuilder sToolPath;
    W_SUCCEED_OR_RETURN(GetInstallationPath(sToolPath));
    sToolPath.AppendPath("sbsrender");

    WStringBuilder sTmp;

    QStringList arguments;
    arguments << "render";

    arguments << "--engine";
    arguments << "d3d11pc";

    arguments << "--input";
    arguments << szSbsarFile;

    arguments << "--input-graph";
    arguments << szGraph;

    if (WStringUtils::IsNullOrEmpty(szGraphOutput) == false)
    {
      arguments << "--input-graph-output";
      arguments << szGraphOutput;
    }

    if (WStringUtils::IsNullOrEmpty(szOutputName) == false)
    {
      arguments << "--output-name";
      arguments << szOutputName;
    }

    arguments << "--output-path";
    arguments << szOutputPath;

    sTmp.SetFormat("$outputsize@{},{}", uiOutputWidth, uiOutputHeight);
    arguments << "--set-value";
    arguments << sTmp.GetData();

    W_SUCCEED_OR_RETURN(WQtEditorApp::GetSingleton()->ExecuteTool(sToolPath, arguments, 600, WLog::GetThreadLocalLogSystem()));

    return WStatus(W_SUCCESS);
  }

  WStatus WriteOutputSize(WStringView sFilePath, WUInt8 uiOutputWidth, WUInt8 uiOutputHeight)
  {
    WFileWriter writer;
    W_SUCCEED_OR_RETURN(writer.Open(sFilePath));

    WStringBuilder tmp;
    tmp.SetFormat("{}x{}", uiOutputWidth, uiOutputHeight);

    W_SUCCEED_OR_RETURN(writer.WriteBytes(tmp.GetData(), tmp.GetElementCount()));

    return WStatus(W_SUCCESS);
  }

  bool HasOutputSizeChanged(WStringView sFilePath, WUInt8 uiOutputWidth, WUInt8 uiOutputHeight)
  {
    WFileReader reader;
    if (reader.Open(sFilePath).Failed())
      return true;

    WStringBuilder tmp;
    tmp.ReadAll(reader);

    WTempHybridArray<WStringView, 2> sizes;
    tmp.Split(false, sizes, "x");

    if (sizes.GetCount() != 2)
      return true;

    WUInt32 uiSize = 0;
    if (WConversionUtils::StringToUInt(sizes[0], uiSize).Failed() || uiSize != uiOutputWidth)
      return true;

    if (WConversionUtils::StringToUInt(sizes[1], uiSize).Failed() || uiSize != uiOutputHeight)
      return true;

    return false;
  }

  WTimestamp GetModifiedTimestamp(WStringView sFilePath)
  {
    WFileStatus status;
    if (WFileSystemModel::GetSingleton()->FindFile(sFilePath, status).Succeeded())
    {
      return status.m_LastModified;
    }

    WFileStats stats;
    if (sFilePath.IsAbsolutePath() && WOSFile::GetFileStats(sFilePath, stats).Succeeded())
    {
      return stats.m_LastModificationTime;
    }
    else if (WFileSystem::GetFileStats(sFilePath, stats).Succeeded())
    {
      return stats.m_LastModificationTime;
    }

    return WTimestamp();
  }

} // namespace

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSubstanceUsage, 1)
  W_ENUM_CONSTANT(WSubstanceUsage::Unknown),
  W_ENUM_CONSTANT(WSubstanceUsage::BaseColor),
  W_ENUM_CONSTANT(WSubstanceUsage::Emissive),
  W_ENUM_CONSTANT(WSubstanceUsage::Height),
  W_ENUM_CONSTANT(WSubstanceUsage::Metallic),
  W_ENUM_CONSTANT(WSubstanceUsage::Mask),
  W_ENUM_CONSTANT(WSubstanceUsage::Normal),
  W_ENUM_CONSTANT(WSubstanceUsage::Occlusion),
  W_ENUM_CONSTANT(WSubstanceUsage::Opacity),
  W_ENUM_CONSTANT(WSubstanceUsage::Roughness),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WSubstanceGraphOutput, WNoBase, 1, WRTTIDefaultAllocator<WSubstanceGraphOutput>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("Name", m_sName),
    W_MEMBER_PROPERTY("Label", m_sLabel),
    W_ENUM_MEMBER_PROPERTY("Usage", WSubstanceUsage, m_Usage),
    W_MEMBER_PROPERTY("NumChannels", m_uiNumChannels)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 4)),
    W_ENUM_MEMBER_PROPERTY("CompressionMode", WTexConvCompressionMode, m_CompressionMode)->AddAttributes(new WDefaultValueAttribute(WTexConvCompressionMode::High)),
    W_ENUM_MEMBER_PROPERTY("MipmapMode", WTexConvMipmapMode, m_MipmapMode),
    W_MEMBER_PROPERTY("PreserveAlphaCoverage", m_bPreserveAlphaCoverage),
    W_MEMBER_PROPERTY("Uuid", m_Uuid)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WSubstanceGraph, WNoBase, 1, WRTTIDefaultAllocator<WSubstanceGraph>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("Name", m_sName),
    W_MEMBER_PROPERTY("OutputWidth", m_uiOutputWidth)->AddAttributes(new WClampValueAttribute(4, 12)),
    W_MEMBER_PROPERTY("OutputHeight", m_uiOutputHeight)->AddAttributes(new WClampValueAttribute(4, 12)),
    W_ARRAY_MEMBER_PROPERTY("Outputs", m_Outputs),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSubstancePackageAssetProperties, 1, WRTTIDefaultAllocator<WSubstancePackageAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SubstanceFile", m_sSubstancePackage)->AddAttributes(new WFileBrowserAttribute("Select Substance File", "*.sbs"), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("OutputPattern", m_sOutputPattern)->AddAttributes(new WDefaultValueAttribute(WStringView("$(graph)_$(label)"))),
    W_ARRAY_MEMBER_PROPERTY("Graphs", m_Graphs)
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSubstancePackageAssetMetaData, 1, WRTTIDefaultAllocator<WSubstancePackageAssetMetaData>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("OutputUuids", m_OutputUuids),
    W_ARRAY_MEMBER_PROPERTY("OutputNames", m_OutputNames)
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSubstancePackageAssetDocument, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("ChannelMode", WTextureChannelMode, m_ChannelMode),
    W_MEMBER_PROPERTY("TextureLod", m_iTextureLod),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSubstancePackageAssetDocument::WSubstancePackageAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument(sDocumentPath, WAssetDocEngineConnection::Simple)
{
  GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WSubstancePackageAssetDocument::OnPropertyChanged, this));
}

WSubstancePackageAssetDocument::~WSubstancePackageAssetDocument()
{
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WSubstancePackageAssetDocument::OnPropertyChanged, this));
}

void WSubstancePackageAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const WSubstancePackageAssetProperties* pProp = GetProperties();

  // Dependencies
  {
    pInfo->m_TransformDependencies.Insert(pProp->m_sSubstancePackage);
    pInfo->m_ThumbnailDependencies.Insert(pProp->m_sSubstancePackage);

    WSet<WString> dependencies;
    ReadDependencies(pProp->m_sSubstancePackage, dependencies).IgnoreResult();
    pInfo->m_TransformDependencies.Union(dependencies);
    pInfo->m_ThumbnailDependencies.Union(dependencies);
  }

  // Outputs
  {
    WStringBuilder sName;

    auto pMetaData = WGetStaticRTTI<WSubstancePackageAssetMetaData>()->GetAllocator()->Allocate<WSubstancePackageAssetMetaData>();
    for (auto& graph : pProp->m_Graphs)
    {
      if (graph.m_bEnabled == false)
        continue;

      for (auto& output : graph.m_Outputs)
      {
        if (output.m_bEnabled == false)
          continue;

        GenerateOutputName(graph, output, sName);
        if (pMetaData->m_OutputNames.Contains(sName))
        {
          WLog::Error("A substance texture named '{}' already exists.", sName);
          continue;
        }

        pMetaData->m_OutputUuids.PushBack(output.m_Uuid);
        pMetaData->m_OutputNames.PushBack(sName);
      }
    }

    pInfo->m_MetaInfo.PushBack(pMetaData);
  }
}

WTransformStatus WSubstancePackageAssetDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& assetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WStringBuilder sAbsolutePackagePath = GetProperties()->m_sSubstancePackage;
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsolutePackagePath))
  {
    return WStatus(WFmt("Couldn't make path absolute: '{0};", sAbsolutePackagePath));
  }

  W_SUCCEED_OR_RETURN(UpdateGraphOutputs(sAbsolutePackagePath, transformFlags.IsSet(WTransformFlags::BackgroundProcessing) == false));

  WStringBuilder sTempDir;
  W_SUCCEED_OR_RETURN(GetTempDir(sTempDir));
  W_SUCCEED_OR_RETURN(WOSFile::CreateDirectoryStructure(sTempDir));

  WTimestamp latestDependencyTimestamp;
  for (auto& sDependency : GetAssetDocumentInfo()->m_TransformDependencies)
  {
    WTimestamp dependencyTimestamp = GetModifiedTimestamp(sDependency);
    if (dependencyTimestamp.Compare(latestDependencyTimestamp, WTimestamp::CompareMode::Newer))
    {
      latestDependencyTimestamp = dependencyTimestamp;
    }
  }

  WStringView sPackageName = sAbsolutePackagePath.GetFileName();

  WStringBuilder sSbsarPath = sTempDir;
  sSbsarPath.AppendPath(sPackageName);
  sSbsarPath.Append(".sbsar");

  WTimestamp sbsarTimestamp = GetModifiedTimestamp(sSbsarPath);

  if (transformFlags.IsSet(WTransformFlags::ForceTransform) ||
      latestDependencyTimestamp.Compare(sbsarTimestamp, WTimestamp::CompareMode::Newer))
  {
    W_SUCCEED_OR_RETURN(RunSbsCooker(sAbsolutePackagePath, sTempDir));

    sbsarTimestamp = WTimestamp::CurrentTimestamp();
  }

  WStringBuilder sOutputName, sPngPath, sTargetFile, sOutputSizeFilePath;
  auto& textureTypeDesc = static_cast<const WSubstancePackageAssetDocumentManager*>(GetDocumentManager())->GetTextureTypeDesc();
  const bool bUpdateThumbnail = pAssetProfile == WAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();
  auto pAssetConfig = pAssetProfile->GetTypeConfig<WTextureAssetProfileConfig>();

  WTempHybridArray<WString, 8> pngPaths;

  for (auto& graph : GetProperties()->m_Graphs)
  {
    if (graph.m_bEnabled == false)
      continue;

    pngPaths.Clear();
    for (auto& output : graph.m_Outputs)
    {
      if (output.m_bEnabled == false)
        continue;

      sPngPath = sTempDir;
      sPngPath.AppendPath(sPackageName);
      sPngPath.Append("_", graph.m_sName, "_", output.m_sName, ".png");

      pngPaths.PushBack(sPngPath);
    }

    sOutputSizeFilePath = sSbsarPath.GetFileDirectory();
    sOutputSizeFilePath.AppendPath(sPackageName);
    sOutputSizeFilePath.Append("_", graph.m_sName, "_OutputSize.txt");

    WTimestamp outputSizeTimestamp = GetModifiedTimestamp(sOutputSizeFilePath);

    if (transformFlags.IsSet(WTransformFlags::ForceTransform) ||
        sbsarTimestamp.Compare(outputSizeTimestamp, WTimestamp::CompareMode::Newer) ||
        HasOutputSizeChanged(sOutputSizeFilePath, graph.m_uiOutputWidth, graph.m_uiOutputHeight))
    {
      WStatus sbsRenderStatus = RunSbsRender(sSbsarPath, graph.m_sName, nullptr, nullptr, sTempDir, graph.m_uiOutputWidth, graph.m_uiOutputHeight);
      if (sbsRenderStatus.Failed())
      {
        // sbsrender.exe sometimes crashes on exit but has written all the outputs anyways so check here whether this was the case
        for (auto& png : pngPaths)
        {
          WTimestamp pngTimestamp = GetModifiedTimestamp(png);
          if (sbsarTimestamp.Compare(pngTimestamp, WTimestamp::CompareMode::Newer))
            return sbsRenderStatus;
        }
      }

      W_SUCCEED_OR_RETURN(WriteOutputSize(sOutputSizeFilePath, graph.m_uiOutputWidth, graph.m_uiOutputHeight));
    }

    WUInt32 uiOutputIndex = 0;
    for (auto& output : graph.m_Outputs)
    {
      if (output.m_bEnabled == false)
        continue;

      GenerateOutputName(graph, output, sOutputName);
      sTargetFile = WStringView(GetDocumentPath()).GetFileDirectory();
      sTargetFile.AppendPath(sOutputName);
      WString sAbsTargetFile = GetAssetDocumentManager()->GetAbsoluteOutputFileName(&textureTypeDesc, sTargetFile, "", pAssetProfile);

      WString sThumbnailFile = GetAssetDocumentManager()->GenerateResourceThumbnailPath(sTargetFile);
      W_SUCCEED_OR_RETURN(RunTexConv(pngPaths[uiOutputIndex], sAbsTargetFile, assetHeader, output, sThumbnailFile, pAssetConfig));

      ++uiOutputIndex;
    }
  }

  return SUPER::InternalTransformAsset(szTargetFile, sOutputTag, pAssetProfile, assetHeader, transformFlags);
}

void WSubstancePackageAssetDocument::OnPropertyChanged(const WDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == WDocumentObjectPropertyEvent::Type::PropertySet && e.m_sProperty == "SubstanceFile")
  {
    WStringBuilder sAbsolutePackagePath = e.m_NewValue.Get<WString>();
    GetProperties()->m_sSubstancePackage = sAbsolutePackagePath;

    bool bSuccess = WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsolutePackagePath) &&
                    UpdateGraphOutputs(sAbsolutePackagePath, true).Succeeded();
    if (bSuccess == false)
    {
      WLog::Error("Substance package not found or invalid '{}'", sAbsolutePackagePath);
    }
  }
}

WResult WSubstancePackageAssetDocument::GetTempDir(WStringBuilder& out_sTempDir) const
{
  auto szDocumentPath = GetDocumentPath();
  const WString sDataDir = WAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);

  WStringBuilder sRelativePath(szDocumentPath);
  W_SUCCEED_OR_RETURN(sRelativePath.MakeRelativeTo(sDataDir));

  out_sTempDir.Set(sDataDir, "/AssetCache/Temp/", sRelativePath.GetFileDirectory());
  out_sTempDir.MakeCleanPath();
  return W_SUCCESS;
}

void WSubstancePackageAssetDocument::GenerateOutputName(const WSubstanceGraph& graph, const WSubstanceGraphOutput& graphOutput, WStringBuilder& out_sOutputName) const
{
  out_sOutputName = GetProperties()->m_sOutputPattern;
  out_sOutputName.ReplaceAll("$(graph)", graph.m_sName);
  out_sOutputName.ReplaceAll("$(name)", graphOutput.m_sName);
  out_sOutputName.ReplaceAll("$(identifier)", graphOutput.m_sName);
  out_sOutputName.ReplaceAll("$(label)", graphOutput.m_sLabel);

  WStringBuilder sUsage;
  WReflectionUtils::EnumerationToString(graphOutput.m_Usage, sUsage, WReflectionUtils::EnumConversionMode::ValueNameOnly);
  out_sOutputName.ReplaceAll("$(usage)", sUsage);
}

WTransformStatus WSubstancePackageAssetDocument::UpdateGraphOutputs(WStringView sAbsolutePath, bool bAllowPropertyModifications)
{
  WStringBuilder sFileContent;
  W_SUCCEED_OR_RETURN(GetSbsContent(sAbsolutePath, sFileContent));

  WTempHybridArray<WSubstanceGraph, 2> graphs;

  QXmlStreamReader reader(sFileContent.GetData());
  W_SUCCEED_OR_RETURN(ReadUntilStartElement(reader, "content"));

  while (reader.atEnd() == false)
  {
    auto tokenType = reader.readNext();
    W_IGNORE_UNUSED(tokenType);

    if (reader.isStartElement() && reader.name() == QLatin1StringView("graph"))
    {
      WSubstanceGraph graph;
      W_SUCCEED_OR_RETURN(ParseGraph(reader, graph));

      if (graph.m_sName.StartsWith("_") == false)
      {
        graphs.PushBack(std::move(graph));
      }
    }
  }

  // Transfer enabled state, label and usage
  WSubstancePackageAssetProperties* pProp = GetProperties();
  for (auto& newGraph : graphs)
  {
    WSubstanceGraph* pExistingGraph = nullptr;
    for (auto& existingGraph : pProp->m_Graphs)
    {
      if (newGraph.m_sName == existingGraph.m_sName)
      {
        newGraph.m_bEnabled = existingGraph.m_bEnabled;
        newGraph.m_uiOutputWidth = existingGraph.m_uiOutputWidth;
        newGraph.m_uiOutputHeight = existingGraph.m_uiOutputHeight;
        pExistingGraph = &existingGraph;
        break;
      }
    }

    if (pExistingGraph == nullptr)
      continue;

    for (auto& newOutput : newGraph.m_Outputs)
    {
      WSubstanceGraphOutput* pExistingOutput = nullptr;
      for (auto& existingOutput : pExistingGraph->m_Outputs)
      {
        if (newOutput.m_sName == existingOutput.m_sName)
        {
          newOutput.m_bEnabled = existingOutput.m_bEnabled;
          newOutput.m_CompressionMode = existingOutput.m_CompressionMode;
          newOutput.m_uiNumChannels = existingOutput.m_uiNumChannels;
          newOutput.m_MipmapMode = existingOutput.m_MipmapMode;
          newOutput.m_bPreserveAlphaCoverage = existingOutput.m_bPreserveAlphaCoverage;
          newOutput.m_Usage = existingOutput.m_Usage;
          newOutput.m_sLabel = existingOutput.m_sLabel;
          break;
        }
      }
    }
  }

  if (pProp->m_Graphs != graphs)
  {
    if (!bAllowPropertyModifications)
    {
      return WTransformStatus(WTransformResult::NeedsImport);
    }

    GetObjectAccessor()->StartTransaction("Update Graphs");

    pProp->m_Graphs = std::move(graphs);

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();
  }

  return WStatus(W_SUCCESS);
}

static const char* s_szTexConvUsageMapping[] = {
  "Auto",      // Unknown,

  "Color",     // BaseColor,
  "Color",     // Emissive,
  "Linear",    // Height,
  "Linear",    // Metallic,
  "Linear",    // Mask,
  "NormalMap", // Normal,
  "Linear",    // Occlusion,
  "Linear",    // Opacity,
  "Linear",    // Roughness,
};

static_assert(W_ARRAY_SIZE(s_szTexConvUsageMapping) == WSubstanceUsage::Count);

static const char* s_szTexConvCompressionMapping[] = {
  "None",
  "Medium",
  "High",
};

static_assert(W_ARRAY_SIZE(s_szTexConvCompressionMapping) == WTexConvCompressionMode::High + 1);

static const char* s_szTexConvMipMapMapping[] = {
  "None",
  "Linear",
  "Kaiser",
};

static_assert(W_ARRAY_SIZE(s_szTexConvMipMapMapping) == WTexConvMipmapMode::Kaiser + 1);

WStatus WSubstancePackageAssetDocument::RunTexConv(const char* szInputFile, const char* szTargetFile, const WAssetFileHeader& assetHeader, const WSubstanceGraphOutput& graphOutput, WStringView sThumbnailFile, const WTextureAssetProfileConfig* pAssetConfig)
{
  QStringList arguments;
  WStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << WConversionUtils::ToString(assetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const WUInt64 uiHash64 = assetHeader.GetFileHash();
    const WUInt32 uiHashLow32 = uiHash64 & 0xFFFFFFFF;
    const WUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.SetFormat("{0}", WArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.SetFormat("{0}", WArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }

  arguments << "-in0";
  arguments << szInputFile;

  arguments << "-out";
  arguments << szTargetFile;

  if (sThumbnailFile.IsEmpty() == false)
  {
    // Thumbnail
    const WStringView sDir = sThumbnailFile.GetFileDirectory();
    WOSFile::CreateDirectoryStructure(sDir).IgnoreResult();

    arguments << "-thumbnailRes";
    arguments << "256";
    arguments << "-thumbnailOut";

    arguments << QString::fromUtf8(sThumbnailFile.GetData(temp));
  }

  arguments << "-compression";
  arguments << s_szTexConvCompressionMapping[graphOutput.m_CompressionMode];

  arguments << "-mipmaps";
  arguments << s_szTexConvMipMapMapping[graphOutput.m_MipmapMode];

  arguments << "-maxRes" << QString::number(pAssetConfig->m_uiMaxResolution);

  arguments << "-usage";
  arguments << s_szTexConvUsageMapping[graphOutput.m_Usage];

  switch (graphOutput.m_uiNumChannels)
  {
    case 1:
      arguments << "-r";
      arguments << "in0.r";
      break;

    case 2:
      arguments << "-rg";
      arguments << "in0.rg";
      break;

    case 3:
      arguments << "-rgb";
      arguments << "in0.rgb";
      break;

    default:
      arguments << "-rgba";
      arguments << "in0.rgba";
      break;
  }

  if (graphOutput.m_bPreserveAlphaCoverage)
  {
    arguments << "-mipsPreserveCoverage";
    arguments << "-mipsAlphaThreshold";
    arguments << "0.5";
  }

  W_SUCCEED_OR_RETURN(WQtEditorApp::GetSingleton()->ExecuteTool("WTexConv", arguments, 180, WLog::GetThreadLocalLogSystem()));

  if (sThumbnailFile.IsEmpty() == false)
  {
    WUInt64 uiThumbnailHash = WAssetCurator::GetSingleton()->GetAssetThumbnailHash(GetGuid());
    W_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnailFile, thumbnailInfo);
  }

  return WStatus(W_SUCCESS);
}
