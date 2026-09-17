#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/RemoteMessage.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcessMessage, 1, WRTTIDefaultAllocator<WProcessMessage>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MessageId", m_uiMessageId),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WRemoteMessage::WRemoteMessage()
  : m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
}

WRemoteMessage::WRemoteMessage(const WRemoteMessage& rhs)
  : m_Storage(rhs.m_Storage)
  , m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
}


WRemoteMessage::WRemoteMessage(WUInt32 uiSystemID, WUInt32 uiMessageID)
  : m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = uiSystemID;
  m_uiMsgID = uiMessageID;
}

void WRemoteMessage::operator=(const WRemoteMessage& rhs)
{
  m_Storage = rhs.m_Storage;
  m_uiApplicationID = rhs.m_uiApplicationID;
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
  m_Reader.SetStorage(&m_Storage);
  m_Writer.SetStorage(&m_Storage);
}

WRemoteMessage::~WRemoteMessage()
{
  m_Reader.SetStorage(nullptr);
  m_Writer.SetStorage(nullptr);
}


W_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_RemoteMessage);
