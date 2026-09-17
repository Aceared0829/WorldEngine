#pragma once

class WRemoteInterface;

/// Interface to give access to the FileServe client for additional tooling needs.
///
/// For now, this interface just gives access to the WRemoteInterface that is used to communicate with the FileServe server.
/// This allows for maximum flexibility sending and receiving custom messages.
class WRemoteToolingInterface
{
public:
  virtual WRemoteInterface* GetRemoteInterface() = 0;
};
