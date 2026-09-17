#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Strings/String.h>

W_CREATE_SIMPLE_TEST(Math, Float16)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "From float and back")
  {
    // default constructor
    W_TEST_BOOL(static_cast<float>(WFloat16()) == 0.0f);

    // Border cases - exact matching needed.
    W_TEST_FLOAT(static_cast<float>(WFloat16(1.0f)), 1.0f, 0);
    W_TEST_FLOAT(static_cast<float>(WFloat16(-1.0f)), -1.0f, 0);
    W_TEST_FLOAT(static_cast<float>(WFloat16(0.0f)), 0.0f, 0);
    W_TEST_FLOAT(static_cast<float>(WFloat16(-0.0f)), -0.0f, 0);
    W_TEST_BOOL(static_cast<float>(WFloat16(WMath::Infinity<float>())) == WMath::Infinity<float>());
    W_TEST_BOOL(static_cast<float>(WFloat16(-WMath::Infinity<float>())) == -WMath::Infinity<float>());
    W_TEST_BOOL(WMath::IsNaN(static_cast<float>(WFloat16(WMath::NaN<float>()))));

    // Some random values.
    W_TEST_FLOAT(static_cast<float>(WFloat16(42.0f)), 42.0f, WMath::LargeEpsilon<float>());
    W_TEST_FLOAT(static_cast<float>(WFloat16(1.e3f)), 1.e3f, WMath::LargeEpsilon<float>());
    W_TEST_FLOAT(static_cast<float>(WFloat16(-1230.0f)), -1230.0f, WMath::LargeEpsilon<float>());
    W_TEST_FLOAT(static_cast<float>(WFloat16(WMath::Pi<float>())), WMath::Pi<float>(), WMath::HugeEpsilon<float>());

    // Denormalized float.
    W_TEST_FLOAT(static_cast<float>(WFloat16(1.e-40f)), 0.0f, 0);
    W_TEST_FLOAT(static_cast<float>(WFloat16(1.e-44f)), 0.0f, 0);

    // Clamping of too large/small values
    // Half only supports 2^-14 to 2^14 (in 10^x this is roughly 4.51) (see Wikipedia)
    W_TEST_FLOAT(static_cast<float>(WFloat16(1.e-10f)), 0.0f, 0);
    W_TEST_BOOL(static_cast<float>(WFloat16(1.e5f)) == WMath::Infinity<float>());
    W_TEST_BOOL(static_cast<float>(WFloat16(-1.e5f)) == -WMath::Infinity<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator ==")
  {
    W_TEST_BOOL(WFloat16(1.0f) == WFloat16(1.0f));
    W_TEST_BOOL(WFloat16(10000000.0f) == WFloat16(10000000.0f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator !=")
  {
    W_TEST_BOOL(WFloat16(1.0f) != WFloat16(-1.0f));
    W_TEST_BOOL(WFloat16(10000000.0f) != WFloat16(10000.0f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRawData / SetRawData")
  {
    WFloat16 f;
    f.SetRawData(23);

    W_TEST_INT(f.GetRawData(), 23);
  }
}
