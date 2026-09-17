#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WDocumentObject;
struct WDocumentTypeDescriptor;

class W_TOOLSFOUNDATION_DLL WDocumentUtils
{
public:
  static WStatus IsValidSaveLocationForDocument(WStringView sDocument, const WDocumentTypeDescriptor** out_pTypeDesc = nullptr);
};
