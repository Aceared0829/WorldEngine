#include <Editor/EditorPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Utilities/CommandLineOptions.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <shellscalingapi.h>
#endif

namespace
{
  /// Writes text to the console of the process that launched this one, if there is one.
  ///
  /// The editor is built as a window application, so it has no console of its own and printf goes
  /// nowhere by default. Attaching to the parent's console is what makes the output visible when it
  /// was started from a shell. When there is no console - the double-clicked case - the text is
  /// silently dropped, because there is nowhere to put it that would not block waiting for a click.
  void PrintToParentConsole(WStringView sText)
  {
    const WStringBuilder sOut(sText);

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
    // Fails when the parent has no console, and also when one is already attached - the latter is not
    // an error, that console can be written to just as well, so only the handle below decides.
    ::AttachConsole(ATTACH_PARENT_PROCESS);

    const HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);

    if (hOut == nullptr || hOut == INVALID_HANDLE_VALUE)
      return;

    // written through the handle rather than printf, because the CRT's stdout is not connected to a
    // console that was only attached just now
    DWORD uiWritten = 0;
    ::WriteFile(hOut, sOut.GetData(), sOut.GetElementCount(), &uiWritten, nullptr);
#else
    // every other platform this builds for has a normal stdout
    printf("%s", sOut.GetData());
    fflush(stdout);
#endif
  }
} // namespace

class WEditorApplication : public WApplication
{
public:
  using SUPER = WApplication;

  WEditorApplication()
    : WApplication("WEditor")
  {
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif
    EnableMemoryLeakReporting(true);

    m_pEditorApp = new WQtEditorApp;
  }

  virtual WResult BeforeCoreSystemsStartup() override
  {
    WStartup::AddApplicationTag("tool");
    WStartup::AddApplicationTag("editor");
    WStartup::AddApplicationTag("editorapp");

    WQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());

    return W_SUCCESS;
  }

  virtual void AfterCoreSystemsShutdown() override
  {
    WQtEditorApp::GetSingleton()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  virtual void Run() override
  {
    {
      WStringBuilder cmdHelp;
      if (WCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, WCommandLineOption::LogAvailableModes::IfHelpRequested, "_Editor;cvar"))
      {
        PrintToParentConsole(cmdHelp);

        QuitApplication();
        return;
      }
    }

    WQtEditorApp::GetSingleton()->StartupEditor();
    {
      const WInt32 iReturnCode = WQtEditorApp::GetSingleton()->RunEditor();
      SetReturnCode(iReturnCode);
    }
    WQtEditorApp::GetSingleton()->ShutdownEditor();

    QuitApplication();
  }

private:
  WQtEditorApp* m_pEditorApp;
};

W_APPLICATION_ENTRY_POINT(WEditorApplication);
