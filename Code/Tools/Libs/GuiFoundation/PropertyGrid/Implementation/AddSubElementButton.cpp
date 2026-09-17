#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QPushButton>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WQtAddSubElementButton::WQtAddSubElementButton(WEnum<WPropertyCategory> containerCategory, WStringView sButtonText)
  : WQtPropertyWidget()
{
  m_ContainerCategory = containerCategory;
  // Reset base class size policy as we are put in a layout that would cause us to vanish instead.
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new QPushButton(this);
  m_pButton->setText(WMakeQString(sButtonText));
  m_pButton->setIcon(QIcon(":/GuiFoundation/Icons/Add.svg"));
  m_pButton->setObjectName("Button");

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  m_pLayout->addSpacerItem(new QSpacerItem(0, 0));
  m_pLayout->setStretch(0, 1);
  m_pLayout->addWidget(m_pButton);
  m_pLayout->addSpacerItem(new QSpacerItem(0, 0));
  m_pLayout->setStretch(2, 1);

  m_pMenu = nullptr;
}

void WQtAddSubElementButton::OnInit()
{
  if (m_pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
  {
    m_pMenu = new QMenu(m_pButton);
    m_pMenu->setToolTipsVisible(true);
    connect(m_pMenu, &QMenu::aboutToShow, this, &WQtAddSubElementButton::onMenuAboutToShow);
    m_pButton->setMenu(m_pMenu);
    m_pButton->setObjectName("Button");

    connect(&m_TypeMenu, &WQtTypeMenu::TypeSelected, this, &WQtAddSubElementButton::OnTypeSelected);
  }

  if (const WMaxArraySizeAttribute* pAttr = m_pProp->GetAttributeByType<WMaxArraySizeAttribute>())
  {
    m_uiMaxElements = pAttr->GetMaxSize();
  }

  if (const WPreventDuplicatesAttribute* pAttr = m_pProp->GetAttributeByType<WPreventDuplicatesAttribute>())
  {
    m_bPreventDuplicates = true;
  }

  QMetaObject::connectSlotsByName(this);
}

void WQtAddSubElementButton::onMenuAboutToShow()
{
  if (m_Items.IsEmpty())
    return;

  if (m_pMenu->isEmpty())
  {
    auto pProp = GetProperty();

    m_TypeMenu.FillMenu(m_pMenu, pProp->GetSpecificType(), pProp->GetFlags().IsSet(WPropertyFlags::Pointer), m_bPreventDuplicates);
  }

  if (m_uiMaxElements > 0) // 0 means unlimited
  {
    QList<QAction*> actions = m_pMenu->actions();

    for (auto& item : m_Items)
    {
      WInt32 iCount = 0;
      m_pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount).AssertSuccess();

      if (iCount >= (WInt32)m_uiMaxElements)
      {
        if (!m_bNoMoreElementsAllowed)
        {
          m_bNoMoreElementsAllowed = true;

          QAction* pAction = new QAction(QString("Maximum allowed elements in array is %1").arg(m_uiMaxElements));
          m_pMenu->insertAction(actions.isEmpty() ? nullptr : actions[0], pAction);

          for (auto pAct : actions)
          {
            pAct->setEnabled(false);
          }
        }

        return;
      }
    }

    if (m_bNoMoreElementsAllowed)
    {
      for (auto pAct : actions)
      {
        pAct->setEnabled(true);
      }

      m_bNoMoreElementsAllowed = false;
      delete m_pMenu->actions()[0]; // remove the dummy action
    }
  }

  if (m_bPreventDuplicates)
  {
    WSet<const WRTTI*> UsedTypes;

    for (auto& item : m_Items)
    {
      WInt32 iCount = 0;
      m_pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount).AssertSuccess();

      for (WInt32 i = 0; i < iCount; ++i)
      {
        WUuid guid = m_pObjectAccessor->Get<WUuid>(item.m_pObject, m_pProp, i);

        if (guid.IsValid())
        {
          UsedTypes.Insert(m_pObjectAccessor->GetObject(guid)->GetType());
        }
      }

      QList<QAction*> actions = m_pMenu->actions();
      for (auto pAct : actions)
      {
        const WRTTI* pRtti = static_cast<const WRTTI*>(pAct->property("type").value<void*>());

        pAct->setEnabled(!UsedTypes.Contains(pRtti));
      }
    }
  }
}

void WQtAddSubElementButton::on_Button_clicked()
{
  auto pProp = GetProperty();

  if (!pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
  {
    OnAction(pProp->GetSpecificType());
  }
}

void WQtAddSubElementButton::OnTypeSelected(QString sTypeName)
{
  const WString typeName = sTypeName.toUtf8().data();

  OnAction(WRTTI::FindTypeByName(typeName));
}

void WQtAddSubElementButton::OnAction(const WRTTI* pRtti)
{
  W_ASSERT_DEV(pRtti != nullptr, "user data retrieval failed");
  WVariant index = (WInt32)-1;

  if (m_ContainerCategory == WPropertyCategory::Map)
  {
    QString text;
    bool bOk = false;
    while (!bOk)
    {
      text = QInputDialog::getText(this, "Set map key for new element", "Key:", QLineEdit::Normal, text, &bOk);
      if (!bOk)
        return;

      index = text.toUtf8().data();
      for (auto& item : m_Items)
      {
        WVariant value;
        WStatus res = m_pObjectAccessor->GetValue(item.m_pObject, m_pProp, value, index);
        if (res.Succeeded())
        {
          bOk = false;
          break;
        }
      }
      if (!bOk)
      {
        WQtUiServices::GetSingleton()->MessageBoxInformation("The selected key is already used in the selection.");
      }
    }
  }

  m_pObjectAccessor->StartTransaction("Add Element");

  WStatus res(W_SUCCESS);
  const bool bIsValueType = WReflectionUtils::IsValueType(m_pProp);
  if (bIsValueType)
  {
    for (auto& item : m_Items)
    {
      if (m_uiMaxElements > 0 && m_pObjectAccessor->GetCount(item.m_pObject, m_pProp) >= (int)m_uiMaxElements)
      {
        res = WStatus("Maximum number of allowed elements reached.");
        break;
      }
      else
      {
        res = m_pObjectAccessor->InsertValue(item.m_pObject, m_pProp, WReflectionUtils::GetDefaultValue(GetProperty(), index), index);
        if (res.Failed())
          break;
      }
    }
  }
  else if (GetProperty()->GetFlags().IsSet(WPropertyFlags::Class))
  {
    for (auto& item : m_Items)
    {
      if (m_uiMaxElements > 0 && m_pObjectAccessor->GetCount(item.m_pObject, m_pProp) >= (int)m_uiMaxElements)
      {
        res = WStatus("Maximum number of allowed elements reached.");
        break;
      }
      else
      {
        WUuid guid;
        res = m_pObjectAccessor->AddObject(item.m_pObject, m_pProp, index, pRtti, guid);
        if (res.Failed())
          break;

        WTempHybridArray<WPropertySelection, 1> selection;
        selection.PushBack({m_pObjectAccessor->GetObject(guid), WVariant()});
        WDefaultObjectState defaultState(m_pType, m_pObjectAccessor, selection);
        defaultState.RevertObject().AssertSuccess();
      }
    }
  }

  if (res.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}
