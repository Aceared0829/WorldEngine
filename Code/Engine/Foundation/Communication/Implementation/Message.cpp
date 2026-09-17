#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Message.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMessage, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

WMessageId WMessage::s_NextMsgId = 0;


void WMessage::PackageForTransfer(const WMessage& msg, WStreamWriter& inout_stream)
{
  const WRTTI* pRtti = msg.GetDynamicRTTI();

  inout_stream << pRtti->GetTypeNameHash();
  inout_stream << (WUInt8)pRtti->GetTypeVersion();

  msg.Serialize(inout_stream);
}

WUniquePtr<WMessage> WMessage::ReplicatePackedMessage(WStreamReader& inout_stream)
{
  WUInt64 uiTypeHash = 0;
  inout_stream >> uiTypeHash;

  WUInt8 uiTypeVersion = 0;
  inout_stream >> uiTypeVersion;

  const WRTTI* pRtti = WRTTI::FindTypeByNameHash(uiTypeHash);
  if (pRtti == nullptr || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  auto pMsg = pRtti->GetAllocator()->Allocate<WMessage>();

  pMsg->Deserialize(inout_stream, uiTypeVersion);

  return pMsg;
}

W_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Message);
