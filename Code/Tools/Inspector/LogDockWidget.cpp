#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Logging/LogEntry.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qlistwidget.h>

WQtLogDockWidget* WQtLogDockWidget::s_pWidget = nullptr;

WQtLogDockWidget::WQtLogDockWidget(ads::CDockManager* pDockManager, QWidget* pParent)
  : ads::CDockWidget(pDockManager, "Log", pParent)
{
  s_pWidget = this;
  setupUi(this);
  LogWidget->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Log"));

  setIcon(QIcon(":/Icons/Icons/Log.svg"));

  this->setWidget(LogWidget);
}

void WQtLogDockWidget::ResetStats()
{
  LogWidget->GetLog()->Clear();
}

void WQtLogDockWidget::Log(const WFormatString& text)
{
  WStringBuilder tmp;

  WLogEntry lm;
  lm.m_sMsg = text.GetText(tmp);
  lm.m_Type = WLogMsgType::InfoMsg;
  lm.m_uiIndentation = 0;
  LogWidget->GetLog()->AddLogMsg(lm);
}

void WQtLogDockWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  WTelemetryMessage Msg;

  while (WTelemetry::RetrieveMessage(' LOG', Msg) == W_SUCCESS)
  {
    WLogEntry lm;
    WInt8 iEventType = 0;

    Msg.GetReader() >> iEventType;
    Msg.GetReader() >> lm.m_uiIndentation;
    Msg.GetReader() >> lm.m_sTag;
    Msg.GetReader() >> lm.m_sMsg;

    if (iEventType == WLogMsgType::EndGroup)
      Msg.GetReader() >> lm.m_fSeconds;

    lm.m_Type = (WLogMsgType::Enum)iEventType;
    s_pWidget->LogWidget->GetLog()->AddLogMsg(lm);
  }
}
