
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Strings/String.h>

class WStreamWriter;
class WStreamReader;

/// This class allows for automatic deduplication of strings written to a stream.
/// To use, create an object of this type on the stack, call Begin() and use the returned
/// WStreamWriter for subsequent serialization operations. Call End() once you want to finish writing
/// deduplicated strings. For a sample see StreamOperationsTest.cpp
class W_FOUNDATION_DLL WStringDeduplicationWriteContext : public WSerializationContext<WStringDeduplicationWriteContext>
{
  W_DECLARE_SERIALIZATION_CONTEXT(WStringDeduplicationWriteContext);

public:
  /// Setup the write context to perform string deduplication.
  WStringDeduplicationWriteContext(WStreamWriter& ref_originalStream);
  ~WStringDeduplicationWriteContext();

  /// Call this method to begin string deduplicaton. You need to use the returned stream writer for subsequent serialization operations until
  /// End() is called.
  WStreamWriter& Begin();

  /// Ends the string deduplication and writes the string table to the original stream
  WResult End();

  /// Internal method to serialize a string.
  void SerializeString(const WStringView& sString, WStreamWriter& ref_writer);

  /// Returns the number of unique strings which were serialized with this instance.
  WUInt32 GetUniqueStringCount() const;

  /// Returns the original stream that was passed to the constructor.
  WStreamWriter& GetOriginalStream() { return m_OriginalStream; }

protected:
  WStreamWriter& m_OriginalStream;

  WDefaultMemoryStreamStorage m_TempStreamStorage;
  WMemoryStreamWriter m_TempStreamWriter;

  WMap<WHybridString<64>, WUInt32> m_DeduplicatedStrings;
};

/// This class to restore strings written to a stream using a WStringDeduplicationWriteContext.
class W_FOUNDATION_DLL WStringDeduplicationReadContext : public WSerializationContext<WStringDeduplicationReadContext>
{
  W_DECLARE_SERIALIZATION_CONTEXT(WStringDeduplicationReadContext);

public:
  /// Setup the string table used internally.
  WStringDeduplicationReadContext(WStreamReader& inout_stream);
  ~WStringDeduplicationReadContext();

  /// Internal method to deserialize a string.
  WStringView DeserializeString(WStreamReader& ref_reader);

protected:
  WDynamicArray<WHybridString<64>> m_DeduplicatedStrings;
};
