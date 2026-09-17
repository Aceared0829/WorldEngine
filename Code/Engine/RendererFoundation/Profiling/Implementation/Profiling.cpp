#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

#if W_ENABLED(W_USE_PROFILING)

struct GPUTimingScope
{
  W_DECLARE_POD_TYPE();

  WGALTimestampHandle m_BeginTimestamp;
  WGALTimestampHandle m_EndTimestamp;
  char m_szName[48];
};

class GPUProfilingSystem
{
public:
  static void ProcessTimestamps(const WGALDeviceEvent& e)
  {
    if (e.m_Type != WGALDeviceEvent::AfterEndFrame)
      return;

    while (!s_TimingScopes.IsEmpty())
    {
      auto& timingScope = s_TimingScopes.PeekFront();

      WTime beginTime;
      WTime endTime;
      WEnum<WGALAsyncResult> resBegin = e.m_pDevice->GetTimestampResult(timingScope.m_BeginTimestamp, beginTime);
      WEnum<WGALAsyncResult> resEnd = e.m_pDevice->GetTimestampResult(timingScope.m_EndTimestamp, endTime);

      if (resBegin == WGALAsyncResult::Expired || resEnd == WGALAsyncResult::Expired)
      {
        s_TimingScopes.PopFront();
      }

      if (resBegin == WGALAsyncResult::Ready && resEnd == WGALAsyncResult::Ready)
      {
        if (!beginTime.IsZero() && !endTime.IsZero())
        {
#  if W_ENABLED(W_COMPILE_FOR_DEBUG)
          static bool warnOnRingBufferOverun = true;
          if (warnOnRingBufferOverun && endTime < beginTime)
          {
            warnOnRingBufferOverun = false;
            WLog::Warning("Profiling end is before start, the DX11 timestamp ring buffer was probably overrun.");
          }
#  endif
          WProfilingSystem::AddGPUScope(timingScope.m_szName, beginTime, endTime);
        }

        s_TimingScopes.PopFront();
      }
      else
      {
        // Timestamps are not available yet
        break;
      }
    }
  }

  static GPUTimingScope& AllocateScope() { return s_TimingScopes.ExpandAndGetRef(); }

private:
  static void OnEngineStartup() { WGALDevice::GetDefaultDevice()->s_Events.AddEventHandler(&GPUProfilingSystem::ProcessTimestamps); }

  static void OnEngineShutdown()
  {
    s_TimingScopes.Clear();
    WGALDevice::GetDefaultDevice()->s_Events.RemoveEventHandler(&GPUProfilingSystem::ProcessTimestamps);
  }

  static WDeque<GPUTimingScope, WStaticsAllocatorWrapper> s_TimingScopes;

  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, GPUProfilingSystem);
};

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, GPUProfilingSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    GPUProfilingSystem::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    GPUProfilingSystem::OnEngineShutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WDeque<GPUTimingScope, WStaticsAllocatorWrapper> GPUProfilingSystem::s_TimingScopes;

//////////////////////////////////////////////////////////////////////////

GPUTimingScope* WProfilingScopeAndMarker::Start(WGALCommandEncoder* pCommandEncoder, const char* szName)
{
  pCommandEncoder->PushMarker(szName);

  auto& timingScope = GPUProfilingSystem::AllocateScope();
  timingScope.m_BeginTimestamp = pCommandEncoder->InsertTimestamp();
  WStringUtils::Copy(timingScope.m_szName, W_ARRAY_SIZE(timingScope.m_szName), szName);

  return &timingScope;
}

void WProfilingScopeAndMarker::Stop(WGALCommandEncoder* pCommandEncoder, GPUTimingScope*& ref_pTimingScope)
{
  pCommandEncoder->PopMarker();
  ref_pTimingScope->m_EndTimestamp = pCommandEncoder->InsertTimestamp();
  ref_pTimingScope = nullptr;
}

WProfilingScopeAndMarker::WProfilingScopeAndMarker(WGALCommandEncoder* pCommandEncoder, const char* szName)
  : WProfilingScope(szName, nullptr, WTime::MakeZero())
  , m_pCommandEncoder(pCommandEncoder)
{
  m_pTimingScope = Start(pCommandEncoder, szName);
}

WProfilingScopeAndMarker::~WProfilingScopeAndMarker()
{
  Stop(m_pCommandEncoder, m_pTimingScope);
}

#endif

W_STATICLINK_FILE(RendererFoundation, RendererFoundation_Profiling_Implementation_Profiling);
