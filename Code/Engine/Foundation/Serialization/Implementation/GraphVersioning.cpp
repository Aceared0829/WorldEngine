#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WTypeVersionInfo, WNoBase, 1, WRTTIDefaultAllocator<WTypeVersionInfo>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("TypeName", GetTypeName, SetTypeName),
    W_ACCESSOR_PROPERTY("ParentTypeName", GetParentTypeName, SetParentTypeName),
    W_MEMBER_PROPERTY("TypeVersion", m_uiTypeVersion),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

const char* WTypeVersionInfo::GetTypeName() const
{
  return m_sTypeName.GetData();
}

void WTypeVersionInfo::SetTypeName(const char* szName)
{
  m_sTypeName.Assign(szName);
}

const char* WTypeVersionInfo::GetParentTypeName() const
{
  return m_sParentTypeName.GetData();
}

void WTypeVersionInfo::SetParentTypeName(const char* szName)
{
  m_sParentTypeName.Assign(szName);
}

void WGraphPatchContext::PatchBaseClass(const char* szType, WUInt32 uiTypeVersion, bool bForcePatch)
{
  WHashedString sType;
  sType.Assign(szType);
  for (WUInt32 uiBaseClassIndex = m_uiBaseClassIndex; uiBaseClassIndex < m_BaseClasses.GetCount(); ++uiBaseClassIndex)
  {
    if (m_BaseClasses[uiBaseClassIndex].m_sType == sType)
    {
      Patch(uiBaseClassIndex, uiTypeVersion, bForcePatch);
      return;
    }
  }
  W_REPORT_FAILURE("Base class of name '{0}' not found in parent types of '{1}'", sType.GetData(), m_pNode->GetType());
}

void WGraphPatchContext::RenameClass(const char* szTypeName)
{
  m_pNode->SetType(m_pGraph->RegisterString(szTypeName));
  m_BaseClasses[m_uiBaseClassIndex].m_sType.Assign(szTypeName);
}


void WGraphPatchContext::RenameClass(const char* szTypeName, WUInt32 uiVersion)
{
  m_pNode->SetType(m_pGraph->RegisterString(szTypeName));
  m_BaseClasses[m_uiBaseClassIndex].m_sType.Assign(szTypeName);
  // After a Patch is applied, the version is always increased. So if we want to change the version we need to reduce it by one so that in the next patch loop the requested version is not skipped.
  W_ASSERT_DEV(uiVersion > 0, "Cannot change the version of a class to 0, target version must be at least 1.");
  m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion = uiVersion - 1;
}

void WGraphPatchContext::ChangeBaseClass(WArrayPtr<WVersionKey> baseClasses)
{
  m_BaseClasses.SetCount(m_uiBaseClassIndex + 1 + baseClasses.GetCount());
  for (WUInt32 i = 0; i < baseClasses.GetCount(); i++)
  {
    m_BaseClasses[m_uiBaseClassIndex + 1 + i] = baseClasses[i];
  }
}

//////////////////////////////////////////////////////////////////////////

WGraphPatchContext::WGraphPatchContext(WGraphVersioning* pParent, WAbstractObjectGraph* pGraph, WAbstractObjectGraph* pTypesGraph)
{
  W_PROFILE_SCOPE("WGraphPatchContext");
  m_pParent = pParent;
  m_pGraph = pGraph;
  if (pTypesGraph)
  {
    WRttiConverterContext context;
    WRttiConverterReader rttiConverter(pTypesGraph, &context);
    WString sDescTypeName = "WReflectedTypeDescriptor";
    auto& nodes = pTypesGraph->GetAllNodes();
    m_TypeToInfo.Reserve(nodes.GetCount());
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->GetType() == sDescTypeName)
      {
        WTypeVersionInfo info;
        rttiConverter.ApplyPropertiesToObject(it.Value(), WGetStaticRTTI<WTypeVersionInfo>(), &info);
        m_TypeToInfo.Insert(info.m_sTypeName, info);
      }
    }
  }
}

void WGraphPatchContext::Patch(WAbstractObjectNode* pNode)
{
  m_pNode = pNode;
  // Build version hierarchy.
  m_BaseClasses.Clear();
  WVersionKey key;
  key.m_sType.Assign(m_pNode->GetType());
  key.m_uiTypeVersion = m_pNode->GetTypeVersion();

  m_BaseClasses.PushBack(key);
  UpdateBaseClasses();

  // Patch
  for (m_uiBaseClassIndex = 0; m_uiBaseClassIndex < m_BaseClasses.GetCount(); ++m_uiBaseClassIndex)
  {
    const WUInt32 uiMaxVersion = m_pParent->GetMaxPatchVersion(m_BaseClasses[m_uiBaseClassIndex].m_sType);
    Patch(m_uiBaseClassIndex, uiMaxVersion, false);
  }
  m_pNode->SetTypeVersion(m_BaseClasses[0].m_uiTypeVersion);
}


