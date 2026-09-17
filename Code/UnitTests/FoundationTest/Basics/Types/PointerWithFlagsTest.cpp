#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Types/PointerWithFlags.h>

W_CREATE_SIMPLE_TEST(Basics, PointerWithFlags)
{
  struct Dummy
  {
    float a = 3.0f;
    int b = 7;
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "General")
  {
    WPointerWithFlags<Dummy, 2> ptr;

    W_TEST_INT(ptr.GetFlags(), 0);
    ptr.SetFlags(3);
    W_TEST_INT(ptr.GetFlags(), 3);

    W_TEST_BOOL(ptr == nullptr);
    W_TEST_BOOL(!ptr);

    W_TEST_INT(ptr.GetFlags(), 3);
    ptr.SetFlags(2);
    W_TEST_INT(ptr.GetFlags(), 2);

    Dummy d1, d2;
    ptr = &d1;
    d2.a = 4;
    d2.b = 8;

    W_TEST_BOOL(ptr.GetPtr() == &d1);
    W_TEST_BOOL(ptr.GetPtr() != &d2);

    W_TEST_INT(ptr.GetFlags(), 2);
    ptr.SetFlags(1);
    W_TEST_INT(ptr.GetFlags(), 1);

    W_TEST_BOOL(ptr == &d1);
    W_TEST_BOOL(ptr != &d2);
    W_TEST_BOOL(ptr);


    W_TEST_FLOAT(ptr->a, 3.0f, 0.0f);
    W_TEST_INT(ptr->b, 7);

    ptr = &d2;

    W_TEST_INT(ptr.GetFlags(), 1);
    ptr.SetFlags(3);
    W_TEST_INT(ptr.GetFlags(), 3);

    W_TEST_BOOL(ptr != &d1);
    W_TEST_BOOL(ptr == &d2);
    W_TEST_BOOL(ptr);

    ptr = nullptr;
    W_TEST_BOOL(!ptr);
    W_TEST_BOOL(ptr == nullptr);

    W_TEST_INT(ptr.GetFlags(), 3);
    ptr.SetFlags(0);
    W_TEST_INT(ptr.GetFlags(), 0);

    WPointerWithFlags<Dummy, 2> ptr2 = ptr;
    W_TEST_BOOL(ptr == ptr2);

    W_TEST_BOOL(ptr2.GetPtr() == ptr.GetPtr());
    W_TEST_BOOL(ptr2.GetFlags() == ptr.GetFlags());

    ptr2.SetFlags(3);
    W_TEST_BOOL(ptr2.GetPtr() == ptr.GetPtr());
    W_TEST_BOOL(ptr2.GetFlags() != ptr.GetFlags());

    // the two Ptrs still compare equal (pointer part is equal, even if flags are different)
    W_TEST_BOOL(ptr == ptr2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Const ptr")
  {
    WPointerWithFlags<const Dummy, 2> ptr;

    Dummy d1, d2;
    ptr = &d1;

    const Dummy* pD1 = &d1;
    const Dummy* pD2 = &d2;

    W_TEST_BOOL(ptr.GetPtr() == pD1);
    W_TEST_BOOL(ptr.GetPtr() != pD2);
  }
}
