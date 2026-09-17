#pragma once

#include <Foundation/Communication/Message.h>
#include <GameEngine/GameEngineDLL.h>

/// Message that is sent to all game objects when a scene or prefab is being exported.
/// This message can be handled in scripts or custom components to e.g. remove editor only objects/components or save custom data.
struct W_GAMEENGINE_DLL WMsgExport : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgExport, WMessage);

  WString m_sDocumentType; ///< The type of document that is being exported, e.g. "Prefab", "Scene", etc.
  WString m_sDocumentGuid; ///< The GUID (as string) of the document that is being exported.
};
