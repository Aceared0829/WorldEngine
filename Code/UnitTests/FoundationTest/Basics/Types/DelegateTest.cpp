#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

namespace
{
  WAllocator* g_pTestAllocator;

  struct WTestAllocatorWrapper
  {
    static WAllocator* GetAllocator() { return g_pTestAllocator; }
  };

  struct TestType
  {
    TestType(){}; // NOLINT: Allow default construction

    WInt32 MethodWithManyParams(WInt32 a, WInt32 b, WInt32 c, WInt32 d, WInt32 e, WInt32 f) { return m_iA + a + b + c + d + e + f; }

    WInt32 Method(WInt32 b) { return b + m_iA; }

    WInt32 ConstMethod(WInt32 b) const { return b + m_iA + 4; }

    virtual WInt32 VirtualMethod(WInt32 b) { return b; }

    mutable WInt32 m_iA;
  };

  struct TestTypeDerived : public TestType
  {
    WInt32 Method(WInt32 b) { return b + 4; }

    virtual WInt32 VirtualMethod(WInt32 b) override { return b + 43; }
  };

  struct BaseA
  {
    virtual ~BaseA() = default;
    virtual void bar() {}

    int m_i1;
  };

  struct BaseB
  {
    virtual ~BaseB() = default;
    virtual void foo() {}
    int m_i2;
  };

  struct ComplexClass : public BaseA, public BaseB
  {
    ComplexClass() { m_ctorDel = WMakeDelegate(&ComplexClass::nonVirtualFunc, this); }

    virtual ~ComplexClass()
    {
      m_dtorDel = WMakeDelegate(&ComplexClass::nonVirtualFunc, this);
      W_TEST_BOOL(m_ctorDel.IsEqualIfComparable(m_dtorDel));
    }
    virtual void bar() override {}
    virtual void foo() override {}



    void nonVirtualFunc()
    {
      m_i1 = 1;
      m_i2 = 2;
      m_i3 = 3;
    }

    int m_i3;

    WDelegate<void()> m_ctorDel;
    WDelegate<void()> m_dtorDel;
  };

  static WInt32 Function(WInt32 b)
  {
    return b + 2;
  }
} // namespace

