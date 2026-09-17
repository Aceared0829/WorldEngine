#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HybridArray.h>

static WInt32 iCallPodConstructor = 0;
static WInt32 iCallPodDestructor = 0;
static WInt32 iCallNonPodConstructor = 0;
static WInt32 iCallNonPodDestructor = 0;

struct WConstructTest
{
public:
  static WHybridArray<void*, 10> s_dtorList;

  WConstructTest() { m_iData = 42; }

  ~WConstructTest() { s_dtorList.PushBack(this); }

  WInt32 m_iData;
};
WHybridArray<void*, 10> WConstructTest::s_dtorList;

static_assert(sizeof(WConstructTest) == 4);


struct PODTest
{
  W_DECLARE_POD_TYPE();

  PODTest() { m_iData = -1; }

  WInt32 m_iData;
};

static const WUInt32 s_uiSize = sizeof(WConstructTest);

W_CREATE_SIMPLE_TEST(Memory, MemoryUtils)
{
  WConstructTest::s_dtorList.Clear();

  W_TEST_BLOCK(WTestBlock::Enabled, "Construct")
  {
    WUInt8 uiRawData[s_uiSize * 5] = {0};
    WConstructTest* pTest = (WConstructTest*)(uiRawData);

    WMemoryUtils::Construct<SkipTrivialTypes, WConstructTest>(pTest + 1, 2);

    W_TEST_INT(pTest[0].m_iData, 0);
    W_TEST_INT(pTest[1].m_iData, 42);
    W_TEST_INT(pTest[2].m_iData, 42);
    W_TEST_INT(pTest[3].m_iData, 0);
    W_TEST_INT(pTest[4].m_iData, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeConstructorFunction")
  {
    WMemoryUtils::ConstructorFunction func = WMemoryUtils::MakeConstructorFunction<SkipTrivialTypes, WConstructTest>();
    W_TEST_BOOL(func != nullptr);

    WUInt8 uiRawData[s_uiSize] = {0};
    WConstructTest* pTest = (WConstructTest*)(uiRawData);

    (*func)(pTest);

    W_TEST_INT(pTest->m_iData, 42);

    func = WMemoryUtils::MakeConstructorFunction<SkipTrivialTypes, PODTest>();
    W_TEST_BOOL(func != nullptr);

    func = WMemoryUtils::MakeConstructorFunction<SkipTrivialTypes, WInt32>();
    W_TEST_BOOL(func == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "DefaultConstruct")
  {
    WUInt32 uiRawData[5]; // not initialized here

    WMemoryUtils::Construct<ConstructAll>(uiRawData + 1, 2);

    W_TEST_INT(uiRawData[1], 0);
    W_TEST_INT(uiRawData[2], 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Construct Copy(Array)")
  {
    WUInt8 uiRawData[s_uiSize * 5] = {0};
    WConstructTest* pTest = (WConstructTest*)(uiRawData);

    WConstructTest copy[2];
    copy[0].m_iData = 43;
    copy[1].m_iData = 44;

    WMemoryUtils::CopyConstructArray<WConstructTest>(pTest + 1, copy, 2);

    W_TEST_INT(pTest[0].m_iData, 0);
    W_TEST_INT(pTest[1].m_iData, 43);
    W_TEST_INT(pTest[2].m_iData, 44);
    W_TEST_INT(pTest[3].m_iData, 0);
    W_TEST_INT(pTest[4].m_iData, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Construct Copy(Element)")
  {
    WUInt8 uiRawData[s_uiSize * 5] = {0};
    WConstructTest* pTest = (WConstructTest*)(uiRawData);

    WConstructTest copy;
    copy.m_iData = 43;

    WMemoryUtils::CopyConstruct<WConstructTest>(pTest + 1, copy, 2);

    W_TEST_INT(pTest[0].m_iData, 0);
    W_TEST_INT(pTest[1].m_iData, 43);
    W_TEST_INT(pTest[2].m_iData, 43);
    W_TEST_INT(pTest[3].m_iData, 0);
    W_TEST_INT(pTest[4].m_iData, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeCopyConstructorFunction")
  {
    WMemoryUtils::CopyConstructorFunction func = WMemoryUtils::MakeCopyConstructorFunction<WConstructTest>();
    W_TEST_BOOL(func != nullptr);

    WUInt8 uiRawData[s_uiSize] = {0};
    WConstructTest* pTest = (WConstructTest*)(uiRawData);

    WConstructTest copy;
    copy.m_iData = 43;

    (*func)(pTest, &copy);

    W_TEST_INT(pTest->m_iData, 43);

    func = WMemoryUtils::MakeCopyConstructorFunction<PODTest>();
    W_TEST_BOOL(func != nullptr);

    func = WMemoryUtils::MakeCopyConstructorFunction<WInt32>();
    W_TEST_BOOL(func != nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Destruct")
  {
    WUInt8 uiRawData[s_uiSize * 5] = {0};
    WConstructTest* pTest = (WConstructTest*)(uiRawData);

    WMemoryUtils::Construct<SkipTrivialTypes, WConstructTest>(pTest + 1, 2);

    W_TEST_INT(pTest[0].m_iData, 0);
    W_TEST_INT(pTest[1].m_iData, 42);
    W_TEST_INT(pTest[2].m_iData, 42);
    W_TEST_INT(pTest[3].m_iData, 0);
    W_TEST_INT(pTest[4].m_iData, 0);

    WConstructTest::s_dtorList.Clear();
    WMemoryUtils::Destruct<WConstructTest>(pTest, 4);
    W_TEST_INT(4, WConstructTest::s_dtorList.GetCount());

    if (WConstructTest::s_dtorList.GetCount() == 4)
    {
      W_TEST_BOOL(WConstructTest::s_dtorList[0] == &pTest[0]);
      W_TEST_BOOL(WConstructTest::s_dtorList[1] == &pTest[1]);
      W_TEST_BOOL(WConstructTest::s_dtorList[2] == &pTest[2]);
      W_TEST_BOOL(WConstructTest::s_dtorList[3] == &pTest[3]);
      W_TEST_INT(pTest[4].m_iData, 0);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeDestructorFunction")
  {
    WMemoryUtils::DestructorFunction func = WMemoryUtils::MakeDestructorFunction<WConstructTest>();
    W_TEST_BOOL(func != nullptr);

    WUInt8 uiRawData[s_uiSize] = {0};
    WConstructTest* pTest = (WConstructTest*)(uiRawData);

    WMemoryUtils::Construct<SkipTrivialTypes>(pTest, 1);
    W_TEST_INT(pTest->m_iData, 42);

    WConstructTest::s_dtorList.Clear();
    (*func)(pTest);
    W_TEST_INT(1, WConstructTest::s_dtorList.GetCount());

    if (WConstructTest::s_dtorList.GetCount() == 1)
    {
      W_TEST_BOOL(WConstructTest::s_dtorList[0] == pTest);
    }

    func = WMemoryUtils::MakeDestructorFunction<PODTest>();
    W_TEST_BOOL(func == nullptr);

    func = WMemoryUtils::MakeDestructorFunction<WInt32>();
    W_TEST_BOOL(func == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy")
  {
    WUInt8 uiRawData[5] = {1, 2, 3, 4, 5};
    WUInt8 uiRawData2[5] = {6, 7, 8, 9, 0};

    W_TEST_INT(uiRawData[0], 1);
    W_TEST_INT(uiRawData[1], 2);
    W_TEST_INT(uiRawData[2], 3);
    W_TEST_INT(uiRawData[3], 4);
    W_TEST_INT(uiRawData[4], 5);

    WMemoryUtils::Copy(uiRawData + 1, uiRawData2 + 2, 3);

    W_TEST_INT(uiRawData[0], 1);
    W_TEST_INT(uiRawData[1], 8);
    W_TEST_INT(uiRawData[2], 9);
    W_TEST_INT(uiRawData[3], 0);
    W_TEST_INT(uiRawData[4], 5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move")
  {
    WUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    W_TEST_INT(uiRawData[0], 1);
    W_TEST_INT(uiRawData[1], 2);
    W_TEST_INT(uiRawData[2], 3);
    W_TEST_INT(uiRawData[3], 4);
    W_TEST_INT(uiRawData[4], 5);

    WMemoryUtils::CopyOverlapped(uiRawData + 1, uiRawData + 3, 2);

    W_TEST_INT(uiRawData[0], 1);
    W_TEST_INT(uiRawData[1], 4);
    W_TEST_INT(uiRawData[2], 5);
    W_TEST_INT(uiRawData[3], 4);
    W_TEST_INT(uiRawData[4], 5);

    WMemoryUtils::CopyOverlapped(uiRawData + 1, uiRawData, 4);

    W_TEST_INT(uiRawData[0], 1);
    W_TEST_INT(uiRawData[1], 1);
    W_TEST_INT(uiRawData[2], 4);
    W_TEST_INT(uiRawData[3], 5);
    W_TEST_INT(uiRawData[4], 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    WUInt8 uiRawData1[5] = {1, 2, 3, 4, 5};
    WUInt8 uiRawData2[5] = {1, 2, 3, 4, 5};
    WUInt8 uiRawData3[5] = {1, 2, 3, 4, 6};

    W_TEST_BOOL(WMemoryUtils::IsEqual(uiRawData1, uiRawData2, 5));
    W_TEST_BOOL(!WMemoryUtils::IsEqual(uiRawData1, uiRawData3, 5));
    W_TEST_BOOL(WMemoryUtils::IsEqual(uiRawData1, uiRawData3, 4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ZeroFill")
  {
    WUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    W_TEST_INT(uiRawData[0], 1);
    W_TEST_INT(uiRawData[1], 2);
    W_TEST_INT(uiRawData[2], 3);
    W_TEST_INT(uiRawData[3], 4);
    W_TEST_INT(uiRawData[4], 5);

    // T*, size_t N overload
    WMemoryUtils::ZeroFill(uiRawData + 1, 3);

    W_TEST_INT(uiRawData[0], 1);
    W_TEST_INT(uiRawData[1], 0);
    W_TEST_INT(uiRawData[2], 0);
    W_TEST_INT(uiRawData[3], 0);
    W_TEST_INT(uiRawData[4], 5);

    // T[N] overload
    WMemoryUtils::ZeroFillArray(uiRawData);

    W_TEST_INT(uiRawData[0], 0);
    W_TEST_INT(uiRawData[1], 0);
    W_TEST_INT(uiRawData[2], 0);
    W_TEST_INT(uiRawData[3], 0);
    W_TEST_INT(uiRawData[4], 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PatternFill")
  {
    WUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    W_TEST_INT(uiRawData[0], 1);
    W_TEST_INT(uiRawData[1], 2);
    W_TEST_INT(uiRawData[2], 3);
    W_TEST_INT(uiRawData[3], 4);
    W_TEST_INT(uiRawData[4], 5);

    // T*, size_t N overload
    WMemoryUtils::PatternFill(uiRawData + 1, 0xAB, 3);

    W_TEST_INT(uiRawData[0], 1);
    W_TEST_INT(uiRawData[1], 0xAB);
    W_TEST_INT(uiRawData[2], 0xAB);
    W_TEST_INT(uiRawData[3], 0xAB);
    W_TEST_INT(uiRawData[4], 5);

    // T[N] overload
    WMemoryUtils::PatternFillArray(uiRawData, 0xCD);

    W_TEST_INT(uiRawData[0], 0xCD);
    W_TEST_INT(uiRawData[1], 0xCD);
    W_TEST_INT(uiRawData[2], 0xCD);
    W_TEST_INT(uiRawData[3], 0xCD);
    W_TEST_INT(uiRawData[4], 0xCD);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compare")
  {
    WUInt32 uiRawDataA[3] = {1, 2, 3};
    WUInt32 uiRawDataB[3] = {3, 4, 5};

    W_TEST_INT(uiRawDataA[0], 1);
    W_TEST_INT(uiRawDataA[1], 2);
    W_TEST_INT(uiRawDataA[2], 3);
    W_TEST_INT(uiRawDataB[0], 3);
    W_TEST_INT(uiRawDataB[1], 4);
    W_TEST_INT(uiRawDataB[2], 5);

    W_TEST_BOOL(WMemoryUtils::Compare(uiRawDataA, uiRawDataB, 3) < 0);
    W_TEST_BOOL(WMemoryUtils::Compare(uiRawDataA + 2, uiRawDataB, 1) == 0);
    W_TEST_BOOL(WMemoryUtils::Compare(uiRawDataB, uiRawDataA, 3) > 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddByteOffset")
  {
    WInt32* pData1 = nullptr;
    pData1 = WMemoryUtils::AddByteOffset(pData1, 13);
    W_TEST_BOOL(pData1 == reinterpret_cast<WInt32*>(13));

    const WInt32* pData2 = nullptr;
    const WInt32* pData3 = WMemoryUtils::AddByteOffset(pData2, 17);
    W_TEST_BOOL(pData3 == reinterpret_cast<WInt32*>(17));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Align / IsAligned")
  {
    {
      WInt32* pData = (WInt32*)1;
      W_TEST_BOOL(!WMemoryUtils::IsAligned(pData, 4));
      pData = WMemoryUtils::AlignBackwards(pData, 4);
      W_TEST_BOOL(pData == reinterpret_cast<WInt32*>(0));
      W_TEST_BOOL(WMemoryUtils::IsAligned(pData, 4));
    }
    {
      WInt32* pData = (WInt32*)2;
      W_TEST_BOOL(!WMemoryUtils::IsAligned(pData, 4));
      pData = WMemoryUtils::AlignBackwards(pData, 4);
      W_TEST_BOOL(pData == reinterpret_cast<WInt32*>(0));
      W_TEST_BOOL(WMemoryUtils::IsAligned(pData, 4));
    }
    {
      WInt32* pData = (WInt32*)3;
      W_TEST_BOOL(!WMemoryUtils::IsAligned(pData, 4));
      pData = WMemoryUtils::AlignBackwards(pData, 4);
      W_TEST_BOOL(pData == reinterpret_cast<WInt32*>(0));
      W_TEST_BOOL(WMemoryUtils::IsAligned(pData, 4));
    }
    {
      WInt32* pData = (WInt32*)4;
      W_TEST_BOOL(WMemoryUtils::IsAligned(pData, 4));
      pData = WMemoryUtils::AlignBackwards(pData, 4);
      W_TEST_BOOL(pData == reinterpret_cast<WInt32*>(4));
      W_TEST_BOOL(WMemoryUtils::IsAligned(pData, 4));
    }

    {
      WInt32* pData = (WInt32*)1;
      W_TEST_BOOL(!WMemoryUtils::IsAligned(pData, 4));
      pData = WMemoryUtils::AlignForwards(pData, 4);
      W_TEST_BOOL(pData == reinterpret_cast<WInt32*>(4));
      W_TEST_BOOL(WMemoryUtils::IsAligned(pData, 4));
    }
    {
      WInt32* pData = (WInt32*)2;
      W_TEST_BOOL(!WMemoryUtils::IsAligned(pData, 4));
      pData = WMemoryUtils::AlignForwards(pData, 4);
      W_TEST_BOOL(pData == reinterpret_cast<WInt32*>(4));
      W_TEST_BOOL(WMemoryUtils::IsAligned(pData, 4));
    }
    {
      WInt32* pData = (WInt32*)3;
      W_TEST_BOOL(!WMemoryUtils::IsAligned(pData, 4));
      pData = WMemoryUtils::AlignForwards(pData, 4);
      W_TEST_BOOL(pData == reinterpret_cast<WInt32*>(4));
      W_TEST_BOOL(WMemoryUtils::IsAligned(pData, 4));
    }
    {
      WInt32* pData = (WInt32*)4;
      W_TEST_BOOL(WMemoryUtils::IsAligned(pData, 4));
      pData = WMemoryUtils::AlignForwards(pData, 4);
      W_TEST_BOOL(pData == reinterpret_cast<WInt32*>(4));
      W_TEST_BOOL(WMemoryUtils::IsAligned(pData, 4));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "POD")
  {
    struct Trivial
    {
      W_DECLARE_POD_TYPE();

      ~Trivial() = default;

      WUInt32 a;
      WUInt32 b;
    };

    static_assert(std::is_trivial<Trivial>::value != 0);
    static_assert(WIsPodType<Trivial>::value == 1);
    static_assert(std::is_trivially_destructible<Trivial>::value != 0);

    struct POD
    {
      W_DECLARE_POD_TYPE();

      WUInt32 a = 2;
      WUInt32 b = 4;

      POD()
      {
        iCallPodConstructor++;
      }

      // this isn't allowed anymore in types that use W_DECLARE_POD_TYPE
      // unfortunately that means we can't do this kind of check either
      //~POD()
      //{
      //  iCallPodDestructor++;
      //}
    };

    static_assert(std::is_trivial<POD>::value == 0);
    static_assert(WIsPodType<POD>::value == 1);

    struct NonPOD
    {
      WUInt32 a = 3;
      WUInt32 b = 5;

      NonPOD()
      {
        iCallNonPodConstructor++;
      }

      ~NonPOD()
      {
        iCallNonPodDestructor++;
      }
    };

    static_assert(std::is_trivial<NonPOD>::value == 0);
    static_assert(WIsPodType<NonPOD>::value == 0);

    struct NonPOD2
    {
      WUInt32 a;
      WUInt32 b;

      ~NonPOD2()
      {
        iCallNonPodDestructor++;
      }
    };

    static_assert(std::is_trivial<NonPOD2>::value == 0); // destructor makes it non-trivial
    static_assert(WIsPodType<NonPOD2>::value == 0);
    static_assert(std::is_trivially_destructible<NonPOD2>::value == 0);

    // check that WMemoryUtils::Construct and WMemoryUtils::Destruct ignore POD types
    {
      WUInt8 mem[sizeof(POD) * 2];

      W_TEST_INT(iCallPodConstructor, 0);
      W_TEST_INT(iCallPodDestructor, 0);

      WMemoryUtils::Construct<SkipTrivialTypes, POD>((POD*)mem, 1);

      W_TEST_INT(iCallPodConstructor, 1);
      W_TEST_INT(iCallPodDestructor, 0);

      WMemoryUtils::Destruct<POD>((POD*)mem, 1);
      W_TEST_INT(iCallPodConstructor, 1);
      W_TEST_INT(iCallPodDestructor, 0);

      iCallPodConstructor = 0;
    }

    // check that WMemoryUtils::Destruct calls the destructor of a non-trivial type
    {
      WUInt8 mem[sizeof(NonPOD2) * 2];

      W_TEST_INT(iCallNonPodDestructor, 0);
      WMemoryUtils::Destruct<NonPOD2>((NonPOD2*)mem, 1);

      W_TEST_INT(iCallNonPodDestructor, 1);

      iCallNonPodDestructor = 0;
    }

    {
      // make sure WMemoryUtils::Construct and WMemoryUtils::Destruct don't touch built-in types

      WInt32 a = 42;
      WMemoryUtils::Construct<SkipTrivialTypes, WInt32>(&a, 1);
      W_TEST_INT(a, 42);
      WMemoryUtils::Destruct<WInt32>(&a, 1);
      W_TEST_INT(a, 42);
    }
  }
}
