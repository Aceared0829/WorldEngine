#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct W_TOOLSFOUNDATION_DLL WToolsTag
{
  WToolsTag() = default;
  WToolsTag(WStringView sCategory, WStringView sName, bool bBuiltIn = false)
    : m_sCategory(sCategory)
    , m_sName(sName)
    , m_bBuiltInTag(bBuiltIn)
  {
  }

  WString m_sCategory;
  WString m_sName;
  bool m_bBuiltInTag = false; ///< If set to true, this is a tag created by code that the user is not allowed to remove
};

class W_TOOLSFOUNDATION_DLL WToolsTagRegistry
{
public:
  /// Removes all tags that are not specified as 'built-in'.
  static void Clear();

  /// Serializes all tags to a DDL stream.
  static void WriteToDDL(WStreamWriter& inout_stream);
  /// Reads tags from a DDL stream.
  static WStatus ReadFromDDL(WStreamReader& inout_stream);

  /// Adds a tag to the registry. Returns true if the tag was valid.
  static bool AddTag(const WToolsTag& tag);
  /// Removes a tag by name. Returns true if the tag was removed.
  static bool RemoveTag(WStringView sName);

  /// Retrieves all tags in the registry.
  static void GetAllTags(WDynamicArray<const WToolsTag*>& out_tags);
  /// Retrieves all tags in the given categories.
  static void GetTagsByCategory(const WArrayPtr<WStringView>& categories, WDynamicArray<const WToolsTag*>& out_tags);

  /// Returns whether a tag with this name is currently registered, regardless of its category.
  ///
  /// Objects may reference tags that were removed from the registry (e.g. after copying objects from another project),
  /// which is used to detect and surface such "dangling" tags in the UI.
  static bool IsTagKnown(WStringView sName);

private:
  static WMap<WString, WToolsTag> s_NameToTags;
};
