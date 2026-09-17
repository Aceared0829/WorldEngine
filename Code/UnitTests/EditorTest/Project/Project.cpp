#include <EditorTest/EditorTestPCH.h>

#include "Project.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <TestFramework/Framework/TestFramework.h>

static WEditorTestProject s_EditorTestProject;

const char* WEditorTestProject::GetTestName() const
{
  return "Project Tests";
}

void WEditorTestProject::SetupSubTests()
{
  AddSubTest("Create Documents", SubTests::ST_CreateDocuments);
  AddSubTest("Create C++ Solution", SubTests::ST_CreateCppSolution);
}

WResult WEditorTestProject::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return W_FAILURE;

  if (SUPER::CreateAndLoadProject("TestProject").Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WResult WEditorTestProject::DeInitializeTest()
{
  if (!m_sProjectPath.IsEmpty())
  {
    // The build results take up vast amounts of memory and prolong test result upload.
    WStringBuilder buildOutput = m_sProjectPath;
    buildOutput.AppendPath("CppSource", "Build");
    WOSFile::DeleteFolder(buildOutput).IgnoreResult();
  }

  // For profiling the doc creation.
  // SafeProfilingData();
  if (SUPER::DeInitializeTest().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WTestAppRun WEditorTestProject::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case ST_CreateDocuments:
      return CreateDocuments();
    case ST_CreateCppSolution:
      return CreateCppSolution();
    default:
      W_REPORT_FAILURE("missing case statement");
  }

  return WTestAppRun::Quit;
}

namespace
{
  /// Whether the engine process can create a document context for this type.
  ///
  /// The engine side gets its context types from the engine plugins that the project's plugin bundles
  /// pull in. EditorTest links some editor plugins directly, though, so their document types are
  /// registered even when the project selects no bundle at all. Creating such a document asserts in the
  /// engine process and takes it down, which then fails every document after it as well.
  bool HasEngineCounterpart(const WDocumentTypeDescriptor* pDesc)
  {
    const WStringView sManagerType = pDesc->m_pManager->GetDynamicRTTI()->GetTypeName();

    for (auto it : WQtEditorApp::GetSingleton()->GetPluginBundles().m_Plugins)
    {
      const WPluginBundle& bundle = it.Value();

      if (bundle.m_bSelected || bundle.m_bMandatory || bundle.m_EditorEnginePlugins.IsEmpty())
        continue;

      // The bundle's own name is what its types are named after, e.g. the 'Jolt' bundle owns
      // WJoltCollisionMeshAssetDocumentManager. Matching on it is enough here: a manager that is
      // registered while its bundle is off can only have come from EditorTest linking that plugin.
      if (sManagerType.FindSubString_NoCase(it.Key()) != nullptr)
        return false;
    }

    return true;
  }
} // namespace

WTestAppRun WEditorTestProject::CreateDocuments()
{
  const auto& allDesc = WDocumentManager::GetAllDocumentDescriptors();
  for (auto it : allDesc)
  {
    auto pDesc = it.Value();

    if (pDesc->m_pManager != nullptr && !HasEngineCounterpart(pDesc))
      continue;

    if (pDesc->m_bCanCreate)
    {
      WStringBuilder sName = m_sProjectPath;
      sName.AppendPath(pDesc->m_sDocumentTypeName);
      sName.ChangeFileExtension(pDesc->m_sFileExtension);
      WDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sName, WDocumentFlags::RequestWindow);
      W_TEST_BOOL(pDoc);
      ProcessEvents();
    }
  }
  // Make sure the engine process did not crash after creating every kind of document.
  W_TEST_BOOL(!WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

  // TODO: Newly created assets actually do not transform cleanly.
  if (false)
  {
    WAssetCurator::GetSingleton()->TransformAllAssets().IgnoreResult();

    WUInt32 uiNumAssets;
    WTempHybridArray<WUInt32, WAssetInfo::TransformState::COUNT> sections;
    WAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

    W_TEST_INT(sections[WAssetInfo::TransformState::TransformError], 0);
    W_TEST_INT(sections[WAssetInfo::TransformState::MissingTransformDependency], 0);
    W_TEST_INT(sections[WAssetInfo::TransformState::MissingPackageDependency], 0);
    W_TEST_INT(sections[WAssetInfo::TransformState::MissingThumbnailDependency], 0);
    W_TEST_INT(sections[WAssetInfo::TransformState::CircularDependency], 0);
  }
  return WTestAppRun::Quit;
}

WTestAppRun WEditorTestProject::CreateCppSolution()
{
  WCppSettings cpp;
  W_TEST_BOOL(cpp.Load().Failed());

  W_TEST_BOOL(!WCppProject::ExistsSolution(cpp));

  cpp.m_sPluginName = "TestPlugin";

  W_TEST_RESULT(cpp.Save());
  W_TEST_RESULT(WCppProject::CleanBuildDir(cpp));
  W_TEST_RESULT(WCppProject::PopulateWithDefaultSources(cpp));
  if (!W_TEST_RESULT(WCppProject::RunCMake(cpp)))
    return WTestAppRun::Quit;

  W_TEST_BOOL(WCppProject::ExistsProjectCMakeListsTxt());
  W_TEST_BOOL(WCppProject::ExistsSolution(cpp));
  W_TEST_RESULT(WCppProject::BuildCodeIfNecessary(cpp));

  WCppProject::UpdatePluginConfig(cpp);
  WQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);

  ProcessEvents(20);

  W_TEST_BOOL(!WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());
  return WTestAppRun::Quit;
}
