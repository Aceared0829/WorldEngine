#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>

WMap<WString, QString> WQtDynamicEnumPropertyWidget::s_LastSearch;

WQtDynamicEnumPropertyWidget::WQtDynamicEnumPropertyWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new QPushButton(this);
  m_pButton->setText("Select");
  m_pButton->setStyleSheet("QPushButton { text-align:left; padding-left:5px; padding-top:3px; padding-bottom:3px; }");

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  m_pLayout->addWidget(m_pButton);
}

void WQtDynamicEnumPropertyWidget::OnInit()
{
  W_ASSERT_DEV(
    m_pProp->GetAttributeByType<WDynamicEnumAttribute>() != nullptr, "WQtDynamicEnumPropertyWidget was created without a WDynamicEnumAttribute!");

  WVariantType::Enum type = m_pProp->GetSpecificType()->GetVariantType();
  W_IGNORE_UNUSED(type);
  W_ASSERT_DEV(type != WVariantType::String && type != WVariantType::HashedString && type != WVariantType::StringView, "WDynamicEnumAttribute can't be used with string types");

  const WDynamicEnumAttribute* pAttr = m_pProp->GetAttributeByType<WDynamicEnumAttribute>();

  m_sEnumAttribute = pAttr->GetDynamicEnumName();

  m_pEnum = &WDynamicEnum::GetDynamicEnum(m_sEnumAttribute);



  m_pMenu = new QMenu(m_pButton);
  m_pMenu->setToolTipsVisible(false);
  connect(m_pMenu, &QMenu::aboutToShow, this, &WQtDynamicEnumPropertyWidget::onMenuAboutToShow);
  m_pButton->setMenu(m_pMenu);
}

void WQtDynamicEnumPropertyWidget::InternalSetValue(const WVariant& value)
{

  m_pButton->setText(WMakeQString(m_pEnum->GetValueName(value.ConvertTo<WInt64>())));
}

void WQtDynamicEnumPropertyWidget::onMenuAboutToShow()
{
  m_pMenu->clear();

  m_pSearchableMenu = new WQtSearchableMenu(m_pMenu);

  connect(m_pSearchableMenu, &WQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant)
    {
      if (variant.typeId() == QMetaType::QString)
      {
        if (variant.toString() == "<cmd>")
        {
          WActionManager::ExecuteAction({}, m_pEnum->GetEditCommand(), WActionContext(const_cast<WDocument*>(m_pGrid->GetDocument())), m_pEnum->GetEditCommandValue()).AssertSuccess();
        }
      }
      else
      {
        InternalSetValue(variant.toLongLong());
        BroadcastValueChanged(variant.toLongLong());
      }

      m_pMenu->close();
      //
    });

  connect(m_pSearchableMenu, &WQtSearchableMenu::SearchTextChanged, m_pMenu,
    [this](const QString& sText)
    { s_LastSearch[m_sEnumAttribute] = sText; });

  if (!m_pEnum->GetEditCommand().IsEmpty())
  {
    m_pSearchableMenu->AddItem("< Edit Values... >", "", QString("<cmd>"), QIcon(":/GuiFoundation/Icons/Edit.svg"));
  }

  const auto& AllValues = m_pEnum->GetAllValidValues();

  for (auto it = AllValues.GetIterator(); it.IsValid(); ++it)
  {
    m_pSearchableMenu->AddItem(it.Value(), "", it.Key());
  }

  m_pMenu->addAction(m_pSearchableMenu);

  // important to do this last to make sure the search bar gets focus
  m_pSearchableMenu->Finalize(s_LastSearch[m_sEnumAttribute]);
}
