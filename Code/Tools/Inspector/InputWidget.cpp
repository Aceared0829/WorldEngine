#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qlistwidget.h>

WQtInputWidget* WQtInputWidget::s_pWidget = nullptr;

WQtInputWidget::WQtInputWidget(ads::CDockManager* pDockManager, QWidget* pParent)
  : ads::CDockWidget(pDockManager, "Input Widget", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(InputWidgetFrame);

  setIcon(QIcon(":/Icons/Icons/InputActions.svg"));

  ResetStats();
}

void WQtInputWidget::ResetStats()
{
  ClearSlots();
  ClearActions();
}

void WQtInputWidget::ClearSlots()
{
  m_InputSlots.Clear();
  TableInputSlots->clear();

  {
    QStringList Headers;
    Headers.append("");
    Headers.append(" Slot ");
    Headers.append(" State ");
    Headers.append(" Value ");
    Headers.append(" Dead Zone ");
    Headers.append(" Flags (Binary) ");

    TableInputSlots->setColumnCount(static_cast<int>(Headers.size()));

    TableInputSlots->setHorizontalHeaderLabels(Headers);
    TableInputSlots->horizontalHeader()->show();
  }
}

void WQtInputWidget::ClearActions()
{
  m_InputActions.Clear();
  TableInputActions->clear();

  {
    QStringList Headers;
    Headers.append("");
    Headers.append(" Action ");
    Headers.append(" State ");
    Headers.append(" Value ");

    for (WInt32 slot = 0; slot < WInputActionConfig::MaxInputSlotAlternatives; ++slot)
      Headers.append(QString(" Slot %1 ").arg(slot + 1));

    TableInputActions->setColumnCount(static_cast<int>(Headers.size()));

    TableInputActions->setHorizontalHeaderLabels(Headers);
    TableInputActions->horizontalHeader()->show();
  }
}

void WQtInputWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  WTelemetryMessage msg;

  bool bUpdateSlotTable = false;
  bool bFillSlotTable = false;
  bool bUpdateActionTable = false;
  bool bFillActionTable = false;

  while (WTelemetry::RetrieveMessage('INPT', msg) == W_SUCCESS)
  {
    if (msg.GetMessageID() == 'SLOT')
    {
      WString sSlotName;
      msg.GetReader() >> sSlotName;

      SlotData& sd = s_pWidget->m_InputSlots[sSlotName];

      msg.GetReader() >> sd.m_uiSlotFlags;

      WUInt8 uiKeyState = 0;
      msg.GetReader() >> uiKeyState;
      sd.m_KeyState = (WKeyState::Enum)uiKeyState;

      msg.GetReader() >> sd.m_fValue;
      msg.GetReader() >> sd.m_fDeadZone;

      if (sd.m_iTableRow == -1)
        bUpdateSlotTable = true;

      bFillSlotTable = true;
    }

    if (msg.GetMessageID() == 'ACTN')
    {
      WString sInputSetName;
      msg.GetReader() >> sInputSetName;

      WString sActionName;
      msg.GetReader() >> sActionName;

      WStringBuilder sFinalName = sInputSetName.GetData();
      sFinalName.Append("::");
      sFinalName.Append(sActionName.GetData());

      ActionData& sd = s_pWidget->m_InputActions[sFinalName.GetData()];

      WUInt8 uiKeyState = 0;
      msg.GetReader() >> uiKeyState;
      sd.m_KeyState = (WKeyState::Enum)uiKeyState;

      msg.GetReader() >> sd.m_fValue;
      msg.GetReader() >> sd.m_bUseTimeScaling;

      for (WUInt32 i = 0; i < WInputActionConfig::MaxInputSlotAlternatives; ++i)
      {
        msg.GetReader() >> sd.m_sTrigger[i];
        msg.GetReader() >> sd.m_fTriggerScaling[i];
      }

      if (sd.m_iTableRow == -1)
        bUpdateActionTable = true;

      bFillActionTable = true;
    }
  }

  if (bUpdateSlotTable)
    s_pWidget->UpdateSlotTable(true);
  else if (bFillSlotTable)
    s_pWidget->UpdateSlotTable(false);

  if (bUpdateActionTable)
    s_pWidget->UpdateActionTable(true);
  else if (bFillActionTable)
    s_pWidget->UpdateActionTable(false);
}

