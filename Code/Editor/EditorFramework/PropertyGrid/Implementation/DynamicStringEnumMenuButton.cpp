#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/EditDynamicEnumsDlg.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumMenuButton.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <ToolsFoundation/Document/Document.h>

WMap<WString, QString> WQtDynamicStringEnumMenuButton::s_LastSearch;

WQtDynamicStringEnumMenuButton::WQtDynamicStringEnumMenuButton(QWidget* pParent)
  : QPushButton(pParent)
{
  setText("Select");
  setStyleSheet("QPushButton { text-align:left; padding-left:5px; padding-top:3px; padding-bottom:3px; }");

  m_pMenu = new QMenu(this);
  m_pMenu->setToolTipsVisible(false);
  connect(m_pMenu, &QMenu::aboutToShow, this, &WQtDynamicStringEnumMenuButton::onMenuAboutToShow);
  setMenu(m_pMenu);
}

void WQtDynamicStringEnumMenuButton::SetEnum(WStringView sEnumName)
{
  m_sEnumName = sEnumName;
  m_pEnum = &WDynamicStringEnum::GetDynamicEnum(m_sEnumName);
}

void WQtDynamicStringEnumMenuButton::SetCurrentValue(WStringView sValue)
{
  setText(WMakeQString(sValue));
}

void WQtDynamicStringEnumMenuButton::onMenuAboutToShow()
{
  if (m_pEnum == nullptr)
    return;

  m_pMenu->clear();

  m_pSearchableMenu = new WQtSearchableMenu(m_pMenu);

  connect(m_pSearchableMenu, &WQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant)
    {
      if (variant.toString() == "<item>")
      {
        Q_EMIT ValueSelected(sName);
      }
      else if (variant.toString() == "<edit>")
      {
        WQtEditDynamicEnumsDlg dlg(m_pEnum, this);
        if (dlg.exec() == QDialog::Accepted)
        {
          WInt32 iEnum = dlg.GetSelectedItem();
          if (iEnum >= 0)
          {
            Q_EMIT ValueSelected(WMakeQString(m_pEnum->GetAllValidValues()[iEnum]));
          }
        }
      }
      else if (variant.toString() == "<cmd>")
      {
        WActionManager::ExecuteAction({}, m_pEnum->GetEditCommand(), WActionContext(const_cast<WDocument*>(m_pDocument)), m_pEnum->GetEditCommandValue()).AssertSuccess();
      }

      m_pMenu->close(); });

  connect(m_pSearchableMenu, &WQtSearchableMenu::SearchTextChanged, m_pMenu,
    [this](const QString& sText)
    { s_LastSearch[m_sEnumName] = sText; });

  {
    WDynamicStringEnum::RefreshValuesEvent e;
    e.m_sEnumName = m_sEnumName;
    e.m_pDocument = m_pDocument;
    e.m_pEnum = m_pEnum;
    WDynamicStringEnum::s_RefreshValuesEvent.Broadcast(e);
  }

  for (const auto& val : m_pEnum->GetAllValidValues())
  {
    m_pSearchableMenu->AddItem(val, "", QString("<item>"));
  }

  if (!m_pEnum->GetEditCommand().IsEmpty())
  {
    m_pSearchableMenu->AddItem("< Edit Values... >", "", QString("<cmd>"), QIcon(":/GuiFoundation/Icons/Edit.svg"));
  }
  else if (!m_pEnum->GetStorageFile().IsEmpty())
  {
    m_pSearchableMenu->AddItem("< Edit Values... >", "", QString("<edit>"), QIcon(":/GuiFoundation/Icons/Edit.svg"));
  }

  m_pMenu->addAction(m_pSearchableMenu);

  // important to do this last to make sure the search bar gets focus
  m_pSearchableMenu->Finalize(s_LastSearch[m_sEnumName]);
}
