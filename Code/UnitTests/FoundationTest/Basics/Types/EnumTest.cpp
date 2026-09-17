#include <FoundationTest/FoundationTestPCH.h>

//////////////////////////////////////////////////////////////////////
// Start of the definition of a example Enum
// It takes quite some lines of code to define a enum,
// but it could be encapsulated into an preprocessor macro if wanted
struct WTestEnumBase
{
  using StorageType = WUInt8; // The storage type for the enum

  enum Enum
  {
    No = 0,
    Yes = 1,
    Default = No // Default initialization
  };
};

using WTestEnum = WEnum<WTestEnumBase>; // The name of the final enum
// End of the definition of a example enum
///////////////////////////////////////////////////////////////////////

struct WTestEnum2Base
{
  using StorageType = WUInt16;

  enum Enum
  {
    Bit1 = W_BIT(0),
    Bit2 = W_BIT(1),
    Default = Bit1
  };
};

using WTestEnum2 = WEnum<WTestEnum2Base>;

// Test if the type actually has the requested size
static_assert(sizeof(WTestEnum) == sizeof(WUInt8));
static_assert(sizeof(WTestEnum2) == sizeof(WUInt16));

W_CREATE_SIMPLE_TEST_GROUP(Basics);

// This takes a c++ enum. Tests the implict conversion
void TakeEnum1(WTestEnum::Enum value) {}

// This takes our own enum type
void TakeEnum2(WTestEnum value) {}

W_CREATE_SIMPLE_TEST(Basics, Enum)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Default initialized enum")
  {
    WTestEnum e1;
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Enum with explicit initialization")
  {
    WTestEnum e2(WTestEnum::Yes);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "This tests if the default initialization works and if the implicit conversion works")
  {
    WTestEnum e1;
    WTestEnum e2(WTestEnum::Yes);

    W_TEST_BOOL(e1 == WTestEnum::No);
    W_TEST_BOOL(e2 == WTestEnum::Yes);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Function call tests")
  {
    WTestEnum e1;

    TakeEnum1(e1);
    TakeEnum2(e1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetValue and SetValue")
  {
    WTestEnum e1;
    W_TEST_INT(e1.GetValue(), 0);
    e1.SetValue(17);
    W_TEST_INT(e1.GetValue(), 17);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Assignment of different values")
  {
    WTestEnum e1, e2;

    e1 = WTestEnum::Yes;
    e2 = WTestEnum::No;
    W_TEST_BOOL(e1 == WTestEnum::Yes);
    W_TEST_BOOL(e2 == WTestEnum::No);

    e1 = e2;
    W_TEST_BOOL(e1 == WTestEnum::No);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Test the | operator")
  {
    WTestEnum2 e3(WTestEnum2::Bit1);
    WTestEnum2 e4(WTestEnum2::Bit2);
    WUInt16 uiBits = (e3 | e4).GetValue();
    W_TEST_BOOL(uiBits == (WTestEnum2::Bit1 | WTestEnum2::Bit2));
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "Test the & operator")
  {
    WTestEnum2 e3(WTestEnum2::Bit1);
    WTestEnum2 e4(WTestEnum2::Bit2);
    WUInt16 uiBits = ((e3 | e4) & e4).GetValue();
    W_TEST_BOOL(uiBits == WTestEnum2::Bit2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Test conversion to int")
  {
    WTestEnum e1;
    int iTest = e1.GetValue();
    W_TEST_BOOL(iTest == WTestEnum::No);
  }
}
