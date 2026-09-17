#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Time/Stopwatch.h>
#include <optional>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP) || W_ENABLED(W_PLATFORM_LINUX)

class ChannelTester
{
public:
  ChannelTester(WIpcChannel* pChannel, bool bPing)
  {
    m_bPing = bPing;
    m_pChannel = pChannel;
    m_pChannel->SetReceiveCallback(WMakeDelegate(&ChannelTester::ReceiveMessageData, this));
    m_pChannel->m_Events.AddEventHandler(WMakeDelegate(&ChannelTester::OnIpcEventReceived, this));
  }
  ~ChannelTester()
  {
    m_pChannel->m_Events.RemoveEventHandler(WMakeDelegate(&ChannelTester::OnIpcEventReceived, this));
    m_pChannel->SetReceiveCallback({});
  }

  void OnIpcEventReceived(const WIpcChannelEvent& e)
  {
    W_LOCK(m_Mutex);
    m_ReceivedEvents.ExpandAndGetRef() = e;
  }

  std::optional<WIpcChannelEvent> WaitForEvents(WTime timeout)
  {
    WStopwatch sw;

    while (sw.GetRunningTotal() < timeout)
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
      W_LOCK(m_Mutex);
      if (!m_ReceivedEvents.IsEmpty())
      {
        WIpcChannelEvent e = m_ReceivedEvents.PeekFront();
        m_ReceivedEvents.PopFront();
        return e;
      }
    }

    return {};
  }

  void ReceiveMessageData(WArrayPtr<const WUInt8> data)
  {
    W_LOCK(m_Mutex);
    if (m_bPing)
    {
      m_pChannel->Send(data);
    }
    else
    {
      m_ReceivedMessages.ExpandAndGetRef() = data;
    }
  }

  std::optional<WDynamicArray<WUInt8>> WaitForMessage(WTime timeout)
  {
    WResult res = m_pChannel->WaitForMessages(timeout);
    if (res.Succeeded())
    {
      W_LOCK(m_Mutex);
      if (m_ReceivedMessages.GetCount() > 0)
      {
        auto res2 = m_ReceivedMessages.PeekFront();
        m_ReceivedMessages.PopFront();
        return res2;
      }
    }
    return {};
  }

private:
  bool m_bPing = false;
  WMutex m_Mutex;
  WIpcChannel* m_pChannel = nullptr;
  WDeque<WDynamicArray<WUInt8>> m_ReceivedMessages;
  WDeque<WIpcChannelEvent> m_ReceivedEvents;
};

