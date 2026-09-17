#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/PluginsWidget.moc.h>

WQtPluginsWidget* WQtPluginsWidget::s_pWidget = nullptr;

WQtPluginsWidget::WQtPluginsWidget(ads::CDockManager* pDockManager, QWidget* pParent)
  : ads::CDockWidget(pDockManager, "Plugins Widget", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(TablePlugins);

  setIcon(QIcon(":/Icons/Icons/Plugin.svg"));

  ResetStats();
}

void WQtPluginsWidget::ResetStats()
{
  m_bUpdatePlugins = true;
  m_Plugins.Clear();
}


void WQtPluginsWidget::UpdateStats()
{
  UpdatePlugins();
}

void WQtPluginsWidget::UpdatePlugins()
{
  if (!m_bUpdatePlugins)
    return;

  m_bUpdatePlugins = false;

  WQtScopedUpdatesDisabled _1(TablePlugins);

  TablePlugins->clear();

  TablePlugins->setRowCount(m_Plugins.GetCount());

  QStringList Headers;
  Headers.append("");
  Headers.append(" Plugin ");
  Headers.append(" Reloadable ");
  Headers.append(" Dependencies ");

  TablePlugins->setColumnCount(static_cast<int>(Headers.size()));

  TablePlugins->setHorizontalHeaderLabels(Headers);

  {
    WStringBuilder sTemp;
    WInt32 iRow = 0;

    for (WMap<WString, PluginsData>::Iterator it = m_Plugins.GetIterator(); it.IsValid(); ++it)
    {
      QLabel* pIcon = new QLabel();
      QIcon icon = WQtUiServices::GetCachedIconResource(":/Icons/Icons/Plugin.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TablePlugins->setCellWidget(iRow, 0, pIcon);

      sTemp.SetFormat("  {0}  ", it.Key());
      TablePlugins->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));

      if (it.Value().m_bReloadable)
        TablePlugins->setCellWidget(iRow, 2, new QLabel("<p><span style=\"font-weight:600; color:#00aa00;\">  Yes  </span></p>"));
      else
        TablePlugins->setCellWidget(iRow, 2, new QLabel("<p><span style=\"font-weight:600; color:#ffaa00;\">  No  </span></p>"));

      ((QLabel*)TablePlugins->cellWidget(iRow, 2))->setAlignment(Qt::AlignHCenter);

      TablePlugins->setCellWidget(iRow, 3, new QLabel(it.Value().m_sDependencies.GetData()));

      ++iRow;
    }
  }

  TablePlugins->resizeColumnsToContents();
}

void WQtPluginsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  WTelemetryMessage Msg;

  while (WTelemetry::RetrieveMessage('PLUG', Msg) == W_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case ' CLR':
      {
        s_pWidget->m_Plugins.Clear();
        s_pWidget->m_bUpdatePlugins = true;
      }
      break;

      case 'DATA':
      {
        WString sName;

        Msg.GetReader() >> sName;

        PluginsData& pd = s_pWidget->m_Plugins[sName.GetData()];
        Msg.GetReader() >> pd.m_bReloadable;

        Msg.GetReader() >> pd.m_sDependencies;

        s_pWidget->m_bUpdatePlugins = true;
      }
      break;
    }
  }
}
