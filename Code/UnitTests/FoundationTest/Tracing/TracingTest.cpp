#include <FoundationTest/FoundationTestPCH.h>

// This test emits various trace events (including cross-thread) to exercise the tracing code path. Use an external tool to capture and verify events (see Utilities/Tracing/Capture-Trace.ps1).

#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>
#include <FoundationTest/Tracing/TraceProvider.h>

namespace
{
  class TracingTestThread : public WThread
  {
  public:
    TracingTestThread(WUInt64 uiAsyncId)
      : WThread("TracingTestThread")
      , m_uiAsyncId(uiAsyncId)
    {
    }

    virtual WUInt32 Run() override
    {
      W_TRACE_SCOPE("WorkerThreadScope", WTraceLevel::Info);
      W_TRACE_EVENT("WorkerThreadEvent", WTraceLevel::Info,
        W_TRACE_VALUE("ThreadValue", (WInt32)99));

      // BEGIN-DOCS-CODE-SNIPPET: tracing-async-activity-end
      // Complete the async activity that was started on the main thread.
      W_TRACE_ASYNC_END("CrossThreadActivity", m_uiAsyncId);
      // END-DOCS-CODE-SNIPPET
      return 0;
    }

  private:
    WUInt64 m_uiAsyncId;
  };
} // namespace

W_CREATE_SIMPLE_TEST_GROUP(Tracing);

W_CREATE_SIMPLE_TEST(Tracing, EmitEvents)
{
  W_LOG_BLOCK("Tracing Test Start");
  WLog::Info("Visible In Trace");

  W_TEST_BLOCK(WTestBlock::Enabled, "Emit Events")
  {
    // BEGIN-DOCS-CODE-SNIPPET: tracing-instant-event
    // Instant event demonstrating all supported value types.
    const void* pDemoPtr = nullptr;
    W_TRACE_EVENT("TestInstantEvent", WTraceLevel::Info,
      W_TRACE_VALUE("BoolField", true),
      W_TRACE_VALUE("Int8Field", (WInt8)-1),
      W_TRACE_VALUE("Int16Field", (WInt16)-16),
      W_TRACE_VALUE("Int32Field", (WInt32)42),
      W_TRACE_VALUE("Int64Field", (WInt64)1234567890LL),
      W_TRACE_VALUE("UInt8Field", (WUInt8)255),
      W_TRACE_VALUE("UInt16Field", (WUInt16)65535),
      W_TRACE_VALUE("UInt32Field", (WUInt32)100),
      W_TRACE_VALUE("UInt64Field", (WUInt64)9876543210ULL),
      W_TRACE_VALUE("FloatField", 3.14f),
      W_TRACE_VALUE("DoubleField", 2.71828),
      W_TRACE_VALUE("StringField", "hello"),
      W_TRACE_VALUE("PointerField", pDemoPtr));
    // END-DOCS-CODE-SNIPPET

    // BEGIN-DOCS-CODE-SNIPPET: tracing-scoped-event
    // Scoped event (RAII begin + end).
    {
      W_TRACE_SCOPE("TestScopedWork", WTraceLevel::Verbose,
        W_TRACE_VALUE("Detail", "scope-payload"));
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(5));
    }
    // END-DOCS-CODE-SNIPPET

    // BEGIN-DOCS-CODE-SNIPPET: tracing-manual-scope
    // Manual scope begin + end (for cases where RAII is not applicable).
    W_TRACE_SCOPE_BEGIN("TestManualScope", WTraceLevel::Info,
      W_TRACE_VALUE("Detail", "manual-scope-payload"));
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(5));
    W_TRACE_SCOPE_END("TestManualScope");
    // END-DOCS-CODE-SNIPPET

    // BEGIN-DOCS-CODE-SNIPPET: tracing-async-activity-start
    // Async activity that spans across threads.
    const WUInt64 uiAsyncId = 123456789ULL;
    W_TRACE_ASYNC_BEGIN("CrossThreadActivity", uiAsyncId, WTraceLevel::Info,
      W_TRACE_VALUE("Resource", "test-resource.dat"));
    // END-DOCS-CODE-SNIPPET

    // Worker thread: emits its own events and completes the async activity.
    WUniquePtr<TracingTestThread> pThread = W_DEFAULT_NEW(TracingTestThread, uiAsyncId);
    pThread->Start();
    pThread->Join();


    // BEGIN-DOCS-CODE-SNIPPET: tracing-flush
    // Flush buffered events to the tracing backend.
    W_TRACE_FLUSH();
    // END-DOCS-CODE-SNIPPET
  }
}
