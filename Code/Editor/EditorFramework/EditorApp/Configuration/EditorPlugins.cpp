#include <EditorFramework/EditorFrameworkPCH.h>

#include "Plugins.h"
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Profiling/Profiling.h>

void WPluginBundle::WriteStateToDDL(WOpenDdlWriter& ref_ddl, const char* szOwnName) const
{
  ref_ddl.BeginObject("PluginState");
  WOpenDdlUtils::StoreString(ref_ddl, szOwnName, "ID");
  WOpenDdlUtils::StoreBool(ref_ddl, m_bSelected, "Selected");
  WOpenDdlUtils::StoreBool(ref_ddl, m_bLoadCopy, "LoadCopy");
  ref_ddl.EndObject();
}

void WPluginBundle::ReadStateFromDDL(WOpenDdlReader& ref_ddl, const char* szOwnName)
{
  m_bSelected = false;

  auto pState = ref_ddl.GetRootElement()->FindChildOfType("PluginState");
  while (pState)
  {
    auto pName = pState->FindChildOfType(WOpenDdlPrimitiveType::String, "ID");
    if (!pName || pName->GetPrimitivesString()[0] != szOwnName)
    {
      pState = pState->GetSibling();
      continue;
    }

    if (auto pVal = pState->FindChildOfType(WOpenDdlPrimitiveType::Bool, "Selected"))
      m_bSelected = pVal->GetPrimitivesBool()[0];
    if (auto pVal = pState->FindChildOfType(WOpenDdlPrimitiveType::Bool, "LoadCopy"))
      m_bLoadCopy = pVal->GetPrimitivesBool()[0];

    break;
  }
}

void WPluginBundleSet::SetFromTemplate(const char* szTemplateName)
{
  for (auto it : m_Plugins)
  {
    WPluginBundle& bundle = it.Value();

    bundle.m_bSelected = bundle.m_EnabledInTemplates.Contains(szTemplateName);
  }
}

void WPluginBundleSet::WriteStateToDDL(WOpenDdlWriter& ref_ddl) const
{
  for (const auto& it : m_Plugins)
  {
    if (it.Value().m_bSelected)
    {
      it.Value().WriteStateToDDL(ref_ddl, it.Key());
    }
  }
}

void WPluginBundleSet::ReadStateFromDDL(WOpenDdlReader& ref_ddl)
{
  for (auto& it : m_Plugins)
  {
    it.Value().ReadStateFromDDL(ref_ddl, it.Key());
  }
}

bool WPluginBundleSet::IsStateEqual(const WPluginBundleSet& rhs) const
{
  if (m_Plugins.GetCount() != rhs.m_Plugins.GetCount())
    return false;

  for (auto it : m_Plugins)
  {
    auto it2 = rhs.m_Plugins.Find(it.Key());

    if (!it2.IsValid())
      return false;

    if (!it.Value().IsStateEqual(it2.Value()))
      return false;
  }

  return true;
}

