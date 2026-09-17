#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/Tag.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Types/TagSet.h>

static_assert(sizeof(WTagSet) == 16);

#if W_ENABLED(W_PLATFORM_64BIT)
static_assert(sizeof(WTag) == 16);
#else
static_assert(sizeof(WTag) == 12);
#endif

W_CREATE_SIMPLE_TEST(Basics, TagSet)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Basic Tag Tests")
  {
    WTagRegistry TempTestRegistry;

    {
      WTag TestTag;
      W_TEST_BOOL(!TestTag.IsValid());
    }

    WHashedString TagName;
    TagName.Assign("BASIC_TAG_TEST");

    const WTag& SecondInstance = TempTestRegistry.RegisterTag(TagName);
    W_TEST_BOOL(SecondInstance.IsValid());

    const WTag* SecondInstance2 = TempTestRegistry.GetTagByName("BASIC_TAG_TEST");

    if (W_TEST_BOOL(SecondInstance2 != nullptr))
    {
      W_ANALYSIS_ASSUME(SecondInstance2 != nullptr);
      W_TEST_BOOL(SecondInstance2->IsValid());

      W_TEST_BOOL(&SecondInstance == SecondInstance2);

      W_TEST_STRING(SecondInstance2->GetTagString(), "BASIC_TAG_TEST");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Basic Tag Registration")
  {
    WTagRegistry TempTestRegistry;

    WTag TestTag;

    W_TEST_BOOL(!TestTag.IsValid());

    W_TEST_BOOL(TempTestRegistry.GetTagByName("TEST_TAG1") == nullptr);

    TestTag = TempTestRegistry.RegisterTag("TEST_TAG1");

    W_TEST_BOOL(TestTag.IsValid());

    W_TEST_BOOL(TempTestRegistry.GetTagByName("TEST_TAG1") != nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Basic Tag Work")
  {
    WTagRegistry TempTestRegistry;

    TempTestRegistry.RegisterTag("TEST_TAG1");

    const WTag* TestTag1 = TempTestRegistry.GetTagByName("TEST_TAG1");
    if (W_TEST_BOOL(TestTag1 != nullptr))
    {
      W_ANALYSIS_ASSUME(TestTag1 != nullptr);

      const WTag& TestTag2 = TempTestRegistry.RegisterTag("TEST_TAG2");

      W_TEST_BOOL(TestTag2.IsValid());

      WTagSet tagSet;

      W_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
      W_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

      tagSet.Set(TestTag2);

      W_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
      W_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
      W_TEST_INT(tagSet.GetNumTagsSet(), 1);

      tagSet.Set(*TestTag1);

      W_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
      W_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
      W_TEST_INT(tagSet.GetNumTagsSet(), 2);

      tagSet.Remove(*TestTag1);

      W_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
      W_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
      W_TEST_INT(tagSet.GetNumTagsSet(), 1);

      WTagSet tagSet2 = tagSet;
      W_TEST_BOOL(tagSet2.IsSet(*TestTag1) == false);
      W_TEST_BOOL(tagSet2.IsSet(TestTag2) == true);
      W_TEST_INT(tagSet2.GetNumTagsSet(), 1);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Many Tags")
  {
    WTagRegistry TempTestRegistry;

    // TagSets have local storage for 1 block (64 tags)
    // Allocate enough tags so the storage overflows (or doesn't start at block 0)
    // for these tests

    WTag RegisteredTags[250];

    // Pre register some tags
    TempTestRegistry.RegisterTag("TEST_TAG1");
    TempTestRegistry.RegisterTag("TEST_TAG2");

    for (WUInt32 i = 0; i < 250; ++i)
    {
      WStringBuilder TagName;
      TagName.SetFormat("TEST_TAG{0}", i);

      RegisteredTags[i] = TempTestRegistry.RegisterTag(TagName.GetData());

      W_TEST_BOOL(RegisteredTags[i].IsValid());
    }

    W_TEST_INT(TempTestRegistry.GetNumTags(), 250);

    // Set all tags
    WTagSet BigTagSet;

    BigTagSet.Set(RegisteredTags[128]);
    BigTagSet.Set(RegisteredTags[64]);
    BigTagSet.Set(RegisteredTags[0]);

    W_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[0]));
    W_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[64]));
    W_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[128]));

    for (WUInt32 i = 0; i < 250; ++i)
    {
      BigTagSet.Set(RegisteredTags[i]);
    }

    for (WUInt32 i = 0; i < 250; ++i)
    {
      W_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (WUInt32 i = 10; i < 60; ++i)
    {
      BigTagSet.Remove(RegisteredTags[i]);
    }

    for (WUInt32 i = 0; i < 10; ++i)
    {
      W_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (WUInt32 i = 10; i < 60; ++i)
    {
      W_TEST_BOOL(!BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (WUInt32 i = 60; i < 250; ++i)
    {
      W_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    // Set tags, but starting outside block 0. This should do no allocation
    WTagSet Non0BlockStartSet;
    Non0BlockStartSet.Set(RegisteredTags[100]);
    W_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[100]));
    W_TEST_BOOL(!Non0BlockStartSet.IsSet(RegisteredTags[0]));

    WTagSet Non0BlockStartSet2 = Non0BlockStartSet;
    W_TEST_BOOL(Non0BlockStartSet2.IsSet(RegisteredTags[100]));
    W_TEST_INT(Non0BlockStartSet2.GetNumTagsSet(), Non0BlockStartSet.GetNumTagsSet());

    // Also test allocating a tag in an earlier block than the first tag allocated in the set
    Non0BlockStartSet.Set(RegisteredTags[0]);
    W_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[100]));
    W_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[0]));

    // Copying a tag set should work as well
    WTagSet SecondTagSet = BigTagSet;

    for (WUInt32 i = 60; i < 250; ++i)
    {
      W_TEST_BOOL(SecondTagSet.IsSet(RegisteredTags[i]));
    }

    for (WUInt32 i = 10; i < 60; ++i)
    {
      W_TEST_BOOL(!SecondTagSet.IsSet(RegisteredTags[i]));
    }

    W_TEST_INT(SecondTagSet.GetNumTagsSet(), BigTagSet.GetNumTagsSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsAnySet")
  {
    WTagRegistry TempTestRegistry;

    // TagSets have local storage for 1 block (64 tags)
    // Allocate enough tags so the storage overflows (or doesn't start at block 0)
    // for these tests

    WTag RegisteredTags[250];

    for (WUInt32 i = 0; i < 250; ++i)
    {
      WStringBuilder TagName;
      TagName.SetFormat("TEST_TAG{0}", i);

      RegisteredTags[i] = TempTestRegistry.RegisterTag(TagName.GetData());

      W_TEST_BOOL(RegisteredTags[i].IsValid());
    }

    WTagSet EmptyTagSet;
    WTagSet SecondEmptyTagSet;

    W_TEST_BOOL(!EmptyTagSet.IsAnySet(SecondEmptyTagSet));
    W_TEST_BOOL(!SecondEmptyTagSet.IsAnySet(EmptyTagSet));


    WTagSet SimpleSingleTagBlock0;
    SimpleSingleTagBlock0.Set(RegisteredTags[0]);

    WTagSet SimpleSingleTagBlock1;
    SimpleSingleTagBlock1.Set(RegisteredTags[0]);

    W_TEST_BOOL(!SecondEmptyTagSet.IsAnySet(SimpleSingleTagBlock0));

    W_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock0));
    W_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock1.Remove(RegisteredTags[0]);
    W_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));

    // Try with different block sizes/offsets (but same bit index)
    SimpleSingleTagBlock1.Set(RegisteredTags[64]);

    W_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    W_TEST_BOOL(!SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock0.Set(RegisteredTags[65]);
    W_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    W_TEST_BOOL(!SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock0.Set(RegisteredTags[64]);
    W_TEST_BOOL(SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    W_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    WTagSet OffsetBlock;
    OffsetBlock.Set(RegisteredTags[65]);
    W_TEST_BOOL(OffsetBlock.IsAnySet(SimpleSingleTagBlock0));
    W_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(OffsetBlock));

    WTagSet OffsetBlock2;
    OffsetBlock2.Set(RegisteredTags[66]);
    W_TEST_BOOL(!OffsetBlock.IsAnySet(OffsetBlock2));
    W_TEST_BOOL(!OffsetBlock2.IsAnySet(OffsetBlock));

    OffsetBlock2.Set(RegisteredTags[65]);
    W_TEST_BOOL(OffsetBlock.IsAnySet(OffsetBlock2));
    W_TEST_BOOL(OffsetBlock2.IsAnySet(OffsetBlock));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Add / Remove / IsEmpty / Clear")
  {
    WTagRegistry TempTestRegistry;

    TempTestRegistry.RegisterTag("TEST_TAG1");

    const WTag* TestTag1 = TempTestRegistry.GetTagByName("TEST_TAG1");
    W_TEST_BOOL(TestTag1 != nullptr);

    const WTag& TestTag2 = TempTestRegistry.RegisterTag("TEST_TAG2");

    W_TEST_BOOL(TestTag2.IsValid());

    WTagSet tagSet;

    W_TEST_BOOL(tagSet.IsEmpty());
    W_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    W_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Clear();

    W_TEST_BOOL(tagSet.IsEmpty());
    W_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    W_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(TestTag2);

    W_TEST_BOOL(!tagSet.IsEmpty());
    W_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    W_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Remove(TestTag2);

    W_TEST_BOOL(tagSet.IsEmpty());
    W_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    W_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(*TestTag1);
    tagSet.Set(TestTag2);

    W_TEST_BOOL(!tagSet.IsEmpty());
    W_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
    W_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Remove(*TestTag1);
    tagSet.Remove(TestTag2);

    W_TEST_BOOL(tagSet.IsEmpty());
    W_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    W_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(*TestTag1);
    tagSet.Set(TestTag2);

    W_TEST_BOOL(!tagSet.IsEmpty());
    W_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
    W_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Clear();

    W_TEST_BOOL(tagSet.IsEmpty());
    W_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    W_TEST_BOOL(tagSet.IsSet(TestTag2) == false);
  }
}
