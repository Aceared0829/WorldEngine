#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Console/Console.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>

static void TelemetryMessage(void* pPassThrough)
{
  WTelemetryMessage Msg;
  WStringBuilder input;

  while (WTelemetry::RetrieveMessage('CMD', Msg) == W_SUCCESS)
  {
    if (Msg.GetMessageID() == 'EXEC' || Msg.GetMessageID() == 'COMP')
    {
      Msg.GetReader() >> input;

      if (WConsole::GetMainConsole())
      {
        if (auto pInt = WConsole::GetMainConsole()->GetCommandInterpreter())
        {
          WCommandInterpreterState s;
          s.m_sInput = input;

          WStringBuilder encoded;

          if (Msg.GetMessageID() == 'EXEC')
          {
            pInt->Interpret(s);
          }
          else
          {
            pInt->AutoComplete(s);
            encoded.AppendFormat(";;00||<{}", s.m_sInput);
          }

          for (const auto& l : s.m_sOutput)
          {
            encoded.AppendFormat(";;{}||{}", WArgI((WInt32)l.m_Type, 2, true), l.m_sText);
          }

          WTelemetryMessage msg;
          msg.SetMessageID('CMD', 'RES');
          msg.GetWriter() << encoded;
          WTelemetry::Broadcast(WTelemetry::Reliable, msg);
        }
      }
    }
  }
}

void AddConsoleEventHandler()
{
  WTelemetry::AcceptMessagesForSystem('CMD', true, TelemetryMessage, nullptr);
}

void RemoveConsoleEventHandler()
{
  WTelemetry::AcceptMessagesForSystem('CMD', false);
}
