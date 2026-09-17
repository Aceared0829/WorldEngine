#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qspinbox.h>

class WCommandInterpreterInspector : public WCommandInterpreter
{
public:
  virtual void Interpret(WCommandInterpreterState& inout_state) override
  {
    WTelemetryMessage Msg;
    Msg.SetMessageID('CMD', 'EXEC');
    Msg.GetWriter() << inout_state.m_sInput;
    WTelemetry::SendToServer(Msg);
  }

  virtual void AutoComplete(WCommandInterpreterState& inout_state) override
  {
    WTelemetryMessage Msg;
    Msg.SetMessageID('CMD', 'COMP');
    Msg.GetWriter() << inout_state.m_sInput;
    WTelemetry::SendToServer(Msg);
  }
};

WQtCVarsWidget* WQtCVarsWidget::s_pWidget = nullptr;

WQtCVarsWidget::WQtCVarsWidget(ads::CDockManager* pDockManager, QWidget* pParent)
  : ads::CDockWidget(pDockManager, "CVars", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(CVarWidget);

  setIcon(QIcon(":/GuiFoundation/Icons/CVar.svg"));

  connect(CVarWidget, &WQtCVarWidget::onBoolChanged, this, &WQtCVarsWidget::BoolChanged);
  connect(CVarWidget, &WQtCVarWidget::onFloatChanged, this, &WQtCVarsWidget::FloatChanged);
  connect(CVarWidget, &WQtCVarWidget::onIntChanged, this, &WQtCVarsWidget::IntChanged);
  connect(CVarWidget, &WQtCVarWidget::onStringChanged, this, &WQtCVarsWidget::StringChanged);

  CVarWidget->GetConsole().SetCommandInterpreter(W_DEFAULT_NEW(WCommandInterpreterInspector));

  ResetStats();
}

void WQtCVarsWidget::ResetStats()
{
  m_CVarsBackup = m_CVars;
  m_CVars.Clear();
  CVarWidget->Clear();
}

void WQtCVarsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  WTelemetryMessage msg;

  bool bUpdateCVarsTable = false;
  bool bFillCVarsTable = false;

  while (WTelemetry::RetrieveMessage('CVAR', msg) == W_SUCCESS)
  {
    if (msg.GetMessageID() == ' CLR')
    {
      s_pWidget->m_CVars.Clear();
    }

    if (msg.GetMessageID() == 'SYNC')
    {
      for (auto it = s_pWidget->m_CVars.GetIterator(); it.IsValid(); ++it)
      {
        auto var = s_pWidget->m_CVarsBackup.Find(it.Key());

        if (var.IsValid() && it.Value().m_uiType == var.Value().m_uiType)
        {
          it.Value().m_bValue = var.Value().m_bValue;
          it.Value().m_fValue = var.Value().m_fValue;
          it.Value().m_sValue = var.Value().m_sValue;
          it.Value().m_iValue = var.Value().m_iValue;
        }
      }

      s_pWidget->CVarWidget->RebuildCVarUI(s_pWidget->m_CVars);

      s_pWidget->SyncAllCVarsToServer();
    }

    if (msg.GetMessageID() == 'DATA')
    {
      WString sName;
      msg.GetReader() >> sName;

      WCVarWidgetData& sd = s_pWidget->m_CVars[sName];

      msg.GetReader() >> sd.m_sPlugin;
      msg.GetReader() >> sd.m_uiType;
      msg.GetReader() >> sd.m_sDescription;

      switch (sd.m_uiType)
      {
        case WCVarType::Bool:
          msg.GetReader() >> sd.m_bValue;
          break;
        case WCVarType::Float:
          msg.GetReader() >> sd.m_fValue;
          break;
        case WCVarType::Int:
          msg.GetReader() >> sd.m_iValue;
          break;
        case WCVarType::String:
          msg.GetReader() >> sd.m_sValue;
          break;
      }

      if (sd.m_bNewEntry)
        bUpdateCVarsTable = true;

      bFillCVarsTable = true;
    }
  }

  if (bUpdateCVarsTable)
    s_pWidget->CVarWidget->RebuildCVarUI(s_pWidget->m_CVars);
  else if (bFillCVarsTable)
    s_pWidget->CVarWidget->UpdateCVarUI(s_pWidget->m_CVars);
}

void WQtCVarsWidget::ProcessTelemetryConsole(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  WTelemetryMessage msg;
  WStringBuilder tmp;

  while (WTelemetry::RetrieveMessage('CMD', msg) == W_SUCCESS)
  {
    if (msg.GetMessageID() == 'RES')
    {
      msg.GetReader() >> tmp;
      s_pWidget->CVarWidget->AddConsoleStrings(tmp);
    }
  }
}

void WQtCVarsWidget::SyncAllCVarsToServer()
{
  for (auto it = m_CVars.GetIterator(); it.IsValid(); ++it)
    SendCVarUpdateToServer(it.Key().GetData(), it.Value());
}

void WQtCVarsWidget::SendCVarUpdateToServer(WStringView sName, const WCVarWidgetData& cvd)
{
  WTelemetryMessage Msg;
  Msg.SetMessageID('SVAR', ' SET');
  Msg.GetWriter() << sName;
  Msg.GetWriter() << cvd.m_uiType;

  switch (cvd.m_uiType)
  {
    case WCVarType::Bool:
      Msg.GetWriter() << cvd.m_bValue;
      break;

    case WCVarType::Float:
      Msg.GetWriter() << cvd.m_fValue;
      break;

    case WCVarType::Int:
      Msg.GetWriter() << cvd.m_iValue;
      break;

    case WCVarType::String:
      Msg.GetWriter() << cvd.m_sValue;
      break;
  }

  WTelemetry::SendToServer(Msg);
}

void WQtCVarsWidget::BoolChanged(WStringView sCVar, bool newValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_bValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void WQtCVarsWidget::FloatChanged(WStringView sCVar, float newValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_fValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void WQtCVarsWidget::IntChanged(WStringView sCVar, int newValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_iValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void WQtCVarsWidget::StringChanged(WStringView sCVar, WStringView sNewValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_sValue = sNewValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}
