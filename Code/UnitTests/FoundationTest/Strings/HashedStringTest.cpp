#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Strings/HashedString.h>

W_CREATE_SIMPLE_TEST(Strings, HashedString)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WHashedString s;
    WHashedString s2;

    s2.Assign("test"); // compile time hashing

    W_TEST_INT(s.GetHash(), 0xef46db3751d8e999llu);
    W_TEST_STRING(s.GetString().GetData(), "");
    W_TEST_BOOL(s.GetString().IsEmpty());

    WTempHashedString ts("test"); // compile time hashing
    W_TEST_INT(ts.GetHash(), 0x4fdcca5ddb678139llu);

    WStringBuilder sb = "test2";
    WTempHashedString ts2(sb.GetData()); // runtime hashing
    W_TEST_INT(ts2.GetHash(), 0x890e0a4c7111eb87llu);

    WTempHashedString ts3(s2);
    W_TEST_INT(ts3.GetHash(), 0x4fdcca5ddb678139llu);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Assign")
  {
    WHashedString s;
    s.Assign("Test"); // compile time hashing

    W_TEST_STRING(s.GetString().GetData(), "Test");
    W_TEST_INT(s.GetHash(), 0xda83efc38a8922b4llu);

    WStringBuilder sb = "test2";
    s.Assign(sb.GetData()); // runtime hashing
    W_TEST_STRING(s.GetString().GetData(), "test2");
    W_TEST_INT(s.GetHash(), 0x890e0a4c7111eb87llu);

    WTempHashedString ts("dummy");
    ts = "test";       // compile time hashing
    W_TEST_INT(ts.GetHash(), 0x4fdcca5ddb678139llu);

    ts = sb.GetData(); // runtime hashing
    W_TEST_INT(ts.GetHash(), 0x890e0a4c7111eb87llu);

    s.Assign("");
    W_TEST_INT(s.GetHash(), 0xef46db3751d8e999llu);
    W_TEST_STRING(s.GetString().GetData(), "");
    W_TEST_BOOL(s.GetString().IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TempHashedString")
  {
    WTempHashedString ts;
    WHashedString hs;

    W_TEST_INT(ts.GetHash(), hs.GetHash());

    W_TEST_INT(ts.GetHash(), 0xef46db3751d8e999llu);

    ts = "Test";
    WTempHashedString ts2 = ts;
    W_TEST_INT(ts.GetHash(), 0xda83efc38a8922b4llu);

    ts = "";
    ts2.Clear();
    W_TEST_INT(ts.GetHash(), 0xef46db3751d8e999llu);
    W_TEST_INT(ts2.GetHash(), 0xef46db3751d8e999llu);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator== / operator!=")
  {
    WHashedString s1, s2, s3, s4;
    s1.Assign("Test1");
    s2.Assign("Test2");
    s3.Assign("Test1");
    s4.Assign("Test2");

    WTempHashedString t1("Test1");
    WTempHashedString t2("Test2");

    W_TEST_STRING(s1.GetString().GetData(), "Test1");
    W_TEST_STRING(s2.GetString().GetData(), "Test2");
    W_TEST_STRING(s3.GetString().GetData(), "Test1");
    W_TEST_STRING(s4.GetString().GetData(), "Test2");

    W_TEST_BOOL(s1 == s1);
    W_TEST_BOOL(s2 == s2);
    W_TEST_BOOL(s3 == s3);
    W_TEST_BOOL(s4 == s4);
    W_TEST_BOOL(t1 == t1);
    W_TEST_BOOL(t2 == t2);

    W_TEST_BOOL(s1 != s2);
    W_TEST_BOOL(s1 == s3);
    W_TEST_BOOL(s1 != s4);
    W_TEST_BOOL(s1 == t1);
    W_TEST_BOOL(s1 != t2);

    W_TEST_BOOL(s2 != s3);
    W_TEST_BOOL(s2 == s4);
    W_TEST_BOOL(s2 != t1);
    W_TEST_BOOL(s2 == t2);

    W_TEST_BOOL(s3 != s4);
    W_TEST_BOOL(s3 == t1);
    W_TEST_BOOL(s3 != t2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copying")
  {
    WHashedString s1;
    s1.Assign("blaa");

    WHashedString s2(s1);
    WHashedString s3;
    s3 = s2;

    W_TEST_BOOL(s1 == s2);
    W_TEST_BOOL(s1 == s3);

    WHashedString s4(std::move(s2));
    WHashedString s5;
    s5 = std::move(s3);

    W_TEST_BOOL(s1 == s4);
    W_TEST_BOOL(s1 == s5);
    W_TEST_BOOL(s1 != s2);
    W_TEST_BOOL(s1 != s3);

    WTempHashedString t1("blaa");

    WTempHashedString t2(t1);
    WTempHashedString t3("urg");
    t3 = t2;

    W_TEST_BOOL(t1 == t2);
    W_TEST_BOOL(t1 == t3);

    t3 = s1;
    W_TEST_INT(t3.GetHash(), s1.GetHash());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator<")
  {
    WHashedString s1, s2, s3;
    s1.Assign("blaa");
    s2.Assign("blub");
    s3.Assign("tut");

    WMap<WHashedString, WInt32> m; // uses operator< internally
    m[s1] = 1;
    m[s2] = 2;
    m[s3] = 3;

    W_TEST_INT(m[s1], 1);
    W_TEST_INT(m[s2], 2);
    W_TEST_INT(m[s3], 3);

    WTempHashedString t1("blaa");
    WTempHashedString t2("blub");
    WTempHashedString t3("tut");

    W_TEST_BOOL((s1 < s1) == (t1 < t1));
    W_TEST_BOOL((s1 < s2) == (t1 < t2));
    W_TEST_BOOL((s1 < s3) == (t1 < t3));

    W_TEST_BOOL((s1 < s1) == (s1 < t1));
    W_TEST_BOOL((s1 < s2) == (s1 < t2));
    W_TEST_BOOL((s1 < s3) == (s1 < t3));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetString")
  {
    WHashedString s1, s2, s3;
    s1.Assign("blaa");
    s2.Assign("blub");
    s3.Assign("tut");

    W_TEST_STRING(s1.GetString().GetData(), "blaa");
    W_TEST_STRING(s2.GetString().GetData(), "blub");
    W_TEST_STRING(s3.GetString().GetData(), "tut");
  }

#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
  W_TEST_BLOCK(WTestBlock::Enabled, "ClearUnusedStrings")
  {
    WHashedString::ClearUnusedStrings();

    {
      WHashedString s1, s2, s3;
      s1.Assign("blaa");
      s2.Assign("blub");
      s3.Assign("tut");
    }

    W_TEST_INT(WHashedString::ClearUnusedStrings(), 3);
    W_TEST_INT(WHashedString::ClearUnusedStrings(), 0);
  }
#endif
}
