#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Stopwatch.h>

namespace
{
  struct SerializableStructWithMethods
  {

    W_DECLARE_POD_TYPE();

    WResult Serialize(WStreamWriter& inout_stream) const
    {
      inout_stream << m_uiMember1;
      inout_stream << m_uiMember2;

      return W_SUCCESS;
    }

    WResult Deserialize(WStreamReader& inout_stream)
    {
      inout_stream >> m_uiMember1;
      inout_stream >> m_uiMember2;

      return W_SUCCESS;
    }

    WInt32 m_uiMember1 = 0x42;
    WInt32 m_uiMember2 = 0x23;
  };
} // namespace

W_CREATE_SIMPLE_TEST(IO, StreamOperation)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Binary Stream Basic Operations (built-in types)")
  {
    WDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    WMemoryStreamWriter StreamWriter(&StreamStorage);

    StreamWriter << (WUInt8)0x42;
    StreamWriter << (WUInt16)0x4223;
    StreamWriter << (WUInt32)0x42232342;
    StreamWriter << (WUInt64)0x4223234242232342;
    StreamWriter << 42.0f;
    StreamWriter << 23.0;
    StreamWriter << (WInt8)0x23;
    StreamWriter << (WInt16)0x2342;
    StreamWriter << (WInt32)0x23422342;
    StreamWriter << (WInt64)0x2342234242232342;

    // Arrays
    {
      WDynamicArray<WUInt32> DynamicArray;
      DynamicArray.PushBack(42);
      DynamicArray.PushBack(23);
      DynamicArray.PushBack(13);
      DynamicArray.PushBack(5);
      DynamicArray.PushBack(0);

      StreamWriter.WriteArray(DynamicArray).IgnoreResult();
    }

    // Create reader
    WMemoryStreamReader StreamReader(&StreamStorage);

    // Read back
    {
      WUInt8 uiVal;
      StreamReader >> uiVal;
      W_TEST_BOOL(uiVal == (WUInt8)0x42);
    }
    {
      WUInt16 uiVal;
      StreamReader >> uiVal;
      W_TEST_BOOL(uiVal == (WUInt16)0x4223);
    }
    {
      WUInt32 uiVal;
      StreamReader >> uiVal;
      W_TEST_BOOL(uiVal == (WUInt32)0x42232342);
    }
    {
      WUInt64 uiVal;
      StreamReader >> uiVal;
      W_TEST_BOOL(uiVal == (WUInt64)0x4223234242232342);
    }

    {
      float fVal;
      StreamReader >> fVal;
      W_TEST_BOOL(fVal == 42.0f);
    }
    {
      double dVal;
      StreamReader >> dVal;
      W_TEST_BOOL(dVal == 23.0f);
    }


    {
      WInt8 iVal;
      StreamReader >> iVal;
      W_TEST_BOOL(iVal == (WInt8)0x23);
    }
    {
      WInt16 iVal;
      StreamReader >> iVal;
      W_TEST_BOOL(iVal == (WInt16)0x2342);
    }
    {
      WInt32 iVal;
      StreamReader >> iVal;
      W_TEST_BOOL(iVal == (WInt32)0x23422342);
    }
    {
      WInt64 iVal;
      StreamReader >> iVal;
      W_TEST_BOOL(iVal == (WInt64)0x2342234242232342);
    }

    {
      WDynamicArray<WUInt32> ReadBackDynamicArray;

      // This element will be removed by the ReadArray function
      ReadBackDynamicArray.PushBack(0xAAu);

      StreamReader.ReadArray(ReadBackDynamicArray).IgnoreResult();

      W_TEST_INT(ReadBackDynamicArray.GetCount(), 5);

      W_TEST_INT(ReadBackDynamicArray[0], 42);
      W_TEST_INT(ReadBackDynamicArray[1], 23);
      W_TEST_INT(ReadBackDynamicArray[2], 13);
      W_TEST_INT(ReadBackDynamicArray[3], 5);
      W_TEST_INT(ReadBackDynamicArray[4], 0);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Binary Stream Arrays of Structs")
  {
    WDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    WMemoryStreamWriter StreamWriter(&StreamStorage);

    // Write out a couple of the structs
    {
      WStaticArray<SerializableStructWithMethods, 16> WriteArray;
      WriteArray.ExpandAndGetRef().m_uiMember1 = 0x5;
      WriteArray.ExpandAndGetRef().m_uiMember1 = 0x6;

      StreamWriter.WriteArray(WriteArray).IgnoreResult();
    }

    // Read back in
    {
      // Create reader
      WMemoryStreamReader StreamReader(&StreamStorage);

      // This intentionally uses a different array type for the read back
      // to verify that it is a) compatible and b) all arrays are somewhat tested
      WTempHybridArray<SerializableStructWithMethods, 1> ReadArray;

      StreamReader.ReadArray(ReadArray).IgnoreResult();

      W_TEST_INT(ReadArray.GetCount(), 2);

      W_TEST_INT(ReadArray[0].m_uiMember1, 0x5);
      W_TEST_INT(ReadArray[0].m_uiMember2, 0x23);

      W_TEST_INT(ReadArray[1].m_uiMember1, 0x6);
      W_TEST_INT(ReadArray[1].m_uiMember2, 0x23);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WSet Stream Operators")
  {
    WDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    WMemoryStreamWriter StreamWriter(&StreamStorage);

    WSet<WString> TestSet;
    TestSet.Insert("Hello");
    TestSet.Insert("World");
    TestSet.Insert("!");

    StreamWriter.WriteSet(TestSet).IgnoreResult();

    WSet<WString> TestSetReadBack;

    TestSetReadBack.Insert("Shouldn't be there after deserialization.");

    WMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadSet(TestSetReadBack).IgnoreResult();

    W_TEST_INT(TestSetReadBack.GetCount(), 3);

    W_TEST_BOOL(TestSetReadBack.Contains("Hello"));
    W_TEST_BOOL(TestSetReadBack.Contains("!"));
    W_TEST_BOOL(TestSetReadBack.Contains("World"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WMap Stream Operators")
  {
    WDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    WMemoryStreamWriter StreamWriter(&StreamStorage);

    WMap<WUInt64, WString> TestMap;
    TestMap.Insert(42, "Hello");
    TestMap.Insert(23, "World");
    TestMap.Insert(5, "!");

    StreamWriter.WriteMap(TestMap).IgnoreResult();

    WMap<WUInt64, WString> TestMapReadBack;

    TestMapReadBack.Insert(1, "Shouldn't be there after deserialization.");

    WMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadMap(TestMapReadBack).IgnoreResult();

    W_TEST_INT(TestMapReadBack.GetCount(), 3);

    W_TEST_BOOL(TestMapReadBack.Contains(42));
    W_TEST_BOOL(TestMapReadBack.Contains(5));
    W_TEST_BOOL(TestMapReadBack.Contains(23));

    W_TEST_BOOL(TestMapReadBack.GetValue(42)->IsEqual("Hello"));
    W_TEST_BOOL(TestMapReadBack.GetValue(5)->IsEqual("!"));
    W_TEST_BOOL(TestMapReadBack.GetValue(23)->IsEqual("World"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WHashTable Stream Operators")
  {
    WDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    WMemoryStreamWriter StreamWriter(&StreamStorage);

    WHashTable<WUInt64, WString> TestHashTable;
    TestHashTable.Insert(42, "Hello");
    TestHashTable.Insert(23, "World");
    TestHashTable.Insert(5, "!");

    StreamWriter.WriteHashTable(TestHashTable).IgnoreResult();

    WMap<WUInt64, WString> TestHashTableReadBack;

    TestHashTableReadBack.Insert(1, "Shouldn't be there after deserialization.");

    WMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadMap(TestHashTableReadBack).IgnoreResult();

    W_TEST_INT(TestHashTableReadBack.GetCount(), 3);

    W_TEST_BOOL(TestHashTableReadBack.Contains(42));
    W_TEST_BOOL(TestHashTableReadBack.Contains(5));
    W_TEST_BOOL(TestHashTableReadBack.Contains(23));

    W_TEST_BOOL(TestHashTableReadBack.GetValue(42)->IsEqual("Hello"));
    W_TEST_BOOL(TestHashTableReadBack.GetValue(5)->IsEqual("!"));
    W_TEST_BOOL(TestHashTableReadBack.GetValue(23)->IsEqual("World"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "String Deduplication")
  {
    WDefaultMemoryStreamStorage StreamStorageNonDeduplicated(4096);
    WDefaultMemoryStreamStorage StreamStorageDeduplicated(4096);

    WHybridString<4> str1 = "Hello World";
    WDynamicString str2 = "Hello World 2";
    WStringBuilder str3 = "Hello Schlumpf";

    // Non deduplicated serialization
    {
      WMemoryStreamWriter StreamWriter(&StreamStorageNonDeduplicated);

      StreamWriter << str1;
      StreamWriter << str2;
      StreamWriter << str1;
      StreamWriter << str3;
      StreamWriter << str1;
      StreamWriter << str2;
    }

    // Deduplicated serialization
    {
      WMemoryStreamWriter StreamWriter(&StreamStorageDeduplicated);

      WStringDeduplicationWriteContext StringDeduplicationContext(StreamWriter);
      auto& DeduplicationWriter = StringDeduplicationContext.Begin();

      DeduplicationWriter << str1;
      DeduplicationWriter << str2;
      DeduplicationWriter << str1;
      DeduplicationWriter << str3;
      DeduplicationWriter << str1;
      DeduplicationWriter << str2;

      StringDeduplicationContext.End().IgnoreResult();

      W_TEST_INT(StringDeduplicationContext.GetUniqueStringCount(), 3);
    }

    W_TEST_BOOL(StreamStorageDeduplicated.GetStorageSize64() < StreamStorageNonDeduplicated.GetStorageSize64());

    // Read the deduplicated strings back
    {
      WMemoryStreamReader StreamReader(&StreamStorageDeduplicated);

      WStringDeduplicationReadContext StringDeduplicationReadContext(StreamReader);

      WHybridString<16> szRead0, szRead1, szRead2;
      WStringBuilder szRead3, szRead4, szRead5;

      StreamReader >> szRead0;
      StreamReader >> szRead1;
      StreamReader >> szRead2;
      StreamReader >> szRead3;
      StreamReader >> szRead4;
      StreamReader >> szRead5;

      W_TEST_STRING(szRead0, szRead2);
      W_TEST_STRING(szRead0, szRead4);
      W_TEST_STRING(szRead1, szRead5);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Array Serialization Performance (bytes)")
  {
    constexpr WUInt32 uiCount = 1024 * 1024 * 10;

    WContiguousMemoryStreamStorage storage(uiCount + 16);

    WMemoryStreamWriter writer(&storage);
    WMemoryStreamReader reader(&storage);

    WDynamicArray<WUInt8> DynamicArray;
    DynamicArray.SetCountUninitialized(uiCount);

    for (WUInt32 i = 0; i < uiCount; ++i)
    {
      DynamicArray[i] = i & 0xFF;
    }

    {
      WStopwatch sw;

      writer.WriteArray(DynamicArray).AssertSuccess();

      WTime t = sw.GetRunningTotal();
      WStringBuilder s;
      s.SetFormat("Write {} byte array: {}", WArgFileSize(uiCount), t);
      WTestFramework::Output(WTestOutput::Details, s);
    }

    {
      WStopwatch sw;

      reader.ReadArray(DynamicArray).IgnoreResult();

      WTime t = sw.GetRunningTotal();
      WStringBuilder s;
      s.SetFormat("Read {} byte array: {}", WArgFileSize(uiCount), t);
      WTestFramework::Output(WTestOutput::Details, s);
    }

    for (WUInt32 i = 0; i < uiCount; ++i)
    {
      W_TEST_INT(DynamicArray[i], i & 0xFF);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Array Serialization Performance (WVec3)")
  {
    constexpr WUInt32 uiCount = 1024 * 1024 * 10;

    WContiguousMemoryStreamStorage storage(uiCount * sizeof(WVec3) + 16);

    WMemoryStreamWriter writer(&storage);
    WMemoryStreamReader reader(&storage);

    WDynamicArray<WVec3> DynamicArray;
    DynamicArray.SetCountUninitialized(uiCount);

    for (WUInt32 i = 0; i < uiCount; ++i)
    {
      DynamicArray[i].Set(i, i + 1, i + 2);
    }

    {
      WStopwatch sw;

      writer.WriteArray(DynamicArray).AssertSuccess();

      WTime t = sw.GetRunningTotal();
      WStringBuilder s;
      s.SetFormat("Write {} vec3 array: {}", WArgFileSize(uiCount * sizeof(WVec3)), t);
      WTestFramework::Output(WTestOutput::Details, s);
    }

    {
      WStopwatch sw;

      reader.ReadArray(DynamicArray).AssertSuccess();

      WTime t = sw.GetRunningTotal();
      WStringBuilder s;
      s.SetFormat("Read {} vec3 array: {}", WArgFileSize(uiCount * sizeof(WVec3)), t);
      WTestFramework::Output(WTestOutput::Details, s);
    }

    for (WUInt32 i = 0; i < uiCount; ++i)
    {
      W_TEST_VEC3(DynamicArray[i], WVec3(i, i + 1, i + 2), 0.01f);
    }
  }
}
