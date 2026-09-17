#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

struct TagComparer
{
  W_ALWAYS_INLINE bool Less(const WToolsTag* a, const WToolsTag* b) const
  {
    if (a->m_sCategory != b->m_sCategory)
      return a->m_sCategory < b->m_sCategory;

    return a->m_sName < b->m_sName;
    ;
  }
};
////////////////////////////////////////////////////////////////////////
// WToolsTagRegistry public functions
////////////////////////////////////////////////////////////////////////

WMap<WString, WToolsTag> WToolsTagRegistry::s_NameToTags;

void WToolsTagRegistry::Clear()
{
  for (auto it = s_NameToTags.GetIterator(); it.IsValid();)
  {
    if (!it.Value().m_bBuiltInTag)
    {
      it = s_NameToTags.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

void WToolsTagRegistry::WriteToDDL(WStreamWriter& inout_stream)
{
  WOpenDdlWriter writer;
  writer.SetOutputStream(&inout_stream);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(WOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);

  for (auto it = s_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    writer.BeginObject("Tag");

    writer.BeginPrimitiveList(WOpenDdlPrimitiveType::String, "Name");
    writer.WriteString(it.Value().m_sName);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(WOpenDdlPrimitiveType::String, "Category");
    writer.WriteString(it.Value().m_sCategory);
    writer.EndPrimitiveList();

    writer.EndObject();
  }
}

WStatus WToolsTagRegistry::ReadFromDDL(WStreamReader& inout_stream)
{
  WOpenDdlReader reader;
  if (reader.ParseDocument(inout_stream).Failed())
  {
    return WStatus("Failed to read data from ToolsTagRegistry stream!");
  }

  // Makes sure not to remove the built-in tags
  Clear();

  const WOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const WOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const WOpenDdlReaderElement* pName = pTags->FindChildOfType(WOpenDdlPrimitiveType::String, "Name");
    const WOpenDdlReaderElement* pCategory = pTags->FindChildOfType(WOpenDdlPrimitiveType::String, "Category");

    if (!pName || !pCategory)
    {
      WLog::Error("Incomplete tag declaration!");
      continue;
    }

    WToolsTag tag;
    tag.m_sName = pName->GetPrimitivesString()[0];
    tag.m_sCategory = pCategory->GetPrimitivesString()[0];

    if (!WToolsTagRegistry::AddTag(tag))
    {
      WLog::Error("Failed to add tag '{0}'", tag.m_sName);
    }
  }

  return WStatus(W_SUCCESS);
}

bool WToolsTagRegistry::AddTag(const WToolsTag& tag)
{
  if (tag.m_sName.IsEmpty())
    return false;

  auto it = s_NameToTags.Find(tag.m_sName);
  if (it.IsValid())
  {
    if (tag.m_bBuiltInTag)
    {
      // Make sure to pass this on, as it is not stored in the DDL file (because we don't want to rely on that)
      it.Value().m_bBuiltInTag = true;
    }

    return true;
  }
  else
  {
    s_NameToTags[tag.m_sName] = tag;
    return true;
  }
}

bool WToolsTagRegistry::RemoveTag(WStringView sName)
{
  auto it = s_NameToTags.Find(sName);
  if (it.IsValid())
  {
    s_NameToTags.Remove(it);
    return true;
  }
  else
  {
    return false;
  }
}

void WToolsTagRegistry::GetAllTags(WDynamicArray<const WToolsTag*>& out_tags)
{
  out_tags.Clear();
  for (auto it = s_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    out_tags.PushBack(&it.Value());
  }

  out_tags.Sort(TagComparer());
}

bool WToolsTagRegistry::IsTagKnown(WStringView sName)
{
  return s_NameToTags.Find(sName).IsValid();
}

void WToolsTagRegistry::GetTagsByCategory(const WArrayPtr<WStringView>& categories, WDynamicArray<const WToolsTag*>& out_tags)
{
  out_tags.Clear();
  for (auto it = s_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    if (std::any_of(cbegin(categories), cend(categories), [&it](const WStringView& sCat)
          { return it.Value().m_sCategory == sCat; }))
    {
      out_tags.PushBack(&it.Value());
    }
  }
  out_tags.Sort(TagComparer());
}
