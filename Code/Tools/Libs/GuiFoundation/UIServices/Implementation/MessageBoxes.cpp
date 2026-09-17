#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

const char* WQtUiServices::GetOwnVersionString()
{
  return W_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_MAJOR) "." W_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_MINOR) "." W_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_PATCH);
}

void WQtUiServices::MessageBoxStatus(const WStatus& s, const char* szFailureMsg, const char* szSuccessMsg, bool bOnlySuccessMsgIfDetails)
{
  WStringBuilder sResult;

  if (s.Succeeded())
  {
    if (WStringUtils::IsNullOrEmpty(szSuccessMsg))
      return;

    if (bOnlySuccessMsgIfDetails && s.GetMessageString().IsEmpty())
      return;

    sResult = szSuccessMsg;

    if (!s.GetMessageString().IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n{0}", s.GetMessageString());

    MessageBoxInformation(sResult);
  }
  else
  {
    sResult = szFailureMsg;

    if (!s.GetMessageString().IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n{0}", s.GetMessageString());

    MessageBoxWarning(sResult);
  }
}

void WQtUiServices::MessageBoxInformation(const WFormatString& msg, WStringView sDontShowAgainID)
{
  WStringBuilder tmp;

  if (IsUnattended())
    ReportSuppressedDialog(WStringBuilder("Information: ", msg.GetText(tmp)));
  else
  {
    QMessageBox box(QApplication::activeWindow());
    box.setWindowTitle(WApplication::GetApplicationInstance()->GetApplicationName().GetData());
    box.setText(QString::fromUtf8(msg.GetTextCStr(tmp)));
    box.setIcon(QMessageBox::Icon::Information);

    QSettings Settings;
    Settings.beginGroup(GetOwnVersionString());
    Settings.beginGroup("msgbox-dontshow");

    if (!sDontShowAgainID.IsEmpty())
    {
      ShowAllDocumentsTemporaryStatusBarMessage(msg, WTime::Seconds(3));

      if (Settings.value(sDontShowAgainID.GetData(tmp), 0) != 0)
      {
        // don't show the messagebox again was selected at some point
        return;
      }

      box.setCheckBox(new QCheckBox("Don't show again"));
    }

    box.exec();

    if (box.checkBox() && box.checkBox()->isChecked())
    {
      Settings.setValue(sDontShowAgainID.GetData(tmp), 1);
    }
  }
}

void WQtUiServices::MessageBoxWarning(const WFormatString& msg)
{
  WStringBuilder tmp;

  if (IsUnattended())
    ReportSuppressedDialog(WStringBuilder("Warning: ", msg.GetText(tmp)));
  else
  {
    QMessageBox::warning(QApplication::activeWindow(), WApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(tmp)), QMessageBox::StandardButton::Ok);
  }
}

QMessageBox::StandardButton WQtUiServices::MessageBoxQuestion(const WFormatString& msg, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton, QMessageBox::StandardButton unattendedButton)
{
  if (IsUnattended())
  {
    WStringBuilder tmp;
    ReportSuppressedDialog(WStringBuilder("Question, answered automatically: ", msg.GetText(tmp)));

    return unattendedButton;
  }
  else
  {
    WStringBuilder tmp;

    return QMessageBox::question(QApplication::activeWindow(), WApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(tmp)), buttons, defaultButton);
  }
}
