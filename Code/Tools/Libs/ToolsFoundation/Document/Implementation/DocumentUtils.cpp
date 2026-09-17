#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentUtils.h>

WStatus WDocumentUtils::IsValidSaveLocationForDocument(WStringView sDocument, const WDocumentTypeDescriptor** out_pTypeDesc)
{
  const WDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (WDocumentManager::FindDocumentTypeFromPath(sDocument, true, pTypeDesc).Failed())
  {
    WStringBuilder sTemp;
    sTemp.SetFormat("The selected file extension '{0}' is not registered with any known type.\nCannot create file '{1}'",
      WPathUtils::GetFileExtension(sDocument), sDocument);
    return WStatus(sTemp.GetData());
  }

  if (WDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(sDocument))
  {
    return WStatus("The selected document is already open. You need to close the document before you can re-create it.");
  }

  if (out_pTypeDesc)
  {
    *out_pTypeDesc = pTypeDesc;
  }
  return WStatus(W_SUCCESS);
}
