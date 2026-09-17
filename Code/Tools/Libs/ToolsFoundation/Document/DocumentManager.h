#pragma once

#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class W_TOOLSFOUNDATION_DLL WDocumentManager : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WDocumentManager, WReflectedClass);

public:
  virtual ~WDocumentManager() = default;

  static const WHybridArray<WDocumentManager*, 16>& GetAllDocumentManagers() { return s_AllDocumentManagers; }

  static WResult FindDocumentTypeFromPath(WStringView sPath, bool bForCreation, const WDocumentTypeDescriptor*& out_pTypeDesc);

  WStatus CanOpenDocument(WStringView sFilePath) const;

  /// Creates a new document.
  /// \param szDocumentTypeName Document type to create. See WDocumentTypeDescriptor.
  /// \param szPath Absolute path to the document to be created.
  /// \param out_pDocument Out parameter for the resulting WDocument. Will be nullptr on failure.
  /// \param flags Flags to define various options like whether a window should be created.
  /// \param pOpenContext An generic context object. Allows for custom data to be passed along into the construction. E.g. inform a sub-document which main document it belongs to.
  /// \return Returns the error in case the operations failed.
  WStatus CreateDocument(
    WStringView sDocumentTypeName, WStringView sPath, WDocument*& out_pDocument, WBitflags<WDocumentFlags> flags = WDocumentFlags::None, const WDocumentObject* pOpenContext = nullptr);

  /// Opens an existing document.
  /// \param szDocumentTypeName Document type to open. See WDocumentTypeDescriptor.
  /// \param szPath Absolute path to the document to be opened.
  /// \param out_pDocument Out parameter for the resulting WDocument. Will be nullptr on failure.
  /// \param flags Flags to define various options like whether a window should be created.
  /// \param pOpenContext  An generic context object. Allows for custom data to be passed along into the construction. E.g. inform a sub-document which main document it belongs to.
  /// \return Returns the error in case the operations failed.
  /// \return Returns the error in case the operations failed.
  WStatus OpenDocument(WStringView sDocumentTypeName, WStringView sPath, WDocument*& out_pDocument,
    WBitflags<WDocumentFlags> flags = WDocumentFlags::AddToRecentFilesList | WDocumentFlags::RequestWindow,
    const WDocumentObject* pOpenContext = nullptr);
  virtual WStatus CloneDocument(WStringView sPath, WStringView sClonePath, WUuid& inout_cloneGuid);
  void CloseDocument(WDocument* pDocument);
  void EnsureWindowRequested(WDocument* pDocument, const WDocumentObject* pOpenContext = nullptr);

  /// Returns a list of all currently open documents that are managed by this document manager
  const WDynamicArray<WDocument*>& GetAllOpenDocuments() const { return m_AllOpenDocuments; }

  WDocument* GetDocumentByPath(WStringView sPath) const;

  static WDocument* GetDocumentByGuid(const WUuid& guid);

  /// If the given document is open, it will be closed. User is not asked about it, unsaved changes are discarded. Returns true if the document
  /// was open and needed to be closed.
  static bool EnsureDocumentIsClosedInAllManagers(WStringView sPath);

  /// If the given document is open, it will be closed. User is not asked about it, unsaved changes are discarded. Returns true if the document
  /// was open and needed to be closed. This function only operates on documents opened by this manager. Use EnsureDocumentIsClosedInAllManagers() to
  /// close documents of any type.
  bool EnsureDocumentIsClosed(WStringView sPath);

  void CloseAllDocumentsOfManager();
  static void CloseAllDocuments();

  struct Event
  {
    enum class Type
    {
      DocumentTypesRemoved,
      DocumentTypesAdded,
      DocumentOpened,
      DocumentWindowRequested,      ///< Sent when the window for a document is needed. Each plugin should check this and see if it can create the desired
                                    ///< window type
      AfterDocumentWindowRequested, ///< Sent after a document window was requested. Can be used to do things after the new window has been opened
      DocumentClosing,
      DocumentClosing2,             // sent after DocumentClosing but before removing the document, use this to do stuff that depends on code executed during
                                    // DocumentClosing
      DocumentClosed,               // this will not point to a valid document anymore, as the document is deleted, use DocumentClosing to get the event before it
                                    // is deleted
    };

    Type m_Type;
    WDocument* m_pDocument = nullptr;
    const WDocumentObject* m_pOpenContext = nullptr;
  };

  struct Request
  {
    enum class Type
    {
      DocumentAllowedToOpen,
    };

    Type m_Type;
    WString m_sDocumentType;
    WString m_sDocumentPath;
    WStatus m_RequestStatus = W_SUCCESS;
  };

  static WCopyOnBroadcastEvent<const Event&> s_Events;
  static WEvent<Request&> s_Requests;

  static const WDocumentTypeDescriptor* GetDescriptorForDocumentType(WStringView sDocumentType);
  static const WMap<WString, const WDocumentTypeDescriptor*>& GetAllDocumentDescriptors();

  void GetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_documentTypes) const;

  using CustomAction = WVariant (*)(const WDocument*);
  static WMap<WString, CustomAction> s_CustomActions;

protected:
  virtual void InternalCloneDocument(WStringView sPath, WStringView sClonePath, const WUuid& documentId, const WUuid& seedGuid, const WUuid& cloneGuid, WAbstractObjectGraph* pHeader, WAbstractObjectGraph* pObjects, WAbstractObjectGraph* pTypes);

private:
  virtual void InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) = 0;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const = 0;

private:
  WStatus CreateOrOpenDocument(bool bCreate, WStringView sDocumentTypeName, WStringView sPath, WDocument*& out_pDocument,
    WBitflags<WDocumentFlags> flags, const WDocumentObject* pOpenContext = nullptr);

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, DocumentManager);

  static void OnPluginEvent(const WPluginEvent& e);

  static void UpdateBeforeUnloadingPlugins(const WPluginEvent& e);
  static void UpdatedAfterLoadingPlugins();

  WDynamicArray<WDocument*> m_AllOpenDocuments;

  static WSet<const WRTTI*> s_KnownManagers;
  static WHybridArray<WDocumentManager*, 16> s_AllDocumentManagers;

  static WMap<WString, const WDocumentTypeDescriptor*> s_AllDocumentDescriptors;
};
