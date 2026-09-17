#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Console/Console.h>
#include <EditorFramework/Panels/CVarPanel/CVarPanel.moc.h>
#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

W_IMPLEMENT_SINGLETON(WQtCVarPanel);

class WCommandInterpreterFwd : public WCommandInterpreter
{
public:
  virtual void Interpret(WCommandInterpreterState& inout_state) override
  {
    WConsoleCmdMsgToEngine msg;
    msg.m_iType = 0;
    msg.m_sCommand = inout_state.m_sInput;

    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  virtual void AutoComplete(WCommandInterpreterState& inout_state) override
  {
    WConsoleCmdMsgToEngine msg;
    msg.m_iType = 1;
    msg.m_sCommand = inout_state.m_sInput;

    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
};

WQtCVarPanel::WQtCVarPanel(ads::CDockManager* pDockManager)
  : WQtApplicationPanel(pDockManager, "Panel.CVar")
  , m_SingletonRegistrar(this)
{
  setIcon(WQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/CVar.svg"));
  setWindowTitle(WMakeQString(WTranslate("Panel.CVar")));
  m_pCVarWidget = new WQtCVarWidget(this);
  m_pCVarWidget->layout()->setContentsMargins(0, 0, 0, 0);
  // m_pCVarWidget->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pCVarWidget);

  WEditorEngineProcessConnection::s_Events.AddEventHandler(WMakeDelegate(&WQtCVarPanel::EngineProcessMsgHandler, this));

  connect(m_pCVarWidget, &WQtCVarWidget::onBoolChanged, this, &WQtCVarPanel::BoolChanged);
  connect(m_pCVarWidget, &WQtCVarWidget::onFloatChanged, this, &WQtCVarPanel::FloatChanged);
  connect(m_pCVarWidget, &WQtCVarWidget::onIntChanged, this, &WQtCVarPanel::IntChanged);
  connect(m_pCVarWidget, &WQtCVarWidget::onStringChanged, this, &WQtCVarPanel::StringChanged);

  m_pCVarWidget->GetConsole().SetCommandInterpreter(W_DEFAULT_NEW(WCommandInterpreterFwd));
}

WQtCVarPanel::~WQtCVarPanel()
{
  WEditorEngineProcessConnection::s_Events.RemoveEventHandler(WMakeDelegate(&WQtCVarPanel::EngineProcessMsgHandler, this));
}

void WQtCVarPanel::ToolsProjectEventHandler(const WToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case WToolsProjectEvent::Type::ProjectClosing:
      m_EngineCVarState.Clear();
      m_pCVarWidget->Clear();

      [[fallthrough]];

    case WToolsProjectEvent::Type::ProjectOpened:
      setEnabled(e.m_Type == WToolsProjectEvent::Type::ProjectOpened);
      break;

    default:
      break;
  }

  WQtApplicationPanel::ToolsProjectEventHandler(e);
}

void WQtCVarPanel::EngineProcessMsgHandler(const WEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case WEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<WCVarMsgToEditor>())
      {
        const WCVarMsgToEditor* pMsg = static_cast<const WCVarMsgToEditor*>(e.m_pMsg);

        bool bExisted = false;
        auto& cvar = m_EngineCVarState.FindOrAdd(pMsg->m_sName, &bExisted).Value();
        cvar.m_sDescription = pMsg->m_sDescription;
        cvar.m_sPlugin = pMsg->m_sPlugin;

        switch (pMsg->m_Value.GetType())
        {
          case WVariantType::Float:
            cvar.m_uiType = WCVarType::Float;
            cvar.m_fValue = pMsg->m_Value.ConvertTo<float>();
            break;
          case WVariantType::Int32:
            cvar.m_uiType = WCVarType::Int;
            cvar.m_iValue = pMsg->m_Value.ConvertTo<int>();
            break;
          case WVariantType::Bool:
            cvar.m_uiType = WCVarType::Bool;
            cvar.m_bValue = pMsg->m_Value.ConvertTo<bool>();
            break;
          case WVariantType::String:
            cvar.m_uiType = WCVarType::String;
            cvar.m_sValue = pMsg->m_Value.ConvertTo<WString>();
            break;
          default:
            break;
        }

        if (!bExisted)
          m_bRebuildUI = true;

        if (!m_bUpdateUI)
        {
          m_bUpdateUI = true;

          // don't do this every single time, otherwise we would spam this during project load
          QTimer::singleShot(100, this, SLOT(UpdateUI()));
        }
      }
      else if (auto pMsg = WDynamicCast<const WConsoleCmdResultMsgToEditor*>(e.m_pMsg))
      {
        m_sCommandResult.Append(pMsg->m_sResult.GetView());
        m_bUpdateConsole = true;
        QTimer::singleShot(100, this, SLOT(UpdateUI()));
      }
    }
    break;
    default:
      break;
  }
}

void WQtCVarPanel::UpdateUI()
{
  if (m_bRebuildUI)
  {
    m_pCVarWidget->RebuildCVarUI(m_EngineCVarState);
  }
  else if (m_bUpdateUI)
  {
    m_pCVarWidget->UpdateCVarUI(m_EngineCVarState);
  }

  if (m_bUpdateConsole)
  {
    m_pCVarWidget->AddConsoleStrings(m_sCommandResult);
  }

  m_sCommandResult.Clear();
  m_bUpdateConsole = false;
  m_bUpdateUI = false;
  m_bRebuildUI = false;
}

void WQtCVarPanel::BoolChanged(const char* szCVar, bool newValue)
{
  WChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtCVarPanel::FloatChanged(const char* szCVar, float newValue)
{
  WChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtCVarPanel::IntChanged(const char* szCVar, int newValue)
{
  WChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtCVarPanel::StringChanged(const char* szCVar, const char* newValue)
{
  WChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
