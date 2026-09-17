#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>

WQtSubsystemsWidget* WQtSubsystemsWidget::s_pWidget = nullptr;

WQtSubsystemsWidget::WQtSubsystemsWidget(ads::CDockManager* pDockManager, QWidget* pParent)
  : ads::CDockWidget(pDockManager, "Subsystem Widget", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(TableSubsystems);

  setIcon(QIcon(":/Icons/Icons/Subsystem.svg"));

  ResetStats();
}

void WQtSubsystemsWidget::ResetStats()
{
  m_bUpdateSubsystems = true;
  m_Subsystems.Clear();
}


void WQtSubsystemsWidget::UpdateStats()
{
  UpdateSubSystems();
}

void WQtSubsystemsWidget::UpdateSubSystems()
{
  if (!m_bUpdateSubsystems)
    return;

  m_bUpdateSubsystems = false;

  WQtScopedUpdatesDisabled _1(TableSubsystems);

  TableSubsystems->clear();

  TableSubsystems->setRowCount(m_Subsystems.GetCount());

  QStringList Headers;
  Headers.append("");
  Headers.append(" SubSystem ");
  Headers.append(" Plugin ");
  Headers.append(" Startup Done ");
  Headers.append(" Dependencies ");

  TableSubsystems->setColumnCount(static_cast<int>(Headers.size()));

  TableSubsystems->setHorizontalHeaderLabels(Headers);

  {
    WStringBuilder sTemp;
    WInt32 iRow = 0;

    for (WMap<WString, SubsystemData>::Iterator it = m_Subsystems.GetIterator(); it.IsValid(); ++it)
    {
      const SubsystemData& ssd = it.Value();

      QLabel* pIcon = new QLabel();
      QIcon icon = WQtUiServices::GetCachedIconResource(":/Icons/Icons/Subsystem.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableSubsystems->setCellWidget(iRow, 0, pIcon);

      sTemp.SetFormat("  {0}  ", it.Key());
      TableSubsystems->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));

      sTemp.SetFormat("  {0}  ", ssd.m_sPlugin);
      TableSubsystems->setCellWidget(iRow, 2, new QLabel(sTemp.GetData()));

      if (ssd.m_bStartupDone[WStartupStage::HighLevelSystems])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#00aa00;\">  Engine  </span></p>"));
      else if (ssd.m_bStartupDone[WStartupStage::CoreSystems])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#5555ff;\">  Core  </span></p>"));
      else if (ssd.m_bStartupDone[WStartupStage::BaseSystems])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#cece00;\">  Base  </span></p>"));
      else
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#ff0000;\">Not Initialized</span></p>"));

      ((QLabel*)TableSubsystems->cellWidget(iRow, 3))->setAlignment(Qt::AlignHCenter);

      sTemp.SetFormat("  {0}  ", ssd.m_sDependencies);
      TableSubsystems->setCellWidget(iRow, 4, new QLabel(sTemp.GetData()));

      ++iRow;
    }
  }

  TableSubsystems->resizeColumnsToContents();
}

void WQtSubsystemsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  WTelemetryMessage Msg;

  while (WTelemetry::RetrieveMessage('STRT', Msg) == W_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case ' CLR':
      {
        s_pWidget->m_Subsystems.Clear();
        s_pWidget->m_bUpdateSubsystems = true;
      }
      break;

      case 'SYST':
      {
        WString sGroup, sSystem;

        Msg.GetReader() >> sGroup;
        Msg.GetReader() >> sSystem;

        WStringBuilder sFinalName = sGroup.GetData();
        sFinalName.Append("::");
        sFinalName.Append(sSystem.GetData());

        SubsystemData& ssd = s_pWidget->m_Subsystems[sFinalName.GetData()];

        Msg.GetReader() >> ssd.m_sPlugin;

        for (WUInt32 i = 0; i < WStartupStage::ENUM_COUNT; ++i)
          Msg.GetReader() >> ssd.m_bStartupDone[i];

        WUInt8 uiDependencies = 0;
        Msg.GetReader() >> uiDependencies;

        WStringBuilder sAllDeps;

        WString sDep;
        for (WUInt8 i = 0; i < uiDependencies; ++i)
        {
          Msg.GetReader() >> sDep;

          if (sAllDeps.IsEmpty())
            sAllDeps = sDep.GetData();
          else
          {
            sAllDeps.Append(" | ");
            sAllDeps.Append(sDep.GetData());
          }
        }

        ssd.m_sDependencies = sAllDeps.GetData();

        s_pWidget->m_bUpdateSubsystems = true;
      }
      break;
    }
  }
}
