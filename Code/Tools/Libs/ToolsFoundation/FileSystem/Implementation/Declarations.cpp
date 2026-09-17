#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <ToolsFoundation/FileSystem/Declarations.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WFileStatus, WNoBase, 3, WRTTIDefaultAllocator<WFileStatus>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("LastModified", m_LastModified),
    W_MEMBER_PROPERTY("Hash", m_uiHash),
    W_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on
