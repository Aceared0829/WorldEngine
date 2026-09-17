#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <GuiFoundation/Models/LogModel.moc.h>


W_IMPLEMENT_SINGLETON(WQtLogPanel);

WQtLogPanel::WQtLogPanel(ads::CDockManager* pDockManager)
  : WQtApplicationPanel(pDockManager, "Panel.Log")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  setIcon(WQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Log.svg"));
  setWindowTitle(WMakeQString(WTranslate("Panel.Log")));
  setWidget(pDummy);

  EditorLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Editor Log"));
  EngineLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Engine Log"));
  CombinedLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Log"));

  WGlobalLog::AddLogWriter(WMakeDelegate(&WQtLogPanel::LogWriter, this));
  WEditorEngineProcessConnection::s_Events.AddEventHandler(WMakeDelegate(&WQtLogPanel::EngineProcessMsgHandler, this));

  QSettings Settings;
  Settings.beginGroup(QLatin1String("LogPanel"));
  {
    splitter->restoreState(Settings.value("Splitter", splitter->saveState()).toByteArray());
  }
  Settings.endGroup();

  connect(EditorLog->GetLog(), &WQtLogModel::NewErrorsOrWarnings, this, &WQtLogPanel::OnNewWarningsOrErrors);
  connect(EngineLog->GetLog(), &WQtLogModel::NewErrorsOrWarnings, this, &WQtLogPanel::OnNewWarningsOrErrors);
  connect(CombinedLog->GetLog(), &WQtLogModel::NewErrorsOrWarnings, this, &WQtLogPanel::OnNewWarningsOrErrors);

  WQtUiServices::GetSingleton()->s_Events.AddEventHandler(WMakeDelegate(&WQtLogPanel::UiServiceEventHandler, this));

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
  pPreferences->m_ChangedEvent.AddEventHandler(WMakeDelegate(&WQtLogPanel::OnPreferenceChange, this));

  m_bCombineLogs = pPreferences->m_bCombinedEditorAndEngineLogs;

  LogWidgets->setCurrentIndex(m_bCombineLogs ? 0 : 1);
}

WQtLogPanel::~WQtLogPanel()
{
  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
  pPreferences->m_ChangedEvent.RemoveEventHandler(WMakeDelegate(&WQtLogPanel::OnPreferenceChange, this));

  QSettings Settings;
  Settings.beginGroup(QLatin1String("LogPanel"));
  {
    Settings.setValue("Splitter", splitter->saveState());
  }
  Settings.endGroup();

  WGlobalLog::RemoveLogWriter(WMakeDelegate(&WQtLogPanel::LogWriter, this));
  WEditorEngineProcessConnection::s_Events.RemoveEventHandler(WMakeDelegate(&WQtLogPanel::EngineProcessMsgHandler, this));
  WQtUiServices::GetSingleton()->s_Events.RemoveEventHandler(WMakeDelegate(&WQtLogPanel::UiServiceEventHandler, this));
}

