#include <Core/CorePCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>

#include <Core/ResourceManager/ResourceManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProfileConfigData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

WProfileConfigData::WProfileConfigData() = default;
WProfileConfigData::~WProfileConfigData() = default;

void WProfileConfigData::SaveRuntimeData(WChunkStreamWriter& inout_stream) const
{
  W_IGNORE_UNUSED(inout_stream);
}

void WProfileConfigData::LoadRuntimeData(WChunkStreamReader& inout_stream)
{
  W_IGNORE_UNUSED(inout_stream);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPlatformProfile, 1, WRTTIDefaultAllocator<WPlatformProfile>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("TargetPlatform", m_sTargetPlatform)->AddAttributes(new WDynamicStringEnumAttribute("TargetPlatformNames"), new WDefaultValueAttribute("Windows")),
    W_ARRAY_MEMBER_PROPERTY("Configs", m_Configs)->AddFlags(WPropertyFlags::PointerOwner)->AddAttributes(new WContainerAttribute(false, false, false)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WPlatformProfile::WPlatformProfile() = default;

WPlatformProfile::~WPlatformProfile()
{
  Clear();
}

void WPlatformProfile::Clear()
{
  for (auto pType : m_Configs)
  {
    pType->GetDynamicRTTI()->GetAllocator()->Deallocate(pType);
  }

  m_Configs.Clear();
}

void WPlatformProfile::AddMissingConfigs()
{
  WRTTI::ForEachDerivedType<WProfileConfigData>(
    [this](const WRTTI* pRtti)
    {
      // find all types derived from WProfileConfigData
      bool bHasTypeAlready = false;

      // check whether we already have an instance of this type
      for (auto pType : m_Configs)
      {
        if (pType && pType->GetDynamicRTTI() == pRtti)
        {
          bHasTypeAlready = true;
          break;
        }
      }

      if (!bHasTypeAlready)
      {
        // if not, allocate one
        WProfileConfigData* pObject = pRtti->GetAllocator()->Allocate<WProfileConfigData>();
        W_ASSERT_DEV(pObject != nullptr, "Invalid profile config");
        WReflectionUtils::SetAllMemberPropertiesToDefault(pRtti, pObject);

        m_Configs.PushBack(pObject);
      }
    },
    WRTTI::ForEachOptions::ExcludeNonAllocatable);

  // in case unknown configs were loaded from disk, remove them
  m_Configs.RemoveAndSwap(nullptr);

  // sort all configs alphabetically
  m_Configs.Sort([](const WProfileConfigData* lhs, const WProfileConfigData* rhs) -> bool
    { return lhs->GetDynamicRTTI()->GetTypeName().Compare(rhs->GetDynamicRTTI()->GetTypeName()) < 0; });
}

const WProfileConfigData* WPlatformProfile::GetTypeConfig(const WRTTI* pRtti) const
{
  for (const auto* pConfig : m_Configs)
  {
    if (pConfig->GetDynamicRTTI() == pRtti)
      return pConfig;
  }

  return nullptr;
}

WProfileConfigData* WPlatformProfile::GetTypeConfig(const WRTTI* pRtti)
{
  // reuse the const-version
  return const_cast<WProfileConfigData*>(((const WPlatformProfile*)this)->GetTypeConfig(pRtti));
}

WResult WPlatformProfile::SaveForRuntime(WStringView sFile) const
{
  WFileWriter file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  WChunkStreamWriter chunk(file);

  chunk.BeginStream(1);

  for (auto* pConfig : m_Configs)
  {
    pConfig->SaveRuntimeData(chunk);
  }

  chunk.EndStream();

  return W_SUCCESS;
}

WResult WPlatformProfile::LoadForRuntime(WStringView sFile)
{
  WFileReader file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  WChunkStreamReader chunk(file);

  chunk.BeginStream();

  while (chunk.GetCurrentChunk().m_bValid)
  {
    for (auto* pConfig : m_Configs)
    {
      pConfig->LoadRuntimeData(chunk);
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  ++m_uiLastModificationCounter;
  return W_SUCCESS;
}



W_STATICLINK_FILE(Core, Core_Configuration_Implementation_PlatformProfile);
