#pragma once

#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Types/UniquePtr.h>

/// A stand-alone application for the WFileServer.
///
/// If W_USE_QT is defined, the GUI from the EditorPluginFileserve is used. Otherwise the server runs as a console application.
///
/// If the command line option "-fs_wait_timeout seconds" is specified, the server waits for a limited time for any client to
/// connect and closes automatically, if no connection is established. Once a client connects, this timeout becomes irrelevant.
/// If the command line option "-fs_close_timeout seconds" is specified, the application automatically shuts down when no
/// client is connected anymore and a certain timeout is reached. Once a client connects, the timeout is reset.
/// This timeout has no effect as long as no client has connected.
class WFileserverApp : public WApplication
{
public:
  using SUPER = WApplication;

  WFileserverApp()
    : WApplication("Fileserve")
  {
  }

  virtual WResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual void Run() override;
  void FileserverEventHandlerConsole(const WFileserverEvent& e);
  void FileserverEventHandler(const WFileserverEvent& e);

  void ShaderMessageHandler(WFileserveClientContext& ref_ctxt, WRemoteMessage& ref_msg, WRemoteInterface& ref_clientChannel, WDelegate<void(const char*)> logActivity);

  WUInt32 m_uiSleepCounter = 0;
  WUInt32 m_uiConnections = 0;
  WTime m_CloseAppTimeout;
  WTime m_TimeTillClosing;
};