W_CREATE_SIMPLE_TEST(Basics, Delegate)
{
  using TestDelegate = WDelegate<WInt32(WInt32)>;
  TestDelegate d;

#if W_ENABLED(W_PLATFORM_64BIT)
  W_TEST_BOOL(sizeof(d) == 32);
#endif

  W_TEST_BLOCK(WTestBlock::Enabled, "Method")
  {
    TestTypeDerived test;
    test.m_iA = 42;

    d = TestDelegate(&TestType::Method, &test);
    W_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::Method, &test)));
    W_TEST_BOOL(d.IsComparable());
    W_TEST_INT(d(4), 46);

    d = TestDelegate(&TestTypeDerived::Method, &test);
    W_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestTypeDerived::Method, &test)));
    W_TEST_BOOL(d.IsComparable());
    W_TEST_INT(d(4), 8);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Method With Many Params")
  {
    using TestDelegateMany = WDelegate<WInt32(WInt32, WInt32, WInt32, WInt32, WInt32, WInt32)>;
    TestDelegateMany many;

    TestType test;
    test.m_iA = 1000000;

    many = TestDelegateMany(&TestType::MethodWithManyParams, &test);
    W_TEST_BOOL(many.IsEqualIfComparable(TestDelegateMany(&TestType::MethodWithManyParams, &test)));
    W_TEST_BOOL(d.IsComparable());
    W_TEST_INT(many(1, 10, 100, 1000, 10000, 100000), 1111111);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Complex Class")
  {
    W_WARNING_PUSH()
    W_WARNING_DISABLE_GCC("-Wfree-nonheap-object")

    ComplexClass* c = new ComplexClass();
    delete c;

    W_WARNING_POP()
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Const Method")
  {
    const TestType constTest;
    constTest.m_iA = 35;

    d = TestDelegate(&TestType::ConstMethod, &constTest);
    W_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::ConstMethod, &constTest)));
    W_TEST_BOOL(d.IsComparable());
    W_TEST_INT(d(4), 43);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Virtual Method")
  {
    TestTypeDerived test;

    d = TestDelegate(&TestType::VirtualMethod, &test);
    W_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::VirtualMethod, &test)));
    W_TEST_BOOL(d.IsComparable());
    W_TEST_INT(d(4), 47);

    d = TestDelegate(&TestTypeDerived::VirtualMethod, &test);
    W_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestTypeDerived::VirtualMethod, &test)));
    W_TEST_BOOL(d.IsComparable());
    W_TEST_INT(d(4), 47);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Function")
  {
    d = &Function;
    W_TEST_BOOL(d.IsEqualIfComparable(&Function));
    W_TEST_BOOL(d.IsComparable());
    W_TEST_INT(d(4), 6);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lambda - no capture")
  {
    d = [](WInt32 i)
    { return i * 4; };
    W_TEST_BOOL(d.IsComparable());
    W_TEST_INT(d(2), 8);

    TestDelegate d2 = d;
    W_TEST_BOOL(d2.IsEqualIfComparable(d));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lambda - capture by value")
  {
    WInt32 c = 20;
    d = [c](WInt32)
    { return c; };
    W_TEST_BOOL(!d.IsComparable());
    W_TEST_INT(d(3), 20);
    c = 10;
    W_TEST_INT(d(3), 20);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lambda - capture by value, mutable")
  {
    WInt32 c = 20;
    d = [c](WInt32) mutable
    { return c; };
    W_TEST_BOOL(!d.IsComparable());
    W_TEST_INT(d(3), 20);
    c = 10;
    W_TEST_INT(d(3), 20);

    d = [c](WInt32 b) mutable -> decltype(b + c)
    {
      auto result = b + c;
      c = 1;
      return result;
    };
    W_TEST_BOOL(!d.IsComparable());
    W_TEST_INT(d(3), 13);
    W_TEST_INT(d(3), 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lambda - capture by reference")
  {
    WInt32 c = 20;
    d = [&c](WInt32 i) -> decltype(i)
    {
      c = 5;
      return i;
    };
    W_TEST_BOOL(!d.IsComparable());
    W_TEST_INT(d(3), 3);
    W_TEST_INT(c, 5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lambda - capture by value of non-pod")
  {
    struct RefCountedInt : public WRefCounted
    {
      RefCountedInt() = default;
      RefCountedInt(int i)
        : m_value(i)
      {
      }
      int m_value;
    };

    WSharedPtr<RefCountedInt> shared = W_DEFAULT_NEW(RefCountedInt, 1);
    W_TEST_INT(shared->GetRefCount(), 1);
    {
      TestDelegate deleteMe = [shared](WInt32 i) -> decltype(i)
      { return 0; };
      W_TEST_BOOL(!deleteMe.IsComparable());
      W_TEST_INT(shared->GetRefCount(), 2);
    }
    W_TEST_INT(shared->GetRefCount(), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lambda - capture lots of things")
  {
    WInt64 a = 10;
    WInt64 b = 20;
    WInt64 c = 30;
    d = [a, b, c](WInt32 i) -> WInt32
    { return static_cast<WInt32>(a + b + c + i); };
    W_TEST_INT(d(6), 66);
    W_TEST_BOOL(!d.IsComparable());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lambda - capture lots of things - custom allocator")
  {
    WInt64 a = 10;
    WInt64 b = 20;
    WInt64 c = 30;
    d = TestDelegate([a, b, c](WInt32 i) -> WInt32
      { return static_cast<WInt32>(a + b + c + i); }, WFoundation::GetAlignedAllocator());
    W_TEST_INT(d(6), 66);
    W_TEST_BOOL(!d.IsComparable());

    d.Invalidate();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lambda - capture lots of things - allocator wrapper")
  {
    WProxyAllocator proxy("DelegateTestAllocator", WFoundation::GetDefaultAllocator());
    g_pTestAllocator = &proxy;

    WInt64 a = 10;
    WInt64 b = 20;
    WInt64 c = 30;

    using TestDelegateWithAllocator = WDelegate<WInt32(WInt32), 16, WTestAllocatorWrapper>;
    TestDelegateWithAllocator d2 = [a, b, c](WInt32 i) -> WInt32
    { return static_cast<WInt32>(a + b + c + i); };

    W_TEST_INT(d2(6), 66);
    W_TEST_BOOL(!d2.IsComparable());
    W_TEST_BOOL(proxy.GetStats().m_uiNumAllocations > 0);

    d2.Invalidate();
    g_pTestAllocator = nullptr;
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move semantics")
  {
    // Move pure function
    {
      d.Invalidate();
      TestDelegate d2 = &Function;
      d = std::move(d2);
      W_TEST_BOOL(d.IsValid());
      W_TEST_BOOL(!d2.IsValid());
      W_TEST_BOOL(d.IsComparable());
      W_TEST_INT(d(4), 6);
    }

    // Move delegate
    WConstructionCounter::Reset();
    d.Invalidate();
    {
      WConstructionCounter value;
      value.m_iData = 666;
      W_TEST_INT(WConstructionCounter::s_iConstructions, 1);
      W_TEST_INT(WConstructionCounter::s_iDestructions, 0);
      TestDelegate d2 = [value](WInt32 i) -> WInt32
      { return value.m_iData; };
      W_TEST_INT(WConstructionCounter::s_iConstructions, 3); // Capture plus moving the lambda.
      W_TEST_INT(WConstructionCounter::s_iDestructions, 1);  // Move of lambda
      d = std::move(d2);
      // Moving a construction counter also counts as construction
      W_TEST_INT(WConstructionCounter::s_iConstructions, 4);
      W_TEST_INT(WConstructionCounter::s_iDestructions, 1);
      W_TEST_BOOL(d.IsValid());
      W_TEST_BOOL(!d2.IsValid());
      W_TEST_BOOL(!d.IsComparable());
      W_TEST_INT(d(0), 666);
    }
    W_TEST_INT(WConstructionCounter::s_iDestructions, 2); // value out of scope
    W_TEST_INT(WConstructionCounter::s_iConstructions, 4);
    d.Invalidate();
    W_TEST_INT(WConstructionCounter::s_iDestructions, 3); // lambda destroyed.
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lambda - Copy")
  {
    d.Invalidate();
    WConstructionCounter::Reset();
    {
      WConstructionCounter value;
      value.m_iData = 666;
      W_TEST_INT(WConstructionCounter::s_iConstructions, 1);
      W_TEST_INT(WConstructionCounter::s_iDestructions, 0);
      TestDelegate d2 = TestDelegate([value](WInt32 i) -> WInt32
        { return value.m_iData; }, WFoundation::GetAlignedAllocator());
      W_TEST_INT(WConstructionCounter::s_iConstructions, 3); // Capture plus moving the lambda.
      W_TEST_INT(WConstructionCounter::s_iDestructions, 1);  // Move of lambda
      d = d2;
      W_TEST_INT(WConstructionCounter::s_iConstructions, 4); // Lambda Copy
      W_TEST_INT(WConstructionCounter::s_iDestructions, 1);
      W_TEST_BOOL(d.IsValid());
      W_TEST_BOOL(d2.IsValid());
      W_TEST_BOOL(!d.IsComparable());
      W_TEST_BOOL(!d2.IsComparable());
      W_TEST_INT(d(0), 666);
      W_TEST_INT(d2(0), 666);
    }
    W_TEST_INT(WConstructionCounter::s_iDestructions, 3); // value and lambda out of scope
    W_TEST_INT(WConstructionCounter::s_iConstructions, 4);
    d.Invalidate();
    W_TEST_INT(WConstructionCounter::s_iDestructions, 4); // lambda destroyed.
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lambda - capture non-copyable type")
  {
    WUniquePtr<WConstructionCounter> data(W_DEFAULT_NEW(WConstructionCounter));
    data->m_iData = 666;
    TestDelegate d2 = [data = std::move(data)](WInt32 i) -> WInt32
    { return data->m_iData; };
    W_TEST_INT(d2(0), 666);
    d = std::move(d2);
    W_TEST_BOOL(d.IsValid());
    W_TEST_BOOL(!d2.IsValid());
    W_TEST_INT(d(0), 666);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WMakeDelegate")
  {
    auto d1 = WMakeDelegate(&Function);
    W_TEST_BOOL(d1.IsEqualIfComparable(WMakeDelegate(&Function)));

    TestType instance;
    auto d2 = WMakeDelegate(&TestType::Method, &instance);
    W_TEST_BOOL(d2.IsEqualIfComparable(WMakeDelegate(&TestType::Method, &instance)));
    auto d3 = WMakeDelegate(&TestType::ConstMethod, &instance);
    W_TEST_BOOL(d3.IsEqualIfComparable(WMakeDelegate(&TestType::ConstMethod, &instance)));
    auto d4 = WMakeDelegate(&TestType::VirtualMethod, &instance);
    W_TEST_BOOL(d4.IsEqualIfComparable(WMakeDelegate(&TestType::VirtualMethod, &instance)));

    TestType instance2;
    auto d2_2 = WMakeDelegate(&TestType::Method, &instance2);
    W_TEST_BOOL(!d2_2.IsEqualIfComparable(d2));

    W_IGNORE_UNUSED(d1);
    W_IGNORE_UNUSED(d2);
    W_IGNORE_UNUSED(d2_2);
    W_IGNORE_UNUSED(d3);
    W_IGNORE_UNUSED(d4);
  }
}
