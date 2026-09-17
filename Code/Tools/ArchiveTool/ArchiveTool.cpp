#include <Foundation/Application/Application.h>
#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>

/* ArchiveTool command line options:

-out <path>
    Path to a file or folder.

    -out specifies the target to pack or unpack things to.
    For packing mode it has to be a file. The file will be overwritten, if it already exists.
    For unpacking, the target should be a folder (may or may not exist) into which the archives get extracted.

    If no -out is specified, it is determined to be where the input file is located.

-unpack <paths>
    One or multiple paths to WArchive files that shall be extracted.

    Example:
      -unpack "path/to/file.WArchive" "another/file.WArchive"

-pack <paths>
    One or multiple paths to folders that shall be packed.

    Example:
      -pack "path/to/folder" "path/to/another/folder"

Description:
    -pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)
    or to unpack multiple archives at the same time.

    If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.
    If all inputs are folders, the mode is 'pack'.
    If all inputs are files, the mode is 'unpack'.

Examples:
    WArchiveTool.exe "C:/Stuff"
      Packs all data in "C:/Stuff" into "C:/Stuff.WArchive"

    WArchiveTool.exe "C:/Stuff" -out "C:/MyStuff.WArchive"
      Packs all data in "C:/Stuff" into "C:/MyStuff.WArchive"

    WArchiveTool.exe "C:/Stuff.WArchive"
      Unpacks all data from the archive into "C:/Stuff"

    WArchiveTool.exe "C:/Stuff.WArchive" -out "C:/MyStuff"
      Unpacks all data from the archive into "C:/MyStuff"
*/

WCommandLineOptionPath opt_Out("_ArchiveTool", "-out", "\
Path to a file or folder.\n\
\n\
-out specifies the target to pack or unpack things to.\n\
For packing mode it has to be a file. The file will be overwritten, if it already exists.\n\
For unpacking, the target should be a folder (may or may not exist) into which the archives get extracted.\n\
\n\
If no -out is specified, it is determined to be where the input file is located.\n\
",
  "");

WCommandLineOptionDoc opt_Unpack("_ArchiveTool", "-unpack", "<paths>", "\
One or multiple paths to WArchive files that shall be extracted.\n\
\n\
Example:\n\
  -unpack \"path/to/file.WArchive\" \"another/file.WArchive\"\n\
",
  "");

WCommandLineOptionDoc opt_Pack("_ArchiveTool", "-pack", "<paths>", "\
One or multiple paths to folders that shall be packed.\n\
\n\
Example:\n\
  -pack \"path/to/folder\" \"path/to/another/folder\"\n\
",
  "");

WCommandLineOptionDoc opt_Desc("_ArchiveTool", "Description:", "", "\
-pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)\n\
or to unpack multiple archives at the same time.\n\
\n\
If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.\n\
If all inputs are folders, the mode is 'pack'.\n\
If all inputs are files, the mode is 'unpack'.\n\
",
  "");

WCommandLineOptionDoc opt_Examples("_ArchiveTool", "Examples:", "", "\
WArchiveTool.exe \"C:/Stuff\"\n\
  Packs all data in \"C:/Stuff\" into \"C:/Stuff.WArchive\"\n\
\n\
WArchiveTool.exe \"C:/Stuff\" -out \"C:/MyStuff.WArchive\"\n\
  Packs all data in \"C:/Stuff\" into \"C:/MyStuff.WArchive\"\n\
\n\
WArchiveTool.exe \"C:/Stuff.WArchive\"\n\
  Unpacks all data from the archive into \"C:/Stuff\"\n\
\n\
WArchiveTool.exe \"C:/Stuff.WArchive\" -out \"C:/MyStuff\"\n\
  Unpacks all data from the archive into \"C:/MyStuff\"\n\
",
  "");

class WArchiveBuilderImpl : public WArchiveBuilder
{
protected:
  virtual void WriteFileResultCallback(WUInt32 uiCurEntry, WUInt32 uiMaxEntries, WStringView sSourceFile, WUInt64 uiSourceSize, WUInt64 uiStoredSize, WTime duration) const override
  {
    const WUInt64 uiPercentage = (uiSourceSize == 0) ? 100 : (uiStoredSize * 100 / uiSourceSize);
    WLog::Info(" [{}%%] {} ({}%%) - {}", WArgU(100 * uiCurEntry / uiMaxEntries, 2), sSourceFile, uiPercentage, duration);
  }
};

