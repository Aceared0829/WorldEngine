#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Threading/Thread.h>
#include <TestFramework/Utilities/TestLogInterface.h>

W_CREATE_SIMPLE_TEST_GROUP(Logging);

namespace
{

  class LogTestLogInterface : public WLogInterface
  {
  public:
    virtual void HandleLogMessage(const WLoggingEventData& le) override
    {
      switch (le.m_EventType)
      {
        case WLogMsgType::Flush:
          m_Result.Append("[Flush]\n");
          return;
        case WLogMsgType::BeginGroup:
          m_Result.Append(">", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::EndGroup:
          m_Result.Append("<", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::ErrorMsg:
          m_Result.Append("E:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::SeriousWarningMsg:
          m_Result.Append("SW:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::WarningMsg:
          m_Result.Append("W:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::SuccessMsg:
          m_Result.Append("S:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::InfoMsg:
          m_Result.Append("I:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::DevMsg:
          m_Result.Append("E:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::DebugMsg:
          m_Result.Append("D:", le.m_sTag, " ", le.m_sText, "\n");
          break;

        default:
          W_REPORT_FAILURE("Invalid msg type");
          break;
      }
    }

    WStringBuilder m_Result;
  };

} // namespace

W_CREATE_SIMPLE_TEST(Logging, Log)
{
  LogTestLogInterface log;
  LogTestLogInterface log2;
  WLogSystemScope logScope(&log);

  W_TEST_BLOCK(WTestBlock::Enabled, "Output")
  {
    W_LOG_BLOCK("Verse 1", "Portal: Still Alive");

    WLog::GetThreadLocalLogSystem()->SetLogLevel(WLogMsgType::All);

    WLog::Success("{0}", "This was a triumph.");
    WLog::Info("{0}", "I'm making a note here:");
    WLog::Error("{0}", "Huge Success");
    WLog::Info("{0}", "It's hard to overstate my satisfaction.");
    WLog::Dev("{0}", "Aperture Science. We do what we must, because we can,");
    WLog::Dev("{0}", "For the good of all of us, except the ones who are dead.");
    WLog::Flush();
    WLog::Flush(); // second flush should be ignored

    {
      W_LOG_BLOCK("Verse 2");

      WLog::GetThreadLocalLogSystem()->SetLogLevel(WLogMsgType::DevMsg);

      WLog::Dev("But there's no sense crying over every mistake.");
      WLog::Debug("You just keep on trying 'till you run out of cake.");
      WLog::Info("And the science gets done, and you make a neat gun");
      WLog::Error("for the people who are still alive.");
    }

    {
      W_LOG_BLOCK("Verse 3");

      WLog::GetThreadLocalLogSystem()->SetLogLevel(WLogMsgType::InfoMsg);

      WLog::Info("I'm not even angry.");
      WLog::Debug("I'm being so sincere right now.");
      WLog::Dev("Even though you broke my heart and killed me.");
      WLog::Info("And tore me to pieces,");
      WLog::Dev("and threw every piece into a fire.");
      WLog::Info("As they burned it hurt because I was so happy for you.");
      WLog::Error("Now these points of data make a beautiful line");
      WLog::Dev("and we're off the beta, we're releasing on time.");
      WLog::Flush();
      WLog::Flush();

      {
        W_LOG_BLOCK("Verse 4");

        WLog::GetThreadLocalLogSystem()->SetLogLevel(WLogMsgType::SuccessMsg);

        WLog::Info("So I'm glad I got burned,");
        WLog::Debug("think of all the things we learned");
        WLog::Debug("for the people who are still alive.");

        {
          WLogSystemScope logScope2(&log2);
          W_LOG_BLOCK("Interlude");
          WLog::Info("Well here we are again. It's always such a pleasure.");
          WLog::Error("Remember when you tried to kill me twice?");
        }

        {
          W_LOG_BLOCK("Verse 5");

          WLog::GetThreadLocalLogSystem()->SetLogLevel(WLogMsgType::WarningMsg);

          WLog::Debug("Go ahead and leave me.");
          WLog::Info("I think I prefer to stay inside.");
          WLog::Dev("Maybe you'll find someone else, to help you.");
          WLog::Dev("Maybe Black Mesa.");
          WLog::Info("That was a joke. Haha. Fat chance.");
          WLog::Warning("Anyway, this cake is great.");
          WLog::Success("It's so delicious and moist.");
          WLog::Dev("Look at me still talking when there's science to do.");
          WLog::Error("When I look up there it makes me glad I'm not you.");
          WLog::Info("I've experiments to run,");
          WLog::SeriousWarning("there is research to be done on the people who are still alive.");
        }
      }
    }
  }

  {
    W_LOG_BLOCK("Verse 6", "Last One");

    WLog::GetThreadLocalLogSystem()->SetLogLevel(WLogMsgType::ErrorMsg);

    WLog::Dev("And believe me I am still alive.");
    WLog::Info("I'm doing science and I'm still alive.");
    WLog::Success("I feel fantastic and I'm still alive.");
    WLog::Warning("While you're dying I'll be still alive.");
    WLog::Error("And when you're dead I will be, still alive.");
    WLog::Debug("Still alive, still alive.");
  }

  /// \todo This test will fail if W_COMPILE_FOR_DEVELOPMENT is disabled.
  /// We also currently don't test WLog::Debug, because our build machines compile in release and then the text below would need to be
  /// different.

  const char* szResult = log.m_Result;
  const char* szExpected = "\
>Portal: Still Alive Verse 1\n\
S: This was a triumph.\n\
I: I'm making a note here:\n\
E: Huge Success\n\
I: It's hard to overstate my satisfaction.\n\
E: Aperture Science. We do what we must, because we can,\n\
E: For the good of all of us, except the ones who are dead.\n\
[Flush]\n\
> Verse 2\n\
E: But there's no sense crying over every mistake.\n\
I: And the science gets done, and you make a neat gun\n\
E: for the people who are still alive.\n\
< Verse 2\n\
> Verse 3\n\
I: I'm not even angry.\n\
I: And tore me to pieces,\n\
I: As they burned it hurt because I was so happy for you.\n\
E: Now these points of data make a beautiful line\n\
[Flush]\n\
> Verse 4\n\
> Verse 5\n\
W: Anyway, this cake is great.\n\
E: When I look up there it makes me glad I'm not you.\n\
SW: there is research to be done on the people who are still alive.\n\
< Verse 5\n\
< Verse 4\n\
< Verse 3\n\
<Portal: Still Alive Verse 1\n\
>Last One Verse 6\n\
E: And when you're dead I will be, still alive.\n\
<Last One Verse 6\n\
";

  W_TEST_STRING(szResult, szExpected);

  const char* szResult2 = log2.m_Result;
  const char* szExpected2 = "\
> Interlude\n\
I: Well here we are again. It's always such a pleasure.\n\
E: Remember when you tried to kill me twice?\n\
< Interlude\n\
";

  W_TEST_STRING(szResult2, szExpected2);
}

W_CREATE_SIMPLE_TEST(Logging, GlobalTestLog)
{
  WLog::GetThreadLocalLogSystem()->SetLogLevel(WLogMsgType::All);

  {
    WTestLogInterface log;
    WTestLogSystemScope scope(&log, true);

    log.ExpectMessage("managed to break", WLogMsgType::ErrorMsg);
    log.ExpectMessage("my heart", WLogMsgType::WarningMsg);
    log.ExpectMessage("see you", WLogMsgType::WarningMsg, 10);

    {
      class LogThread : public WThread
      {
      public:
        virtual WUInt32 Run() override
        {
          WLog::Warning("I see you!");
          WLog::Debug("Test debug");
          return 0;
        }
      };

      LogThread thread[10];

      for (WUInt32 i = 0; i < 10; ++i)
      {
        thread[i].Start();
      }

      WLog::Error("The only thing you managed to break so far");
      WLog::Warning("is my heart");

      for (WUInt32 i = 0; i < 10; ++i)
      {
        thread[i].Join();
      }
    }
  }
}
