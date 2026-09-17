#include <Inspector/InspectorPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/RenderGraphWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>
#include <QApplication>
#include <QSettings>
#include <qstylefactory.h>

class WInspectorApp : public WApplication
{
public:
  using SUPER = WApplication;

  WInspectorApp()
    : WApplication("WInspector")
  {
  }

  void SetStyleSheet()
  {
    QApplication::setStyle(QStyleFactory::create("fusion"));
    QPalette palette;

    palette.setColor(QPalette::WindowText, QColor(200, 200, 200, 255));
    palette.setColor(QPalette::Button, QColor(100, 100, 100, 255));
    palette.setColor(QPalette::Light, QColor(97, 97, 97, 255));
    palette.setColor(QPalette::Midlight, QColor(59, 59, 59, 255));
    palette.setColor(QPalette::Dark, QColor(37, 37, 37, 255));
    palette.setColor(QPalette::Mid, QColor(45, 45, 45, 255));
    palette.setColor(QPalette::Text, QColor(200, 200, 200, 255));
    palette.setColor(QPalette::BrightText, QColor(37, 37, 37, 255));
    palette.setColor(QPalette::ButtonText, QColor(200, 200, 200, 255));
    palette.setColor(QPalette::Base, QColor(42, 42, 42, 255));
    palette.setColor(QPalette::Window, QColor(68, 68, 68, 255));
    palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 255));
    palette.setColor(QPalette::Highlight, QColor(103, 141, 178, 255));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255, 255));
    palette.setColor(QPalette::Link, QColor(0, 0, 238, 255));
    palette.setColor(QPalette::LinkVisited, QColor(82, 24, 139, 255));
    palette.setColor(QPalette::AlternateBase, QColor(46, 46, 46, 255));
    QBrush NoRoleBrush(QColor(0, 0, 0, 255), Qt::NoBrush);
    palette.setBrush(QPalette::NoRole, NoRoleBrush);
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220, 255));
    palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0, 255));

    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128, 255));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(80, 80, 80, 255));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105, 255));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255, 255));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128, 255));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(86, 117, 148, 255));

    QApplication::setPalette(palette);
  }

  virtual WResult BeforeCoreSystemsStartup() override
  {
    WStartup::AddApplicationTag("tool");
    WStartup::AddApplicationTag("inspector");

    return WApplication::BeforeCoreSystemsStartup();
  }

  virtual void Run() override
  {
    int iArgs = GetArgumentCount();
    char** cArgs = (char**)GetArgumentsArray();

    QApplication app(iArgs, cArgs);
    QCoreApplication::setOrganizationDomain("www.ezengine.net");
    QCoreApplication::setOrganizationName("WorldEngine Project");
    QCoreApplication::setApplicationName("WInspector");
    QCoreApplication::setApplicationVersion("1.0.0");

    SetStyleSheet();

    WQtMainWindow MainWindow;

    WTelemetry::AcceptMessagesForSystem('CVAR', true, WQtCVarsWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('CMD', true, WQtCVarsWidget::ProcessTelemetryConsole, nullptr);
    WTelemetry::AcceptMessagesForSystem(' LOG', true, WQtLogDockWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem(' MEM', true, WQtMemoryWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('TIME', true, WQtTimeWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem(' APP', true, WQtMainWindow::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('FILE', true, WQtFileWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('INPT', true, WQtInputWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('STRT', true, WQtSubsystemsWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('STAT', true, WQtMainWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('PLUG', true, WQtPluginsWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('EVNT', true, WQtGlobalEventsWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('RFLC', true, WQtReflectionWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('TRAN', true, WQtDataWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('RESM', true, WQtResourceWidget::ProcessTelemetry, nullptr);
    WTelemetry::AcceptMessagesForSystem('RGPH', true, WQtRenderGraphWidget::ProcessTelemetry, nullptr);

    QSettings Settings;
    const QString sServer = Settings.value("LastConnection", QLatin1String("localhost:1040")).toString();

    // -url and -port can override the stored connection address
    const WStringView sUrlArg = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-url");
    const WInt32 iPortArg = WCommandLineUtils::GetGlobalInstance()->GetIntOption("-port", -1);

    WStringBuilder sConnectTo(sServer.toUtf8().data());
    if (!sUrlArg.IsEmpty() || iPortArg > 0)
    {
      const WStringView sHost = sUrlArg.IsEmpty() ? WStringView("localhost") : sUrlArg;
      if (iPortArg > 0)
        sConnectTo.SetFormat("{}:{}", sHost, iPortArg);
      else
        sConnectTo = sHost;

      // Persist the command-line override so the Connect dialog reflects it
      const QString sConnectToQt = QString::fromUtf8(sConnectTo.GetData());
      Settings.setValue("LastConnection", sConnectToQt);
      MainWindow.SetConnectionTarget(sConnectToQt);
    }

    WTelemetry::ConnectToServer(sConnectTo).IgnoreResult();

    MainWindow.show();
    SetReturnCode(app.exec());

    WTelemetry::CloseConnection();
    QuitApplication();
  }
};

W_APPLICATION_ENTRY_POINT(WInspectorApp);
