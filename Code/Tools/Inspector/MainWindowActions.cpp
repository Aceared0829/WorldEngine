#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/RenderGraphWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>

void WQtMainWindow::on_ActionShowWindowLog_triggered()
{
  WQtLogDockWidget::s_pWidget->toggleView(ActionShowWindowLog->isChecked());
  WQtLogDockWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowMemory_triggered()
{
  WQtMemoryWidget::s_pWidget->toggleView(ActionShowWindowMemory->isChecked());
  WQtMemoryWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowTime_triggered()
{
  WQtTimeWidget::s_pWidget->toggleView(ActionShowWindowTime->isChecked());
  WQtTimeWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowInput_triggered()
{
  WQtInputWidget::s_pWidget->toggleView(ActionShowWindowInput->isChecked());
  WQtInputWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowCVar_triggered()
{
  WQtCVarsWidget::s_pWidget->toggleView(ActionShowWindowCVar->isChecked());
  WQtCVarsWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowReflection_triggered()
{
  WQtReflectionWidget::s_pWidget->toggleView(ActionShowWindowReflection->isChecked());
  WQtReflectionWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowSubsystems_triggered()
{
  WQtSubsystemsWidget::s_pWidget->toggleView(ActionShowWindowSubsystems->isChecked());
  WQtSubsystemsWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowPlugins_triggered()
{
  WQtPluginsWidget::s_pWidget->toggleView(ActionShowWindowPlugins->isChecked());
  WQtPluginsWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowFile_triggered()
{
  WQtFileWidget::s_pWidget->toggleView(ActionShowWindowFile->isChecked());
  WQtFileWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowGlobalEvents_triggered()
{
  WQtGlobalEventsWidget::s_pWidget->toggleView(ActionShowWindowGlobalEvents->isChecked());
  WQtGlobalEventsWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowData_triggered()
{
  WQtDataWidget::s_pWidget->toggleView(ActionShowWindowData->isChecked());
  WQtDataWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowResource_triggered()
{
  WQtResourceWidget::s_pWidget->toggleView(ActionShowWindowResource->isChecked());
  WQtResourceWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionShowWindowRenderGraph_triggered()
{
  WQtRenderGraphWidget::s_pWidget->toggleView(ActionShowWindowRenderGraph->isChecked());
  WQtRenderGraphWidget::s_pWidget->raise();
}

void WQtMainWindow::on_ActionOnTopWhenConnected_triggered()
{
  SetAlwaysOnTop(WhenConnected);
}

void WQtMainWindow::on_ActionAlwaysOnTop_triggered()
{
  SetAlwaysOnTop(Always);
}

void WQtMainWindow::on_ActionNeverOnTop_triggered()
{
  SetAlwaysOnTop(Never);
}
