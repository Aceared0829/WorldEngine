#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/IpcChannel.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

class WRemoteInterface;
class WRemoteMessage;

class W_FOUNDATION_DLL WIpcChannelEnet : public WIpcChannel
{
public:
  WIpcChannelEnet(WStringView sAddress, Mode::Enum mode);
  ~WIpcChannelEnet();

protected:
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;
  virtual bool RequiresRegularTick() override { return true; }
  virtual void Tick() override;
  void NetworkMessageHandler(WRemoteMessage& msg);
  void EnetEventHandler(const WRemoteEvent& e);

  WString m_sAddress;
  WTime m_LastConnectAttempt = WTime::MakeZero();
  WUniquePtr<WRemoteInterface> m_pNetwork;
};

#endif
