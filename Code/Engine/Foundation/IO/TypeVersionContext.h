#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/SerializationContext.h>

class WStreamWriter;
class WStreamReader;

/// This class allows for writing type versions to a stream in a centralized place so that
/// each object doesn't need to write its own version manually.
///
/// To use, create an object of this type on the stack, call Begin() and use the returned
/// WStreamWriter for subsequent serialization operations. Call AddType to add a type and its parent types to the version table.
/// Call End() once you want to finish writing the type versions.
class W_FOUNDATION_DLL WTypeVersionWriteContext : public WSerializationContext<WTypeVersionWriteContext>
{
  W_DECLARE_SERIALIZATION_CONTEXT(WTypeVersionWriteContext);

public:
  WTypeVersionWriteContext();
  ~WTypeVersionWriteContext();

  /// Call this method to begin collecting type version info. You need to use the returned stream writer for subsequent serialization operations until
  /// End() is called.
  WStreamWriter& Begin(WStreamWriter& ref_originalStream);

  /// Ends the type version collection and writes the data to the original stream.
  WResult End();

  /// Adds the given type and its parent types to the version table.
  void AddType(const WRTTI* pRtti);

  /// Manually write the version table to the given stream.
  /// Can be used instead of Begin()/End() if all necessary types are available in one place anyways.
  void WriteTypeVersions(WStreamWriter& inout_stream) const;

  /// Returns the original stream that was passed to Begin().
  WStreamWriter& GetOriginalStream() { return *m_pOriginalStream; }

protected:
  WStreamWriter* m_pOriginalStream = nullptr;

  WDefaultMemoryStreamStorage m_TempStreamStorage;
  WMemoryStreamWriter m_TempStreamWriter;

  WHashSet<const WRTTI*> m_KnownTypes;
};

/// Use this class to restore type versions written to a stream using a WTypeVersionWriteContext.
class W_FOUNDATION_DLL WTypeVersionReadContext : public WSerializationContext<WTypeVersionReadContext>
{
  W_DECLARE_SERIALIZATION_CONTEXT(WTypeVersionReadContext);

public:
  /// Reads the type version table from the stream
  WTypeVersionReadContext(WStreamReader& inout_stream);
  ~WTypeVersionReadContext();

  WUInt32 GetTypeVersion(const WRTTI* pRtti) const;

protected:
  WHashTable<const WRTTI*, WUInt32> m_TypeVersions;
};
