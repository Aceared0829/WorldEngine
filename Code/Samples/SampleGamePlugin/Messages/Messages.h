#pragma once

#include <Foundation/Communication/Message.h>

// BEGIN-DOCS-CODE-SNIPPET: message-decl
struct WMsgSetText : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgSetText, WMessage);

  WString m_sText;
};
// END-DOCS-CODE-SNIPPET
