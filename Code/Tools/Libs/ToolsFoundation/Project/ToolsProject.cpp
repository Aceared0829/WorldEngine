#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/PathUtils.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

W_IMPLEMENT_SINGLETON(WToolsProject);

WEvent<const WToolsProjectEvent&, WMutex> WToolsProject::s_Events;
WEvent<WToolsProjectRequest&> WToolsProject::s_Requests;


WToolsProjectRequest::WToolsProjectRequest()
{
  m_Type = Type::CanCloseProject;
  m_bCanClose = true;
  m_iContainerWindowUniqueIdentifier = 0;
}

WToolsProject::WToolsProject(WStringView sProjectPath)
  : m_SingletonRegistrar(this)
{
  m_bIsClosing = false;

  WStringBuilder sPath = sProjectPath;
  sPath.MakeCleanPath();

  // on Windows the same file can be referenced with an upper or lower case drive letter,
  // normalizing it here makes sure that all string comparisons against the project path work
  WPathUtils::NormalizeWindowsDriveLetter(sPath);

  m_sProjectPath = sPath;
  W_ASSERT_DEV(!m_sProjectPath.IsEmpty(), "Path cannot be empty.");
}

WToolsProject::~WToolsProject() = default;

WStatus WToolsProject::Create()
{
  {
    WOSFile ProjectFile;
    if (ProjectFile.Open(m_sProjectPath, WFileOpenMode::Write).Failed())
    {
      return WStatus(WFmt("Could not open/create the project file for writing: '{0}'", m_sProjectPath));
    }
    else
    {
      WStringView szToken = "WEditor Project File";

      W_SUCCEED_OR_RETURN(ProjectFile.Write(szToken.GetStartPointer(), szToken.GetElementCount() + 1));
      ProjectFile.Close();
    }
  }

  {
    WToolsProjectEvent e;
    e.m_pProject = this;
    e.m_Type = WToolsProjectEvent::Type::ProjectCreated;
    s_Events.Broadcast(e);
  }

  W_SUCCEED_OR_RETURN(Open());

  // if this file already exists, the project was created from a template and should not get additional setup
  WStringBuilder path(GetProjectDirectory(), "/Scenes/Main.WScene");
  if (!WOSFile::ExistsFile(path))
  {
    WToolsProjectEvent e;
    e.m_pProject = this;
    e.m_Type = WToolsProjectEvent::Type::ProjectFirstSetup;
    s_Events.Broadcast(e);
  }

  return WStatus(W_SUCCESS);
}

WStatus WToolsProject::Open()
{
  WOSFile ProjectFile;
  if (ProjectFile.Open(m_sProjectPath, WFileOpenMode::Read).Failed())
  {
    return WStatus(WFmt("Could not open the project file for reading: '{0}'", m_sProjectPath));
  }

  ProjectFile.Close();

  WToolsProjectEvent e;
  e.m_pProject = this;
  e.m_Type = WToolsProjectEvent::Type::ProjectOpened;
  s_Events.Broadcast(e);

  return WStatus(W_SUCCESS);
}

void WToolsProject::CreateSubFolder(WStringView sFolder) const
{
  WStringBuilder sPath;

  sPath = m_sProjectPath;
  sPath.PathParentDirectory();
  sPath.AppendPath(sFolder);

  WOSFile::CreateDirectoryStructure(sPath).IgnoreResult();
}

void WToolsProject::CloseProject()
{
  if (GetSingleton())
  {
    GetSingleton()->m_bIsClosing = true;

    WToolsProjectEvent e;
    e.m_pProject = GetSingleton();
    e.m_Type = WToolsProjectEvent::Type::ProjectClosing;
    s_Events.Broadcast(e);

    WDocumentManager::CloseAllDocuments();

    delete GetSingleton();

    e.m_Type = WToolsProjectEvent::Type::ProjectClosed;
    s_Events.Broadcast(e);
  }
}

void WToolsProject::SaveProjectState()
{
  if (GetSingleton())
  {
    WToolsProjectEvent e;
    e.m_pProject = GetSingleton();
    e.m_Type = WToolsProjectEvent::Type::ProjectSaveState;
    s_Events.Broadcast(e, 1);
  }
}

bool WToolsProject::CanCloseProject()
{
  if (GetSingleton() == nullptr)
    return true;

  WToolsProjectRequest e;
  e.m_Type = WToolsProjectRequest::Type::CanCloseProject;
  e.m_bCanClose = true;
  s_Requests.Broadcast(e, 1); // when the save dialog pops up and the user presses 'Save' we need to allow one more recursion

  return e.m_bCanClose;
}

bool WToolsProject::CanCloseDocuments(WArrayPtr<WDocument*> documents)
{
  if (GetSingleton() == nullptr)
    return true;

  WToolsProjectRequest e;
  e.m_Type = WToolsProjectRequest::Type::CanCloseDocuments;
  e.m_bCanClose = true;
  e.m_Documents = documents;
  s_Requests.Broadcast(e);

  return e.m_bCanClose;
}

