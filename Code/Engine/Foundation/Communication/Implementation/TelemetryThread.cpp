#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/Thread.h>

class WTelemetryThread : public WThread
{
public:
  WTelemetryThread()
    : WThread("WTelemetryThread")
  {
    m_bKeepRunning = true;
  }

  volatile bool m_bKeepRunning;

private:
  virtual WUInt32 Run()
  {
    WTime LastPing;

    while (m_bKeepRunning)
    {
      WTelemetry::UpdateNetwork();

      // Send a Ping every once in a while
      if (WTelemetry::s_ConnectionMode == WTelemetry::Client)
      {
        WTime tNow = WTime::Now();

        if (tNow - LastPing > WTime::MakeFromMilliseconds(500))
        {
          LastPing = tNow;

          WTelemetry::UpdateServerPing();
        }
      }

      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    }

    return 0;
  }
};

static WTelemetryThread* g_pBroadcastThread = nullptr;
WMutex WTelemetry::s_TelemetryMutex;


WMutex& WTelemetry::GetTelemetryMutex()
{
  return s_TelemetryMutex;
}

void WTelemetry::StartTelemetryThread()
{
  if (!g_pBroadcastThread)
  {
    g_pBroadcastThread = W_DEFAULT_NEW(WTelemetryThread);
    g_pBroadcastThread->Start();
  }
}

void WTelemetry::StopTelemetryThread()
{
  if (g_pBroadcastThread)
  {
    g_pBroadcastThread->m_bKeepRunning = false;
    g_pBroadcastThread->Join();

    W_DEFAULT_DELETE(g_pBroadcastThread);
  }
}