class WArchiveReaderImpl : public WArchiveReader
{
public:
protected:
  virtual bool ExtractNextFileCallback(WUInt32 uiCurEntry, WUInt32 uiMaxEntries, WStringView sSourceFile) const override
  {
    WLog::Info(" [{}%%] {}", WArgU(100 * uiCurEntry / uiMaxEntries, 2), sSourceFile);
    return true;
  }


  virtual bool ExtractFileProgressCallback(WUInt64 bytesWritten, WUInt64 bytesTotal) const override
  {
    // WLog::Dev("   {}%%", WArgU(100 * bytesWritten / bytesTotal));
    return true;
  }
};

class WArchiveTool : public WApplication
{
public:
  using SUPER = WApplication;

  enum class ArchiveMode
  {
    Auto,
    Pack,
    Unpack,
  };

  ArchiveMode m_Mode = ArchiveMode::Auto;

  WDynamicArray<WString> m_sInputs;
  WString m_sOutput;

  WArchiveTool()
    : WApplication("ArchiveTool")
  {
  }

  WResult ParseArguments()
  {
    if (GetArgumentCount() <= 1)
    {
      WLog::Error("No arguments given");
      return W_FAILURE;
    }

    WCommandLineUtils& cmd = *WCommandLineUtils::GetGlobalInstance();

    m_sOutput = opt_Out.GetOptionValue(WCommandLineOption::LogMode::Always);

    WStringBuilder path;

    if (cmd.GetStringOptionArguments("-pack") > 0)
    {
      m_Mode = ArchiveMode::Pack;
      const WUInt32 args = cmd.GetStringOptionArguments("-pack");

      if (args == 0)
      {
        WLog::Error("-pack option expects at least one argument");
        return W_FAILURE;
      }

      for (WUInt32 a = 0; a < args; ++a)
      {
        m_sInputs.PushBack(cmd.GetAbsolutePathOption("-pack", a));

        if (!WOSFile::ExistsDirectory(m_sInputs.PeekBack()))
        {
          WLog::Error("-pack input path is not a valid directory: '{}'", m_sInputs.PeekBack());
          return W_FAILURE;
        }
      }
    }
    else if (cmd.GetStringOptionArguments("-unpack") > 0)
    {
      m_Mode = ArchiveMode::Unpack;
      const WUInt32 args = cmd.GetStringOptionArguments("-unpack");

      if (args == 0)
      {
        WLog::Error("-unpack option expects at least one argument");
        return W_FAILURE;
      }

      for (WUInt32 a = 0; a < args; ++a)
      {
        m_sInputs.PushBack(cmd.GetAbsolutePathOption("-unpack", a));

        if (!WOSFile::ExistsFile(m_sInputs.PeekBack()))
        {
          WLog::Error("-unpack input file does not exist: '{}'", m_sInputs.PeekBack());
          return W_FAILURE;
        }
      }
    }
    else
    {
      bool bInputsFolders = true;
      bool bInputsFiles = true;

      for (WUInt32 a = 1; a < GetArgumentCount(); ++a)
      {
        const WStringView sArg = GetArgument(a);

        if (sArg.IsEqual_NoCase("-out"))
          break;

        m_sInputs.PushBack(WOSFile::MakePathAbsoluteWithCWD(sArg));

        if (!WOSFile::ExistsDirectory(m_sInputs.PeekBack()))
          bInputsFolders = false;
        if (!WOSFile::ExistsFile(m_sInputs.PeekBack()))
          bInputsFiles = false;
      }

      if (bInputsFolders && !bInputsFiles)
      {
        m_Mode = ArchiveMode::Pack;
      }
      else if (bInputsFiles && !bInputsFolders)
      {
        m_Mode = ArchiveMode::Unpack;
      }
      else
      {
        WLog::Error("Inputs are ambiguous. Specify only folders for packing or only files for unpacking. Use -out as last argument to "
                     "specify a target.");
        return W_FAILURE;
      }
    }

    WLog::Info("Mode is: {}", m_Mode == ArchiveMode::Pack ? "pack" : "unpack");
    WLog::Info("Inputs:");

    for (const auto& input : m_sInputs)
    {
      WLog::Info("  '{}'", input);
    }

    WLog::Info("Output: '{}'", m_sOutput);

    return W_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    // Add the empty data directory to access files via absolute paths
    WFileSystem::AddDataDirectory("", "App", ":", WDataDirUsage::AllowWrites).IgnoreResult();

    WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
    WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    // prevent further output during shutdown
    WGlobalLog::RemoveLogWriter(WLogWriter::Console::LogMessageHandler);
    WGlobalLog::RemoveLogWriter(WLogWriter::VisualStudio::LogMessageHandler);

    SUPER::BeforeCoreSystemsShutdown();
  }

