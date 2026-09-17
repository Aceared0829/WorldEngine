#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Logging/Log.h>

void WFileserverApp::FileserverEventHandlerConsole(const WFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case WFileserverEvent::Type::None:
      WLog::Error("Invalid Fileserver event type");
      break;

    case WFileserverEvent::Type::ServerStarted:
    {
      WLog::Info("WFileserver is running");
    }
    break;

    case WFileserverEvent::Type::ServerStopped:
    {
      WLog::Info("WFileserver was shut down");
    }
    break;

    case WFileserverEvent::Type::ClientConnected:
    {
      WLog::Success("Client connected");
    }
    break;

    case WFileserverEvent::Type::MountDataDir:
    {
      WLog::Info("Mounted data directory '{0}' ({1})", e.m_szName, e.m_szPath);
    }
    break;

    case WFileserverEvent::Type::UnmountDataDir:
    {
      WLog::Info("Unmount request for data directory '{0}' ({1})", e.m_szName, e.m_szPath);
    }
    break;

    case WFileserverEvent::Type::FileDownloadRequest:
    {
      if (e.m_FileState == WFileserveFileState::NonExistant)
        WLog::Dev("Request: (N/A) '{0}'", e.m_szPath);

      if (e.m_FileState == WFileserveFileState::SameHash)
        WLog::Dev("Request: (HASH) '{0}'", e.m_szPath);

      if (e.m_FileState == WFileserveFileState::SameTimestamp)
        WLog::Dev("Request: (TIME) '{0}'", e.m_szPath);

      if (e.m_FileState == WFileserveFileState::NonExistantEither)
        WLog::Dev("Request: (N/AE) '{0}'", e.m_szPath);

      if (e.m_FileState == WFileserveFileState::Different)
        WLog::Info("Request: '{0}' ({1} bytes)", e.m_szPath, e.m_uiSizeTotal);
    }
    break;

    case WFileserverEvent::Type::FileDownloading:
    {
      WLog::Debug("Transfer: {0}/{1} bytes", e.m_uiSentTotal, e.m_uiSizeTotal, e.m_szPath);
    }
    break;

    case WFileserverEvent::Type::FileDownloadFinished:
    {
      if (e.m_FileState == WFileserveFileState::Different)
        WLog::Info("Transfer done.");
    }
    break;

    case WFileserverEvent::Type::FileDeleteRequest:
    {
      WLog::Warning("File Deletion: '{0}'", e.m_szPath);
    }
    break;

    case WFileserverEvent::Type::FileUploading:
      WLog::Debug("Upload: {0}/{1} bytes", e.m_uiSentTotal, e.m_uiSizeTotal, e.m_szPath);
      break;

    case WFileserverEvent::Type::FileUploadFinished:
      WLog::Info("Upload finished: {0}", e.m_szPath);
      break;

    default:
      break;
  }
}
