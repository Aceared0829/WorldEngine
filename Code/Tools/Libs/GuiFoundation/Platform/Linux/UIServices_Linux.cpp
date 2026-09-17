#include <GuiFoundation/GuiFoundationPCH.h>

#if W_ENABLED(W_PLATFORM_LINUX)

#  include <GuiFoundation/UIServices/UIServices.moc.h>

void WQtUiServices::OpenInExplorer(WStringView sPath, bool bIsFile)
{
  QStringList args;
  WStringBuilder parentDir;

  if (bIsFile)
  {
    parentDir = sPath;
    parentDir = parentDir.GetFileDirectory();
    sPath = parentDir.GetData();
  }
  args << QDir::toNativeSeparators(WMakeQString(sPath));

  QProcess::startDetached("xdg-open", args);
}

void WQtUiServices::OpenWith(WStringView sPath0)
{
  WStringBuilder sPath = sPath0;
  sPath.MakeCleanPath();
  sPath.MakePathSeparatorsNative();

  WLog::Error("WQtUiServices::OpenWith() not implemented on Linux");
}

WStatus WQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  WLog::Error("WQtUiServices::OpenInVsCode() not implemented on Linux");
  return WStatus(W_FAILURE);
}

#endif
