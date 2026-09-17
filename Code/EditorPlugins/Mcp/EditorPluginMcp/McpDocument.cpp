#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpDocument.h>
#include <Mcp/McpTool.h>

#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

WDocument* WMcpDocument::Find(WStringView sIdentifier)
{
  if (sIdentifier.IsEmpty())
    return nullptr;

  // TryConvertStringToUuid() rather than ConvertStringToUuid(), which asserts on anything that is not
  // already a uuid and so cannot be used to find out whether the input is one. The input here is a
  // path just as often as a guid.
  if (WUuid guid; WConversionUtils::TryConvertStringToUuid(sIdentifier, guid).Succeeded())
  {
    // A guid that is not open is not a path either, so there is nothing else to try.
    return WDocumentManager::GetDocumentByGuid(guid);
  }

  WStringBuilder sPath = sIdentifier;
  sPath.MakeCleanPath();

  if (sPath.IsEmpty())
    return nullptr;

  // Matched against the open documents' own paths rather than resolved through the data directories
  // first. Only documents that are already open can match, so their paths are the complete set of right
  // answers - and going through WQtEditorApp::MakeParentDataDirectoryRelativePathAbsolute() instead
  // would dereference WSubAsset::m_pAssetInfo, which is null for an asset the curator knows but has no
  // file information for. That crashed the editor when a path was passed for a document that had just
  // been closed.
  for (WDocumentManager* pManager : WDocumentManager::GetAllDocumentManagers())
  {
    if (WPathUtils::IsAbsolutePath(sPath))
    {
      if (WDocument* pDoc = pManager->GetDocumentByPath(sPath))
        return pDoc;

      continue;
    }

    // A relative path matches when it is a trailing path component sequence of the absolute one, so
    // both "Testing Chambers/Prefabs/Barrel.WPrefab" and "Prefabs/Barrel.WPrefab" find it.
    for (WDocument* pDoc : pManager->GetAllOpenDocuments())
    {
      if (pDoc == nullptr)
        continue;

      WStringBuilder sDocPath = pDoc->GetDocumentPath();
      sDocPath.MakeCleanPath();

      if (sDocPath.EndsWith_NoCase(sPath))
      {
        // Guards against 'Barrel.WPrefab' matching 'BigBarrel.WPrefab': what precedes the match has
        // to be a path separator, not more filename.
        const WUInt32 uiDocLen = sDocPath.GetElementCount();
        const WUInt32 uiPathLen = sPath.GetElementCount();

        if (uiDocLen == uiPathLen || sDocPath.GetData()[uiDocLen - uiPathLen - 1] == '/')
          return pDoc;
      }
    }
  }

  return nullptr;
}

void WMcpDocument::SetNotOpenError(WMcpToolResult& out_result, WStringView sIdentifier)
{
  // Names both accepted forms and the tool that lists what is open, because an agent that guessed a
  // path wrong otherwise has no way to find out what it should have passed.
  WStringBuilder s;
  s.SetFormat("No open document matches '{}'. Pass the guid or the path of a document that is currently open - "
              "'document_list' returns both for every open document. To open a document that is not open yet, use 'document_open'.",
    sIdentifier);

  out_result.SetError(s);
}

const WDocumentObject* WMcpDocument::ResolveObject(const WDocument* pDocument, WStringView sObjectGuid, WStringBuilder& out_sError)
{
  const WDocumentObjectManager* pManager = pDocument->GetObjectManager();

  if (!sObjectGuid.IsEmpty())
  {
    if (!WConversionUtils::IsStringUuid(sObjectGuid))
    {
      out_sError.SetFormat("'{}' is not a valid object guid. Omit 'object' to address the document's top level object.", sObjectGuid);
      return nullptr;
    }

    const WUuid guid = WConversionUtils::ConvertStringToUuid(sObjectGuid);
    const WDocumentObject* pObject = pManager->GetObject(guid);

    // The document's root is a real object with a stable guid - WUuid::MakeStableUuidFromString("DocumentRoot"),
    // the same in every document - so it is reachable by name even though no tool ever reports it. It is
    // an implementation detail with no parent and no type a caller can act on, and handing it out lets
    // the object tools try to delete, move or duplicate it, which dereferences its absent parent.
    if (pObject != nullptr && pObject == pManager->GetRootObject())
    {
      out_sError = "That guid is the document's internal root object, which cannot be read or modified. Omit 'object' to address the "
                   "document's top level object, or use 'object_tree' to list the objects that can be acted on.";
      return nullptr;
    }

    if (pObject == nullptr)
    {
      out_sError.SetFormat("The document has no object with guid '{}'. Object guids come from a previous 'object_tree' or "
                           "'object_properties' call and are not stable across editor sessions.",
        sObjectGuid);
    }

    return pObject;
  }

  const WDocumentObject* pRoot = pManager->GetRootObject();

  if (pRoot == nullptr || pRoot->GetChildren().IsEmpty())
  {
    out_sError = "The document has no top level object.";
    return nullptr;
  }

  if (pRoot->GetChildren().GetCount() > 1)
  {
    out_sError.SetFormat("The document has {} top level objects, so there is no single one to default to. Pass 'object' with the guid "
                         "of the one to act on - 'object_tree' lists them.",
      pRoot->GetChildren().GetCount());
    return nullptr;
  }

  return pRoot->GetChildren()[0];
}

WString WMcpDocument::GetObjectName(const WDocumentObject* pObject)
{
  const WRTTI* pType = pObject->GetType();

  if (pType == nullptr || pType->FindPropertyByName("Name") == nullptr)
    return {};

  const WVariant name = pObject->GetTypeAccessor().GetValue("Name");

  if (!name.IsValid() || !name.CanConvertTo<WString>())
    return {};

  return name.ConvertTo<WString>();
}
