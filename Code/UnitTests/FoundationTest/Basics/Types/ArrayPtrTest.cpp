#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>

template <typename T>
static void testArrayPtr(WArrayPtr<T> arrayPtr, typename WArrayPtr<T>::PointerType extectedPtr, WUInt32 uiExpectedCount)
{
  W_TEST_BOOL(arrayPtr.GetPtr() == extectedPtr);
  W_TEST_INT(arrayPtr.GetCount(), uiExpectedCount);
}

// static void TakeConstArrayPtr(WArrayPtr<const int> cint)
//{
//}
//
// static void TakeConstArrayPtr2(WArrayPtr<const int*> cint, WArrayPtr<const int* const> cintc)
//{
//}

W_CREATE_SIMPLE_TEST(Basics, ArrayPtr)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Empty Constructor")
  {
    WArrayPtr<WInt32> Empty;

    W_TEST_BOOL(Empty.GetPtr() == nullptr);
    W_TEST_BOOL(Empty.GetCount() == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WInt32 pIntData[] = {1, 2, 3, 4, 5};

    WArrayPtr<WInt32> ap(pIntData, 3);
    W_TEST_BOOL(ap.GetPtr() == pIntData);
    W_TEST_BOOL(ap.GetCount() == 3);

    WArrayPtr<WInt32> ap2(pIntData, 0u);
    W_TEST_BOOL(ap2.GetPtr() == nullptr);
    W_TEST_BOOL(ap2.GetCount() == 0);

    WArrayPtr<WInt32> ap3(pIntData);
    W_TEST_BOOL(ap3.GetPtr() == pIntData);
    W_TEST_BOOL(ap3.GetCount() == 5);

    WArrayPtr<WInt32> ap4(ap);
    W_TEST_BOOL(ap4.GetPtr() == pIntData);
    W_TEST_BOOL(ap4.GetCount() == 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WMakeArrayPtr")
  {
    WInt32 pIntData[] = {1, 2, 3, 4, 5};

    testArrayPtr(WMakeArrayPtr(pIntData, 3), pIntData, 3);
    testArrayPtr(WMakeArrayPtr(pIntData, 0), nullptr, 0);
    testArrayPtr(WMakeArrayPtr(pIntData), pIntData, 5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator=")
  {
    WInt32 pIntData[] = {1, 2, 3, 4, 5};

    WArrayPtr<WInt32> ap(pIntData, 3);
    W_TEST_BOOL(ap.GetPtr() == pIntData);
    W_TEST_BOOL(ap.GetCount() == 3);

    WArrayPtr<WInt32> ap2;
    ap2 = ap;

    W_TEST_BOOL(ap2.GetPtr() == pIntData);
    W_TEST_BOOL(ap2.GetCount() == 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear")
  {
    WInt32 pIntData[] = {1, 2, 3, 4, 5};

    WArrayPtr<WInt32> ap(pIntData, 3);
    W_TEST_BOOL(ap.GetPtr() == pIntData);
    W_TEST_BOOL(ap.GetCount() == 3);

    ap.Clear();

    W_TEST_BOOL(ap.GetPtr() == nullptr);
    W_TEST_BOOL(ap.GetCount() == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator== / operator!= / operator<")
  {
    WInt32 pIntData[] = {1, 2, 3, 4, 5};

    WArrayPtr<WInt32> ap1(pIntData, 3);
    WArrayPtr<WInt32> ap2(pIntData, 3);
    WArrayPtr<WInt32> ap3(pIntData, 4);
    WArrayPtr<WInt32> ap4(pIntData + 1, 3);

    W_TEST_BOOL(ap1 == ap2);
    W_TEST_BOOL(ap1 != ap3);
    W_TEST_BOOL(ap1 != ap4);

    W_TEST_BOOL(ap1 < ap3);
    WInt32 pIntData2[] = {1, 2, 4};
    WArrayPtr<WInt32> ap5(pIntData2, 3);
    W_TEST_BOOL(ap1 < ap5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator[]")
  {
    WInt32 pIntData[] = {1, 2, 3, 4, 5};

    WArrayPtr<WInt32> ap(pIntData + 1, 3);
    W_TEST_INT(ap[0], 2);
    W_TEST_INT(ap[1], 3);
    W_TEST_INT(ap[2], 4);
    ap[2] = 10;
    W_TEST_INT(ap[2], 10);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "const operator[]")
  {
    WInt32 pIntData[] = {1, 2, 3, 4, 5};

    const WArrayPtr<WInt32> ap(pIntData + 1, 3);
    W_TEST_INT(ap[0], 2);
    W_TEST_INT(ap[1], 3);
    W_TEST_INT(ap[2], 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CopyFrom")
  {
    WInt32 pIntData1[] = {1, 2, 3, 4, 5};
    WInt32 pIntData2[] = {6, 7, 8, 9, 0};

    WArrayPtr<WInt32> ap1(pIntData1 + 1, 3);
    WArrayPtr<WInt32> ap2(pIntData2 + 2, 3);

    ap1.CopyFrom(ap2);

    W_TEST_INT(pIntData1[0], 1);
    W_TEST_INT(pIntData1[1], 8);
    W_TEST_INT(pIntData1[2], 9);
    W_TEST_INT(pIntData1[3], 0);
    W_TEST_INT(pIntData1[4], 5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetSubArray")
  {
    WInt32 pIntData1[] = {1, 2, 3, 4, 5};

    WArrayPtr<WInt32> ap1(pIntData1, 5);
    WArrayPtr<WInt32> ap2 = ap1.GetSubArray(2, 3);

    W_TEST_BOOL(ap2.GetPtr() == &pIntData1[2]);
    W_TEST_BOOL(ap2.GetCount() == 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Const Conversions")
  {
    WInt32 pIntData1[] = {1, 2, 3, 4, 5};
    WArrayPtr<WInt32> ap1(pIntData1);
    WArrayPtr<const WInt32> ap2(ap1);
    WArrayPtr<const WInt32> ap3(pIntData1);
    ap2 = ap1; // non const to const assign
    ap3 = ap2; // const to const assign
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Empty Constructor (const)")
  {
    WArrayPtr<const WInt32> Empty;

    W_TEST_BOOL(Empty.GetPtr() == nullptr);
    W_TEST_BOOL(Empty.GetCount() == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor (const)")
  {
    const WInt32 pIntData[] = {1, 2, 3, 4, 5};

    WArrayPtr<const WInt32> ap(pIntData, 3);
    W_TEST_BOOL(ap.GetPtr() == pIntData);
    W_TEST_BOOL(ap.GetCount() == 3);

    WArrayPtr<const WInt32> ap2(pIntData, 0u);
    W_TEST_BOOL(ap2.GetPtr() == nullptr);
    W_TEST_BOOL(ap2.GetCount() == 0);

    WArrayPtr<const WInt32> ap3(pIntData);
    W_TEST_BOOL(ap3.GetPtr() == pIntData);
    W_TEST_BOOL(ap3.GetCount() == 5);

    WArrayPtr<const WInt32> ap4(ap);
    W_TEST_BOOL(ap4.GetPtr() == pIntData);
    W_TEST_BOOL(ap4.GetCount() == 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator=  (const)")
  {
    const WInt32 pIntData[] = {1, 2, 3, 4, 5};

    WArrayPtr<const WInt32> ap(pIntData, 3);
    W_TEST_BOOL(ap.GetPtr() == pIntData);
    W_TEST_BOOL(ap.GetCount() == 3);

    WArrayPtr<const WInt32> ap2;
    ap2 = ap;

    W_TEST_BOOL(ap2.GetPtr() == pIntData);
    W_TEST_BOOL(ap2.GetCount() == 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear (const)")
  {
    const WInt32 pIntData[] = {1, 2, 3, 4, 5};

    WArrayPtr<const WInt32> ap(pIntData, 3);
    W_TEST_BOOL(ap.GetPtr() == pIntData);
    W_TEST_BOOL(ap.GetCount() == 3);

    ap.Clear();

    W_TEST_BOOL(ap.GetPtr() == nullptr);
    W_TEST_BOOL(ap.GetCount() == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator== / operator!=  (const)")
  {
    WInt32 pIntData[] = {1, 2, 3, 4, 5};

    WArrayPtr<WInt32> ap1(pIntData, 3);
    WArrayPtr<const WInt32> ap2(pIntData, 3);
    WArrayPtr<const WInt32> ap3(pIntData, 4);
    WArrayPtr<const WInt32> ap4(pIntData + 1, 3);

    W_TEST_BOOL(ap1 == ap2);
    W_TEST_BOOL(ap3 != ap1);
    W_TEST_BOOL(ap1 != ap4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator[]  (const)")
  {
    const WInt32 pIntData[] = {1, 2, 3, 4, 5};

    WArrayPtr<const WInt32> ap(pIntData + 1, 3);
    W_TEST_INT(ap[0], 2);
    W_TEST_INT(ap[1], 3);
    W_TEST_INT(ap[2], 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "const operator[] (const)")
  {
    const WInt32 pIntData[] = {1, 2, 3, 4, 5};

    const WArrayPtr<const WInt32> ap(pIntData + 1, 3);
    W_TEST_INT(ap[0], 2);
    W_TEST_INT(ap[1], 3);
    W_TEST_INT(ap[2], 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetSubArray (const)")
  {
    const WInt32 pIntData1[] = {1, 2, 3, 4, 5};

    WArrayPtr<const WInt32> ap1(pIntData1, 5);
    WArrayPtr<const WInt32> ap2 = ap1.GetSubArray(2, 3);

    W_TEST_BOOL(ap2.GetPtr() == &pIntData1[2]);
    W_TEST_BOOL(ap2.GetCount() == 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "STL Iterator")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    WArrayPtr<WInt32> ptr1 = a1;

    // STL sort
    std::sort(begin(ptr1), end(ptr1));

    for (WInt32 i = 1; i < 1000; ++i)
    {
      W_TEST_BOOL(ptr1[i - 1] <= ptr1[i]);
    }

    // foreach
    WUInt32 prev = 0;
    for (WUInt32 val : ptr1)
    {
      W_TEST_BOOL(prev <= val);
      prev = val;
    }

    // const array
    const WDynamicArray<WInt32>& a2 = a1;

    const WArrayPtr<const WInt32> ptr2 = a2;

    // STL lower bound
    auto lb = std::lower_bound(begin(ptr2), end(ptr2), 400);
    W_TEST_BOOL(*lb == ptr2[400]);
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "STL Reverse Iterator")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    WArrayPtr<WInt32> ptr1 = a1;

    // STL sort
    std::sort(rbegin(ptr1), rend(ptr1));

    for (WInt32 i = 1; i < 1000; ++i)
    {
      W_TEST_BOOL(ptr1[i - 1] >= ptr1[i]);
    }

    // foreach
    WUInt32 prev = 1000;
    for (WUInt32 val : ptr1)
    {
      W_TEST_BOOL(prev >= val);
      prev = val;
    }

    // const array
    const WDynamicArray<WInt32>& a2 = a1;

    const WArrayPtr<const WInt32> ptr2 = a2;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(ptr2), rend(ptr2), 400);
    W_TEST_BOOL(*lb == ptr2[1000 - 400 - 1]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    WDynamicArray<WInt32> a0;
    WArrayPtr<WInt32> a1 = a0;

    for (WInt32 i = -100; i < 100; ++i)
      W_TEST_BOOL(!a1.Contains(i));

    for (WInt32 i = 0; i < 100; ++i)
      a0.PushBack(i);
    for (WInt32 i = 0; i < 100; ++i)
      a0.PushBack(i);

    a1 = a0;

    for (WInt32 i = 0; i < 100; ++i)
    {
      W_TEST_BOOL(a1.Contains(i));
      W_TEST_INT(a1.IndexOf(i), i);
      W_TEST_INT(a1.IndexOf(i, 100), i + 100);
      W_TEST_INT(a1.LastIndexOf(i), i + 100);
      W_TEST_INT(a1.LastIndexOf(i, 100), i);
    }
  }

  // "Implicit Conversions"
  //{
  //  {
  //    WHybridArray<int, 4> data;
  //    TakeConstArrayPtr(data);
  //    TakeConstArrayPtr(data.GetArrayPtr());
  //  }
  //  {
  //    WHybridArray<int*, 4> data;
  //    //TakeConstArrayPtr2(data, data); // does not compile
  //    TakeConstArrayPtr2(data.GetArrayPtr(), data.GetArrayPtr());
  //  }
  //}
}
