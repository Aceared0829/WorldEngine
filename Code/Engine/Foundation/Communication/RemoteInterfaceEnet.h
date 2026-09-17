#pragma once

#include <Foundation/Communication/RemoteInterface.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

/// An implementation for WRemoteInterface built on top of Enet
class W_FOUNDATION_DLL WRemoteInterfaceEnet : public WRemoteInterface
{
public:
  ~WRemoteInterfaceEnet();

  /// Allocates a new instance with the given allocator
  static WInternal::NewInstance<WRemoteInterfaceEnet> Make(WAllocator* pAllocator = WFoundation::GetDefaultAllocator());

  /// The port through which the connection was started
  WUInt16 GetPort() const { return m_uiPort; }

private:
  WRemoteInterfaceEnet();
  friend class WRemoteInterfaceEnetImpl;

protected:
  WUInt16 m_uiPort = 0;
};

#endif
