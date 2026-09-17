#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/ThreadUtils.h>

static void OSFileEventHandler(const WOSFile::EventData& e)
{
  if (!WTelemetry::IsConnectedToClient())
    return;

  WTelemetryMessage Msg;
  Msg.GetWriter() << e.m_iFileID;

  switch (e.m_EventType)
  {
    case WOSFile::EventType::FileOpen:
    {
      Msg.SetMessageID('FILE', 'OPEN');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << (WUInt8)e.m_FileMode;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case WOSFile::EventType::FileRead:
    {
      Msg.SetMessageID('FILE', 'READ');
      Msg.GetWriter() << e.m_uiBytesAccessed;
    }
    break;

    case WOSFile::EventType::FileWrite:
    {
      Msg.SetMessageID('FILE', 'WRIT');
      Msg.GetWriter() << e.m_uiBytesAccessed;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case WOSFile::EventType::FileClose:
    {
      Msg.SetMessageID('FILE', 'CLOS');
    }
    break;

    case WOSFile::EventType::FileExists:
    case WOSFile::EventType::DirectoryExists:
    {
      Msg.SetMessageID('FILE', 'EXST');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case WOSFile::EventType::FileDelete:
    {
      Msg.SetMessageID('FILE', ' DEL');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case WOSFile::EventType::MakeDir:
    {
      Msg.SetMessageID('FILE', 'CDIR');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case WOSFile::EventType::FileCopy:
    {
      Msg.SetMessageID('FILE', 'COPY');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_sFile2;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case WOSFile::EventType::FileStat:
    {
      Msg.SetMessageID('FILE', 'STAT');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case WOSFile::EventType::FileCasing:
    {
      Msg.SetMessageID('FILE', 'CASE');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case WOSFile::EventType::None:
      break;
  }

  WUInt8 uiThreadType = 0;

  if (WThreadUtils::IsMainThread())
    uiThreadType = 1 << 0;
  else if (WTaskSystem::GetCurrentThreadWorkerType() == WWorkerThreadType::FileAccess)
    uiThreadType = 1 << 1;
  else
    uiThreadType = 1 << 2;

  Msg.GetWriter() << e.m_Duration.GetSeconds();
  Msg.GetWriter() << uiThreadType;

  WTelemetry::Broadcast(WTelemetry::Reliable, Msg);
}

void AddOSFileEventHandler()
{
  WOSFile::AddEventHandler(OSFileEventHandler);
}

void RemoveOSFileEventHandler()
{
  WOSFile::RemoveEventHandler(OSFileEventHandler);
}