void WQtInputWidget::UpdateSlotTable(bool bRecreate)
{
  WQtScopedUpdatesDisabled _1(TableInputSlots);

  if (bRecreate)
  {
    TableInputSlots->clear();
    TableInputSlots->setRowCount(m_InputSlots.GetCount());

    QStringList Headers;
    Headers.append("");
    Headers.append(" Slot ");
    Headers.append(" State ");
    Headers.append(" Value ");
    Headers.append(" Dead Zone ");
    Headers.append(" Flags (Binary) ");

    TableInputSlots->setColumnCount(static_cast<int>(Headers.size()));

    TableInputSlots->setHorizontalHeaderLabels(Headers);
    TableInputSlots->horizontalHeader()->show();

    WStringBuilder sTemp;

    WInt32 iRow = 0;
    for (WMap<WString, SlotData>::Iterator it = m_InputSlots.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      sTemp.SetFormat("  {0}  ", it.Key());

      QLabel* pIcon = new QLabel();
      QIcon icon = WQtUiServices::GetCachedIconResource(":/Icons/Icons/InputSlots.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableInputSlots->setCellWidget(iRow, 0, pIcon);

      TableInputSlots->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));
      TableInputSlots->setCellWidget(iRow, 2, new QLabel("????????????"));
      TableInputSlots->setCellWidget(iRow, 3, new QLabel("????????"));
      TableInputSlots->setCellWidget(iRow, 4, new QLabel("????????"));
      TableInputSlots->setCellWidget(iRow, 5, new QLabel("????????????????"));

      const WUInt32 uiFlags = it.Value().m_uiSlotFlags;

      // Flags
      {
        WStringBuilder sFlags;
        sFlags.SetPrintf("  %16b  ", uiFlags);

        QLabel* pFlags = (QLabel*)TableInputSlots->cellWidget(iRow, 5);
        pFlags->setAlignment(Qt::AlignRight);
        pFlags->setText(sFlags.GetData());
      }

      // Flags Tooltip
      {
        // in VS 2012 at least the snprintf fails when "yes" and "no" are passed directly, instead of as const char* variables
        const char* szYes = "<b>yes</b>";
        const char* szNo = "no";

        WStringBuilder tt("<p>");
        tt.AppendFormat("ReportsRelativeValues: {0}<br>", (uiFlags & WInputSlotFlags::ReportsRelativeValues) ? szYes : szNo);
        tt.AppendFormat("ValueBinaryZeroOrOne: {0}<br>", (uiFlags & WInputSlotFlags::ValueBinaryZeroOrOne) ? szYes : szNo);
        tt.AppendFormat("ValueRangeZeroToOne: {0}<br>", (uiFlags & WInputSlotFlags::ValueRangeZeroToOne) ? szYes : szNo);
        tt.AppendFormat("ValueRangeZeroToInf: {0}<br>", (uiFlags & WInputSlotFlags::ValueRangeZeroToInf) ? szYes : szNo);
        tt.AppendFormat("Pressable: {0}<br>", (uiFlags & WInputSlotFlags::Pressable) ? szYes : szNo);
        tt.AppendFormat("Holdable: {0}<br>", (uiFlags & WInputSlotFlags::Holdable) ? szYes : szNo);
        tt.AppendFormat("HalfAxis: {0}<br>", (uiFlags & WInputSlotFlags::HalfAxis) ? szYes : szNo);
        tt.AppendFormat("FullAxis: {0}<br>", (uiFlags & WInputSlotFlags::FullAxis) ? szYes : szNo);
        tt.AppendFormat("RequiresDeadZone: {0}<br>", (uiFlags & WInputSlotFlags::RequiresDeadZone) ? szYes : szNo);
        tt.AppendFormat("ValuesAreNonContinuous: {0}<br>", (uiFlags & WInputSlotFlags::ValuesAreNonContinuous) ? szYes : szNo);
        tt.AppendFormat("ActivationDependsOnOthers: {0}<br>", (uiFlags & WInputSlotFlags::ActivationDependsOnOthers) ? szYes : szNo);
        tt.AppendFormat("NeverTimeScale: {0}<br>", (uiFlags & WInputSlotFlags::NeverTimeScale) ? szYes : szNo);
        tt.Append("</p>");

        TableInputSlots->cellWidget(iRow, 5)->setToolTip(tt.GetData());
      }

      ++iRow;
    }

    TableInputSlots->resizeColumnsToContents();
  }

  {
    WStringBuilder sTemp;

    WInt32 iRow = 0;
    for (WMap<WString, SlotData>::Iterator it = m_InputSlots.GetIterator(); it.IsValid(); ++it)
    {
      QLabel* pState = (QLabel*)TableInputSlots->cellWidget(iRow, 2);
      pState->setAlignment(Qt::AlignHCenter);

      switch (it.Value().m_KeyState)
      {
        case WKeyState::Down:
          pState->setText("Down");
          break;
        case WKeyState::Pressed:
          pState->setText("Pressed");
          break;
        case WKeyState::Released:
          pState->setText("Released");
          break;
        case WKeyState::Up:
          pState->setText("");
          break;
      }

      // Value
      {
        QLabel* pValue = (QLabel*)TableInputSlots->cellWidget(iRow, 3);
        pValue->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fValue == 0.0f)
          pValue->setText("");
        else
        {
          sTemp.SetFormat(" {0} ", WArgF(it.Value().m_fValue, 4));
          pValue->setText(sTemp.GetData());
        }
      }

      // Dead-zone
      {
        QLabel* pDeadZone = (QLabel*)TableInputSlots->cellWidget(iRow, 4);
        pDeadZone->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fDeadZone == 0.0f)
          pDeadZone->setText("");
        else
          pDeadZone->setText(QString::number(it.Value().m_fDeadZone, 'f', 2));
      }

      ++iRow;
    }
  }
}

