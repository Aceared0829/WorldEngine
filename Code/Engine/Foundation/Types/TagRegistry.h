
#pragma once

class WHashedString;
class WTempHashedString;
class WTag;
class WStreamWriter;
class WStreamReader;

#include <Foundation/Containers/Map.h>
#include <Foundation/Threading/Mutex.h>

/// The tag registry for tags in tag sets.
///
/// Normal usage of the tag registry is to get the global tag registry instance via WTagRegistry::GetGlobalRegistry()
/// and to use this instance to register and get tags.
/// Certain special cases (e.g. tests) may actually need their own instance of the tag registry.
/// Note however that tags which were registered with one registry shouldn't be used with tag sets filled
/// with tags from another registry since there may be conflicting tag assignments.
/// The tag registry registration and tag retrieval functions are thread safe due to a mutex.
class W_FOUNDATION_DLL WTagRegistry
{
public:
  WTagRegistry();

  static WTagRegistry& GetGlobalRegistry();

  /// Ensures the tag with the given name exists and returns a pointer to it.
  const WTag& RegisterTag(WStringView sTagString); // [tested]

  /// Ensures the tag with the given name exists and returns a pointer to it.
  const WTag& RegisterTag(const WHashedString& sTagString); // [tested]

  /// Searches for a tag with the given name and returns a pointer to it
  const WTag* GetTagByName(const WTempHashedString& sTagString) const; // [tested]

  /// Searches for a tag with the given murmur hash. This function is only for backwards compatibility.
  const WTag* GetTagByMurmurHash(WUInt32 uiMurmurHash) const;

  /// Returns the tag with the given index.
  const WTag* GetTagByIndex(WUInt32 uiIndex) const;

  /// Returns the number of registered tags.
  WUInt32 GetNumTags() const;

  /// Loads the saved state and integrates it into this registry. Does not discard previously registered tag information. This function is only
  /// for backwards compatibility.
  WResult Load(WStreamReader& inout_stream);

protected:
  mutable WMutex m_TagRegistryMutex;

  WMap<WTempHashedString, WTag> m_RegisteredTags;
  WDeque<WTag*> m_TagsByIndex;
};
