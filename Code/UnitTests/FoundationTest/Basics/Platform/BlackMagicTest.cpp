#include <FoundationTest/FoundationTestPCH.h>

#define INT_DECLARE(name, n) int name = n;

namespace
{
  W_EXPAND_ARGS_WITH_INDEX(INT_DECLARE, heinz, klaus);
}

W_CREATE_SIMPLE_TEST(Basics, BlackMagic)
{
  W_TEST_INT(heinz, 0);
  W_TEST_INT(klaus, 1);
}
