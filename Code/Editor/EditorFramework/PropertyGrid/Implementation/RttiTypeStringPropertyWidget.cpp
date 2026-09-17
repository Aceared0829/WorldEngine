#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/RttiTypeStringPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>

WQtRttiTypeStringPropertyWidget::WQtRttiTypeStringPropertyWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new QPushButton(this);
  m_pButton->setText("Select Type");
  m_pButton->setObjectName("Button");

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  m_pLayout->addWidget(m_pButton);
}

void WQtRttiTypeStringPropertyWidget::OnInit()
{
  m_pMenu = new QMenu(m_pButton);
  m_pMenu->setToolTipsVisible(true);
  connect(m_pMenu, &QMenu::aboutToShow, this, &WQtRttiTypeStringPropertyWidget::onMenuAboutToShow);
  m_pButton->setMenu(m_pMenu);
  m_pButton->setObjectName("Button");

  connect(&m_TypeMenu, &WQtTypeMenu::TypeSelected, this, &WQtRttiTypeStringPropertyWidget::OnTypeSelected);
}

void WQtRttiTypeStringPropertyWidget::InternalSetValue(const WVariant& value)
{
  const WString sTypeName = value.ConvertTo<WString>();

  const WRTTI* pRtti = WRTTI::FindTypeByName(sTypeName);

  if (pRtti == nullptr)
  {
    m_pButton->setText("Select Type");
    m_pButton->setIcon(QIcon());
    return;
  }

  const WCategoryAttribute* pCatA = pRtti->GetAttributeByType<WCategoryAttribute>();
  const WColorAttribute* pColA = pRtti->GetAttributeByType<WColorAttribute>();

  WColor iconColor = WColor::MakeZero();

  if (pColA)
  {
    iconColor = pColA->GetColor();
  }
  else if (pCatA && iconColor == WColor::MakeZero())
  {
    iconColor = WColorScheme::GetCategoryColor(pCatA->GetCategory(), WColorScheme::CategoryColorUsage::MenuEntryIcon);
  }

  WStringBuilder sIconName;
  sIconName.Set(":/TypeIcons/", sTypeName, ".svg");
  const QIcon actionIcon = WQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);

  m_pButton->setText(sTypeName.GetData());
  m_pButton->setIcon(actionIcon);
}

void WQtRttiTypeStringPropertyWidget::onMenuAboutToShow()
{
  if (m_pMenu->isEmpty())
  {
    const WRttiTypeStringAttribute* pTypeAttr = m_pProp->GetAttributeByType<WRttiTypeStringAttribute>();
    const WRTTI* pBaseType = WRTTI::FindTypeByName(pTypeAttr->GetBaseType());

    m_TypeMenu.FillMenu(m_pMenu, pBaseType, true, false);
  }
}

void WQtRttiTypeStringPropertyWidget::OnTypeSelected(QString sTypeName)
{
  const WString typeName = sTypeName.toUtf8().data();

  BroadcastValueChanged(typeName);
}
