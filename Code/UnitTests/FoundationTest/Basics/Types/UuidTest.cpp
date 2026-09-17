#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/Uuid.h>


W_CREATE_SIMPLE_TEST(Basics, Uuid)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Uuid Generation")
  {
    WUuid ShouldBeInvalid;

    W_TEST_BOOL(ShouldBeInvalid.IsValid() == false);

    WUuid FirstGenerated = WUuid::MakeUuid();
    W_TEST_BOOL(FirstGenerated.IsValid());

    WUuid SecondGenerated = WUuid::MakeUuid();
    W_TEST_BOOL(SecondGenerated.IsValid());

    W_TEST_BOOL(!(FirstGenerated == SecondGenerated));
    W_TEST_BOOL(FirstGenerated != SecondGenerated);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Uuid Serialization")
  {
    WUuid Uuid;
    W_TEST_BOOL(Uuid.IsValid() == false);

    Uuid = WUuid::MakeUuid();
    W_TEST_BOOL(Uuid.IsValid());

    WDefaultMemoryStreamStorage StreamStorage;

    // Create reader
    WMemoryStreamReader StreamReader(&StreamStorage);

    // Create writer
    WMemoryStreamWriter StreamWriter(&StreamStorage);

    StreamWriter << Uuid;

    WUuid ReadBack;
    W_TEST_BOOL(ReadBack.IsValid() == false);

    StreamReader >> ReadBack;

    W_TEST_BOOL(ReadBack == Uuid);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Stable Uuid From String")
  {
    WUuid uuid1 = WUuid::MakeStableUuidFromString("TEST 1");
    WUuid uuid2 = WUuid::MakeStableUuidFromString("TEST 2");
    WUuid uuid3 = WUuid::MakeStableUuidFromString("TEST 1");

    W_TEST_BOOL(uuid1 == uuid3);
    W_TEST_BOOL(uuid1 != uuid2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Uuid Combine")
  {
    WUuid uuid1 = WUuid::MakeUuid();
    WUuid uuid2 = WUuid::MakeUuid();
    WUuid combined = uuid1;
    combined.CombineWithSeed(uuid2);
    W_TEST_BOOL(combined != uuid1);
    W_TEST_BOOL(combined != uuid2);
    combined.RevertCombinationWithSeed(uuid2);
    W_TEST_BOOL(combined == uuid1);

    WUuid hashA = uuid1;
    hashA.HashCombine(uuid2);
    WUuid hashB = uuid2;
    hashA.HashCombine(uuid1);
    W_TEST_BOOL(hashA != hashB);
  }
}
