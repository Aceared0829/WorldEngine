#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>

static WInt32 iTestData1 = 0;
static WInt32 iTestData2 = 0;

// The following event handlers are automatically registered, nothing else needs to be done here

W_ON_GLOBAL_EVENT(TestGlobalEvent1)
{
  iTestData1 += param0.Get<WInt32>();
}

W_ON_GLOBAL_EVENT(TestGlobalEvent2)
{
  iTestData2 += param0.Get<WInt32>();
}

W_ON_GLOBAL_EVENT_ONCE(TestGlobalEvent3)
{
  // this handler will be executed only once, even if the event is broadcast multiple times
  iTestData2 += 42;
}

static bool g_bFirstRun = true;

W_CREATE_SIMPLE_TEST(Communication, GlobalEvent)
{
  iTestData1 = 0;
  iTestData2 = 0;

  W_TEST_INT(iTestData1, 0);
  W_TEST_INT(iTestData2, 0);

  WGlobalEvent::Broadcast("TestGlobalEvent1", 1);

  W_TEST_INT(iTestData1, 1);
  W_TEST_INT(iTestData2, 0);

  WGlobalEvent::Broadcast("TestGlobalEvent1", 2);

  W_TEST_INT(iTestData1, 3);
  W_TEST_INT(iTestData2, 0);

  WGlobalEvent::Broadcast("TestGlobalEvent1", 3);

  W_TEST_INT(iTestData1, 6);
  W_TEST_INT(iTestData2, 0);

  WGlobalEvent::Broadcast("TestGlobalEvent2", 4);

  W_TEST_INT(iTestData1, 6);
  W_TEST_INT(iTestData2, 4);

  WGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  W_TEST_INT(iTestData1, 6);

  if (g_bFirstRun)
  {
    g_bFirstRun = false;
    W_TEST_INT(iTestData2, 46);
  }
  else
  {
    W_TEST_INT(iTestData2, 4);
    iTestData2 += 42;
  }

  WGlobalEvent::Broadcast("TestGlobalEvent2", 5);

  W_TEST_INT(iTestData1, 6);
  W_TEST_INT(iTestData2, 51);

  WGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  W_TEST_INT(iTestData1, 6);
  W_TEST_INT(iTestData2, 51);

  WGlobalEvent::Broadcast("TestGlobalEvent2", 6);

  W_TEST_INT(iTestData1, 6);
  W_TEST_INT(iTestData2, 57);

  WGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  W_TEST_INT(iTestData1, 6);
  W_TEST_INT(iTestData2, 57);

  WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);

  WGlobalEvent::PrintGlobalEventStatistics();

  WGlobalLog::RemoveLogWriter(WLogWriter::Console::LogMessageHandler);
}
