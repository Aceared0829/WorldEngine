#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/Reflection.h>

/// \todo Add move semantics for WRemoteMessage

/// Encapsulates all the data that is transmitted when sending or receiving a message with WRemoteInterface
class W_FOUNDATION_DLL WRemoteMessage
{
public:
  WRemoteMessage();
  WRemoteMessage(WUInt32 uiSystemID, WUInt32 uiMessageID);
  WRemoteMessage(const WRemoteMessage& rhs);
  ~WRemoteMessage();
  void operator=(const WRemoteMessage& rhs);

  /// \name Sending
  ///@{

  /// For setting the message IDs before sending it
  W_ALWAYS_INLINE void SetMessageID(WUInt32 uiSystemID, WUInt32 uiMessageID)
  {
    m_uiSystemID = uiSystemID;
    m_uiMsgID = uiMessageID;
  }

  /// Returns a stream writer to append data to the message
  W_ALWAYS_INLINE WStreamWriter& GetWriter() { return m_Writer; }


  ///@}

  /// \name Receiving
  ///@{

  /// Returns a stream reader for reading the message data
  W_ALWAYS_INLINE WStreamReader& GetReader() { return m_Reader; }
  W_ALWAYS_INLINE WUInt32 GetApplicationID() const { return m_uiApplicationID; }
  W_ALWAYS_INLINE WUInt32 GetSystemID() const { return m_uiSystemID; }
  W_ALWAYS_INLINE WUInt32 GetMessageID() const { return m_uiMsgID; }
  W_ALWAYS_INLINE WArrayPtr<const WUInt8> GetMessageData() const
  {
    return {m_Storage.GetData(), m_Storage.GetStorageSize32()};
  }

  ///@}

private:
  friend class WRemoteInterface;

  WUInt32 m_uiApplicationID = 0;
  WUInt32 m_uiSystemID = 0;
  WUInt32 m_uiMsgID = 0;

  WContiguousMemoryStreamStorage m_Storage;
  WMemoryStreamReader m_Reader;
  WMemoryStreamWriter m_Writer;
};

/// Base class for IPC messages transmitted by WIpcChannel.
class W_FOUNDATION_DLL WProcessMessage : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WProcessMessage, WReflectedClass);

public:
  WProcessMessage() = default;
  WUInt64 m_uiMessageId = 0;
};