void WQtInputWidget::UpdateActionTable(bool bRecreate)
{
  WQtScopedUpdatesDisabled _1(TableInputActions);

  if (bRecreate)
  {
    TableInputActions->clear();
    TableInputActions->setRowCount(m_InputActions.GetCount());

    QStringList Headers;
    Headers.append("");
    Headers.append(" Action ");
    Headers.append(" State ");
    Headers.append(" Value ");

    for (WInt32 slot = 0; slot < WInputActionConfig::MaxInputSlotAlternatives; ++slot)
      Headers.append(QString(" Slot %1 ").arg(slot + 1));

    TableInputActions->setColumnCount(static_cast<int>(Headers.size()));

    TableInputActions->setHorizontalHeaderLabels(Headers);
    TableInputActions->horizontalHeader()->show();

    WStringBuilder sTemp;

    WInt32 iRow = 0;
    for (WMap<WString, ActionData>::Iterator it = m_InputActions.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      sTemp.SetFormat("  {0}  ", it.Key());

      QLabel* pIcon = new QLabel();
      QIcon icon = WQtUiServices::GetCachedIconResource(":/Icons/Icons/InputActions.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableInputActions->setCellWidget(iRow, 0, pIcon);

      TableInputActions->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));
      TableInputActions->setCellWidget(iRow, 2, new QLabel("????????????"));
      TableInputActions->setCellWidget(iRow, 3, new QLabel("????????????????????????"));
      TableInputActions->setCellWidget(iRow, 4, new QLabel(""));
      TableInputActions->setCellWidget(iRow, 5, new QLabel(""));
      TableInputActions->setCellWidget(iRow, 6, new QLabel(""));

      // Trigger Slots

      for (WInt32 slot = 0; slot < WInputActionConfig::MaxInputSlotAlternatives; ++slot)
      {
        if (it.Value().m_sTrigger[slot].IsEmpty())
          sTemp = "  ";
        else
          sTemp.SetFormat("  [Scale: {0}] {1}  ", WArgF(it.Value().m_fTriggerScaling[slot], 2), it.Value().m_sTrigger[slot]);

        QLabel* pValue = (QLabel*)TableInputActions->cellWidget(iRow, 4 + slot);
        pValue->setText(sTemp.GetData());
      }

      ++iRow;
    }

    TableInputActions->resizeColumnsToContents();
  }

  {
    WStringBuilder sTemp;

    WInt32 iRow = 0;
    for (WMap<WString, ActionData>::Iterator it = m_InputActions.GetIterator(); it.IsValid(); ++it)
    {
      QLabel* pState = (QLabel*)TableInputActions->cellWidget(iRow, 2);
      pState->setAlignment(Qt::AlignHCenter);

      switch (it.Value().m_KeyState)
      {
        case WKeyState::Down:
          pState->setText("Down");
          break;
        case WKeyState::Pressed:
          pState->setText("Pressed");
          break;
        case WKeyState::Released:
          pState->setText("Released");
          break;
        case WKeyState::Up:
          pState->setText("");
          break;
      }

      // Value
      {
        QLabel* pValue = (QLabel*)TableInputActions->cellWidget(iRow, 3);
        pValue->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fValue == 0.0f)
          pValue->setText("");
        else
        {
          if (it.Value().m_bUseTimeScaling)
            sTemp.SetFormat(" {0} (Time-Scaled) ", WArgF(it.Value().m_fValue, 4));
          else
            sTemp.SetFormat(" {0} (Absolute) ", WArgF(it.Value().m_fValue, 4));

          pValue->setText(sTemp.GetData());
        }
      }


      ++iRow;
    }
  }
}

void WQtInputWidget::on_ButtonClearSlots_clicked()
{
  ClearSlots();
}

void WQtInputWidget::on_ButtonClearActions_clicked()
{
  ClearActions();
}
