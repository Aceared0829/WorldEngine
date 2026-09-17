#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/CheckVersion.moc.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QNetworkReply>
#include <QProcess>

static WString GetVersionFilePath()
{
  WStringBuilder sTemp = WOSFile::GetTempDataFolder();
  sTemp.AppendPath("WEditor/version-page.htm");
  return sTemp;
}

PageDownloader::PageDownloader(const QString& sUrl)
{
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  QStringList args;

  args << "-Command";
  args << QString("(Invoke-webrequest -URI \"%1\" -UseBasicParsing).Content > \"%2\"").arg(sUrl).arg(GetVersionFilePath().GetData());

  m_pProcess = W_DEFAULT_NEW(QProcess);
  connect(m_pProcess.Borrow(), &QProcess::finished, this, &PageDownloader::DownloadDone);
  m_pProcess->start("C:\\Windows\\System32\\WindowsPowershell\\v1.0\\powershell.exe", args);
#else
  W_ASSERT_NOT_IMPLEMENTED;
#endif
}

void PageDownloader::DownloadDone(int exitCode, QProcess::ExitStatus exitStatus)
{
  m_pProcess = nullptr;

  WOSFile file;
  if (file.Open(GetVersionFilePath(), WFileOpenMode::Read).Failed())
    return;

  WDataBuffer content;
  file.ReadAll(content);

  content.PushBack('\0');
  content.PushBack('\0');

  const WUInt16* pStart = (WUInt16*)content.GetData();
  if (WUnicodeUtils::SkipUtf16BomLE(pStart))
  {
    m_sDownloadedPage = WStringWChar(pStart);
  }
  else
  {
    const char* szUtf8 = (const char*)content.GetData();
    m_sDownloadedPage = WStringWChar(szUtf8);
  }

  WOSFile::DeleteFile(GetVersionFilePath()).IgnoreResult();

  Q_EMIT FinishedDownload();
}

WQtVersionChecker::WQtVersionChecker()
{
  m_sKnownLatestVersion = GetOwnVersion();
  m_sConfigFile = ":appdata/VersionCheck.ddl";
}

void WQtVersionChecker::Initialize()
{
  m_bRequireOnlineCheck = true;

  WFileStats fs;
  if (WFileSystem::GetFileStats(m_sConfigFile, fs).Failed())
    return;

  WFileReader file;
  if (file.Open(m_sConfigFile).Failed())
    return;

  WOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return;

  auto pRoot = ddl.GetRootElement();
  if (pRoot == nullptr)
    return;

  auto pLatest = pRoot->FindChild("KnownLatest");
  if (pLatest == nullptr || !pLatest->HasPrimitives(WOpenDdlPrimitiveType::String))
    return;

  m_sKnownLatestVersion = pLatest->GetPrimitivesString()[0];

  const WTimestamp nextCheck = fs.m_LastModificationTime + WTime::MakeFromHours(24);

  if (nextCheck.Compare(WTimestamp::CurrentTimestamp(), WTimestamp::CompareMode::Newer))
  {
    // everything fine, we already checked within the last 24 hours

    m_bRequireOnlineCheck = false;
    return;
  }
}

WResult WQtVersionChecker::StoreKnownVersion()
{
  WFileWriter file;
  if (file.Open(m_sConfigFile).Failed())
    return W_FAILURE;

  WOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);
  WOpenDdlUtils::StoreString(ddl, m_sKnownLatestVersion, "KnownLatest");

  return W_SUCCESS;
}

bool WQtVersionChecker::Check(bool bForce)
{
#if W_DISABLED(W_PLATFORM_WINDOWS_DESKTOP)
  W_ASSERT_DEV(!bForce, "The version check is not yet implemented on this platform.");
  return false;
#endif

  if (bForce)
  {
    // to trigger a 'new release available' signal
    m_sKnownLatestVersion = GetOwnVersion();
    m_bForceCheck = true;
  }

  if (m_bCheckInProgresss)
    return false;

  if (!bForce && !m_bRequireOnlineCheck)
  {
    Q_EMIT VersionCheckCompleted(false, false);
    return false;
  }

  m_bCheckInProgresss = true;

  m_pVersionPage = W_DEFAULT_NEW(PageDownloader, "https://ezengine.net/pages/getting-started/binaries.html");

  connect(m_pVersionPage.Borrow(), &PageDownloader::FinishedDownload, this, &WQtVersionChecker::PageDownloaded);

  return true;
}

const char* WQtVersionChecker::GetOwnVersion() const
{
  return WQtUiServices::GetOwnVersionString();
}

const char* WQtVersionChecker::GetKnownLatestVersion() const
{
  return m_sKnownLatestVersion;
}

bool WQtVersionChecker::IsLatestNewer() const
{
  const char* szParsePos;
  WUInt32 own[3] = {0, 0, 0};
  WUInt32 cur[3] = {0, 0, 0};

  szParsePos = GetOwnVersion();
  for (WUInt32 i : {0, 1, 2})
  {
    if (WConversionUtils::StringToUInt(szParsePos, own[i], &szParsePos).Failed())
      break;

    if (*szParsePos == '.')
      ++szParsePos;
    else
      break;
  }

  szParsePos = GetKnownLatestVersion();
  for (WUInt32 i : {0, 1, 2})
  {
    if (WConversionUtils::StringToUInt(szParsePos, cur[i], &szParsePos).Failed())
      break;

    if (*szParsePos == '.')
      ++szParsePos;
    else
      break;
  }

  // 'major'
  if (own[0] > cur[0])
    return false;
  if (own[0] < cur[0])
    return true;

  // 'minor'
  if (own[1] > cur[1])
    return false;
  if (own[1] < cur[1])
    return true;

  // 'patch'
  return own[2] < cur[2];
}

void WQtVersionChecker::PageDownloaded()
{
  m_bCheckInProgresss = false;
  WStringBuilder sPage = m_pVersionPage->GetDownloadedData();

  m_pVersionPage = nullptr;

  if (sPage.IsEmpty())
  {
    WLog::Warning("Could not download release notes page.");
    return;
  }

  const char* szVersionStartTag = "<!--<VERSION>-->";
  const char* szVersionEndTag = "<!--</VERSION>-->";

  const char* pVersionStart = sPage.FindSubString(szVersionStartTag);
  const char* pVersionEnd = sPage.FindSubString(szVersionEndTag, pVersionStart);

  if (pVersionStart == nullptr || pVersionEnd == nullptr)
  {
    WLog::Warning("Version check failed.");
    return;
  }

  WStringBuilder sVersion;
  sVersion.SetSubString_FromTo(pVersionStart + WStringUtils::GetStringElementCount(szVersionStartTag), pVersionEnd);

  const bool bNewRelease = m_sKnownLatestVersion != sVersion;

  // make sure to modify the file, even if the version is the same
  m_sKnownLatestVersion = sVersion;
  if (StoreKnownVersion().Failed())
  {
    WLog::Warning("Could not store the last known version file.");
  }

  Q_EMIT VersionCheckCompleted(bNewRelease, m_bForceCheck);
}