  static WArchiveBuilder::InclusionMode PackFileCallback(WStringView sFile)
  {
    const WStringView ext = WPathUtils::GetFileExtension(sFile);

    if (ext.IsEqual_NoCase("jpg") || ext.IsEqual_NoCase("jpeg") || ext.IsEqual_NoCase("png"))
      return WArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("zip") || ext.IsEqual_NoCase("7z"))
      return WArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("mp3") || ext.IsEqual_NoCase("ogg"))
      return WArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("dds"))
      return WArchiveBuilder::InclusionMode::Compress_zstd_fast;

    return WArchiveBuilder::InclusionMode::Compress_zstd_average;
  }

  WResult Pack()
  {
    WArchiveBuilderImpl archive;

    for (const auto& folder : m_sInputs)
    {
      archive.AddFolder(folder, WArchiveCompressionMode::Compressed_zstd, PackFileCallback);
    }

    if (m_sOutput.IsEmpty())
    {
      WStringBuilder sArchive = m_sInputs[0];
      sArchive.Append(".WArchive");

      m_sOutput = sArchive;
    }

    m_sOutput = WOSFile::MakePathAbsoluteWithCWD(m_sOutput);

    WLog::Info("Writing archive to '{}'", m_sOutput);
    if (archive.WriteArchive(m_sOutput).Failed())
    {
      WLog::Error("Failed to write the WArchive");

      return W_FAILURE;
    }

    return W_SUCCESS;
  }

  WResult Unpack()
  {
    for (const auto& file : m_sInputs)
    {
      WLog::Info("Extracting archive '{}'", file);

      // if the file has a custom archive file extension, just register it as 'allowed'
      // we assume that the user only gives us files that are WArchives
      if (!WArchiveUtils::IsAcceptedArchiveFileExtensions(WPathUtils::GetFileExtension(file)))
      {
        WArchiveUtils::GetAcceptedArchiveFileExtensions().PushBack(WPathUtils::GetFileExtension(file));
      }

      WArchiveReaderImpl reader;
      W_SUCCEED_OR_RETURN(reader.OpenArchive(file));

      WStringBuilder sOutput = m_sOutput;

      if (sOutput.IsEmpty())
      {
        sOutput = file;
        sOutput.RemoveFileExtension();
      }

      if (reader.ExtractAllFiles(sOutput).Failed())
      {
        WLog::Error("File extraction failed.");
        return W_FAILURE;
      }
    }

    return W_SUCCESS;
  }

  virtual void Run() override
  {
    {
      WStringBuilder cmdHelp;
      if (WCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, WCommandLineOption::LogAvailableModes::IfHelpRequested, "_ArchiveTool"))
      {
        WLog::Print(cmdHelp);
        QuitApplication();
        return;
      }
    }

    WStopwatch sw;

    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      QuitApplication();
      return;
    }

    if (m_Mode == ArchiveMode::Pack)
    {
      if (Pack().Failed())
      {
        WLog::Error("Packaging files failed");
        SetReturnCode(2);
      }

      WLog::Success("Finished packing archive in {}", sw.GetRunningTotal());
      QuitApplication();
      return;
    }

    if (m_Mode == ArchiveMode::Unpack)
    {
      if (Unpack().Failed())
      {
        WLog::Error("Extracting files failed");
        SetReturnCode(3);
      }

      WLog::Success("Finished extracting archive in {}", sw.GetRunningTotal());
      QuitApplication();
      return;
    }

    WLog::Error("Unknown mode");
    QuitApplication();
  }
};

W_APPLICATION_ENTRY_POINT(WArchiveTool);
