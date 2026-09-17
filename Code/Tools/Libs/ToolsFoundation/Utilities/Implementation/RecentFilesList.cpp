#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Utilities/RecentFilesList.h>

void WRecentFilesList::Insert(WStringView sFile, WInt32 iContainerWindow)
{
  WStringBuilder sCleanPath = sFile;
  sCleanPath.MakeCleanPath();

  WString s = sCleanPath;

  for (WUInt32 i = 0; i < m_Files.GetCount(); i++)
  {
    if (m_Files[i].m_File == s)
    {
      m_Files.RemoveAtAndCopy(i);
      break;
    }
  }
  m_Files.PushFront(RecentFile(s, iContainerWindow));

  if (m_Files.GetCount() > m_uiMaxElements)
    m_Files.SetCount(m_uiMaxElements);
}

void WRecentFilesList::Save(WStringView sFile)
{
  WDeferredFileWriter File;
  File.SetOutput(sFile);

  for (const RecentFile& file : m_Files)
  {
    WStringBuilder sTemp;
    sTemp.SetFormat("{0}|{1}", file.m_File, file.m_iContainerWindow);
    File.WriteBytes(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
    File.WriteBytes("\n", sizeof(char)).IgnoreResult();
  }

  if (File.Close().Failed())
    WLog::Error("Unable to open file '{0}' for writing!", sFile);
}

void WRecentFilesList::Load(WStringView sFile)
{
  m_Files.Clear();

  WFileReader File;
  if (File.Open(sFile).Failed())
    return;

  WStringBuilder sAllLines;
  sAllLines.ReadAll(File);

  WTempHybridArray<WStringView, 16> Lines;
  sAllLines.Split(false, Lines, "\n");

  WStringBuilder sTemp, sTemp2;

  for (const WStringView& sv : Lines)
  {
    sTemp = sv;
    WTempHybridArray<WStringView, 2> Parts;
    sTemp.Split(false, Parts, "|");

    if (!WOSFile::ExistsFile(Parts[0].GetData(sTemp2)))
      continue;

    if (Parts.GetCount() == 1)
    {
      m_Files.PushBack(RecentFile(Parts[0], 0));
    }
    else if (Parts.GetCount() == 2)
    {
      WStringBuilder sContainer = Parts[1];
      WInt32 iContainerWindow = 0;
      WConversionUtils::StringToInt(sContainer, iContainerWindow).IgnoreResult();
      m_Files.PushBack(RecentFile(Parts[0], iContainerWindow));
    }
  }
}
