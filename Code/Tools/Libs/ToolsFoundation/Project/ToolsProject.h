#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WToolsProject;
class WDocument;

struct WToolsProjectEvent
{
  enum class Type
  {
    ProjectCreated,
    ProjectOpened,
    ProjectFirstSetup,
    ProjectSaveState,
    ProjectClosing,
    ProjectClosed,
    ProjectConfigChanged, ///< Sent when global project configuration data was changed and thus certain menus would need to update their content (or
                          ///< just deselect any item, forcing the user to reselect and thus update state)
    SaveAll,              ///< When sent, this shall save all outstanding modifications
  };

  WToolsProject* m_pProject;
  Type m_Type;
};

struct WToolsProjectRequest
{
  WToolsProjectRequest();

  enum class Type
  {
    CanCloseProject,        ///< Can we close the project? Listener needs to set m_bCanClose if not.
    CanCloseDocuments,      ///< Can we close the documents in m_Documents? Listener needs to set m_bCanClose if not.
    SuggestContainerWindow, ///< m_Documents contains one element that a container window should be suggested for and written to
                            ///< m_iContainerWindowUniqueIdentifier.
    GetPathForDocumentGuid,
  };

  Type m_Type;
  bool m_bCanClose;                        ///< When the event is sent, interested code can set this to false to prevent closing.
  WDynamicArray<WDocument*> m_Documents; ///< In case of 'CanCloseDocuments', these will be the documents in question.
  WInt32
    m_iContainerWindowUniqueIdentifier;    ///< In case of 'SuggestContainerWindow', the ID of the container to be used for the docs in m_Documents.

  WUuid m_documentGuid;
  WStringBuilder m_sAbsDocumentPath;
};

class W_TOOLSFOUNDATION_DLL WToolsProject
{
  W_DECLARE_SINGLETON(WToolsProject);

public:
  static WEvent<const WToolsProjectEvent&, WMutex> s_Events;
  static WEvent<WToolsProjectRequest&> s_Requests;

public:
  static bool IsProjectOpen() { return GetSingleton() != nullptr; }
  static bool IsProjectClosing() { return (GetSingleton() != nullptr && GetSingleton()->m_bIsClosing); }
  static void CloseProject();
  static void SaveProjectState();
  /// Returns true when the project can be closed. Uses WToolsProjectRequest::Type::CanCloseProject event.
  static bool CanCloseProject();
  /// Returns true when the given list of documents can be closed. Uses WToolsProjectRequest::Type::CanCloseDocuments event.
  static bool CanCloseDocuments(WArrayPtr<WDocument*> documents);
  /// Returns the unique ID of the container window this document should use for its window. Uses
  /// WToolsProjectRequest::Type::SuggestContainerWindow event.
  static WInt32 SuggestContainerWindow(WDocument* pDoc);
  /// Resolve document GUID into an absolute path.
  WStringBuilder GetPathForDocumentGuid(const WUuid& guid);
  static WStatus OpenProject(WStringView sProjectPath);
  static WStatus CreateProject(WStringView sProjectPath);

  /// Broadcasts the SaveAll event, though otherwise has no direct effect.
  static void BroadcastSaveAll();

  /// Sent when global project configuration data was changed and thus certain menus would need to update their content (or just deselect any
  /// item, forcing the user to reselect and thus update state)
  static void BroadcastConfigChanged();

  /// Returns the path to the 'WProject' file
  const WString& GetProjectFile() const { return m_sProjectPath; }

  /// Returns the short name of the project (extracted from the path).
  ///
  /// \param bSanitize Whether to replace whitespace and other problematic characters, such that it can be used in code.
  const WString GetProjectName(bool bSanitize) const;

  /// Returns the path in which the 'WProject' file is stored
  WString GetProjectDirectory() const;

  /// Returns the directory path in which project settings etc. should be stored
  WString GetProjectDataFolder() const;

  /// Starts at the  given document and then searches the tree upwards until it finds an WProject file.
  static WString FindProjectDirectoryForDocument(WStringView sDocumentPath);

  bool IsDocumentInAllowedRoot(WStringView sDocumentPath, WString* out_pRelativePath = nullptr) const;

  void AddAllowedDocumentRoot(WStringView sPath);

  /// Makes sure the given sub-folder exists inside the project directory
  void CreateSubFolder(WStringView sFolder) const;

private:
  static WStatus CreateOrOpenProject(WStringView sProjectPath, bool bCreate);

private:
  WToolsProject(WStringView sProjectPath);
  ~WToolsProject();

  WStatus Create();
  WStatus Open();

private:
  bool m_bIsClosing;
  WString m_sProjectPath;
  WHybridArray<WString, 4> m_AllowedDocumentRoots;
};