void WQtLogPanel::OnNewWarningsOrErrors(const char* szText, bool bError)
{
  m_uiKnownNumWarnings = EditorLog->GetLog()->GetNumSeriousWarnings() + EditorLog->GetLog()->GetNumWarnings() +
                         EngineLog->GetLog()->GetNumSeriousWarnings() + EngineLog->GetLog()->GetNumWarnings() +
                         CombinedLog->GetLog()->GetNumSeriousWarnings() + CombinedLog->GetLog()->GetNumWarnings();
  m_uiKnownNumErrors = EditorLog->GetLog()->GetNumErrors() + EngineLog->GetLog()->GetNumErrors() + CombinedLog->GetLog()->GetNumErrors();

  WQtUiServices::Event::TextType type = WQtUiServices::Event::Info;

  WUInt32 uiShowNumWarnings = 0;
  WUInt32 uiShowNumErrors = 0;

  if (m_uiKnownNumWarnings > m_uiIgnoreNumWarnings)
  {
    uiShowNumWarnings = m_uiKnownNumWarnings - m_uiIgnoreNumWarnings;
    type = WQtUiServices::Event::Warning;
  }
  else
  {
    m_uiIgnoreNumWarnings = m_uiKnownNumWarnings;
  }

  if (m_uiKnownNumErrors > m_uiIgnoredNumErrors)
  {
    uiShowNumErrors = m_uiKnownNumErrors - m_uiIgnoredNumErrors;
    type = WQtUiServices::Event::Error;
  }
  else
  {
    m_uiIgnoredNumErrors = m_uiKnownNumErrors;
  }

  WStringBuilder tmp;
  if (uiShowNumErrors > 0)
  {
    tmp.AppendFormat("{} Errors", uiShowNumErrors);
  }
  if (uiShowNumWarnings > 0)
  {
    tmp.AppendWithSeparator(",", " ");
    tmp.AppendFormat("{} Warnings", uiShowNumWarnings);
  }

  WQtUiServices::GetSingleton()->ShowAllDocumentsPermanentStatusBarMessage(tmp, type);

  if (!WStringUtils::IsNullOrEmpty(szText))
  {
    WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(
      WFmt("{}: {}", bError ? "Error" : "Warning", szText), WTime::MakeFromSeconds(10));
  }
}

void WQtLogPanel::ToolsProjectEventHandler(const WToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case WToolsProjectEvent::Type::ProjectClosing:
      CombinedLog->GetLog()->Clear();
      EditorLog->GetLog()->Clear();
      EngineLog->GetLog()->Clear();
      [[fallthrough]];

    case WToolsProjectEvent::Type::ProjectOpened:
      setEnabled(e.m_Type == WToolsProjectEvent::Type::ProjectOpened);
      break;

    default:
      break;
  }

  WQtApplicationPanel::ToolsProjectEventHandler(e);
}

void WQtLogPanel::LogWriter(const WLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  WLogEntry msg(e);

  if (m_bCombineLogs)
    CombinedLog->GetLog()->AddLogMsg(msg);
  else
    EditorLog->GetLog()->AddLogMsg(msg);

  if (msg.m_sTag == "EditorStatus")
  {
    WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(WFmt(msg.m_sMsg), WTime::MakeFromSeconds(5));
  }
}

void WQtLogPanel::EngineProcessMsgHandler(const WEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case WEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (const WLogMsgToEditor* pMsg = WDynamicCast<const WLogMsgToEditor*>(e.m_pMsg))
      {
        if (m_bCombineLogs)
          CombinedLog->GetLog()->AddLogMsg(pMsg->m_Entry);
        else
          EngineLog->GetLog()->AddLogMsg(pMsg->m_Entry);
      }
    }
    break;

    default:
      return;
  }
}

void WQtLogPanel::UiServiceEventHandler(const WQtUiServices::Event& e)
{
  if (e.m_Type == WQtUiServices::Event::ClickedDocumentPermanentStatusBarText)
  {
    EnsureVisible();

    m_uiIgnoredNumErrors = m_uiKnownNumErrors;
    m_uiIgnoreNumWarnings = m_uiKnownNumWarnings;

    WQtUiServices::GetSingleton()->ShowAllDocumentsPermanentStatusBarMessage(nullptr, WQtUiServices::Event::Info);
  }
}

void WQtLogPanel::OnPreferenceChange(WPreferences* pref)
{
  if (WEditorPreferencesUser* pPref = WDynamicCast<WEditorPreferencesUser*>(pref))
  {
    if (m_bCombineLogs != pPref->m_bCombinedEditorAndEngineLogs)
    {
      m_bCombineLogs = pPref->m_bCombinedEditorAndEngineLogs;

      LogWidgets->setCurrentIndex(m_bCombineLogs ? 0 : 1);
    }
  }
}