void TestIPCChannel(WIpcChannel* pServer, ChannelTester* pServerTester, WIpcChannel* pClient, ChannelTester* pClientTester)
{
  auto MessageMatches = [](const WStringView& sReference, const WDataBuffer& msg) -> bool
  {
    WStringView sTemp(reinterpret_cast<const char*>(msg.GetData()), msg.GetCount());
    return sTemp == sReference;
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "Connect")
  {
    W_TEST_BOOL(pServer->GetConnectionState() == WIpcChannel::ConnectionState::Disconnected);
    W_TEST_BOOL(pClient->GetConnectionState() == WIpcChannel::ConnectionState::Disconnected);
    {
      auto res = pServerTester->WaitForEvents(WTime::MakeFromMilliseconds(100));
      W_TEST_BOOL(!res.has_value());
      auto res2 = pClientTester->WaitForEvents(WTime::MakeFromMilliseconds(100));
      W_TEST_BOOL(!res2.has_value());
    }
    {
      W_TEST_RESULT(pServer->Connect());
      auto res = pServerTester->WaitForEvents(WTime::MakeFromMilliseconds(100));
      W_TEST_BOOL(res.has_value() && res->m_Type == WIpcChannelEvent::Connecting);
      W_TEST_BOOL(pServer->GetConnectionState() == WIpcChannel::ConnectionState::Connecting);
    }
    {
      W_TEST_RESULT(pClient->Connect());
      auto res = pClientTester->WaitForEvents(WTime::MakeFromMilliseconds(100));
      W_TEST_BOOL(res.has_value() && res->m_Type == WIpcChannelEvent::Connecting);
    }
    auto res = pServerTester->WaitForEvents(WTime::MakeFromSeconds(3));
    W_TEST_BOOL(res.has_value() && res->m_Type == WIpcChannelEvent::Connected);
    auto res2 = pClientTester->WaitForEvents(WTime::MakeFromSeconds(3));
    W_TEST_BOOL(res2.has_value() && res2->m_Type == WIpcChannelEvent::Connected);

    if (!W_TEST_BOOL(pServer->GetConnectionState() == WIpcChannel::ConnectionState::Connected))
      return;
    if (!W_TEST_BOOL(pClient->GetConnectionState() == WIpcChannel::ConnectionState::Connected))
      return;
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Connect When Already Connected")
  {
    W_TEST_BOOL(pServer->Connect().Failed());
    W_TEST_BOOL(pClient->Connect().Failed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ClientSend")
  {
    WStringView sMsg = "TestMessage"_wsv;

    W_TEST_BOOL(pClient->Send(WConstByteArrayPtr(reinterpret_cast<const WUInt8*>(sMsg.GetStartPointer()), sMsg.GetElementCount())));

    auto res = pServerTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res.has_value() && res->m_Type == WIpcChannelEvent::NewMessages);
    auto res2 = pClientTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res2.has_value() && res2->m_Type == WIpcChannelEvent::NewMessages);

    auto res3 = pClientTester->WaitForMessage(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res3.has_value() && MessageMatches(sMsg, res3.value()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ServerSend")
  {
    WStringView sMsg = "TestMessage2"_wsv;

    W_TEST_BOOL(pServer->Send(WConstByteArrayPtr(reinterpret_cast<const WUInt8*>(sMsg.GetStartPointer()), sMsg.GetElementCount())));

    auto res2 = pClientTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res2.has_value() && res2->m_Type == WIpcChannelEvent::NewMessages);

    auto res3 = pClientTester->WaitForMessage(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res3.has_value() && MessageMatches(sMsg, res3.value()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ClientDisconnect")
  {
    pClient->Disconnect();
    pClient->Disconnect();

    auto res = pServerTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res.has_value() && res->m_Type == WIpcChannelEvent::Disconnected);
    auto res2 = pClientTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res2.has_value() && res2->m_Type == WIpcChannelEvent::Disconnected);

    W_TEST_BOOL(pServer->GetConnectionState() == WIpcChannel::ConnectionState::Disconnected);
    W_TEST_BOOL(pClient->GetConnectionState() == WIpcChannel::ConnectionState::Disconnected);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Reconnect")
  {
    {
      W_TEST_RESULT(pServer->Connect());
      auto res = pServerTester->WaitForEvents(WTime::MakeFromMilliseconds(100));
      W_TEST_BOOL(res.has_value() && res->m_Type == WIpcChannelEvent::Connecting);
      W_TEST_BOOL(pServer->GetConnectionState() == WIpcChannel::ConnectionState::Connecting);
    }
    {
      W_TEST_RESULT(pClient->Connect());
      auto res = pClientTester->WaitForEvents(WTime::MakeFromMilliseconds(100));
      W_TEST_BOOL(res.has_value() && res->m_Type == WIpcChannelEvent::Connecting);
    }

    auto res = pServerTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res.has_value() && res->m_Type == WIpcChannelEvent::Connected);
    auto res2 = pClientTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res2.has_value() && res2->m_Type == WIpcChannelEvent::Connected);

    if (!W_TEST_BOOL(pServer->GetConnectionState() == WIpcChannel::ConnectionState::Connected))
      return;
    if (!W_TEST_BOOL(pClient->GetConnectionState() == WIpcChannel::ConnectionState::Connected))
      return;
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ClientSend after reconnect")
  {
    WStringView sMsg = "TestMessage"_wsv;

    W_TEST_BOOL(pClient->Send(WConstByteArrayPtr(reinterpret_cast<const WUInt8*>(sMsg.GetStartPointer()), sMsg.GetElementCount())));

    auto res = pServerTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res.has_value() && res->m_Type == WIpcChannelEvent::NewMessages);
    auto res2 = pClientTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res2.has_value() && res2->m_Type == WIpcChannelEvent::NewMessages);

    auto res3 = pClientTester->WaitForMessage(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res3.has_value() && MessageMatches(sMsg, res3.value()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ServerDisconnect")
  {
    pServer->Disconnect();
    pServer->Disconnect();

    auto res = pServerTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res.has_value() && res->m_Type == WIpcChannelEvent::Disconnected);
    auto res2 = pClientTester->WaitForEvents(WTime::MakeFromSeconds(1));
    W_TEST_BOOL(res2.has_value() && res2->m_Type == WIpcChannelEvent::Disconnected);

    W_TEST_BOOL(pServer->GetConnectionState() == WIpcChannel::ConnectionState::Disconnected);
    W_TEST_BOOL(pClient->GetConnectionState() == WIpcChannel::ConnectionState::Disconnected);
  }
}

W_CREATE_SIMPLE_TEST(Communication, IpcChannel_Network)
{
  WUniquePtr<WIpcChannel> pServer = WIpcChannel::CreateNetworkChannel("127.0.0.1:1050"_wsv, WIpcChannel::Mode::Server);
  WUniquePtr<ChannelTester> pServerTester = W_DEFAULT_NEW(ChannelTester, pServer.Borrow(), true);

  WUniquePtr<WIpcChannel> pClient = WIpcChannel::CreateNetworkChannel("127.0.0.1:1050"_wsv, WIpcChannel::Mode::Client);
  WUniquePtr<ChannelTester> pClientTester = W_DEFAULT_NEW(ChannelTester, pClient.Borrow(), false);

  TestIPCChannel(pServer.Borrow(), pServerTester.Borrow(), pClient.Borrow(), pClientTester.Borrow());

  pClientTester.Clear();
  pClient.Clear();

  pServerTester.Clear();
  pServer.Clear();
}


W_CREATE_SIMPLE_TEST(Communication, IpcChannel_Pipe)
{
  WUniquePtr<WIpcChannel> pServer = WIpcChannel::CreatePipeChannel("WorldEngine_unit_test_channel", WIpcChannel::Mode::Server);
  WUniquePtr<ChannelTester> pServerTester = W_DEFAULT_NEW(ChannelTester, pServer.Borrow(), true);

  WUniquePtr<WIpcChannel> pClient = WIpcChannel::CreatePipeChannel("WorldEngine_unit_test_channel", WIpcChannel::Mode::Client);
  WUniquePtr<ChannelTester> pClientTester = W_DEFAULT_NEW(ChannelTester, pClient.Borrow(), false);

  TestIPCChannel(pServer.Borrow(), pServerTester.Borrow(), pClient.Borrow(), pClientTester.Borrow());

  pClientTester.Clear();
  pClient.Clear();

  pServerTester.Clear();
  pServer.Clear();
}

#endif
