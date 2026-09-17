#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Messages/ExportMessage.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgExport);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgExport, 1, WRTTIDefaultAllocator<WMsgExport>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("DocumentType", m_sDocumentType),
    W_MEMBER_PROPERTY("DocumentGuid", m_sDocumentGuid),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

W_STATICLINK_FILE(GameEngine, GameEngine_Messages_Implementation_ExportMessage);