WInt32 WToolsProject::SuggestContainerWindow(WDocument* pDoc)
{
  if (pDoc == nullptr)
  {
    return 0;
  }
  WToolsProjectRequest e;
  e.m_Type = WToolsProjectRequest::Type::SuggestContainerWindow;
  e.m_Documents.PushBack(pDoc);
  s_Requests.Broadcast(e);

  return e.m_iContainerWindowUniqueIdentifier;
}

WStringBuilder WToolsProject::GetPathForDocumentGuid(const WUuid& guid)
{
  WToolsProjectRequest e;
  e.m_Type = WToolsProjectRequest::Type::GetPathForDocumentGuid;
  e.m_documentGuid = guid;
  s_Requests.Broadcast(e, 1); // this can be sent while CanCloseProject is processed, so allow one additional recursion depth
  return e.m_sAbsDocumentPath;
}

WStatus WToolsProject::CreateOrOpenProject(WStringView sProjectPath, bool bCreate)
{
  CloseProject();

  new WToolsProject(sProjectPath);

  WStatus ret(W_SUCCESS);

  if (bCreate)
  {
    ret = GetSingleton()->Create();
    WToolsProject::SaveProjectState();
  }
  else
  {
    ret = GetSingleton()->Open();
  }

  if (ret.Failed())
  {
    delete GetSingleton();
  }

  return ret;
}

WStatus WToolsProject::OpenProject(WStringView sProjectPath)
{
  WStatus status = CreateOrOpenProject(sProjectPath, false);

  return status;
}

WStatus WToolsProject::CreateProject(WStringView sProjectPath)
{
  return CreateOrOpenProject(sProjectPath, true);
}

void WToolsProject::BroadcastSaveAll()
{
  WToolsProjectEvent e;
  e.m_pProject = GetSingleton();
  e.m_Type = WToolsProjectEvent::Type::SaveAll;

  s_Events.Broadcast(e);
}

void WToolsProject::BroadcastConfigChanged()
{
  WToolsProjectEvent e;
  e.m_pProject = GetSingleton();
  e.m_Type = WToolsProjectEvent::Type::ProjectConfigChanged;

  s_Events.Broadcast(e);
}

void WToolsProject::AddAllowedDocumentRoot(WStringView sPath)
{
  WStringBuilder s = sPath;
  s.MakeCleanPath();
  s.Trim("", "/");

  m_AllowedDocumentRoots.PushBack(s);
}


bool WToolsProject::IsDocumentInAllowedRoot(WStringView sDocumentPath, WString* out_pRelativePath) const
{
  for (WUInt32 i = m_AllowedDocumentRoots.GetCount(); i > 0; --i)
  {
    const auto& root = m_AllowedDocumentRoots[i - 1];

    WStringBuilder s = sDocumentPath;
    if (!s.IsPathBelowFolder(root))
      continue;

    if (out_pRelativePath)
    {
      WStringBuilder sText = sDocumentPath;
      sText.MakeRelativeTo(root).IgnoreResult();

      *out_pRelativePath = sText;
    }

    return true;
  }

  return false;
}

const WString WToolsProject::GetProjectName(bool bSanitize) const
{
  WStringBuilder sTemp = WToolsProject::GetSingleton()->GetProjectFile();
  sTemp.PathParentDirectory();
  sTemp.Trim("/");

  if (!bSanitize)
    return sTemp.GetFileName();

  const WStringBuilder sOrgName = sTemp.GetFileName();
  sTemp.Clear();

  bool bAnyAscii = false;

  for (WStringIterator it = sOrgName.GetIteratorFront(); it.IsValid(); ++it)
  {
    const WUInt32 c = it.GetCharacter();

    if (!WStringUtils::IsIdentifierDelimiter_C_Code(c))
    {
      bAnyAscii = true;

      // valid character to be used in C as an identifier
      sTemp.Append(c);
    }
    else if (c == ' ')
    {
      // skip
    }
    else
    {
      sTemp.AppendFormat("{}", WArgU(c, 1, false, 16));
    }
  }

  if (!bAnyAscii)
  {
    const WUInt32 uiHash = WHashingUtils::xxHash32String(sTemp);
    sTemp.SetFormat("Project{}", uiHash);
  }

  if (sTemp.IsEmpty())
  {
    sTemp = "Project";
  }

  if (sTemp.GetCharacterCount() > 20)
  {
    sTemp.Shrink(0, sTemp.GetCharacterCount() - 20);
  }

  return sTemp;
}

WString WToolsProject::GetProjectDirectory() const
{
  WStringBuilder s = GetProjectFile();

  s.PathParentDirectory();
  s.Trim("", "/\\");

  return s;
}

WString WToolsProject::GetProjectDataFolder() const
{
  WStringBuilder s = GetProjectDirectory();
  s.AppendPath("Editor");

  return s;
}

WString WToolsProject::FindProjectDirectoryForDocument(WStringView sDocumentPath)
{
  WStringBuilder sPath = sDocumentPath;
  sPath.PathParentDirectory();

  WStringBuilder sTemp;

  while (!sPath.IsEmpty())
  {
    sTemp = sPath;
    sTemp.AppendPath("WProject");

    if (WOSFile::ExistsFile(sTemp))
      return sPath;

    sPath.PathParentDirectory();
  }

  return "";
}
