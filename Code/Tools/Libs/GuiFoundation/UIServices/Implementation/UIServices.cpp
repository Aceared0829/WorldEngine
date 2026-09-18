#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Stopwatch.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

W_IMPLEMENT_SINGLETON(WQtUiServices);

WEvent<const WQtUiServices::Event&, WMutex> WQtUiServices::s_Events;
WCopyOnBroadcastEvent<const WQtUiServices::TickEvent&> WQtUiServices::s_TickEvent;

WMap<WString, QIcon> WQtUiServices::s_IconsCache;
WMap<WString, QImage> WQtUiServices::s_ImagesCache;
WMap<WString, QPixmap> WQtUiServices::s_PixmapsCache;
bool WQtUiServices::s_bHeadless;
bool WQtUiServices::s_bUnattended;
WHybridArray<WString, 4> WQtUiServices::s_SuppressedDialogs;
WHybridArray<WString, 4> WQtUiServices::s_FailedAsserts;
WQtUiServices::TickEvent WQtUiServices::s_LastTickEvent;

static WQtUiServices* g_pInstance = nullptr;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, QtUiServices)

  ON_CORESYSTEMS_STARTUP
  {
    g_pInstance = W_DEFAULT_NEW(WQtUiServices);
    WQtUiServices::GetSingleton()->Init();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    W_DEFAULT_DELETE(g_pInstance);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WQtUiServices::WQtUiServices()
  : m_SingletonRegistrar(this)
{
  qRegisterMetaType<WUuid>();
  m_pColorDlg = nullptr;
}


bool WQtUiServices::IsHeadless()
{
  return s_bHeadless;
}


void WQtUiServices::SetHeadless(bool bHeadless)
{
  s_bHeadless = bHeadless;
}

bool WQtUiServices::IsUnattended()
{
  // without any window there is nowhere to show a dialog, so headless always implies unattended
  return s_bUnattended || s_bHeadless;
}

void WQtUiServices::SetUnattended()
{
  s_bUnattended = true;
}

void WQtUiServices::ReportSuppressedDialog(WStringView sDescription)
{
  WLog::Warning("Dialog not shown, because no user is present: {}", sDescription);

  // A suppressed dialog often means an operation didn't do what it was asked to, and code reacting to
  // that may well open the next one. Dropping the excess keeps the first, most relevant entries.
  constexpr WUInt32 uiMaxEntries = 16;

  if (s_SuppressedDialogs.GetCount() < uiMaxEntries)
  {
    s_SuppressedDialogs.PushBack(sDescription);
  }
}

bool WQtUiServices::SuppressModalWindow(WStringView sDescription)
{
  if (!IsUnattended())
    return false;

  ReportSuppressedDialog(sDescription);
  return true;
}

namespace
{
  WAssertHandler g_PreviousAssertHandler = nullptr;

  bool UnattendedAssertHandler(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
  {
    if (!WQtUiServices::IsUnattended())
    {
      // A user is sitting in front of this: the normal dialog and debugger break are what they want.
      if (g_PreviousAssertHandler != nullptr)
        return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

      return true;
    }

    WStringBuilder sReport;
    sReport.SetFormat("{}({}) in {}: '{}' {}", szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

    WQtUiServices::ReportFailedAssert(sReport);

    // false means 'do not break'. The assert dialog would block the main thread with nobody there to
    // dismiss it, which is worse than continuing into whatever the assert was guarding against.
    return false;
  }
} // namespace

void WQtUiServices::ReportFailedAssert(WStringView sReport)
{
  WLog::Error("Assert failed with no user present: {}", sReport);

  // One broken invariant tends to trip the next assert immediately, so the list is bounded the same way
  // the dialog list is. The first entry is the one that explains the rest.
  constexpr WUInt32 uiMaxEntries = 8;

  if (s_FailedAsserts.GetCount() < uiMaxEntries)
  {
    s_FailedAsserts.PushBack(sReport);
  }
}

WArrayPtr<const WString> WQtUiServices::GetFailedAsserts()
{
  return s_FailedAsserts;
}

void WQtUiServices::ClearFailedAsserts()
{
  s_FailedAsserts.Clear();
}

WArrayPtr<const WString> WQtUiServices::GetSuppressedDialogs()
{
  return s_SuppressedDialogs;
}

void WQtUiServices::ClearSuppressedDialogs()
{
  s_SuppressedDialogs.Clear();
}

WQtScopedUnattended::WQtScopedUnattended()
{
  m_bPrevUnattended = WQtUiServices::s_bUnattended;
  WQtUiServices::s_bUnattended = true;
}

WQtScopedUnattended::~WQtScopedUnattended()
{
  WQtUiServices::s_bUnattended = m_bPrevUnattended;
}

void WQtUiServices::SaveState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    Settings.setValue("ColorDlgGeom", m_ColorDlgGeometry);
  }
  Settings.endGroup();
}

WTime g_Total = WTime::MakeZero();

const QIcon& WQtUiServices::GetCachedIconResource(WStringView sIdentifier, WColor svgTintColor)
{
  WStringBuilder sFullIdentifier = sIdentifier;
  auto& map = s_IconsCache;

  const bool bNeedsColoring = svgTintColor != WColor::MakeZero() && sIdentifier.EndsWith_NoCase(".svg");

  if (bNeedsColoring)
  {
    sFullIdentifier.AppendFormat("-{}", WColorGammaUB(svgTintColor));
  }

  auto it = map.Find(sFullIdentifier);

  if (it.IsValid())
    return it.Value();

  if (bNeedsColoring)
  {
    WStopwatch sw;

    // read the icon from the Qt virtual file system (QResource)
    QFile file(WString(sIdentifier).GetData());
    if (!file.open(QIODeviceBase::OpenModeFlag::ReadOnly))
    {
      // if it doesn't exist, return an empty QIcon

      map[sFullIdentifier] = QIcon();
      return map[sFullIdentifier];
    }

    // get the entire SVG file content
    WStringBuilder sContent = QString(file.readAll()).toUtf8().data();

    // replace the occurrence of the color white ("#FFFFFF") with the desired target color
    {
      const WColorGammaUB color8 = svgTintColor;

      WStringBuilder rep;
      rep.SetFormat("#{}{}{}", WArgU(color8.r, 2, true, 16), WArgU(color8.g, 2, true, 16), WArgU(color8.b, 2, true, 16));

      sContent.ReplaceAll_NoCase("#ffffff", rep);

      rep.Append(";");
      sContent.ReplaceAll_NoCase("#fff;", rep);
      sContent.ReplaceAll_NoCase("white;", rep);
      rep.Shrink(0, 1);

      rep.Prepend("\"");
      rep.Append("\"");
      sContent.ReplaceAll_NoCase("\"#fff\"", rep);
    }

    // hash the content AFTER the color replacement, so it includes the custom color change
    const WUInt32 uiSrcHash = WHashingUtils::xxHash32String(sContent);

    // file the path to the temp file, including the source hash
    const WStringBuilder sTempFolder = WOSFile::GetTempDataFolder("WEditor/QIcons");
    WStringBuilder sTempIconFile(sTempFolder, "/", sIdentifier.GetFileName());
    sTempIconFile.AppendFormat("-{}.svg", uiSrcHash);

    // only write to the file system, if the target file doesn't exist yet, this saves more than half the time
    if (!WOSFile::ExistsFile(sTempIconFile))
    {
      // now write the new SVG file back to a dummy file
      // yes, this is as stupid as it sounds, we really write the file BACK TO THE FILESYSTEM, rather than doing this stuff in-memory
      // that's because I wasn't able to figure out whether we can somehow read a QIcon from a string rather than from file
      // it doesn't appear to be easy at least, since we can only give it a path, not a memory stream or anything like that
      {
        // necessary for Qt to be able to write to the folder
        WOSFile::CreateDirectoryStructure(sTempFolder).AssertSuccess();

        QFile fileOut(sTempIconFile.GetData());
        if (fileOut.open(QIODeviceBase::OpenModeFlag::WriteOnly))
        {
          fileOut.write(sContent.GetData(), sContent.GetElementCount());
          fileOut.flush();
          fileOut.close();
        }
      }
    }

    QIcon icon(sTempIconFile.GetData());

    if (!icon.pixmap(QSize(16, 16)).isNull())
      map[sFullIdentifier] = icon;
    else
      map[sFullIdentifier] = QIcon();

    WTime local = sw.GetRunningTotal();
    g_Total += local;

    // kept here for debug purposes, but don't waste time on logging
    // WLog::Info("Icon load time: {}, total = {}", local, g_Total);
  }
  else
  {
    const QString sFile = WString(sIdentifier).GetData();

    if (QFile::exists(sFile)) // prevent Qt from spamming warnings about non-existing files by checking this manually
    {
      QIcon icon(sFile);

      // Workaround for QIcon being stupid and treating failed to load icons as not-null.
      if (!icon.pixmap(QSize(16, 16)).isNull())
        map[sFullIdentifier] = icon;
      else
        map[sFullIdentifier] = QIcon();
    }
    else
      map[sFullIdentifier] = QIcon();
  }

  return map[sFullIdentifier];
}


const QImage& WQtUiServices::GetCachedImageResource(const char* szIdentifier)
{
  const WString sIdentifier = szIdentifier;
  auto& map = s_ImagesCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QImage(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}

const QPixmap& WQtUiServices::GetCachedPixmapResource(const char* szIdentifier)
{
  const WString sIdentifier = szIdentifier;
  auto& map = s_PixmapsCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QPixmap(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}

WResult WQtUiServices::AddToGitIgnore(const char* szGitIgnoreFile, const char* szPattern)
{
  WStringBuilder ignoreFile;

  {
    WFileReader file;
    if (file.Open(szGitIgnoreFile).Succeeded())
    {
      ignoreFile.ReadAll(file);
    }
  }

  ignoreFile.Trim("\n\r");

  const WUInt32 len = WStringUtils::GetStringElementCount(szPattern);

  // pattern already present ?
  if (const char* szFound = ignoreFile.FindSubString(szPattern))
  {
    if (szFound == ignoreFile.GetData() || // right at the start
        *(szFound - 1) == '\n')            // after a new line
    {
      const char end = *(szFound + len);

      if (end == '\0' || end == '\r' || end == '\n') // line does not continue with an extended pattern
      {
        return W_SUCCESS;
      }
    }
  }

  ignoreFile.AppendWithSeparator("\n", szPattern);
  ignoreFile.Append("\n\n");

  {
    WFileWriter file;
    W_SUCCEED_OR_RETURN(file.Open(szGitIgnoreFile));

    W_SUCCEED_OR_RETURN(file.WriteBytes(ignoreFile.GetData(), ignoreFile.GetElementCount()));
  }

  return W_SUCCESS;
}

void WQtUiServices::CheckForUpdates()
{
  Event e;
  e.m_Type = Event::Type::CheckForUpdates;
  s_Events.Broadcast(e);
}

void WQtUiServices::GotoLinkTarget(WStringView sLinkTarget)
{
  Event e;
  e.m_Type = Event::Type::GotoLinkTarget;
  e.m_sText = sLinkTarget;
  s_Events.Broadcast(e);
}

void WQtUiServices::Init()
{
  // Installed unconditionally, because unattended mode is entered and left again at runtime
  // (WQtScopedUnattended) - the handler decides per assert, rather than being swapped in and out.
  if (g_PreviousAssertHandler == nullptr)
  {
    g_PreviousAssertHandler = WGetAssertHandler();
    WSetAssertHandler(UnattendedAssertHandler);
  }

  s_LastTickEvent.m_fRefreshRate = 60.0;
  if (QScreen* pScreen = QApplication::primaryScreen())
  {
    s_LastTickEvent.m_fRefreshRate = pScreen->refreshRate();
  }

  QTimer::singleShot((WInt32)WMath::Floor(1000.0 / s_LastTickEvent.m_fRefreshRate), this, SLOT(TickEventHandler()));
}

void WQtUiServices::TickEventHandler()
{
  W_PROFILE_SCOPE("TickEvent");

  W_ASSERT_DEV(!m_bIsDrawingATM, "Implementation error");
  WTime startTime = WTime::Now();
  s_LastTickEvent.m_uiFrame++;
  s_LastTickEvent.m_Time = startTime;

  bool bFrameRequested = false;
  {
    s_LastTickEvent.m_Type = TickEvent::Type::BeforeFrame;
    s_LastTickEvent.m_uiFrameRequest = 0;
    s_LastTickEvent.m_uiForceCancelFrame = 0;
    s_TickEvent.Broadcast(s_LastTickEvent);
    bFrameRequested = s_LastTickEvent.m_uiFrameRequest != 0 && s_LastTickEvent.m_uiForceCancelFrame == 0;
  }

  if (bFrameRequested)
  {
    m_bIsDrawingATM = true;
    s_LastTickEvent.m_Type = TickEvent::Type::StartFrame;
    s_TickEvent.Broadcast(s_LastTickEvent);
    s_LastTickEvent.m_Type = TickEvent::Type::EndFrame;
    s_TickEvent.Broadcast(s_LastTickEvent);
    m_bIsDrawingATM = false;
  }
  const WTime endTime = WTime::Now();
  WTime lastFrameTime = endTime - startTime;

  WTime delay = WTime::MakeFromMilliseconds(1000.0 / s_LastTickEvent.m_fRefreshRate);
  delay -= lastFrameTime;
  delay = WMath::Max(delay, WTime::MakeZero());

  QTimer::singleShot((WInt32)WMath::Floor(delay.GetMilliseconds()), this, SLOT(TickEventHandler()));
}

void WQtUiServices::LoadState()
{
  W_PROFILE_SCOPE("LoadState");
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    m_ColorDlgGeometry = Settings.value("ColorDlgGeom").toByteArray();
  }
  Settings.endGroup();
}

void WQtUiServices::ShowAllDocumentsTemporaryStatusBarMessage(const WFormatString& msg, WTime timeOut)
{
  WStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowDocumentTemporaryStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_Time = timeOut;

  s_Events.Broadcast(e, 1);
}

void WQtUiServices::ShowAllDocumentsPermanentStatusBarMessage(const WFormatString& msg, Event::TextType type)
{
  WStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowDocumentPermanentStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_TextType = type;

  s_Events.Broadcast(e, 1);
}

void WQtUiServices::ShowGlobalStatusBarMessage(const WFormatString& msg)
{
  WStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowGlobalStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_Time = WTime::MakeFromSeconds(0);

  s_Events.Broadcast(e);
}


WResult WQtUiServices::OpenFileInDefaultProgram(WStringView sPath)
{
  return QDesktopServices::openUrl(QUrl::fromLocalFile(WMakeQString(sPath))) ? W_SUCCESS : W_FAILURE;
}

WResult WQtUiServices::OpenInVisualStudio(WStringView sPath)
{
  QString sVSExe;

#if W_ENABLED(W_PLATFORM_WINDOWS)
  QSettings settings("\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Applications\\VSLauncher.exe\\Shell\\Open\\Command", QSettings::NativeFormat);
  QString sVSKey = settings.value(".", "").value<QString>();

  if (sVSKey.length() > 5)
  {
    sVSExe = sVSKey.left(sVSKey.length() - 5).replace("\\", "/").replace("\"", "");
  }

  if (sVSExe.isEmpty() || !QFile::exists(sVSExe))
  {
    QStringList vsWhereCandidates;
    vsWhereCandidates << QStandardPaths::findExecutable("vswhere.exe");
    vsWhereCandidates << QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles(x86)")) + "/Microsoft Visual Studio/Installer/vswhere.exe";
    vsWhereCandidates << QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles")) + "/Microsoft Visual Studio/Installer/vswhere.exe";

    for (const QString& vsWhere : vsWhereCandidates)
    {
      if (vsWhere.isEmpty() || !QFile::exists(vsWhere))
        continue;

      QProcess process;
      process.start(vsWhere, {"-latest", "-products", "*", "-property", "installationPath"}, QIODevice::ReadOnly);
      if (!process.waitForFinished(3000) || process.exitCode() != 0)
        continue;

      const QString installPath = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
      const QString candidate = QDir(installPath).filePath("Common7/IDE/devenv.exe");
      if (QFile::exists(candidate))
      {
        sVSExe = candidate;
        break;
      }
    }
  }

  if (sVSExe.isEmpty() || !QFile::exists(sVSExe))
  {
    sVSExe = QStandardPaths::findExecutable("devenv.exe");

    if (sVSExe.isEmpty())
    {
      const QStringList roots = {
        QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles")),
        QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles(x86)"))};
      const QStringList versions = {"18", "2026", "2022", "2019"};
      const QStringList editions = {"Community", "Professional", "Enterprise", "BuildTools"};

      for (const QString& root : roots)
      {
        for (const QString& version : versions)
        {
          for (const QString& edition : editions)
          {
            const QString candidate = root + "/Microsoft Visual Studio/" + version + "/" + edition + "/Common7/IDE/devenv.exe";
            if (QFile::exists(candidate))
            {
              sVSExe = candidate;
              break;
            }
          }
          if (!sVSExe.isEmpty())
            break;
        }
        if (!sVSExe.isEmpty())
          break;
      }
    }
  }
#endif

  if (sVSExe.isEmpty() || !QFile::exists(sVSExe))
  {
    WLog::Error("Could not locate Visual Studio automatically.");
    return W_FAILURE;
  }

  QStringList arguments;
  arguments.push_back(WMakeQString(sPath));

  if (!QProcess::startDetached(sVSExe, arguments))
  {
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WQtUiServices::OpenInRider(WStringView sPath)
{
  QString sRiderPath;

#if W_ENABLED(W_PLATFORM_WINDOWS)
  QSettings settings("\\HKEY_CURRENT_USER\\SOFTWARE\\JetBrains\\Toolbox\\", QSettings::NativeFormat);
  QString sToolboxKey = settings.value(".", "").value<QString>();

  QString sToolboxPath = sToolboxKey.replace("\\", "/").replace("\"", "");
  if (sToolboxPath.isEmpty())
  {
    sToolboxPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation).split("Local/", Qt::KeepEmptyParts, Qt::CaseInsensitive).first();
    sToolboxPath += "Local/JetBrains/Toolbox/.settings.json";
  }
  else
  {
    sToolboxPath.append("/../.settings.json");
  }

  if (QFile::exists(sToolboxPath))
  {
    QFile file(sToolboxPath);
    if (file.open(QIODevice::ReadOnly))
    {
      QByteArray rawData = file.readAll();
      QJsonDocument doc = QJsonDocument::fromJson(rawData);

      QJsonObject rootObject = doc.object();
      QJsonValue shellPathValue = rootObject.value("shell_scripts");
      QJsonObject shellPathObject = shellPathValue.toObject();
      sRiderPath = shellPathObject.value("location").toString().replace("\\", "/").replace("\"", "");
      sRiderPath.append("/rider.cmd");
      file.close();
    }
  }
  else
  {
    sRiderPath = "rider64.exe";
  }

#elif W_ENABLED(W_PLATFORM_LINUX)
  if (QFile::exists("/opt/rider/bin/rider.sh"))
  {
    sRiderPath = "/opt/clion/bin/rider.sh";
  }
  else
  {
    // Maybe its in path????
    sRiderPath = "rider.sh";
  }
#else
  return W_FAILURE;
#endif

  QStringList arguments;
  arguments.push_back(WMakeQString(sPath));

  if (!QProcess::startDetached(sRiderPath, arguments))
  {
    return W_FAILURE;
  }

  return W_SUCCESS;
}

namespace WQtUtils
{
  bool IsEquivalentQtKey(const QKeyEvent* e, Qt::Key reference)
  {
    // X11 (xcb) keycodes are hardware scan codes + 8 offset, Wayland uses raw evdev codes
#if W_ENABLED(W_PLATFORM_LINUX)
    const quint32 SC_OFFSET = (QGuiApplication::platformName() == "xcb") ? 8 : 0;
#else
    constexpr quint32 SC_OFFSET = 0;
#endif

    const quint32 nativeScanCode = e->nativeScanCode();
    switch (reference)
    {
      case Qt::Key_A:
        return nativeScanCode == 30 + SC_OFFSET;
      case Qt::Key_B:
        return nativeScanCode == 48 + SC_OFFSET;
      case Qt::Key_C:
        return nativeScanCode == 46 + SC_OFFSET;
      case Qt::Key_D:
        return nativeScanCode == 32 + SC_OFFSET;
      case Qt::Key_E:
        return nativeScanCode == 18 + SC_OFFSET;
      case Qt::Key_F:
        return nativeScanCode == 33 + SC_OFFSET;
      case Qt::Key_G:
        return nativeScanCode == 34 + SC_OFFSET;
      case Qt::Key_H:
        return nativeScanCode == 35 + SC_OFFSET;
      case Qt::Key_I:
        return nativeScanCode == 23 + SC_OFFSET;
      case Qt::Key_J:
        return nativeScanCode == 36 + SC_OFFSET;
      case Qt::Key_K:
        return nativeScanCode == 37 + SC_OFFSET;
      case Qt::Key_L:
        return nativeScanCode == 38 + SC_OFFSET;
      case Qt::Key_M:
        return nativeScanCode == 50 + SC_OFFSET;
      case Qt::Key_N:
        return nativeScanCode == 49 + SC_OFFSET;
      case Qt::Key_O:
        return nativeScanCode == 24 + SC_OFFSET;
      case Qt::Key_P:
        return nativeScanCode == 25 + SC_OFFSET;
      case Qt::Key_Q:
        return nativeScanCode == 16 + SC_OFFSET;
      case Qt::Key_R:
        return nativeScanCode == 19 + SC_OFFSET;
      case Qt::Key_S:
        return nativeScanCode == 31 + SC_OFFSET;
      case Qt::Key_T:
        return nativeScanCode == 20 + SC_OFFSET;
      case Qt::Key_U:
        return nativeScanCode == 22 + SC_OFFSET;
      case Qt::Key_V:
        return nativeScanCode == 47 + SC_OFFSET;
      case Qt::Key_W:
        return nativeScanCode == 17 + SC_OFFSET;
      case Qt::Key_X:
        return nativeScanCode == 45 + SC_OFFSET;
      case Qt::Key_Y:
        return nativeScanCode == 21 + SC_OFFSET;
      case Qt::Key_Z:
        return nativeScanCode == 44 + SC_OFFSET;

      default:
        WLog::Dev("IsEquivalentQtKey: Undefined scancode mapping for key: {} (pressed: {})", (int)reference, nativeScanCode);
        break;
    }

    return e->key() == reference;
  }
} // namespace WQtUtils
