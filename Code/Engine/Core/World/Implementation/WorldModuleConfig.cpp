#include <Core/CorePCH.h>

#include <Core/World/WorldModule.h>
#include <Core/World/WorldModuleConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

WResult WWorldModuleConfig::Save()
{
  m_InterfaceImpls.Sort();

  WStringBuilder sPath;
  sPath = ":project/WorldModules.ddl";

  WFileWriter file;
  if (file.Open(sPath).Failed())
    return W_FAILURE;

  WOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(WOpenDdlWriter::TypeStringMode::Compliant);

  for (auto& interfaceImpl : m_InterfaceImpls)
  {
    writer.BeginObject("InterfaceImpl");

    WOpenDdlUtils::StoreString(writer, interfaceImpl.m_sInterfaceName, "Interface");
    WOpenDdlUtils::StoreString(writer, interfaceImpl.m_sImplementationName, "Implementation");

    writer.EndObject();
  }

  return W_SUCCESS;
}

void WWorldModuleConfig::Load()
{
  const char* szPath = ":project/WorldModules.ddl";

  W_LOG_BLOCK("WWorldModuleConfig::Load()", szPath);

  m_InterfaceImpls.Clear();

  WFileReader file;
  if (file.Open(szPath).Failed())
  {
    WLog::Dev("World module config file is not available: '{0}'", szPath);
    return;
  }
  else
  {
    WLog::Success("World module config file is available: '{0}'", szPath);
  }

  WOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, WLog::GetThreadLocalLogSystem()).Failed())
  {
    WLog::Error("Failed to parse world module config file '{0}'", szPath);
    return;
  }

  const WOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const WOpenDdlReaderElement* pInterfaceImpl = pTree->GetFirstChild(); pInterfaceImpl != nullptr;
       pInterfaceImpl = pInterfaceImpl->GetSibling())
  {
    if (!pInterfaceImpl->IsCustomType("InterfaceImpl"))
      continue;

    const WOpenDdlReaderElement* pInterface = pInterfaceImpl->FindChildOfType(WOpenDdlPrimitiveType::String, "Interface");
    const WOpenDdlReaderElement* pImplementation = pInterfaceImpl->FindChildOfType(WOpenDdlPrimitiveType::String, "Implementation");

    // this prevents duplicates
    AddInterfaceImplementation(pInterface->GetPrimitivesString()[0], pImplementation->GetPrimitivesString()[0]);
  }
}

void WWorldModuleConfig::Apply()
{
  W_LOG_BLOCK("WWorldModuleConfig::Apply");

  for (const auto& interfaceImpl : m_InterfaceImpls)
  {
    WWorldModuleFactory::GetInstance()->RegisterInterfaceImplementation(interfaceImpl.m_sInterfaceName, interfaceImpl.m_sImplementationName);
  }
}

void WWorldModuleConfig::AddInterfaceImplementation(WStringView sInterfaceName, WStringView sImplementationName)
{
  for (auto& interfaceImpl : m_InterfaceImpls)
  {
    if (interfaceImpl.m_sInterfaceName == sInterfaceName)
    {
      interfaceImpl.m_sImplementationName = sImplementationName;
      return;
    }
  }

  m_InterfaceImpls.PushBack({sInterfaceName, sImplementationName});
}

void WWorldModuleConfig::RemoveInterfaceImplementation(WStringView sInterfaceName)
{
  for (WUInt32 i = 0; i < m_InterfaceImpls.GetCount(); ++i)
  {
    if (m_InterfaceImpls[i].m_sInterfaceName == sInterfaceName)
    {
      m_InterfaceImpls.RemoveAtAndCopy(i);
      return;
    }
  }
}