WResult WPluginBundle::ReadBundleFromDDL(WOpenDdlReader& ref_ddl)
{
  W_LOG_BLOCK("Reading plugin info file");

  auto pInfo = ref_ddl.GetRootElement()->FindChildOfType("PluginInfo");

  if (pInfo == nullptr)
  {
    WLog::Error("'PluginInfo' root object is missing");
    return W_FAILURE;
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::Bool, "Mandatory"))
    m_bMandatory = pElement->GetPrimitivesBool()[0];

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::Bool, "AllowEnableReload"))
    m_bAllowEnableReload = pElement->GetPrimitivesBool()[0];

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "DisplayName"))
    m_sDisplayName = pElement->GetPrimitivesString()[0];

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "Description"))
    m_sDescription = pElement->GetPrimitivesString()[0];

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "EditorPlugins"))
  {
    m_EditorPlugins.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_EditorPlugins[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "EditorEnginePlugins"))
  {
    m_EditorEnginePlugins.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_EditorEnginePlugins[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "RuntimePlugins"))
  {
    m_RuntimePlugins.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_RuntimePlugins[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "PackageDependencies"))
  {
    m_PackageDependencies.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_PackageDependencies[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "PackageDependenciesDev"))
  {
    m_PackageDependenciesDev.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_PackageDependenciesDev[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "PackageDependenciesDebug"))
  {
    m_PackageDependenciesDebug.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_PackageDependenciesDebug[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "PackageDependenciesShipping"))
  {
    m_PackageDependenciesShipping.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_PackageDependenciesShipping[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "DataDirectories"))
  {
    m_DataDirectories.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_DataDirectories[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "RequiredPlugins"))
  {
    m_RequiredBundles.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_RequiredBundles[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "ExclusiveFeatures"))
  {
    m_ExclusiveFeatures.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_ExclusiveFeatures[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "EnabledInTemplates"))
  {
    m_EnabledInTemplates.SetCount(pElement->GetNumPrimitives());
    for (WUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_EnabledInTemplates[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(WOpenDdlPrimitiveType::String, "CMakeTargetName"))
    m_sCMakeTargetName = pElement->GetPrimitivesString()[0];

  m_bMissing = false;

  return W_SUCCESS;
}

void WQtEditorApp::DetectAvailablePluginBundles(WStringView sSearchDirectory)
{
#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS)
  // find all WPluginBundle files
  {
    WStringBuilder sSearch = sSearchDirectory;

    sSearch.AppendPath("*.WPluginBundle");

    WStringBuilder sPath, sPlugin;

    WFileSystemIterator fsit;
    for (fsit.StartSearch(sSearch.GetData(), WFileSystemIteratorFlags::ReportFiles); fsit.IsValid(); fsit.Next())
    {
      sPlugin = fsit.GetStats().m_sName;
      sPlugin.RemoveFileExtension();

      fsit.GetStats().GetFullPath(sPath);

      WFileReader file;
      if (file.Open(sPath).Succeeded())
      {
        WOpenDdlReader ddl;
        if (ddl.ParseDocument(file).Failed())
        {
          WLog::Error("Failed to parse plugin bundle file: '{}'", sPath);
        }
        else
        {
          m_PluginBundles.m_Plugins[sPlugin].ReadBundleFromDDL(ddl).IgnoreResult();
        }
      }
    }
  }

  // additionally, find all *Plugin.dll files that are not mentioned in any WPluginBundle and treat them as fake plugin bundles
  if (false) // sometimes useful, but not how it's supposed to be
  {
    WStringBuilder sSearch = WOSFile::GetApplicationDirectory();

    sSearch.AppendPath("*Plugin.dll");

    WStringBuilder sPlugin;

    auto isUsedInBundle = [this](const WStringBuilder& sPlugin) -> bool
    {
      for (auto pit : m_PluginBundles.m_Plugins)
      {
        if (pit.Key().IsEqual_NoCase(sPlugin))
          return true;

        const WPluginBundle& val = pit.Value();

        for (const auto& rt : val.m_RuntimePlugins)
        {
          if (rt.IsEqual_NoCase(sPlugin))
            return true;
        }
      }

      return false;
    };

    WFileSystemIterator fsit;
    for (fsit.StartSearch(sSearch.GetData(), WFileSystemIteratorFlags::ReportFiles); fsit.IsValid(); fsit.Next())
    {
      sPlugin = fsit.GetStats().m_sName;
      sPlugin.RemoveFileExtension();

      if (isUsedInBundle(sPlugin))
        continue;

      auto& newp = m_PluginBundles.m_Plugins[sPlugin];
      newp.m_RuntimePlugins.PushBack(sPlugin);
      newp.m_sDescription = "No WPluginBundle file is present for this plugin.";

      sPlugin.Shrink(0, 6);
      newp.m_sDisplayName = sPlugin;
    }
  }
#else
  W_ASSERT_NOT_IMPLEMENTED;
#endif
}

void WQtEditorApp::LoadEditorPlugins()
{
  W_PROFILE_SCOPE("LoadEditorPlugins");
  DetectAvailablePluginBundles(WOSFile::GetApplicationDirectory());

  WPlugin::InitializeStaticallyLinkedPlugins();
}
