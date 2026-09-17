#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Types/RefCounted.h>

class RefCountedTestClass : public WRefCounted
{
public:
  WUInt32 m_uiDummyMember = 0x42u;
};

W_CREATE_SIMPLE_TEST(Basics, RefCounted)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Ref Counting")
  {
    RefCountedTestClass Instance;

    W_TEST_BOOL(Instance.GetRefCount() == 0);
    W_TEST_BOOL(!Instance.IsReferenced());

    Instance.AddRef();

    W_TEST_BOOL(Instance.GetRefCount() == 1);
    W_TEST_BOOL(Instance.IsReferenced());

    /// Test scoped ref pointer
    {
      WScopedRefPointer<RefCountedTestClass> ScopeTester(&Instance);

      W_TEST_BOOL(Instance.GetRefCount() == 2);
      W_TEST_BOOL(Instance.IsReferenced());
    }

    /// Test assignment of scoped ref pointer
    {
      WScopedRefPointer<RefCountedTestClass> ScopeTester;

      ScopeTester = &Instance;

      W_TEST_BOOL(Instance.GetRefCount() == 2);
      W_TEST_BOOL(Instance.IsReferenced());

      WScopedRefPointer<RefCountedTestClass> ScopeTester2;

      ScopeTester2 = ScopeTester;

      W_TEST_BOOL(Instance.GetRefCount() == 3);
      W_TEST_BOOL(Instance.IsReferenced());

      WScopedRefPointer<RefCountedTestClass> ScopeTester3(ScopeTester);

      W_TEST_BOOL(Instance.GetRefCount() == 4);
      W_TEST_BOOL(Instance.IsReferenced());
    }

    /// Test copy constructor for WRefCounted
    {
      RefCountedTestClass inst2(Instance);
      RefCountedTestClass inst3;
      inst3 = Instance;

      W_TEST_BOOL(Instance.GetRefCount() == 1);
      W_TEST_BOOL(Instance.IsReferenced());

      W_TEST_BOOL(inst2.GetRefCount() == 0);
      W_TEST_BOOL(!inst2.IsReferenced());

      W_TEST_BOOL(inst3.GetRefCount() == 0);
      W_TEST_BOOL(!inst3.IsReferenced());
    }

    W_TEST_BOOL(Instance.GetRefCount() == 1);
    W_TEST_BOOL(Instance.IsReferenced());

    Instance.ReleaseRef();

    W_TEST_BOOL(Instance.GetRefCount() == 0);
    W_TEST_BOOL(!Instance.IsReferenced());
  }
}