void WGraphPatchContext::Patch(WUInt32 uiBaseClassIndex, WUInt32 uiTypeVersion, bool bForcePatch)
{
  if (bForcePatch)
  {
    m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion = WMath::Min(m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion, uiTypeVersion - 1);
  }
  while (m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion < uiTypeVersion)
  {
    // Don't move this out of the loop, needed to support renaming a class which will change the key.
    WVersionKey key = m_BaseClasses[uiBaseClassIndex];
    key.m_uiTypeVersion += 1;
    const WGraphPatch* pPatch = nullptr;
    if (m_pParent->m_NodePatches.TryGetValue(key, pPatch))
    {
      pPatch->Patch(*this, m_pGraph, m_pNode);
      uiTypeVersion = m_pParent->GetMaxPatchVersion(m_BaseClasses[m_uiBaseClassIndex].m_sType);
    }
    // Don't use a ref to the key as the array might get resized during patching.
    // Patch function can change the type and version so we need to read m_uiTypeVersion again instead of just writing key.m_uiTypeVersion;
    m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion++;
  }
}

void WGraphPatchContext::UpdateBaseClasses()
{
  for (;;)
  {
    WHashedString sParentType;
    if (WTypeVersionInfo* pInfo = m_TypeToInfo.GetValue(m_BaseClasses.PeekBack().m_sType))
    {
      m_BaseClasses.PeekBack().m_uiTypeVersion = pInfo->m_uiTypeVersion;
      sParentType = pInfo->m_sParentTypeName;
    }
    else if (const WRTTI* pType = WRTTI::FindTypeByName(m_BaseClasses.PeekBack().m_sType.GetData()))
    {
      m_BaseClasses.PeekBack().m_uiTypeVersion = pType->GetTypeVersion();
      if (pType->GetParentType())
      {
        sParentType.Assign(pType->GetParentType()->GetTypeName());
      }
      else
        sParentType = WHashedString();
    }
    else
    {
      WLog::Error("Can't patch base class, parent type of '{0}' unknown.", m_BaseClasses.PeekBack().m_sType.GetData());
      break;
    }

    if (sParentType.IsEmpty())
      break;

    WVersionKey key;
    key.m_sType = std::move(sParentType);
    key.m_uiTypeVersion = 0;
    m_BaseClasses.PushBack(key);
  }
}

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_SINGLETON(WGraphVersioning);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, GraphVersioning)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WGraphVersioning);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WGraphVersioning* pDummy = WGraphVersioning::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WGraphVersioning::WGraphVersioning()
  : m_SingletonRegistrar(this)
{
  WPlugin::Events().AddEventHandler(WMakeDelegate(&WGraphVersioning::PluginEventHandler, this));

  UpdatePatches();
}

WGraphVersioning::~WGraphVersioning()
{
  WPlugin::Events().RemoveEventHandler(WMakeDelegate(&WGraphVersioning::PluginEventHandler, this));
}

void WGraphVersioning::PatchGraph(WAbstractObjectGraph* pGraph, WAbstractObjectGraph* pTypesGraph)
{
  W_PROFILE_SCOPE("PatchGraph");

  WGraphPatchContext context(this, pGraph, pTypesGraph);
  for (const WGraphPatch* pPatch : m_GraphPatches)
  {
    pPatch->Patch(context, pGraph, nullptr);
  }

  auto& nodes = pGraph->GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    WAbstractObjectNode* pNode = it.Value();
    context.Patch(pNode);
  }
}

void WGraphVersioning::PluginEventHandler(const WPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case WPluginEvent::AfterLoadingBeforeInit:
    case WPluginEvent::AfterUnloading:
      UpdatePatches();
      break;
    default:
      break;
  }
}

void WGraphVersioning::UpdatePatches()
{
  m_GraphPatches.Clear();
  m_NodePatches.Clear();
  m_MaxPatchVersion.Clear();

  WVersionKey key;
  WGraphPatch* pInstance = WGraphPatch::GetFirstInstance();

  while (pInstance)
  {
    switch (pInstance->GetPatchType())
    {
      case WGraphPatch::PatchType::NodePatch:
      {
        key.m_sType.Assign(pInstance->GetType());
        key.m_uiTypeVersion = pInstance->GetTypeVersion();
        m_NodePatches.Insert(key, pInstance);

        if (WUInt32* pMax = m_MaxPatchVersion.GetValue(key.m_sType))
        {
          *pMax = WMath::Max(*pMax, key.m_uiTypeVersion);
        }
        else
        {
          m_MaxPatchVersion[key.m_sType] = key.m_uiTypeVersion;
        }
      }
      break;
      case WGraphPatch::PatchType::GraphPatch:
      {
        m_GraphPatches.PushBack(pInstance);
      }
      break;
    }
    pInstance = pInstance->GetNextInstance();
  }

  m_GraphPatches.Sort([](const WGraphPatch* a, const WGraphPatch* b) -> bool
    { return a->GetTypeVersion() < b->GetTypeVersion(); });
}

WUInt32 WGraphVersioning::GetMaxPatchVersion(const WHashedString& sType) const
{
  if (const WUInt32* pMax = m_MaxPatchVersion.GetValue(sType))
  {
    return *pMax;
  }
  return 0;
}

W_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_GraphVersioning);
