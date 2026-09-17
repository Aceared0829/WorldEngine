#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct W_TOOLSFOUNDATION_DLL WActiveDocumentChange
{
  const WDocument* m_pOldDocument;
  const WDocument* m_pNewDocument;
};

/// Tracks existing and active WDocument.
///
/// While the IDocumentManager manages documents of a certain context,
/// this class simply keeps track of the overall number of documents and the currently active one.
class W_TOOLSFOUNDATION_DLL WDocumentRegistry
{
public:
  static bool RegisterDocument(const WDocument* pDocument);
  static bool UnregisterDocument(const WDocument* pDocument);

  static WArrayPtr<const WDocument*> GetDocuments() { return s_Documents; }

  static void SetActiveDocument(const WDocument* pDocument);
  static const WDocument* GetActiveDocument();

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, DocumentRegistry);

  static void Startup();
  static void Shutdown();

public:
  // static WEvent<WDocumentChange&> m_DocumentAddedEvent;
  // static WEvent<WDocumentChange&> m_DocumentRemovedEvent;
  static WEvent<WActiveDocumentChange&> m_ActiveDocumentChanged;

private:
  static WHybridArray<const WDocument*, 16> s_Documents;
  static WDocument* s_pActiveDocument;
};
